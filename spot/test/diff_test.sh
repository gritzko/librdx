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
# With word-level comment splitting, "// " is shared EQ prefix.
check_unique "Old comment line about processing"
# Lines only in new should appear at most once (as INS)
check_unique "New comment about the processing step"

if [ "$FAILS" -gt 0 ]; then
    echo "FAILED: $FAILS checks failed"
    echo "--- full output ---"
    echo "$OUT"
    exit 1
fi

echo "PASSED"

# --- inline highlighting (hili) test ---
# When a single token changes mid-line, the context prefix must appear
# exactly once — no duplicate full-line copies.

HILI_OLD="$DATADIR/hili_old.c"
HILI_NEW="$DATADIR/hili_new.c"

HOUT=$("$SPOT" --diff "$HILI_OLD" "$HILI_NEW" 2>&1 | perl -pe 's/\e\[[0-9;]*m//g')
HFAILS=0

echo "=== inline highlight test ==="

# The line prefix before the changed token must appear exactly once
check_hili() {
    local line="$1"
    local count
    count=$(echo "$HOUT" | grep -cF "$line" || true)
    if [ "$count" -gt 1 ]; then
        echo "FAIL: '$line' appears $count times (expected 1)"
        HFAILS=$((HFAILS + 1))
    elif [ "$count" -eq 0 ]; then
        echo "FAIL: '$line' not found in output"
        HFAILS=$((HFAILS + 1))
    else
        echo "  OK: '$line'"
    fi
}

check_hili "u32 tlo = (ti > 0) ?"
check_hili "u32 thi ="
check_hili "process(tlo, thi);"

if [ "$HFAILS" -gt 0 ]; then
    echo "FAILED: $HFAILS inline highlight checks failed"
    echo "--- full hili output ---"
    echo "$HOUT"
    exit 1
fi

echo "PASSED"

# --- Multi-hunk coloring test ---
# Tok/hili offsets must be hunk-text-relative. Before the fix, second and
# subsequent hunks had absolute offsets, making all bytes the same color.
# Here two change regions are separated by 8 unchanged lines (> 2*CTX_LINES),
# producing two hunks. With SPOT_COLOR=1 we verify both have correct ANSI.

MULTI_OLD="$DATADIR/multi_old.c"
MULTI_NEW="$DATADIR/multi_new.c"

MOUT=$(SPOT_COLOR=1 "$SPOT" --diff "$MULTI_OLD" "$MULTI_NEW" 2>&1)
MFAILS=0

echo "=== multi-hunk coloring test ==="

# ESC[48;5;217m = bg 217 (DEL), ESC[48;5;157m = bg 157 (INS)
BG_DEL=$(printf '\033[48;5;217m')
BG_INS=$(printf '\033[48;5;157m')

check_multi_color() {
    local label="$1"
    local pattern="$2"
    local count
    count=$(echo "$MOUT" | grep -cF "$pattern" || true)
    if [ "$count" -eq 0 ]; then
        echo "FAIL: $label not found"
        MFAILS=$((MFAILS + 1))
    else
        echo "  OK: $label"
    fi
}

# Hunk 1: MSTOpen has DEL bg, HITOpen has INS bg
check_multi_color "hunk1 DEL bg on MSTOpen" "${BG_DEL}MSTOpen"
check_multi_color "hunk1 INS bg on HITOpen" "${BG_INS}HITOpen"

# Hunk 2: MSTSeek has DEL bg, HITSeek has INS bg
check_multi_color "hunk2 DEL bg on MSTSeek" "${BG_DEL}MSTSeek"
check_multi_color "hunk2 INS bg on HITSeek" "${BG_INS}HITSeek"

# Hunk 2: keywords must have syntax color (ESC[94m = bright blue)
KW_BLUE=$(printf '\033[94m')
check_multi_color "hunk2 keyword coloring" "${KW_BLUE}if"

if [ "$MFAILS" -gt 0 ]; then
    echo "FAILED: $MFAILS multi-hunk coloring checks failed"
    echo "--- raw output (cat -v) ---"
    echo "$MOUT" | cat -v
    exit 1
fi

echo "PASSED"

# --- NEIL cascade test ---
# When consecutive lines change (MSTXxx -> HITXxx), the unchanged arguments
# like "(heap, &count);" must NOT be duplicated as DEL+INS.  NEIL must
# protect EQ regions that contain a code line before the newline.

NEIL_OLD="$DATADIR/neil_old.c"
NEIL_NEW="$DATADIR/neil_new.c"

NOUT=$("$SPOT" --diff "$NEIL_OLD" "$NEIL_NEW" 2>&1 | perl -pe 's/\e\[[0-9;]*m//g')
NFAILS=0

echo "=== NEIL cascade test ==="

check_neil() {
    local line="$1"
    local count
    count=$(echo "$NOUT" | grep -cF "$line" || true)
    if [ "$count" -gt 1 ]; then
        echo "FAIL: '$line' appears $count times (expected 1)"
        NFAILS=$((NFAILS + 1))
    elif [ "$count" -eq 0 ]; then
        echo "FAIL: '$line' not found in output"
        NFAILS=$((NFAILS + 1))
    else
        echo "  OK: '$line'"
    fi
}

# Shared arguments between renamed calls must appear once (EQ), not twice
check_neil "(heap, &count);"
check_neil "(heap, &index);"
check_neil "(heap, &value);"

if [ "$NFAILS" -gt 0 ]; then
    echo "FAILED: $NFAILS NEIL cascade checks failed"
    echo "--- full output ---"
    echo "$NOUT"
    exit 1
fi

echo "PASSED"
