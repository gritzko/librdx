#ifndef ABC_LEX_H
#define ABC_LEX_H

#include "INT.h"
#include "PRO.h"

con ok64 LEXfail = 0x30b65aa1395;
con ok64 LEXnoroom = 0x31cf3db3ca1395;

#define LEXenum 0

typedef struct {
    $u8c text;

    u8c$ mod;

    u8$ enm;
    u8$ fns;
    u8$ act;
    u8$ syn;
    $u8c cur;
    u32 ruleno;
} LEXstate;

ok64 LEXlexer(LEXstate *state);

#endif
