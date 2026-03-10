#!/bin/bash
# CLI integration test for the `be` binary
# Exercises every non-network porcelain command from be/CLI.md
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BE="${1:-$SCRIPT_DIR/../../build/be/be}"
BE="$(cd "$(dirname "$BE")" && pwd)/$(basename "$BE")"

TMPDIR=$(mktemp -d /tmp/be-cli-test.XXXXXX)
export HOME="$TMPDIR"
export ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_leaks=0"
WORK="$TMPDIR/work"
mkdir -p "$WORK"

cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

pass() { echo "  PASS: $1"; }
fail() { echo "  FAIL: $1"; exit 1; }

# Create test source files
echo "int a = 1;" > "$WORK/a.c"
echo "int b = 2;" > "$WORK/b.c"
echo "int c = 3;" > "$WORK/c.c"

# =============================================================
echo "--- 6.d: be post //repo/project (init) ---"
cd "$WORK"
"$BE" post //testrepo/@me/proj
test -f .be || fail ".be not created"
test -d "$HOME/.be/testrepo" || fail "depot not created"
pass "init created .be and depot"

# =============================================================
echo "--- 6.a: be post (all files) ---"
sleep 1
echo "int a = 10;" > "$WORK/a.c"
"$BE" post
pass "post all files"

# =============================================================
echo "--- 6.a2: be post (unchanged files not re-posted) ---"
sleep 1
OUT=$("$BE" post 2>&1)
if echo "$OUT" | grep -q "OK"; then
    echo "$OUT"
    fail "unchanged files were re-posted"
fi
pass "post skips unchanged files"

# =============================================================
echo "--- 6.b: be post file.c (specific file) ---"
sleep 1
echo "int b = 20;" > "$WORK/b.c"
"$BE" post b.c
pass "post specific file"

# =============================================================
echo "--- 5.a: be get (refresh all) ---"
echo "GARBAGE" > "$WORK/a.c"
echo "GARBAGE" > "$WORK/b.c"
"$BE" get
grep -q "int a = 10" "$WORK/a.c" || fail "a.c not restored"
grep -q "int b = 20" "$WORK/b.c" || fail "b.c not restored"
pass "get refreshed worktree"

# =============================================================
echo "--- 5.b: be get file.c (specific file) ---"
echo "GARBAGE" > "$WORK/b.c"
"$BE" get b.c
grep -q "int b = 20" "$WORK/b.c" || fail "b.c not restored"
pass "get specific file"

# =============================================================
echo "--- come (switch to branch) ---"
"$BE" come ?feat
grep -q "feat" "$WORK/.be" || fail ".be missing feat branch"
pass "come switched to feat"

# =============================================================
echo "--- 6.b on branch: post on feat ---"
echo "int f = 1;" > "$WORK/feat.c"
echo "int a = 99;" > "$WORK/a.c"
"$BE" post feat.c
"$BE" post
pass "post on feat branch"

# =============================================================
echo "--- 5.c: be get ?branch ---"
"$BE" get ?main
grep -q "main" "$WORK/.be" || fail ".be missing main"
pass "get ?main (active switch + refresh)"

# =============================================================
echo "--- 5.d: be get ?brA&brB (blend) ---"
"$BE" get '?main&feat'
# feat.c should exist (posted on feat, visible in blend)
test -f "$WORK/feat.c" || fail "feat.c not in blend"
pass "get blend main&feat"

# =============================================================
echo "--- 7.a: be put ?branch (merge) ---"
"$BE" come ?main
"$BE" put ?feat
grep -q "int a = 99" "$WORK/a.c" || fail "merge did not apply feat changes"
pass "put merged feat into main"

# =============================================================
echo "--- 8.a: be delete file.c (path with /) ---"
mkdir -p "$WORK/del"
echo "int d = 1;" > "$WORK/del/d.c"
"$BE" post
"$BE" delete del/d.c
pass "delete file with path (tombstone written)"

# =============================================================
echo "--- 8.a2: be delete bare-file.c (flat filename) ---"
echo "int x = 1;" > "$WORK/x.c"
"$BE" post x.c
"$BE" delete x.c
pass "delete bare file (tombstone written)"

# =============================================================
echo "--- 8.b: be delete ?branch ---"
"$BE" come ?delbr
echo "int d = 4;" > "$WORK/d.c"
"$BE" post d.c
"$BE" come ?main
"$BE" delete ?delbr
pass "delete branch (? prefix)"

# =============================================================
echo "--- 8.b2: be delete branch (auto-detect) ---"
"$BE" come ?delbr2
echo "int d = 5;" > "$WORK/d2.c"
"$BE" post d2.c
"$BE" come ?main
"$BE" delete delbr2
pass "delete branch (auto-detect)"

# =============================================================
echo "--- mark (milestone) ---"
"$BE" mark v1
pass "mark milestone"

# =============================================================
echo "--- lay (waypoint) ---"
"$BE" lay
pass "lay waypoint"

# =============================================================
echo "--- 6.c: be post sub/ (subdir selective post) ---"
mkdir -p "$WORK/sub"
echo "int s = 1;" > "$WORK/sub/s.c"
echo "int s2 = 2;" > "$WORK/sub/s2.c"
"$BE" post sub/
echo "GARBAGE" > "$WORK/sub/s.c"
echo "GARBAGE" > "$WORK/sub/s2.c"
"$BE" get
grep -q "int s = 1" "$WORK/sub/s.c" || fail "sub/s.c not restored"
grep -q "int s2 = 2" "$WORK/sub/s2.c" || fail "sub/s2.c not restored"
pass "post sub/ + get subdir roundtrip"

# =============================================================
echo "--- 6.c2: be post sub/file.c (multi-component path) ---"
echo "int s = 10;" > "$WORK/sub/s.c"
"$BE" post sub/s.c
echo "GARBAGE" > "$WORK/sub/s.c"
"$BE" get sub/s.c
grep -q "int s = 10" "$WORK/sub/s.c" || fail "sub/s.c not restored"
pass "post sub/s.c roundtrip"

# =============================================================
echo "--- be (no args, status) ---"
cd "$WORK"
"$BE" > /dev/null
pass "status (no args)"

# =============================================================
echo "--- 6.f: be post //forked (checkpoint/fork) ---"
cd "$WORK"
"$BE" post //forked
test -d "$HOME/.be/forked" || fail "forked depot not created"
pass "checkpoint into //forked"

# =============================================================
echo "--- 5.e: be get //repo/project (local depot checkout) ---"
WORK2="$TMPDIR/work2"
mkdir -p "$WORK2"
cd "$WORK2"
"$BE" get //testrepo/@me/proj
test -f "$WORK2/proj/.be" || fail ".be not created in work2/proj"
grep -q "int a = 99" "$WORK2/proj/a.c" || fail "a.c not checked out"
pass "local depot checkout"
cd "$WORK"

# =============================================================
echo "--- 9.a: be grep (substring search) ---"
cd "$WORK"
OUT=$("$BE" grep "int a")
echo "$OUT" | grep -q "a.c:.*int a" || fail "grep did not find a.c with line"
pass "be grep bare text (file:line format)"

OUT=$("$BE" grep "#int b")
echo "$OUT" | grep -q "b.c:.*int b" || fail "grep #fragment did not find b.c"
pass "be grep #fragment"

OUT=$("$BE" grep "nonexistent_zzz_qqq")
test -z "$OUT" || fail "grep found phantom match"
pass "be grep no match"

# =============================================================
echo "--- be diff (colored worktree diff) ---"
cd "$WORK"
# First ensure clean state
"$BE" post
sleep 1
# Modify a file
echo "int a = 999;" > "$WORK/a.c"
OUT=$("$BE" diff 2>&1)
echo "$OUT" | grep -q "a.c" || fail "diff did not show a.c header"
echo "$OUT" | grep -q "999" || fail "diff did not show new value 999"
pass "be diff all files"

# Diff specific file
OUT=$("$BE" diff a.c 2>&1)
echo "$OUT" | grep -q "a.c" || fail "diff a.c did not show header"
echo "$OUT" | grep -q "999" || fail "diff a.c did not show new value"
pass "be diff specific file"

# No diff for unmodified file
OUT=$("$BE" diff b.c 2>&1)
test -z "$OUT" || fail "diff b.c should be empty"
pass "be diff no changes"

# Restore
"$BE" get
pass "diff tests done"

# =============================================================
# Skipped: not yet implemented
echo "--- SKIP: 3.a PUT file.sst (SST ingest) ---"
echo "--- SKIP: 3.c PUT //repo (cross-depot ingest) ---"
echo "--- SKIP: 7.e be put file.sst ---"
# Skipped: network-dependent
echo "--- SKIP: 3.d PUT http://remote ---"
echo "--- SKIP: 5.f be get http://remote ---"
echo "--- SKIP: 6.e be post http://remote ---"
echo "--- SKIP: 7.c be put http://remote ---"

echo ""
echo "=== All CLI tests passed ==="
