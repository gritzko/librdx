#include "LEX.rl.h"

ok64 LEXonName($cu8c tok, LEXstate *state) {
    $u8feed(state->syn, state->mod);
    return $u8feed(state->syn, tok);
}
ok64 LEXonOp($cu8c tok, LEXstate *state) { return $u8feed(state->syn, tok); }
ok64 LEXonClass($cu8c tok, LEXstate *state) { return $u8feed(state->syn, tok); }
ok64 LEXonRange($cu8c tok, LEXstate *state) { return $u8feed(state->syn, tok); }
ok64 LEXonString($cu8c tok, LEXstate *state) {
    return $u8feed(state->syn, tok);
}
ok64 LEXonQString($cu8c tok, LEXstate *state) {
    return $u8feed(state->syn, tok);
}
ok64 LEXonSpace($cu8c tok, LEXstate *state) { return $u8feed(state->syn, tok); }

ok64 LEXonEntity($cu8c tok, LEXstate *state) { return OK; }
ok64 LEXonExpr($cu8c tok, LEXstate *state) { return OK; }
ok64 LEXonRep($cu8c tok, LEXstate *state) { return OK; }

ok64 LEXonEq($cu8c tok, LEXstate *state) {
    a$strc(t, " = ( ");
    return $u8feed(state->syn, t);
}

ok64 LEXonRuleName($cu8c tok, LEXstate *state) {
    $set(state->cur, tok);
    u8c$ mod = state->mod;
    state->ruleno++;

    if (**tok < 'A' || **tok > 'Z') return OK;

    a$strc(tmpl,
           "action $s$s0 { mark0[$s$s] = p - text[0]; }\n"
           "action $s$s1 {\n"
           "    tok[0] = text[0] + mark0[$s$s];\n"
           "    tok[1] = p;\n"
           "    call($son$s, tok, state); \n"
           "}\n");
    $feedf(state->act, tmpl, mod, tok, mod, tok, mod, tok, mod, tok, mod, tok);

    a$strc(enmtmpl, "\t$s$s = $senum+$u,\n");
    $feedf(state->enm, enmtmpl, mod, tok, mod, state->ruleno);

    a$strc(fnstmpl, "ok64 $son$s ($$cu8c tok, $sstate* state);\n");
    $feedf(state->fns, fnstmpl, mod, tok, mod);
    return OK;
}
ok64 LEXonLine($cu8c tok, LEXstate *state) {
    size_t l = (4 + $len(tok) + 2) * 2 + 4;
    if ($len(state->syn) < l) return LEXnoroom;
    u8c$ mod = state->mod;
    u8c$ cur = state->cur;
    if (**cur >= 'A' && **cur <= 'Z') {
        a$strc(tmpl, " )  >$s$s0 %$s$s1;\n");
        $feedf(state->syn, tmpl, mod, cur, mod, cur);
    } else {
        $u8feed2(state->syn, ' ', ')');
        $u8feed2(state->syn, ';', '\n');
    }
    return OK;
}
ok64 LEXonRoot($cu8c tok, LEXstate *state) { return OK; }
