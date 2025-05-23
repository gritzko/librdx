#ifndef DW_C_H
#define DW_C_H

#include "abc/BUF.h"

static const ok64 CLEXfail = 0x3153a1aa5b70;
static const ok64 CLEXnoroom = 0x3153a1cb3db3cf1;

#define CLEXenum 0

#define CLEX_TEMPL_C 0
#define CLEX_TEMPL_GO 1
#define CLEX_TEMPL_LANG_LEN 2

#define CLEX_TEMPL_LEN 10

extern const u8c *CLEX_TEMPL[CLEX_TEMPL_LANG_LEN][CLEX_TEMPL_LEN][2];

typedef struct {
    $u8c text;
    u8$ rdx;
} CLEXstate;

ok64 CLEXlexer(CLEXstate *state);

#endif
