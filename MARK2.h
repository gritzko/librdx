#ifndef ABC_MARK2_H
#define ABC_MARK2_H

#include "01.h"
#include "FILE.h"
#include "INT.h"
#include "MARK.h"

con ok64 MARK2fail = 0x30b65a8251b296;

// max 30 bits
typedef enum {
    MARK2_PLAIN = 0,
    MARK2_CODE = 1,
    MARK2_LINK = 2,
    MARK2_STRESS = 3,
    MARK2_EMPH = 4,
} markline;

typedef u64 link64;

typedef MARKstate MARK2state;

ok64 MARK2lexer(MARK2state* state);

fun ok64 _MARK2sp($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
fun ok64 _MARK2nl($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
fun ok64 _MARK2nonl($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
fun ok64 _MARK2space($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
fun ok64 _MARK2plain($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
fun ok64 _MARK2ref($cu8c text, $cu8c tok, MARK2state* state) { return OK; }

fun ok64 _MARK2em($cu8c text, $cu8c tok, MARK2state* state) {
    $print(text);
    a$str(s, "em: ");
    $print(s);
    $print(tok);
    printf("\nlen %lu f%lu t%lu\n", $len(tok), tok[0] - text[0],
           tok[1] - text[0]);
    for (u64 p = tok[0] - state->doc[0] + 1; p < tok[1] - state->doc[0]; ++p)
        *Batp(state->fmt, p) |= MARK2_EMPH;
    return OK;
}

fun ok64 _MARK2toemph($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
fun ok64 _MARK2emph($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
fun ok64 _MARK2inline($cu8c text, $cu8c tok, MARK2state* state) { return OK; }
fun ok64 _MARK2root($cu8c text, $cu8c tok, MARK2state* state) { return OK; }

#endif
