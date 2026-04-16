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

TAGS=${TAGS:-"v2.8.4 v2.8.5 v2.8.6 v2.9.0 v2.9.0-rc0 v2.9.0-rc1 v2.9.0-rc2 v2.9.1 v2.9.2 v2.9.3 v2.9.4 v2.9.5"}

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

    # --- git checkout in background, be get in foreground ---
    git -C "$TMILL/git01" checkout --quiet "refs/tags/${TAG}" &
    GIT_PID=$!

    cd "$TMILL/be01"
    be get "//localhost${REPO}?refs/tags/${TAG}" 2>&1 | grep -v "^keeper: round"

    wait $GIT_PID

    # --- rsync dry-run: full content comparison ---
    RDIFF=$(rsync -rlcn --delete \
        --exclude='/.git/' --exclude='/.dogs/' \
        "$TMILL/git01/" "$TMILL/be01/" 2>&1)

    BE_N=$(find "$TMILL/be01" -not -path '*/.dogs/*' -not -path '*/.git/*' -type f | wc -l)

    if [ -z "$RDIFF" ]; then
        echo "PASS: $TAG ($BE_N files)"
    else
        echo "FAIL: $TAG"
        echo "$RDIFF" | head -10
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "=== $TOTAL tags, $FAIL failures ==="
test "$FAIL" = "0"
