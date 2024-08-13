#ifndef ABC_MARK_H
#define ABC_MARK_H

#include "01.h"
#include "INT.h"
#include "OK.h"

con ok64 MARKfail = 0xc2d96a51b296;

typedef enum {
    MARK_H1 = '1',
    MARK_H2 = '2',
    MARK_H3 = '3',
    MARK_H4 = '4',
    MARK_HLINE = '~',
    MARK_COMA = ',',
    MARK_INDENT = ' ',
    MARK_OLIST = '.',
    MARK_ULIST = '-',
    MARK_LIST = '*',
    MARK_LINK = '[',
    MARK_QUOTE = '>',
} markdiv;

con u8 MARK_BITS = 4;

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

fun ok64 _MARKpushdiv(MARKstate* state, u8 div) {
    if (state->divlen >= 8) return noroom;
    state->div._8[state->divlen] = div;
    ++state->divlen;
    return OK;
}

fun ok64 _MARKhline($cu8c text, $cu8c tok, MARKstate* state) {
    _MARKpushdiv(state, MARK_HLINE);
    return OK;
}

fun ok64 _MARKolist($cu8c text, $cu8c tok, MARKstate* state) {
    fprintf(stderr, "olist\n");
    _MARKpushdiv(state, MARK_OLIST);
    return OK;
}

fun ok64 _MARKulist($cu8c text, $cu8c tok, MARKstate* state) {
    fprintf(stderr, "ulist\n");
    _MARKpushdiv(state, MARK_ULIST);
    return OK;
}

fun ok64 _MARKdiv($cu8c text, $cu8c tok, MARKstate* state) {
    return Bu64feed1(state->divs, state->div._64[0]);
}

fun ok64 _MARKline($cu8c text, $cu8c tok, MARKstate* state) {
    fprintf(stderr, "line\n");
    state->div._64[0] = 0;
    state->divlen = 0;
    return Bu8cpfeed1(state->lines, tok[0]);
}

ok64 MARKlexer(MARKstate* state);

ok64 MARK2lexer(MARKstate* state);

ok64 MARK2lex(MARKstate* state, $u8c text);

ok64 MARKparse(MARKstate* state);

ok64 MARKhtml($u8 into, MARKstate const* state);

ok64 MARKansi($u8 into, MARKstate const* state);

fun ok64 _MARKh1($cu8c text, $cu8c tok, MARKstate* state) {
    fprintf(stderr, "h1\n");
    _MARKpushdiv(state, MARK_H1);
    return OK;
}
fun ok64 _MARKh2($cu8c text, $cu8c tok, MARKstate* state) {
    fprintf(stderr, "h2\n");
    _MARKpushdiv(state, MARK_H2);
    return OK;
}
fun ok64 _MARKh3($cu8c text, $cu8c tok, MARKstate* state) {
    fprintf(stderr, "h3\n");
    _MARKpushdiv(state, MARK_H3);
    return OK;
}
fun ok64 _MARKh4($cu8c text, $cu8c tok, MARKstate* state) {
    fprintf(stderr, "h4\n");
    _MARKpushdiv(state, MARK_H4);
    return OK;
}
fun ok64 _MARKindent($cu8c text, $cu8c tok, MARKstate* state) {
    fprintf(stderr, "ind\n");
    _MARKpushdiv(state, MARK_INDENT);
    return OK;
}
fun ok64 _MARKh($cu8c text, $cu8c tok, MARKstate* state) { return OK; }
fun ok64 _MARKlndx($cu8c text, $cu8c tok, MARKstate* state) { return OK; }
fun ok64 _MARKlink($cu8c text, $cu8c tok, MARKstate* state) { return OK; }
fun ok64 _MARKnest($cu8c text, $cu8c tok, MARKstate* state) { return OK; }
fun ok64 _MARKterm($cu8c text, $cu8c tok, MARKstate* state) { return OK; }
fun ok64 _MARKroot($cu8c text, $cu8c tok, MARKstate* state) { return OK; }

#endif
