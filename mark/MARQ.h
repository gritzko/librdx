#ifndef ABC_MARQ_H
#define ABC_MARQ_H

#include "abc/01.h"
#include "abc/ANSI.h"
#include "abc/FILE.h"
#include "abc/INT.h"
#include "abc/LEX.h"

con ok64 MARQfail = 0x58a6daaa5b70;
#define MARQenum 16

// max 8 bits
typedef enum {
    MARQ_MARKUP = 0,
    MARQ_CODE = 1,
    MARQ_LINK = 2,
    MARQ_STRONG = 3,
    MARQ_EMPH = 4,
    MARQ_MAX_FMT = 5,
} MARQfmt;

con u8 MARQesc[] = {GRAY, DARK_GREEN, UNDERLINE, BOLD, HIGHLIGHT};

typedef u64 link64;

#define MARQ_MAX_OPEN_BRACKETS 31

typedef struct {
    u8cs text;
    $u8 fmt;

    // 0 ~ nothing
    u64 brackets[MARQ_MAX_OPEN_BRACKETS + 1];
} MARQstate;

ok64 MARQlexer(MARQstate* state);

ok64 MARQANSI($u8 $into, u8cs const txt, $u8c const fmt);

ok64 MARQHTML($u8 $into, u8cs txt, $u8c fmt);

#endif
