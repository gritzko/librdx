#ifndef ABC_LEX_H
#define ABC_LEX_H

#include "INT.h"
#include "PRO.h"

con ok64 LEXfail = 0x30b65aa1395;
con ok64 LEXnoroom = 0x31cf3db3ca1395;

#define LEXenum 0

// can zero this structure to restart the parser;
// that includes the marks array, whatever its size is.
typedef struct {
    $u8c text;
    int cs;
    u64 mark0[0];
} LEXbase;

typedef struct {
    LEXbase lex;
    u64 _[32];

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
