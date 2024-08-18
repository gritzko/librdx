#include "MARK2.h"

ok64 MARK2onRef0($cu8c tok, MARK2state* state) { return OK; }

ok64 MARK2onRef1($cu8c tok, MARK2state* state) { return OK; }

ok64 MARK2onEm($cu8c tok, MARK2state* state) {
    a$str(s, "```\n");
    $print(s);
    $print(tok);
    $print(s);
    size_t f = tok[0] - state->doc[0];
    size_t t = tok[1] - state->doc[0];
    for (size_t i = f; i < t; ++i) Bat(state->fmt, i) |= 1 << MARK2_EMPH;
    return OK;
}

ok64 MARK2onStA0($cu8c tok, MARK2state* state) {
    a$str(s, "<<<\n");
    $print(s);
    $print(tok);
    $print(s);
    size_t f = tok[0] - state->doc[0];
    state->mark2[MARK2_STRONG] = f + 1;
    return OK;
}

ok64 MARK2onStA1($cu8c tok, MARK2state* state) {
    a$str(s, ">>>\n");
    $print(s);
    $print(tok);
    $print(s);
    size_t t = tok[0] - state->doc[0] + 2;
    size_t f = state->mark2[MARK2_STRONG];
    state->mark2[MARK2_STRONG] = 0;
    for (size_t i = f; i < t; ++i) Bat(state->fmt, i) |= 1 << MARK2_STRONG;
    return OK;
}

ok64 MARK2onStB($cu8c tok, MARK2state* state) {
    a$str(s, "```\n");
    $print(s);
    $print(tok);
    $print(s);
    size_t f = tok[0] - state->doc[0];
    size_t t = tok[1] - state->doc[0];
    for (size_t i = f; i < t; ++i) Bat(state->fmt, i) |= MARK2_STRONG;
    return OK;
}
ok64 MARK2onRoot($cu8c tok, MARK2state* state) { return OK; }

fun ok64 _MARK2em1($cu8c text, $cu8c tok, MARK2state* state) {
    printf("\nlen %lu f%lu t%lu\n", $len(tok), tok[0] - text[0],
           tok[1] - text[0]);
    for (u64 p = tok[0] - state->doc[0] + 1; p < tok[1] - state->doc[0]; ++p)
        *Batp(state->fmt, p) |= MARK2_EMPH;
    return OK;
}

fun ok64 _MARK2em2($cu8c text, $cu8c tok, MARK2state* state) {
    $print(text);
    a$str(s, "em2: ");
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
