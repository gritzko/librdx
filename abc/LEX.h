#ifndef ABC_LEX_H
#define ABC_LEX_H

#include "INT.h"

con ok64 LEXfail = 0x30b65aa1395;
con ok64 LEXnoroom = 0x31cf3db3ca1395;

#define LEXenum 0

#define LEX_TEMPL_C 0
#define LEX_TEMPL_GO 1
#define LEX_TEMPL_LANG_LEN 2

#define LEX_TEMPL_LEN 10

extern const u8c *LEX_TEMPL[LEX_TEMPL_LANG_LEN][LEX_TEMPL_LEN][2];

typedef struct {
    $u8c text;
    int lang;
    u8B ct;

    u8c$ mod;

    $u8c cur;
    u32 ruleno;
} LEXstate;

ok64 LEXlexer(LEXstate *state);

#endif
