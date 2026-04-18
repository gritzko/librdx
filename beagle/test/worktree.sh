#!/bin/sh
#  worktree.sh — toy worktree clone + commit-from-worktree flow.
#
#  1. Create primary repo, commit a seed file.
#  2. `be get <primary>` in a fresh dir → expect .dogs/{keeper,graf,
#     spot} as symlinks into primary, .dogs/sniff as a real dir.
#  3. Verify shared keeper is live (seed commit reachable through the
#     symlink).
#  4. In the worktree: put a new file, delete the seed, post a commit.
#     Check the new commit lives in primary's shared keeper, and its
#     parent is the seed.
#
set -eu

BIN=${BIN:-$(dirname "$0")/../../build-debug/bin}
BE="$BIN/be"
KEEPER="$BIN/keeper"

TMP=${TMPDIR:-/tmp}/be-worktree-$$
trap 'rm -rf "$TMP"' EXIT INT TERM

fail() { echo "FAIL: $*" >&2; exit 1; }
note() { echo "  - $*"; }

# Current commit of the worktree at $1 = last SHA in keeper refs
# keyed by `file:///<abs-path>`.
head_hex_of() {
    awk -v p="file://$1" -F'\t' '$2==p {sha=$3} END {gsub(/^\?/,"",sha); print sha}' \
        "$1/.dogs/keeper/refs"
}

# --- 1. primary ------------------------------------------------------
echo "=== 1. primary: seed commit ==="
PRIM="$TMP/prim"; mkdir -p "$PRIM"; cd "$PRIM"
echo hello > README
"$BE" post --seq -m "seed" >/dev/null
SEED_HEAD=$(head_hex_of "$PRIM")
[ -n "$SEED_HEAD" ] || fail "primary HEAD empty after post"
note "primary HEAD=$SEED_HEAD"
for d in keeper graf spot sniff; do
    [ -d "$PRIM/.dogs/$d" ] || fail "primary missing .dogs/$d"
done

# --- 2. worktree from primary ---------------------------------------
echo "=== 2. be get <primary> creates worktree ==="
WT="$TMP/wt"; mkdir -p "$WT"; cd "$WT"
"$BE" get --seq "$PRIM" >/dev/null 2>/dev/null || true
for name in keeper graf spot; do
    [ -L "$WT/.dogs/$name" ] \
        || fail ".dogs/$name is not a symlink"
    tgt=$(readlink "$WT/.dogs/$name")
    [ "$tgt" = "$PRIM/.dogs/$name" ] \
        || fail ".dogs/$name -> $tgt, expected $PRIM/.dogs/$name"
done
[ -d "$WT/.dogs/sniff" ] && [ ! -L "$WT/.dogs/sniff" ] \
    || fail ".dogs/sniff should be a real dir"
note "symlinks: keeper/graf/spot -> primary, sniff local"

# --- 3. shared keeper reachable through the symlink -----------------
echo "=== 3. worktree sees primary's objects ==="
"$KEEPER" verify ".#$SEED_HEAD" 2>&1 | grep -q '0 failed' \
    || fail "seed commit not reachable through symlinked keeper"
note "seed commit reachable via symlink"

# --- 4. put / delete / post from the worktree -----------------------
echo "=== 4. put/del/post from worktree ==="
# Check out the primary's HEAD into the worktree first.
"$BE" get --seq "?$SEED_HEAD" >/dev/null 2>&1 \
    || fail "be get ?HEAD failed in worktree"
[ -f README ] || fail "README not checked out in worktree"
note "worktree checkout OK"

# Add a new file, remove the seed, commit.
echo goodbye > CHANGES
"$BE" put --seq CHANGES >/dev/null
"$BE" delete --seq README >/dev/null
"$BE" post --seq -m "worktree edit" >/dev/null

WT_HEAD=$(head_hex_of "$WT")
[ -n "$WT_HEAD" ] || fail "worktree HEAD empty after post"
[ "$WT_HEAD" != "$SEED_HEAD" ] || fail "worktree HEAD did not advance"
note "worktree HEAD=$WT_HEAD"

# The new commit lives in primary's shared keeper.
cd "$PRIM" && "$KEEPER" verify ".#$WT_HEAD" 2>&1 | grep -q '0 failed' \
    || fail "new commit missing from shared keeper"
cd "$WT"
note "new commit in shared keeper"

# Primary's worktree entry is unchanged (each worktree's file:// key
# is distinct so they don't collide).
PRIM_HEAD=$(head_hex_of "$PRIM")
[ "$PRIM_HEAD" = "$SEED_HEAD" ] \
    || fail "primary HEAD moved ($PRIM_HEAD, was $SEED_HEAD)"
note "primary HEAD still at seed (per-worktree ref respected)"

echo "=== worktree OK ==="
