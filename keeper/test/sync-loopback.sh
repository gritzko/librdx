#!/bin/sh
#  sync-loopback.sh — exercise keeper fetch end-to-end over file:// .
#
#  Seeds repo A with one commit, runs `keeper get file:///path/to/A`
#  from an empty repo B (which spawns `keeper upload-pack` locally
#  via WIRE), then verifies B can resolve the seed commit from its
#  own store.

set -e

BIN=${BIN:-$(dirname "$0")/../../build-debug/bin}
export PATH="$BIN:$PATH"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

A="$TMP/A"
B="$TMP/B"
mkdir -p "$A/.dogs/keeper" "$B/.dogs/keeper"

echo "=== seed A with one commit ==="
cd "$A"
echo "hello sync" >greeting.txt
sniff          >/dev/null  # scan the worktree into sniff's index
sniff post -m "initial" . >/dev/null  # commit → creates a keeper pack
SEED_SHA=$(keeper refs 2>/dev/null \
    | awk '/\?heads\/master[ \t]+→ / {print $3; exit}' \
    | sed 's/^?//')
[ -n "$SEED_SHA" ] || { echo "FAIL: no seed ref"; exit 1; }

cd "$B"
echo "=== sync B ← A ==="
keeper get "file://$A" 2>&1

if keeper verify "#$SEED_SHA" >/dev/null 2>&1; then
    echo "PASS: seed commit reachable in B"
else
    echo "FAIL: seed commit not reachable in B"
    exit 1
fi
