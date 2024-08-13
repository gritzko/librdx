#ifndef ABC_MARK2_H
#define ABC_MARK2_H

#include "01.h"
#include "INT.h"
#include "MARK.h"

con ok64 MARK2fail = 0x30b65a8251b296;

// max 30 bits
typedef enum {
    MARK_LINE_PLAIN,
    MARK_LINE_CODE = 1,
    MARK_LINE_LINK = 2,
    MARK_LINE_STRESS = 4,
    MARK_LINE_EMPH = 8,
} markline;

typedef u64 link64;

typedef MARKstate MARK2state;

ok64 MARK2lexer(MARK2state* state);

ok64 _MARK2sp($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2nl($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2nonl($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2space($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2plain($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2ref($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2tostress($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2stress($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2toemph($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2emph($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2inline($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
ok64 _MARK2root($cu8c text, $cu8c tok, MARK2state* state) { return OK; }

#endif
