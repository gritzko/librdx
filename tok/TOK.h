#ifndef TOK_TOK_H
#define TOK_TOK_H

#include "abc/INT.h"

con ok64 TOKBAD = 0x75850b28d;
con ok64 TOKFAIL = 0x1d6143ca495;

// --- Packed u32 token: top 5 bits = tag-'A', bottom 27 bits = end offset ---
// Tags: D=comment, G=string, L=number, H=preproc, R=keyword, P=punct, S=default
#define TOK_OFF_MASK  ((1u << 27) - 1)
#define TOK_OFF(t)    ((t) & TOK_OFF_MASK)
#define TOK_TAG(t)    ((u8)('A' + ((t) >> 27)))
#define TOK_PACK(tag, off) (((u32)((tag) - 'A') << 27) | ((off) & TOK_OFF_MASK))

// Get source slice for token i (tokens are contiguous end offsets).
#define TOK_VAL(out, toks, base, i) do {           \
    u32 _lo = ((i) > 0) ? TOK_OFF((toks)[0][(i) - 1]) : 0; \
    u32 _hi = TOK_OFF((toks)[0][(i)]);              \
    (out)[0] = (base) + _lo;                        \
    (out)[1] = (base) + _hi;                        \
} while(0)

typedef ok64 (*TOKcb)(u8 tag, u8cs tok, void *ctx);

typedef struct {
    u8cs data;
    TOKcb cb;
    void *ctx;
} TOKstate;

ok64 TOKLexer(TOKstate *state, u8csc ext);

#endif
