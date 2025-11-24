//
// Created by gritzko on 11/20/25.
//
#include "JDR.h"

con ok64 badsyntax	= 0x26968dfdcb897c;

// user functions (callbacks) for the parser
ok64 JDRonNL(utf8cs tok, JDRstate* state) { state->len++; }
ok64 JDRonUtf8cp1(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonUtf8cp2(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonUtf8cp3(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonUtf8cp4(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonInt(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_INT;
    return i64decdrain(&state->i, tok);
}
ok64 JDRonFloat(utf8cs tok, JDRstate* state) {
    u64 l = $len(tok);
    if (unlikely(l>32)) return badsyntax;
    ok64 o = utf8sDrainFloat(tok, &state->f);
    state->type = RDX_TYPE_FLOAT;
    state->mark = '1';
    return o;
}
ok64 JDRonTerm(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_TERM;
    state->mark = '1';
    $mv(state->t, tok);
    return OK;
}
ok64 JDRonRef(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_REF;
    state->mark = '1';
    return RDXutf8sDrainID(tok, &state->r);
}
ok64 JDRonString(utf8cs tok, JDRstate* state) {
    $mv(state->s, tok);
    state->type = RDX_TYPE_STRING;
    state->mark = '1';
    state->enc = RDX_UTF_ENC_UTF8_ESC;
    return OK;
}
ok64 JDRonMLString(utf8cs tok, JDRstate* state) {
    $mv(state->s, tok);
    state->type = RDX_TYPE_STRING;
    state->mark = '1';
    state->enc = RDX_UTF_ENC_UTF8_ESC_ML;
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
    state->mark = '(';
    return OK;
}
ok64 JDRonCloseP(utf8cs tok, JDRstate* state) {
    state->type = 0;
    state->mark = ')';
    return NODATA;
}
ok64 JDRonOpenL(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_LINEAR;
    state->mark = '[';
    return OK;
}
ok64 JDRonCloseL(utf8cs tok, JDRstate* state) {
    state->type = 0;
    state->mark = ']';
    return NODATA;
}
ok64 JDRonOpenE(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_EULER;
    state->mark = '{';
    return OK;
}
ok64 JDRonCloseE(utf8cs tok, JDRstate* state) {
    state->type = 0;
    state->mark = '}';
    return OK;
}
ok64 JDRonOpenX(utf8cs tok, JDRstate* state) {
    state->type = RDX_TYPE_MULTIX;
    state->mark = '<';
    return OK;
}
ok64 JDRonCloseX(utf8cs tok, JDRstate* state) {
    state->type = 0;
    state->mark = '>';
    return OK;
}
ok64 JDRonComma(utf8cs tok, JDRstate* state) {
    state->type = 0;
    state->mark = ',';
    return OK;
}
ok64 JDRonColon(utf8cs tok, JDRstate* state) {
    state->type = 0;
    state->mark = ':';
    return OK;
}
ok64 JDRonOpen(utf8cs tok, JDRstate* state) {
    state->mark = 0;
    return OK;
}
ok64 JDRonClose(utf8cs tok, JDRstate* state) {
    state->mark = 1;
    return OK;
}
ok64 JDRonInter(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonFIRST(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonToken(utf8cs tok, JDRstate* state) { return OK; }
ok64 JDRonRoot(utf8cs tok, JDRstate* state) { return OK; }
