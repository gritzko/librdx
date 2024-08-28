#ifndef ABC_MARK_H
#define ABC_MARK_H

#include <stdint.h>

#include "01.h"
#include "INT.h"
#include "MARQ.h"
#include "OK.h"

con ok64 MARKfail = 0xc2d96a51b296;
con ok64 MARKnospace = 0x99e5d37cf251b296;
#define MARKenum 0

typedef enum {
    MARK_H1 = 1,
    MARK_H2 = 2,
    MARK_H3 = 3,
    MARK_H4 = 4,
    MARK_HLINE = 5,
    MARK_CODE = 6,
    MARK_INDENT = 7,
    MARK_OLIST = 8,
    MARK_ULIST = 9,
    MARK_LINK = 10,
    MARK_QUOTE = 11,
} MARKdiv;

extern char MARKdivascii[];

typedef struct {
    MARQstate marq;

    LEXbase lex;
    size_t _[32];

    $u8 into;

    u64 stack;
    u64 div;
} MARKstate;

ok64 MARKlexer(MARKstate* state);

#endif
