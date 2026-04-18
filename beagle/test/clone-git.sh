#!/bin/sh
#  clone-git.sh — clone localhost:$HOME/src/git via `be get`, compare
#  the resulting worktree with a reference `git clone` of the same repo.
#
#  Run: BIN=build-debug/bin sh beagle/test/clone-git.sh
#
#  Uses three scratch dirs under $HOME/tmp/clone/:
#    git01 — dogs clone (be get)
#    git02 — git reference clone (git clone --no-checkout + checkout HEAD)
#    git03 — reserved for a second dogs clone at a different ref (optional)

set -e

BIN=${BIN:-$(dirname "$0")/../../build-debug/bin}
export PATH="$BIN:$PATH"

REPO=${REPO:-$HOME/src/git}
CLONE=${CLONE:-$HOME/tmp/clone}
REF=${REF:-HEAD}

rm -rf "$CLONE/git01" "$CLONE/git02"
mkdir -p "$CLONE/git01"

# --- git reference clone ---
echo "=== git clone reference ==="
git clone --quiet --no-checkout "$REPO" "$CLONE/git02"
if [ "$REF" = "HEAD" ]; then
    git -C "$CLONE/git02" checkout --quiet
else
    git -C "$CLONE/git02" checkout --quiet "$REF"
fi

# --- dogs clone via be get ---
echo "=== be get ==="
cd "$CLONE/git01"
git init --quiet .
mkdir -p .dogs/keeper

if [ "$REF" = "HEAD" ]; then
    be get "//localhost${REPO}" 2>&1 | grep -v "^keeper: round" || true
else
    be get "//localhost${REPO}?${REF}" 2>&1 | grep -v "^keeper: round" || true
fi

# --- compare worktrees (rsync dry-run) ---
echo "=== diff ==="
RDIFF=$(rsync -rlcn --delete \
    --exclude='/.git/' --exclude='/.dogs/' \
    "$CLONE/git02/" "$CLONE/git01/" 2>&1)

G1=$(find "$CLONE/git01" -not -path '*/.dogs/*' -not -path '*/.git/*' -type f | wc -l)
G2=$(find "$CLONE/git02" -not -path '*/.git/*' -type f | wc -l)
echo "git01 (be get): $G1 files"
echo "git02 (git):    $G2 files"

if [ -z "$RDIFF" ]; then
    echo "PASS: worktrees identical at $REF"
    exit 0
fi

echo "FAIL: worktrees differ"
echo "$RDIFF" | head -20
exit 1
