#!/bin/sh
#  workflow.sh — be-frontend dispatch test.
#
#  Mirrors sniff/test/workflow.sh but drives every verb through the
#  `be` dispatcher so we also exercise the keeper/spot/graf pipeline
#  wiring (a sniff-only green doesn't catch a missing keeper push or
#  a stale spot index).  Test semantics match the new ULOG-only model:
#  put/delete only append rows, POST walks baseline + wt and applies
#  the change-set rules, GET prunes stamped files no longer in tree.
#
#      BIN=build-debug/bin sh beagle/test/workflow.sh
#
set -eu

BIN=${BIN:-$(dirname "$0")/../../build-debug/bin}
BE="$BIN/be"
export PATH="$BIN:$PATH"

TMP=${TMP:-$HOME/tmp}
TEST_ID=${TEST_ID:-BEworkflow}
TMP=$TMP/$$/$TEST_ID
mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT INT TERM

fail() { echo "FAIL: $*" >&2; exit 1; }
note() { echo "  - $*"; }

want_file() {
    path=$1; want=$2
    [ -f "$path" ] || fail "$path missing"
    got=$(cat "$path")
    [ "$got" = "$want" ] || fail "$path: want [$want] got [$got]"
}
want_missing() { [ ! -e "$1" ] || fail "$1 should be gone"; }

#  Latest `post` row's URI fragment = current HEAD sha.
head_hex() {
    awk -F'\t' '$2 == "post" { last = $3 } END {
        n = index(last, "#"); if (n > 0) print substr(last, n + 1)
    }' .sniff
}

# ------------------------------------------------------------------
# Scenario 1: be post on a fresh dir auto-stages the single file
# ------------------------------------------------------------------
echo "=== 1. be post (auto-stage) on a fresh worktree ==="
D1="$TMP/r1"; mkdir -p "$D1"; cd "$D1"
echo hello > README
"$BE" post -m "initial" >/dev/null
H1=$(head_hex)
[ -n "$H1" ] || fail "HEAD unset after be post"
note "HEAD=$H1"

# ------------------------------------------------------------------
# Scenario 2: be get rebuilds a wiped worktree
# ------------------------------------------------------------------
echo "=== 2. be get on a wiped worktree ==="
rm -f README
"$BE" get "$H1" >/dev/null
want_file README "hello"
note "README restored by be get"

# ------------------------------------------------------------------
# Scenario 3: be put a; be put b; be post — explicit rows in ULOG
# ------------------------------------------------------------------
echo "=== 3. be put a; be put b; be post ==="
D3="$TMP/r3"; mkdir -p "$D3"; cd "$D3"
echo alpha > a.txt
echo bravo > b.txt

"$BE" put a.txt >/dev/null
awk -F'\t' '$2 == "put" && $3 == "a.txt"' .sniff | grep -q . \
    || fail "no \`put a.txt\` row in ULOG"
"$BE" put b.txt >/dev/null
awk -F'\t' '$2 == "put" && $3 == "b.txt"' .sniff | grep -q . \
    || fail "no \`put b.txt\` row in ULOG"
note "ULOG records two put rows"

"$BE" post -m "a+b" >/dev/null
C3=$(head_hex)
note "HEAD=$C3"

# Verify: fresh worktree + be get restores both files.
D3b="$TMP/r3b"; mkdir -p "$D3b"; cd "$D3b"
cp -r "$D3/.dogs" .
"$BE" get "$C3" >/dev/null
want_file a.txt "alpha"
want_file b.txt "bravo"
note "both files present after be get"

# ------------------------------------------------------------------
# Scenario 4: implicit-all-dirty via bare post (no put/delete rows)
# ------------------------------------------------------------------
echo "=== 4. modify + bare be post (implicit all-dirty) ==="
cd "$D3b"
sleep 1                               # force distinct mtime
echo alpha-v2 > a.txt
"$BE" post -m "a-v2" >/dev/null
C4=$(head_hex)
[ "$C4" != "$C3" ] || fail "HEAD unchanged after modify + post"
note "HEAD=$C4"

D4b="$TMP/r4b"; mkdir -p "$D4b"; cd "$D4b"
cp -r "$D3b/.dogs" .
"$BE" get "$C4" >/dev/null
want_file a.txt "alpha-v2"
want_file b.txt "bravo"
note "a.txt updated on disk after be get"

# ------------------------------------------------------------------
# Scenario 5: be delete <file> drops just that path (POST unlinks it)
# ------------------------------------------------------------------
echo "=== 5. be delete a.txt ==="
cd "$D4b"
"$BE" delete a.txt >/dev/null
"$BE" post -m "drop a" >/dev/null
want_missing a.txt                    # POST must unlink explicit deletes
C5=$(head_hex)

D5b="$TMP/r5b"; mkdir -p "$D5b"; cd "$D5b"
cp -r "$D4b/.dogs" .
"$BE" get "$C5" >/dev/null
want_missing a.txt
want_file b.txt "bravo"
note "a.txt removed, b.txt preserved"

# ------------------------------------------------------------------
# Scenario 6: implicit-delete via vanished tracked file
# ------------------------------------------------------------------
echo "=== 6. bare be delete (implicit sweep via missing file) ==="
cd "$D5b"
"$BE" get "$C3" >/dev/null            # restore two-file state
want_file a.txt "alpha"
want_file b.txt "bravo"
rm a.txt                              # vanish one without a `delete` row
"$BE" delete >/dev/null               # no-op (bare); sweep happens at post
"$BE" post -m "auto-delete" >/dev/null
C6=$(head_hex)

D6b="$TMP/r6b"; mkdir -p "$D6b"; cd "$D6b"
cp -r "$D5b/.dogs" .
"$BE" get "$C6" >/dev/null
want_missing a.txt
want_file b.txt "bravo"
note "auto-delete took out a.txt"

echo "=== all be-dispatch scenarios passed ==="
