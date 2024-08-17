#ifndef ABC_MARK_H
#define ABC_MARK_H

#include <stdint.h>

#include "01.h"
#include "INT.h"
#include "OK.h"

con ok64 MARKfail = 0xc2d96a51b296;
con ok64 MARKnospace = 0x99e5d37cf251b296;
#define MARKenum 0
#define MARK2enum 16

typedef enum {
    MARK_H1 = '1',
    MARK_H2 = '2',
    MARK_H3 = '3',
    MARK_H4 = '4',
    MARK_HLINE = '~',
    MARK_P = 'p',
    MARK_COMA = ',',
    MARK_INDENT = '_',
    MARK_OLIST = '.',
    MARK_ULIST = '-',
    MARK_LIST = '*',
    MARK_LINK = '[',
    MARK_QUOTE = '>',
} markdiv;

con u8 MARK_BITS = 4;

con u64 MARK_ALL_INDENTS = 0x2020202020202020;

fun b8 MARKindents(u64 v, u8 tab) {
    u64 expect = u64bytecap(MARK_ALL_INDENTS, tab);
    return u64bytecap(v, tab) == expect;
}

// con u32 MARK_CLOSE = 1U << 31;
con u32 NOT_DIV = 0x1U << 30;

typedef u64 link64;

bitpick(link64, pos, 0, 32);
bitpick(link64, len, 32, 16);
bitpick(link64, lit, 32 + 16, 8);

typedef struct {
    $u8c doc;
    $u8c text;
    int cs;
    int tbc;

    size_t mark0[32];
    size_t mark2[256];  // FIXME

    w64 div;
    u8 divlen;

    Bu8cp lines;
    Bu64 divs;
    Bu8 fmt;
    Bu64 links;
} MARKstate;

ok64 MARKstatealloc(MARKstate* state, $u8c text);
ok64 MARKstatereset(MARKstate* state);
ok64 MARKstatefree(MARKstate* state);

fun u8c$ MARKline$(MARKstate const* state, u64 lno) {
    return state->lines[0] + lno;
}

ok64 MARKparse(MARKstate* state);

ok64 MARKlexer(MARKstate* state);

ok64 MARK2lexer(MARKstate* state);

ok64 MARKhtml($u8 into, MARKstate const* state);

ok64 MARKansi($u8 into, MARKstate const* state);

#endif
