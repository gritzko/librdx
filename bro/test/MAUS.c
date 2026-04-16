#include "bro/MAUS.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

typedef struct {
    const char *input;     // raw escape sequence
    int  expect_consumed;  // bytes parsed (0 = invalid/incomplete)
    u8   type;
    u8   button;
    u16  row;
    u16  col;
} MAUSCase;

static const MAUSCase CASES[] = {
    // --- Press ---
    {"\x1b[<0;5;10M",   10, MAUS_PRESS,   MAUS_LEFT,  10, 5},
    {"\x1b[<1;1;1M",     9, MAUS_PRESS,   MAUS_MID,    1, 1},
    {"\x1b[<2;80;24M",  11, MAUS_PRESS,   MAUS_RIGHT, 24, 80},

    // --- Release ---
    {"\x1b[<0;5;10m",   10, MAUS_RELEASE, MAUS_LEFT,  10, 5},
    {"\x1b[<2;80;24m",  11, MAUS_RELEASE, MAUS_RIGHT, 24, 80},

    // --- Drag (button + 32) ---
    {"\x1b[<32;5;10M",  11, MAUS_DRAG,    MAUS_LEFT,  10, 5},
    {"\x1b[<34;5;10M",  11, MAUS_DRAG,    MAUS_RIGHT, 10, 5},

    // --- Wheel ---
    {"\x1b[<64;5;10M",  11, MAUS_WHEEL,   MAUS_UP,    10, 5},
    {"\x1b[<65;5;10M",  11, MAUS_WHEEL,   MAUS_DOWN,  10, 5},
    {"\x1b[<64;1;1M",   10, MAUS_WHEEL,   MAUS_UP,     1, 1},

    // --- Multi-digit fields ---
    {"\x1b[<0;120;200M",13, MAUS_PRESS,   MAUS_LEFT,  200, 120},

    // --- Invalid: too short ---
    {"\x1b[<0",          0, 0, 0, 0, 0},
    {"\x1b[<",           0, 0, 0, 0, 0},
    {"\x1b[<0;5",        0, 0, 0, 0, 0},
    {"\x1b[<0;5;10",     0, 0, 0, 0, 0},

    // --- Invalid: wrong prefix ---
    {"\x1b[A0;5;10M",    0, 0, 0, 0, 0},
    {"[<0;5;10M",        0, 0, 0, 0, 0},

    // --- Invalid: missing terminator ---
    {"\x1b[<0;5;10X",    0, 0, 0, 0, 0},

    // --- Invalid: garbage in number field ---
    {"\x1b[<a;5;10M",    0, 0, 0, 0, 0},
};

#define NCASES (sizeof(CASES) / sizeof(CASES[0]))

ok64 MAUSTestTable() {
    sane(1);
    for (size_t i = 0; i < NCASES; i++) {
        const MAUSCase *tc = &CASES[i];
        int ilen = (int)strlen(tc->input);
        // Special handling for the NUL-bearing test cases (none here)

        MAUSevent ev = {};
        int got = MAUSParse(&ev, (u8 const *)tc->input, ilen);

        if (got != tc->expect_consumed) {
            fprintf(stderr,
                    "FAIL [%zu] '%s' (len %d): consumed got %d want %d\n",
                    i, tc->input, ilen, got, tc->expect_consumed);
            fail(TESTFAIL);
        }
        if (got == 0) continue;  // failure cases — fields don't matter

        if (ev.type != tc->type) {
            fprintf(stderr, "FAIL [%zu] '%s': type got %u want %u\n",
                    i, tc->input, ev.type, tc->type);
            fail(TESTFAIL);
        }
        if (ev.button != tc->button) {
            fprintf(stderr, "FAIL [%zu] '%s': button got %u want %u\n",
                    i, tc->input, ev.button, tc->button);
            fail(TESTFAIL);
        }
        if (ev.row != tc->row) {
            fprintf(stderr, "FAIL [%zu] '%s': row got %u want %u\n",
                    i, tc->input, ev.row, tc->row);
            fail(TESTFAIL);
        }
        if (ev.col != tc->col) {
            fprintf(stderr, "FAIL [%zu] '%s': col got %u want %u\n",
                    i, tc->input, ev.col, tc->col);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 MAUStest() {
    sane(1);
    call(MAUSTestTable);
    done;
}

TEST(MAUStest);
