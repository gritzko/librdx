#!/bin/sh
#  mill.sh — treadmill integration test
#
#  Creates a toy repo, clones via keeper+sniff and git, compares trees.
#  Run: BIN=build-debug/bin sh beagle/test/mill.sh
#
set -e

BIN=${BIN:-$(dirname "$0")/../../build-debug/bin}
export PATH="$BIN:$PATH"

TMILL=${TMILL:-/tmp/mill-$$}
trap 'rm -rf "$TMILL"' EXIT

mkdir -p "$TMILL"

echo "=== mill: full clone + verify ==="

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

# --- 3. Dogs clone: keeper fetch + sniff checkout ---
mkdir -p "$TMILL/be01"
cd "$TMILL/be01"
git init --quiet .
mkdir -p .dogs/keeper

keeper get "//localhost${TMILL}/origin" 2>&1

# Verify refs
KEEP_HEAD=$(keeper get ".?refs/heads/master" 2>/dev/null)
echo "keeper HEAD: $KEEP_HEAD"
test "$KEEP_HEAD" = "$GIT_HEAD" || { echo "FAIL: HEAD mismatch"; exit 1; }
echo "PASS: refs match"

# Verify commit content
KEEP_COMMIT=$(keeper get ".#${GIT_HEAD}" 2>/dev/null)
GIT_COMMIT=$(git -C "$TMILL/git01" cat-file -p HEAD 2>/dev/null)
test "$KEEP_COMMIT" = "$GIT_COMMIT" || { echo "FAIL: commit mismatch"; exit 1; }
echo "PASS: commit matches"

# Checkout via sniff
sniff index 2>&1
sniff checkout "$KEEP_HEAD" 2>&1

# --- 4. Compare worktrees ---
# Use find + diff on individual files (busybox diff lacks --exclude)
FAIL=0
cd "$TMILL/git01"
for f in $(find . -not -path './.git/*' -type f | sort); do
    if [ ! -f "$TMILL/be01/$f" ]; then
        echo "FAIL: missing $f in be01"
        FAIL=1
    elif ! diff -q "$f" "$TMILL/be01/$f" >/dev/null 2>&1; then
        echo "FAIL: $f differs"
        FAIL=1
    fi
done
# Check for extra files in be01
cd "$TMILL/be01"
for f in $(find . -not -path './.dogs/*' -not -path './.git/*' -type f | sort); do
    if [ ! -f "$TMILL/git01/$f" ]; then
        echo "FAIL: extra $f in be01"
        FAIL=1
    fi
done

if [ "$FAIL" = "0" ]; then
    echo "PASS: trees are identical"
else
    exit 1
fi

echo "=== mill OK ==="
