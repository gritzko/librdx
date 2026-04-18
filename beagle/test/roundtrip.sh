#!/bin/sh
#  roundtrip.sh — full git <-> be round-trip through ssh localhost.
#
#    1. Seed a bare git origin + a working tree with one commit.
#    2. `be get ssh://localhost:<origin>?master` — clone via git-upload-pack.
#    3. `be get <clone>` in a fresh dir — make a worktree off the clone.
#    4. Edit in both the clone and the worktree.
#    5. `be post -m ...` in the worktree (local commit).
#    6. `be post ssh://localhost:<origin>?master` (push the commit).
#    7. Fetch origin into a fresh git clone and diff against our worktree.
#
#  Requires passwordless ssh to localhost (same as POSTtest did).
set -eu

BIN=${BIN:-$(dirname "$0")/../../build-debug/bin}
BE="$BIN/be"
KEEPER="$BIN/keeper"

TMP=${TMPDIR:-/tmp}/be-roundtrip-$$
trap 'rm -rf "$TMP"' EXIT INT TERM
mkdir -p "$TMP"

fail() { echo "FAIL: $*" >&2; exit 1; }
note() { echo "  - $*"; }

ORIGIN="$TMP/origin.git"
SEED="$TMP/seed"
CLONE="$TMP/clone"
WT="$TMP/wt"
VERIFY="$TMP/verify"

# --- 1. seed origin with a single commit ---
echo "=== 1. seed origin ==="
git init --bare "$ORIGIN" >/dev/null 2>&1
git init "$SEED" >/dev/null 2>&1
git -C "$SEED" config user.email t@t
git -C "$SEED" config user.name  Test
git -C "$SEED" checkout -b master >/dev/null 2>&1 || true
echo alpha > "$SEED/a.txt"
echo bravo > "$SEED/b.txt"
git -C "$SEED" add .
git -C "$SEED" commit -m seed >/dev/null
git -C "$SEED" push "$ORIGIN" master:master 2>/dev/null
SEED_SHA=$(git --git-dir="$ORIGIN" rev-parse master)
note "seed=$SEED_SHA"

# --- 2. be get via ssh ---
echo "=== 2. be get ssh://localhost:<origin>?master ==="
mkdir -p "$CLONE"; cd "$CLONE"
"$BE" get --seq "ssh://localhost$ORIGIN?master" >/dev/null 2>&1 \
    || fail "be get (clone) failed"
[ -f a.txt ] && [ -f b.txt ] || fail "clone missing seed files"
note "clone has a.txt, b.txt"

# --- 3. be get <clone> → worktree ---
echo "=== 3. be get <clone> (worktree) ==="
mkdir -p "$WT"; cd "$WT"
"$BE" get --seq "$CLONE" >/dev/null 2>&1 || true
[ -L "$WT/.dogs/keeper" ] || fail "worktree keeper not a symlink"
[ -f a.txt ] && [ -f b.txt ] || fail "worktree missing checked-out files"
note "worktree has symlinked keeper + checked-out files"

# --- 4. edits in clone and worktree ---
echo "=== 4. edits ==="
sleep 1                                      # ensure mtime differs from checkout
echo alpha-CLONE > "$CLONE/a.txt"
echo charlie > "$WT/c.txt"
echo bravo-WT > "$WT/b.txt"
rm -f "$WT/a.txt"
note "clone edited a.txt; worktree adds c.txt, modifies b.txt, removes a.txt"

# --- 5. be post -m in the worktree ---
echo "=== 5. be post -m (local commit) ==="
cd "$WT"
"$BE" post --seq -m "worktree commit" >/dev/null 2>&1 \
    || fail "be post -m failed"
# Read the committed SHA from keeper refs keyed by file://<WT>.
WT_SHA=$(awk -v p="file://$WT" -F'\t' '$2==p {sha=$3} END {gsub(/^\?/,"",sha); print sha}' \
    "$WT/.dogs/keeper/refs")
[ ${#WT_SHA} -eq 40 ] || fail "no 40-hex worktree commit recorded"
[ "$WT_SHA" != "$SEED_SHA" ] || fail "worktree commit didn't advance"
note "worktree commit=$WT_SHA"

# --- 6. be post ssh://... → push ---
echo "=== 6. be post ssh://localhost:<origin>?master ==="
"$BE" post --seq "ssh://localhost$ORIGIN?master" 2>&1 \
    | grep -q "keeper: pushed" || fail "keeper push didn't happen"
note "push reported"

# --- 7. verify origin matches worktree ---
echo "=== 7. verify ==="
ORIGIN_SHA=$(git --git-dir="$ORIGIN" rev-parse master)
[ "$ORIGIN_SHA" = "$WT_SHA" ] \
    || fail "origin master=$ORIGIN_SHA != worktree=$WT_SHA"
note "origin master == worktree commit"

# Fresh git checkout of origin, diff against our worktree.
git clone -q "$ORIGIN" "$VERIFY"
# Compare just the tracked files, not .git vs .dogs.
( cd "$VERIFY" && git ls-files ) | sort > "$TMP/git.list"
( cd "$WT" && find . -type f \
    -not -path './.dogs/*' -not -name '.be' \
    | sed 's|^\./||' ) | sort > "$TMP/wt.list"
diff -u "$TMP/git.list" "$TMP/wt.list" || fail "file sets differ"
while IFS= read -r f; do
    diff "$VERIFY/$f" "$WT/$f" \
        || fail "content differs: $f"
done < "$TMP/git.list"
note "all tracked files match"

echo "=== roundtrip OK ==="
