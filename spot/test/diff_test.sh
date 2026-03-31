#!/bin/bash
# Test spot --diff output for artifacts (duplicated lines).
# Lines that exist in both old and new files should appear ONCE in diff output,
# never as both DEL+INS (which shows them twice without colors).
set -e

SPOT="${1:-./build-debug/spot/spot}"
DATADIR="$(dirname "$0")/data"

OLD="$DATADIR/diff_old.c"
NEW="$DATADIR/diff_new.c"

if [ ! -x "$SPOT" ]; then
    echo "FAIL: spot binary not found at $SPOT"
    exit 1
fi

OUT=$("$SPOT" --diff "$OLD" "$NEW" 2>&1 | perl -pe 's/\e\[[0-9;]*m//g')
FAILS=0

# Lines present in both old and new must appear at most once in diff output.
# These are context lines that should be EQ, not DEL+INS duplicates.
check_unique() {
    local line="$1"
    local count
    count=$(echo "$OUT" | grep -cF "$line" || true)
    if [ "$count" -gt 1 ]; then
        echo "FAIL: '$line' appears $count times (expected 1)"
        FAILS=$((FAILS + 1))
    elif [ "$count" -eq 0 ]; then
        echo "FAIL: '$line' not found in output"
        FAILS=$((FAILS + 1))
    else
        echo "  OK: '$line'"
    fi
}

echo "=== diff artifact test ==="
check_unique "if (flag != OK) {"
check_unique "init_data();"
check_unique "done_old();"
check_unique "if (map) unmap(map);"

# Lines only in old should appear at most once (as DEL)
check_unique "// Old comment line about processing"
# Lines only in new should appear at most once (as INS)
check_unique "// New comment about the processing step"

if [ "$FAILS" -gt 0 ]; then
    echo "FAILED: $FAILS checks failed"
    echo "--- full output ---"
    echo "$OUT"
    exit 1
fi

echo "PASSED"
