#!/bin/bash
# Test rdx query command
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RDX="${1:-$SCRIPT_DIR/../../build-release/rdx/rdx}"
RDX="$(cd "$(dirname "$RDX")" && pwd)/$(basename "$RDX")"

echo "=== Testing rdx query command ==="

# Test 1: Simple key lookup in Euler set
echo "--- Test 1: Simple key lookup ---"
RESULT=$(echo '"name"' | "$RDX" q '{"name": "test", "value": 42}')
EXPECTED='(
    "name",
    "test"
)'
if [ "$RESULT" = "$EXPECTED" ]; then
    echo "PASS: Found 'name' key"
else
    echo "FAIL: Expected tuple (name, test)"
    echo "Got: $RESULT"
    exit 1
fi

# Test 2: Another key lookup
echo "--- Test 2: Another key lookup ---"
RESULT=$(echo '"value"' | "$RDX" q '{"name": "test", "value": 42}')
EXPECTED='(
    "value",
    42
)'
if [ "$RESULT" = "$EXPECTED" ]; then
    echo "PASS: Found 'value' key"
else
    echo "FAIL: Expected tuple (value, 42)"
    echo "Got: $RESULT"
    exit 1
fi

# Test 3: Missing key returns error
echo "--- Test 3: Missing key ---"
if echo '"missing"' | "$RDX" q '{"name": "test", "value": 42}' >/dev/null 2>&1; then
    echo "FAIL: Expected error for missing key"
    exit 1
else
    echo "PASS: Missing key returns error"
fi

# Test 4: Query from file input
echo "--- Test 4: Query from file ---"
TMPFILE=$(mktemp --suffix=.jdr)
echo '{"foo": "bar", "baz": 123}' > "$TMPFILE"
RESULT=$(echo '"foo"' | "$RDX" q "$TMPFILE")
rm "$TMPFILE"
EXPECTED='(
    "foo",
    "bar"
)'
if [ "$RESULT" = "$EXPECTED" ]; then
    echo "PASS: Query from file works"
else
    echo "FAIL: Expected tuple (foo, bar)"
    echo "Got: $RESULT"
    exit 1
fi

# Test 5: Query with integer key
echo "--- Test 5: Integer key in Euler set ---"
RESULT=$(echo '1' | "$RDX" q '{1: "one", 2: "two"}')
EXPECTED='(
    1,
    "one"
)'
if [ "$RESULT" = "$EXPECTED" ]; then
    echo "PASS: Integer key lookup works"
else
    echo "FAIL: Expected tuple (1, one)"
    echo "Got: $RESULT"
    exit 1
fi

# Test 6: Empty input
echo "--- Test 6: Empty query ---"
RESULT=$(echo '' | "$RDX" q '{"name": "test"}')
EXPECTED='{
    (
        "name",
        "test"
    )
}'
if [ "$RESULT" = "$EXPECTED" ]; then
    echo "PASS: Empty query returns root"
else
    echo "FAIL: Expected root element"
    echo "Got: $RESULT"
    exit 1
fi

echo ""
echo "=== All query tests passed ==="
