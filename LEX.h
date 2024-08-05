#ifndef ABC_LEX_H
#define ABC_LEX_H

#include "INT.h"
#include "PRO.h"

con ok64 LEXfail = 0x30b65aa1395;
con ok64 LEXnoroom = 0x31cf3db3ca1395;

typedef struct {
    $u8c text;
    int cs;
    int tbc;
    u8c$ mod;
    u8$ enm;
    u8$ fns;
    u8$ act;
    u8$ syn;
    $u8c cur;
    u32 ruleno;
} LEXstate;

fun ok64 _LEXname($cu8c text, $cu8c tok, LEXstate *state) {
    $u8feed(state->syn, state->mod);
    return $u8feed(state->syn, tok);
}
fun ok64 _LEXop($cu8c text, $cu8c tok, LEXstate *state) {
    return $u8feed(state->syn, tok);
}
fun ok64 _LEXclass($cu8c text, $cu8c tok, LEXstate *state) {
    return $u8feed(state->syn, tok);
}
fun ok64 _LEXstring($cu8c text, $cu8c tok, LEXstate *state) {
    return $u8feed(state->syn, tok);
}
fun ok64 _LEXspace($cu8c text, $cu8c tok, LEXstate *state) {
    return $u8feed(state->syn, tok);
}

fun ok64 _LEXentity($cu8c text, $cu8c tok, LEXstate *state) { return OK; }
fun ok64 _LEXexpr($cu8c text, $cu8c tok, LEXstate *state) { return OK; }
fun ok64 _LEXeq($cu8c text, $cu8c tok, LEXstate *state) {
    a$strc(t, " = ( ");
    return $u8feed(state->syn, t);
}

fun ok64 _LEXrulename($cu8c text, $cu8c tok, LEXstate *state) {
    $set(state->cur, tok);
    u8c$ mod = state->mod;
    state->ruleno++;

    a$strc(tmpl,
           "action $s$s0 { lexpush($s$s); }\n"
           "action $s$s1 { lexpop($s$s); call(_$s$s, text, tok, state); }\n");
    $feedf(state->act, tmpl, mod, tok, mod, tok, mod, tok, mod, tok, mod, tok);

    a$strc(enmtmpl, "\t$s$s = $s+$u,\n");
    $feedf(state->enm, enmtmpl, mod, tok, mod, state->ruleno);

    a$strc(fnstmpl, "ok64 _$s$s ($$cu8c text, $$cu8c tok, $sstate* state);\n");
    $feedf(state->fns, fnstmpl, mod, tok, mod);
    return OK;
}
fun ok64 _LEXline($cu8c text, $cu8c tok, LEXstate *state) {
    size_t l = (4 + $len(tok) + 2) * 2 + 4;
    if ($len(state->syn) < l) return LEXnoroom;
    u8c$ mod = state->mod;
    u8c$ cur = state->cur;
    a$strc(tmpl, " ) >$s$s0 %$s$s1;\n");
    $feedf(state->syn, tmpl, mod, cur, mod, cur);
    return OK;
}
fun ok64 _LEXroot($cu8c text, $cu8c tok, LEXstate *state) { return OK; }

ok64 LEXlexer(LEXstate *state);

#endif
