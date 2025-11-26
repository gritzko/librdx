//
// Created by gritzko on 11/20/25.
//
#include "JDR.h"

fun void firstCheck(utf8cs tok, JDRstate* state) {
    state->len = (state->len & ~1) + 2;
    b8 inlinep =
        state->prnt == RDX_TYPE_TUPLE && (state - 1)->enc;
    b8 colon = tok[1] < state->data[1] && *tok[1] == ':';
    if (inlinep==colon) return;
    if (colon) {
        state->plex[0] = tok[0];
        state->plex[1] = state->data[1];
        state->enc = state->type;
        state->type = RDX_TYPE_TUPLE;
        state->data[0] = tok[0];
    } else {
        state->data[0] = state->data[1];
    }
}
// user functions (callbacks) for the parser
ok64 JDRonNL(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonUtf8cp1(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonUtf8cp2(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonUtf8cp3(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonUtf8cp4(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonInt(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_INT;
    i64decdrain(&state->i, tok);
    return OK;
}
ok64 JDRonFloat(utf8cs tok, JDRstate* state) {
    u64 l = $len(tok);
    if (unlikely(l > 32)) return RDXBAD;
    utf8sDrainFloat(tok, &state->f);
    state->type = RDX_TYPE_FLOAT;
    return OK;
}
ok64 JDRonTerm(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_TERM;
    $mv(state->t, tok);
    return OK;
}
ok64 JDRonRef(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_REF;
    RDXutf8sDrainID(tok, &state->r);
    return OK;
}
ok64 JDRonString(utf8cs tok, JDRstate* state) {
    $mv(state->s, tok);
    state->type = RDX_TYPE_STRING;
    return OK;
}
ok64 JDRonMLString(utf8cs tok, JDRstate* state) {
    $mv(state->s, tok);
    state->type = RDX_TYPE_STRING;
    return OK;
}
ok64 JDRonStamp(utf8cs tok, JDRstate* state) {
    return RDXutf8sDrainID(tok, &state->id);
}
ok64 JDRonNoStamp(utf8cs tok, JDRstate* state) {
    zero(state->id);
    return OK;
}
ok64 JDRonOpenP(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_TUPLE;
    return OK;
}
ok64 JDRonCloseP(utf8cs tok, JDRstate* state) {
    return OK;
}
ok64 JDRonOpenL(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_LINEAR;
    return OK;
}
ok64 JDRonCloseL(utf8cs tok, JDRstate* state) {
    return OK;
}
ok64 JDRonOpenE(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_EULER;
    return OK;
}
ok64 JDRonCloseE(utf8cs tok, JDRstate* state) {
    return OK;
}
ok64 JDRonOpenX(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_MULTIX;
    return OK;
}
ok64 JDRonCloseX(utf8cs tok, JDRstate* state) {
    // FIXME check prnt
    return OK;
}
ok64 JDRonComma(utf8cs tok, JDRstate* state) {
    state->type = 0;
    return OK;
}
ok64 JDRonColon(utf8cs tok, JDRstate* state) {
    state->type = 0;
    return OK;
}
ok64 JDRonOpen(utf8cs tok, JDRstate* state) {
    state->plex[0] = tok[1];
    state->plex[1] = state->data[1];
    return NEXT;
}
ok64 JDRonClose(utf8cs tok, JDRstate* state) {
    state->type = 0;
    return END;
}
ok64 JDRonInter(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonFIRST(utf8cs tok, JDRstate* state) {
    firstCheck(tok, state);
    return NEXT;
}
ok64 JDRonToken(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonRoot(utf8cs tok, JDRstate* state) { return OK; }
