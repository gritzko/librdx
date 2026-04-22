#!/bin/sh
#  workflow.sh — sniff standalone get/put/post/delete integration test.
#
#  Drives the `sniff` CLI against a toy worktree with no remote: every
#  scenario exercises the staged-tree model introduced in the PUT/POST
#  rework.  Run either directly via
#
#      BIN=build-debug/bin sh sniff/test/workflow.sh
#
#  or through ctest (see sniff/test/CMakeLists.txt).
#
set -eu

BIN=${BIN:-$(dirname "$0")/../../build-debug/bin}
SNIFF="$BIN/sniff"
KEEPER="$BIN/keeper"

TMP=${TMPDIR:-/tmp}/sniff-workflow-$$
trap 'rm -rf "$TMP"' EXIT INT TERM

fail() { echo "FAIL: $*" >&2; exit 1; }
note() { echo "  - $*"; }

want_file() {
    path=$1; want=$2
    [ -f "$path" ] || fail "$path missing"
    got=$(cat "$path")
    [ "$got" = "$want" ] || fail "$path: want [$want] got [$got]"
}

want_missing() {
    [ ! -e "$1" ] || fail "$1 should be gone"
}

# Current worktree commit = tail SHA in sniff/at.log.
# Lines: <ron60-time>\t?<branch>\t?<sha>
head_hex() {
    awk -F'\t' 'END {gsub(/^\?/,"",$3); print $3}' .sniff/at.log
}

# Pack log files at .dogs/NNNNN.keeper.  Count them as a proxy for
# "did we write anything new?" — each KEEPPackFeed grows the pack.
npacks() { ls .dogs/*.keeper 2>/dev/null | wc -l | tr -d ' '; }

# ------------------------------------------------------------------
# Scenario 1: empty dir -> write file -> post auto-stages -> commit
# ------------------------------------------------------------------
echo "=== 1. initial post auto-stages worktree ==="
D1="$TMP/r1"
mkdir -p "$D1"; cd "$D1"
echo "hello" > README.md
"$SNIFF" post -m "initial" >/dev/null
H1=$(head_hex)
[ -n "$H1" ] || fail "HEAD unset after post"
note "HEAD=$H1"

# ------------------------------------------------------------------
# Scenario 2: checkout the commit into a fresh dir, files reappear
# ------------------------------------------------------------------
echo "=== 2. get on a wiped worktree restores files ==="
rm -f README.md
"$SNIFF" get "$H1" >/dev/null
want_file README.md "hello"
note "README.md restored from $H1"

# ------------------------------------------------------------------
# Scenario 3: two PUTs accumulate, then POST commits the staged tree
# ------------------------------------------------------------------
echo "=== 3. put a; put b; post ==="
D3="$TMP/r3"
mkdir -p "$D3"; cd "$D3"
echo alpha > a.txt
echo bravo > b.txt
T1=$("$SNIFF" put a.txt 2>&1 | awk '/staged tree/ {print $NF}')
[ -n "$T1" ] || fail "no staged tree after put a"
T2=$("$SNIFF" put b.txt 2>&1 | awk '/staged tree/ {print $NF}')
[ -n "$T2" ] || fail "no staged tree after put b"
[ "$T1" != "$T2" ] || fail "tree did not change between puts"
note "tree after put a: $T1"
note "tree after put b: $T2 (accumulated)"

"$SNIFF" post -m "a+b" >/dev/null
C3=$(head_hex)
[ -n "$C3" ] || fail "HEAD unset"
note "HEAD after post=$C3"

# Checkout into fresh dir verifies both files really landed in the tree.
D3b="$TMP/r3b"
mkdir -p "$D3b"; cd "$D3b"
# Share the keeper store by copying .dogs/ from r3.
cp -r "$D3/.dogs" .
rm -rf .sniff  # fresh sniff state
"$SNIFF" get "$C3" >/dev/null
want_file a.txt "alpha"
want_file b.txt "bravo"
note "both files present after get"

# ------------------------------------------------------------------
# Scenario 4: modify + bare `sniff put` (no args) stages everything dirty
# ------------------------------------------------------------------
echo "=== 4. put with no args = stage all dirty ==="
cd "$D3b"
sleep 1                               # mtime resolution
echo alpha-two > a.txt                # modify
T4=$("$SNIFF" put 2>&1 | awk '/staged tree/ {print $NF}')
[ -n "$T4" ] || fail "no staged tree after bare put"
note "staged tree=$T4"
"$SNIFF" post -m "modify a" >/dev/null
C4=$(head_hex)
[ "$C4" != "$C3" ] || fail "HEAD unchanged after modify+post"
note "new HEAD=$C4"

D4b="$TMP/r4b"; mkdir -p "$D4b"; cd "$D4b"
cp -r "$D3b/.dogs" .
rm -rf .sniff
"$SNIFF" get "$C4" >/dev/null
want_file a.txt "alpha-two"
want_file b.txt "bravo"
note "modified content on disk after get"

# ------------------------------------------------------------------
# Scenario 5: `sniff delete foo` drops only foo; tree keeps the rest
# ------------------------------------------------------------------
echo "=== 5. delete foo ==="
cd "$D4b"
"$SNIFF" delete a.txt >/dev/null
"$SNIFF" post -m "drop a" >/dev/null
C5=$(head_hex)
note "HEAD after delete=$C5"

D5b="$TMP/r5b"; mkdir -p "$D5b"; cd "$D5b"
cp -r "$D4b/.dogs" .
rm -rf .sniff
"$SNIFF" get "$C5" >/dev/null
want_missing a.txt
want_file b.txt "bravo"
note "a.txt pruned, b.txt preserved"

# ------------------------------------------------------------------
# Scenario 6: `sniff delete` with no args stages every missing file
# ------------------------------------------------------------------
echo "=== 6. delete with no args = stage all missing ==="
cd "$D5b"
# Check out a state with two files first.
"$SNIFF" get "$C3" >/dev/null
want_file a.txt "alpha"
want_file b.txt "bravo"
rm a.txt                              # remove from disk; still tracked
T6=$("$SNIFF" delete 2>&1 | awk '/staged tree/ {print $NF}')
[ -n "$T6" ] || fail "no staged tree after bare delete"
"$SNIFF" post -m "auto-delete" >/dev/null
C6=$(head_hex)

D6b="$TMP/r6b"; mkdir -p "$D6b"; cd "$D6b"
cp -r "$D5b/.dogs" .
rm -rf .sniff
"$SNIFF" get "$C6" >/dev/null
want_missing a.txt
want_file b.txt "bravo"
note "a.txt removed by auto-delete"

echo "=== all workflow scenarios passed ==="
