#!/bin/sh
#  mill-tags.sh — incremental tag-by-tag treadmill
#
#  Clones ~/src/git at v2.8.4, checks out, then updates through
#  each subsequent tag, comparing worktrees with git at each step.
#
#  Run: BIN=build-debug/bin sh beagle/test/mill-tags.sh
#
set -e

BIN=${BIN:-$(dirname "$0")/../../build-debug/bin}
export PATH="$BIN:$PATH"

TMILL=${TMILL:-$HOME/tmp/mill-tags-$$}
REPO=${REPO:-$HOME/src/git}
trap 'rm -rf "$TMILL"' EXIT

TAGS="v2.8.4 v2.8.5 v2.8.6 v2.9.0 v2.9.0-rc0 v2.9.0-rc1 v2.9.0-rc2 v2.9.1 v2.9.2 v2.9.3 v2.9.4 v2.9.5"

mkdir -p "$TMILL/be01" "$TMILL/git01"

# --- Init dogs repo ---
cd "$TMILL/be01"
git init --quiet .
mkdir -p .dogs/keeper

# --- Init git reference repo ---
git clone --quiet --no-checkout "$REPO" "$TMILL/git01"

FAIL=0
TOTAL=0
for TAG in $TAGS; do
    TOTAL=$((TOTAL + 1))
    echo ""
    echo "=== $TAG ==="

    # --- be get: fetch + checkout ---
    cd "$TMILL/be01"
    be get "//localhost${REPO}?refs/tags/${TAG}" 2>&1 | grep -v "^keeper: round"

    # --- git checkout reference ---
    git -C "$TMILL/git01" checkout --quiet "refs/tags/${TAG}"

    # --- compare file counts ---
    BE_N=$(find "$TMILL/be01" -not -path '*/.dogs/*' -not -path '*/.git/*' -type f | wc -l)
    GIT_N=$(find "$TMILL/git01" -not -path '*/.git/*' -type f | wc -l)

    if [ "$BE_N" != "$GIT_N" ]; then
        echo "FAIL $TAG: file count be=$BE_N git=$GIT_N"
        FAIL=$((FAIL + 1))
        continue
    fi

    # --- spot check 20 files ---
    cd "$TMILL/git01"
    TAG_FAIL=0
    for f in $(find . -not -path './.git/*' -type f | sort | head -20); do
        if [ ! -f "$TMILL/be01/$f" ]; then
            echo "  MISS: $f"
            TAG_FAIL=1
        elif ! diff -q "$f" "$TMILL/be01/$f" >/dev/null 2>&1; then
            echo "  DIFF: $f"
            TAG_FAIL=1
        fi
    done

    if [ "$TAG_FAIL" = "0" ]; then
        echo "PASS: $TAG ($BE_N files)"
    else
        echo "FAIL: $TAG"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "=== $TOTAL tags, $FAIL failures ==="
test "$FAIL" = "0"
