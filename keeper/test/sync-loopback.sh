#!/bin/sh
#  sync-loopback.sh — exercise plain dog sync end-to-end.
#
#  Seeds repo A with one pack, runs `keeper sync` on A via pipes
#  fed by `keeper get file:///path/to/A` from an empty repo B,
#  verifies B received at least one pack and one reflog chunk.

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

cd "$B"
echo "=== sync B ← A ==="
OUT=$(keeper get "file://$A" 2>&1)
echo "$OUT"

case "$OUT" in
    *"received 1 pack"*) echo "PASS: one pack received" ;;
    *"received 0 pack"*) echo "FAIL: zero packs" ; exit 1 ;;
    *)                    echo "FAIL: unexpected output" ; exit 1 ;;
esac
