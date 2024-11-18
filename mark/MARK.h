#ifndef ABC_MARK_H
#define ABC_MARK_H

#include <stdint.h>

#include "abc/01.h"
#include "abc/INT.h"
#include "MARQ.h"
#include "abc/OK.h"

con ok64 MARKfail = 0xc2d96a51b296;
con ok64 MARKnoroom = 0xc73cf6cf251b296;
con ok64 MARKbadrec = 0x9e9da896651b296;
con ok64 MARKmiss = 0xdf7b7151b296;
#define MARKenum 0

typedef enum {
    MARK_H1 = 1,
    MARK_H2 = 2,
    MARK_H3 = 3,
    MARK_H4 = 4,
    MARK_HLINE = 5,
    MARK_CODE = 6,
    MARK_INDENT = 7,
    MARK_OLIST = 8,
    MARK_ULIST = 9,
    MARK_LINK = 10,
    MARK_QUOTE = 11,
    MARK_END = 12,
} MARKdiv;

extern char MARKdivascii[];
extern $u8c MARKdivcanon[];

typedef struct {
    // The wiki-text to be parsed.
    $u8c text;
    // Inline formatting
    $u8 fmt;
    // Current line div stack.
    u64 _div;
    // Each one points to the first character of a line,
    // likely one after '\n'. The last one is $term(text).
    u8cpB lineB;
    // Line div stacks, indices match those of `lines`.
    u64B divB;
    u64B pB;
} MARKstate;

fun u8c$ MARKline(MARKstate const* state, u64 ndx) {
    return state->lineB[0] + ndx;
}

ok64 MARKlexer(MARKstate* state);

ok64 MARKMARQ(MARKstate* state);

ok64 MARKHTML($u8 into, MARKstate const* state);

ok64 MARKANSI($u8 into, u32 width, MARKstate const* state);

#endif
