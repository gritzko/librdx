#ifndef ABC_MARK2_H
#define ABC_MARK2_H

#include "01.h"
#include "FILE.h"
#include "INT.h"
#include "MARK.h"

con ok64 MARK2fail = 0x30b65a8251b296;

// max 8 bits
typedef enum {
    MARK2_MARKUP = 0,
    MARK2_CODE = 1,
    MARK2_LINK = 2,
    MARK2_STRONG = 3,
    MARK2_EMPH = 4,
} markline;

typedef u64 link64;

typedef MARKstate MARK2state;

ok64 MARK2lexer(MARK2state* state);
#endif
