#ifndef TOK_TOK_H
#define TOK_TOK_H

#include "abc/INT.h"

con ok64 TOKBAD = 0x75850b28d;
con ok64 TOKFAIL = 0x1d6143ca495;

typedef ok64 (*TOKcb)(u8 tag, u8cs tok, void *ctx);

typedef struct {
    u8cs data;
    TOKcb cb;
    void *ctx;
} TOKstate;

ok64 TOKLexer(TOKstate *state, u8csc ext);

#endif
