#!/bin/sh
#  sync-ssh.sh — exercise the plain dog sync protocol over ssh localhost.
#
#  Flow (under $HOME/tmp/plain/):
#    1. a01 seeds with a commit (sniff post).
#    2. b01 clones from a01 via `keeper get be://localhost/tmp/plain/a01`.
#    3. b01 makes a second commit, `keeper post be://localhost/tmp/plain/a01`
#       pushes it back.  a01 must then have both packs + B's commit.
#    4. a01 makes a third commit, b01 pulls it down.  b01 must then have
#       all three commits.
#
#  Requires passwordless ssh to localhost and $HOME/tmp writable.
#  Gated by WITH_SSH in keeper/test/CMakeLists.txt.  The harness exports
#  DOG_REMOTE_PATH so the remote `keeper` resolves to this build's binary
#  without touching the remote user's shell rc.

set -eu

BIN=${BIN:-$(dirname "$0")/../../build-debug/bin}
KEEPER="$BIN/keeper"
SNIFF="$BIN/sniff"

#  The remote ssh side needs to find our `keeper` — prepend our BIN.
export DOG_REMOTE_PATH="$BIN"
export PATH="$BIN:$PATH"

ROOT="$HOME/tmp/plain"
rm -rf "$ROOT"
trap 'rm -rf "$ROOT"' EXIT INT TERM

A="$ROOT/a01"
B="$ROOT/b01"
mkdir -p "$A/.dogs/keeper" "$B/.dogs/keeper"

fail() { echo "FAIL: $*" >&2; exit 1; }
note() { echo "  - $*"; }

#  Pre-flight: ssh localhost must work non-interactively.
if ! ssh -o BatchMode=yes -o ConnectTimeout=5 localhost true >/dev/null 2>&1; then
    echo "SKIP: ssh localhost not available (WITH_SSH)"
    exit 0
fi

# --- 1. seed a01 ---------------------------------------------------
echo "=== 1. seed a01 ==="
cd "$A"
echo "alpha" > a.txt
"$SNIFF"         >/dev/null 2>&1
"$SNIFF" post -m seedA . >/dev/null 2>&1
SEED_SHA=$("$KEEPER" refs 2>/dev/null | awk '/\?heads\/master[ \t]+→ / {print $3; exit}' | sed 's/^?//')
[ -n "$SEED_SHA" ] || fail "a01: no seed ref"
note "a01 HEAD $SEED_SHA"

# --- 2. clone b01 ← a01 --------------------------------------------
echo "=== 2. clone b01 ← a01 over be:// ==="
cd "$B"
"$KEEPER" get "be://localhost/tmp/plain/a01" 2>&1 | sed 's/^/    /'
"$KEEPER" verify "#$SEED_SHA" >/dev/null 2>&1 \
    || fail "b01: seed commit not verifiable after clone"
note "b01 received + verified seed commit"

# --- 3. b01 commits, pushes to a01 ---------------------------------
echo "=== 3. b01 commits + pushes to a01 ==="
cd "$B"
echo "bravo from B" > b.txt
"$SNIFF"         >/dev/null 2>&1
"$SNIFF" post -m fromB . >/dev/null 2>&1
B_SHA=$("$KEEPER" refs 2>/dev/null | awk '/\?heads\/master[ \t]+→ / {print $3; exit}' | sed 's/^?//')
[ -n "$B_SHA" ] && [ "$B_SHA" != "$SEED_SHA" ] \
    || fail "b01: post produced no new commit"
note "b01 new commit $B_SHA"
"$KEEPER" post "be://localhost/tmp/plain/a01" 2>&1 | sed 's/^/    /'
#  Verify a01 can now retrieve B's commit.
cd "$A"
"$KEEPER" verify "#$B_SHA" >/dev/null 2>&1 \
    || fail "a01: pushed commit not verifiable"
note "a01 received + verified B's commit"

# --- 4. a01 commits, b01 pulls -------------------------------------
echo "=== 4. a01 commits + b01 pulls ==="
cd "$A"
echo "charlie from A" > c.txt
"$SNIFF"         >/dev/null 2>&1
"$SNIFF" post -m fromA . >/dev/null 2>&1
A_SHA=$("$KEEPER" refs 2>/dev/null | awk '/\?heads\/master[ \t]+→ / {print $3; exit}' | sed 's/^?//')
[ -n "$A_SHA" ] && [ "$A_SHA" != "$B_SHA" ] \
    || fail "a01: post produced no new commit"
note "a01 new commit $A_SHA"
cd "$B"
"$KEEPER" get "be://localhost/tmp/plain/a01" 2>&1 | sed 's/^/    /'
"$KEEPER" verify "#$A_SHA" >/dev/null 2>&1 \
    || fail "b01: pulled commit not verifiable"
note "b01 received + verified A's follow-up commit"

echo "=== sync-ssh OK ==="
