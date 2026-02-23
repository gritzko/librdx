#ifndef ABC_LEX_H
#define ABC_LEX_H

#include "BUF.h"

con ok64 LEXFAIL = 0x153a13ca495;
con ok64 LEXnoroom = 0x153a1cb3db3cf1;

#define LEXenum 0

#define LEX_TEMPL_C 0
#define LEX_TEMPL_GO 1
#define LEX_TEMPL_LANG_LEN 2

#define LEX_TEMPL_LEN 10

extern const u8c *LEX_TEMPL[LEX_TEMPL_LANG_LEN][LEX_TEMPL_LEN][2];

typedef struct {
    u8cs data;
    int lang;
    u8bp ct;

    u8c$ mod;

    u8cs cur;
    u32 ruleno;
} LEXstate;

ok64 LEXLexer(LEXstate *state);

#endif
