#!/bin/bash
# Test HTTP server and client (pull command)
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RDX="${1:-$SCRIPT_DIR/../../build-release/rdx/rdx}"
RDX="$(cd "$(dirname "$RDX")" && pwd)/$(basename "$RDX")"

# Create two temporary directories: server and client repos
SERVER_DIR=$(mktemp -d)
CLIENT_DIR=$(mktemp -d)
PORT=$((30000 + RANDOM % 10000))

cleanup() {
    kill $SERVER_PID 2>/dev/null || true
    rm -rf "$SERVER_DIR" "$CLIENT_DIR"
}
trap cleanup EXIT

echo "=== Setting up server repo in $SERVER_DIR ==="
cd "$SERVER_DIR"
"$RDX" init

# Create test data
cat > a.jdr << 'EOF'
{"name": "Alice", "count": 42}
EOF
cat > b.jdr << 'EOF'
{"items": ["x", "y", "z"]}
EOF

"$RDX" add a.jdr
"$RDX" add b.jdr

# Create another branch
"$RDX" fork test
"$RDX" use @test
cat > c.jdr << 'EOF'
{"nested": {"value": 123}}
EOF
"$RDX" add c.jdr
"$RDX" use @main

echo ""
echo "=== Starting server on port $PORT ==="
"$RDX" serve http://127.0.0.1:$PORT &
SERVER_PID=$!
sleep 1

echo ""
echo "=== Testing server endpoints ==="
echo "--- GET / ---"
curl -s http://127.0.0.1:$PORT/
echo ""

echo "--- GET /main ---"
curl -s http://127.0.0.1:$PORT/main
echo ""

REF=$(curl -s http://127.0.0.1:$PORT/main | head -1)
echo "--- GET /main/$REF (binary, first 48 bytes) ---"
curl -s "http://127.0.0.1:$PORT/main/$REF" | xxd | head -3
echo ""

echo "--- GET /main/$REF.jdr (converted) ---"
curl -s "http://127.0.0.1:$PORT/main/$REF.jdr"
echo ""

echo ""
echo "=== Setting up client repo in $CLIENT_DIR ==="
cd "$CLIENT_DIR"
"$RDX" init

echo ""
echo "=== Testing pull (all branches) ==="
"$RDX" pull http://127.0.0.1:$PORT

echo ""
echo "=== Verifying pulled data ==="
echo "Branches:"
ls -la .rdx/branches/

echo ""
echo "Main branch files:"
ls -la .rdx/branches/main/

echo ""
echo "Test branch files:"
ls -la .rdx/branches/test/

echo ""
echo "=== Converting pulled data to JDR ==="
for f in .rdx/branches/main/*.skil; do
    echo "--- $f ---"
    "$RDX" jdr "$f"
done

echo ""
echo "=== Testing pull of single branch ==="
rm -rf .rdx/branches/main
"$RDX" pull http://127.0.0.1:$PORT/main
ls -la .rdx/branches/main/

echo ""
echo "=== Test removing stale files ==="
# Delete a file on server
rm "$SERVER_DIR/.rdx/branches/main/"*.skil 2>/dev/null | head -1 || true
# Add new file
cd "$SERVER_DIR"
cat > new.jdr << 'EOF'
{"new": true}
EOF
"$RDX" add new.jdr

# Pull again - should remove old files and add new one
cd "$CLIENT_DIR"
echo "Before pull:"
ls .rdx/branches/main/
"$RDX" pull http://127.0.0.1:$PORT/main
echo "After pull:"
ls .rdx/branches/main/

echo ""
echo "=== All tests passed ==="
