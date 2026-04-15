#!/bin/sh
#  mill.sh — treadmill integration test
#
#  Creates a toy repo, clones via `be get` and `git clone`, compares.
#  Run: BIN=build-debug/bin sh beagle/test/mill.sh
#
set -e

BIN=${BIN:-$(dirname "$0")/../../build-debug/bin}
export PATH="$BIN:$PATH"

TMILL=${TMILL:-/tmp/mill-$$}
trap 'rm -rf "$TMILL"' EXIT

mkdir -p "$TMILL"

echo "=== mill: be get + verify ==="

# --- 1. Create toy origin ---
T=$(mktemp -d)
git init --quiet "$T"
git -C "$T" config user.email "test@dogs"
git -C "$T" config user.name "Test Dog"
mkdir -p "$T/src" "$T/doc"
echo 'int main(){return 0;}' > "$T/src/main.c"
echo '#include <stdio.h>' > "$T/src/util.h"
echo 'hello world' > "$T/README"
echo 'some docs' > "$T/doc/guide.txt"
git -C "$T" add .
git -C "$T" commit --quiet -m 'initial commit'
echo 'int helper() { return 1; }' >> "$T/src/main.c"
echo 'updated docs' > "$T/doc/guide.txt"
echo 'new file' > "$T/CHANGELOG"
git -C "$T" add .
git -C "$T" commit --quiet -m 'second commit'
git init --quiet --bare "$TMILL/origin"
git -C "$T" remote add origin "$TMILL/origin"
git -C "$T" push --quiet origin master
rm -rf "$T"

GIT_HEAD=$(git -C "$TMILL/origin" rev-parse HEAD)
echo "origin HEAD: $GIT_HEAD"

# --- 2. Git clone (reference) ---
git clone --quiet "$TMILL/origin" "$TMILL/git01"

# --- 3. Dogs clone via be get ---
mkdir -p "$TMILL/be01"
cd "$TMILL/be01"
git init --quiet .
mkdir -p .dogs/keeper

be get "//localhost${TMILL}/origin" 2>&1

# --- 4. Verify refs ---
KEEP_HEAD=$(keeper get ".?refs/heads/master" 2>/dev/null)
echo "keeper HEAD: $KEEP_HEAD"
test "$KEEP_HEAD" = "$GIT_HEAD" || { echo "FAIL: HEAD mismatch"; exit 1; }
echo "PASS: refs match"

# --- 5. Compare worktrees ---
FAIL=0
cd "$TMILL/git01"
find . -not -path './.git/*' -type f -print0 | while IFS= read -r -d '' f; do
    if [ ! -f "$TMILL/be01/$f" ]; then
        echo "MISS: $f"; FAIL=1
    elif ! diff -q "$f" "$TMILL/be01/$f" >/dev/null 2>&1; then
        echo "DIFF: $f"; FAIL=1
    fi
done
cd "$TMILL/be01"
for f in $(find . -not -path './.dogs/*' -not -path './.git/*' -type f); do
    if [ ! -f "$TMILL/git01/$f" ]; then
        echo "EXTRA: $f"; FAIL=1
    fi
done

BE_N=$(find "$TMILL/be01" -not -path '*/.dogs/*' -not -path '*/.git/*' -type f | wc -l)
GIT_N=$(find "$TMILL/git01" -not -path '*/.git/*' -type f | wc -l)
echo "be01: $BE_N files, git01: $GIT_N files"
test "$BE_N" = "$GIT_N" || { echo "FAIL: file count mismatch"; exit 1; }

echo "PASS: trees are identical"
echo "=== mill OK ==="
