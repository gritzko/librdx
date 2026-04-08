#ifndef TOK_LLT_H
#define TOK_LLT_H

#include "TOK.h"

con ok64 LLTBAD = 0x30ca3d4d;
con ok64 LLTFAIL = 0xc32972895;

typedef struct {
    u8cs data;
    TOKcb cb;
    void *ctx;
} LLTstate;

ok64 LLTLexer(LLTstate *state);

#endif
