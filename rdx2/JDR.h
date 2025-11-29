//
// Created by gritzko on 11/20/25.
//
#include "RDX.h"

typedef rdx JDRstate;

con ok64 JDRBAD = 0x4cd6cb28d;

fun u8 JDRPeek(utf8cs tok, rdxp x) { return tok[1] < x->data[1] ? *tok[1] : 0; }

// user functions (callbacks) for the parser
fun ok64 JDRonNL(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonUtf8cp1(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonUtf8cp2(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonUtf8cp3(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonUtf8cp4(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonInt(utf8cs tok, rdxp x) {
    x->type = RDX_TYPE_INT;
    i64decdrain(&x->i, tok);
    return OK;
}
fun ok64 JDRonFloat(utf8cs tok, rdxp x) {
    u64 l = $len(tok);
    if (unlikely(l > 32)) return RDXBAD;
    utf8sDrainFloat(tok, &x->f);
    x->type = RDX_TYPE_FLOAT;
    return OK;
}
fun ok64 JDRonTerm(utf8cs tok, rdxp x) {
    x->type = RDX_TYPE_TERM;
    $mv(x->t, tok);
    return OK;
}
fun ok64 JDRonRef(utf8cs tok, rdxp x) {
    x->type = RDX_TYPE_REF;
    RDXutf8sDrainID(tok, &x->r);
    return OK;
}
fun ok64 JDRonString(utf8cs tok, rdxp x) {
    $mv(x->s, tok);
    x->type = RDX_TYPE_STRING;
    x->cformat = RDX_UTF_ENC_UTF8_ESC;
    return OK;
}
fun ok64 JDRonMLString(utf8cs tok, rdxp x) {
    $mv(x->s, tok);
    x->type = RDX_TYPE_STRING;
    x->cformat = RDX_UTF_ENC_UTF8_ESC_ML;  // ?
    return OK;
}
fun ok64 JDRonStamp(utf8cs tok, rdxp x) { return RDXutf8sDrainID(tok, &x->id); }
fun ok64 JDRonNoStamp(utf8cs tok, rdxp x) {
    zero(x->id);
    return OK;
}
fun ok64 JDRonInlineComma(utf8cs tok, rdxp x) { return RDXBADNEST; }
fun ok64 JDRonComma(utf8cs tok, rdxp x) {
    if (x->format == RDX_FORMAT_JDR_PIN) return JDRonInlineComma(tok, x);
    if (x->len & 1) {
        x->type = 0;
        x->len += 1;
        return OK;
    } else {
        x->type = RDX_TYPE_TUPLE;
        x->cformat = RDX_FORMAT_JDR_PIN;
        x->len += 2;
        $null(x->plex);
        return NEXT;
    }
}
fun ok64 JDRonInlineColon(utf8cs tok, rdxp x) {
    if (x->len & 1) {
        x->type = 0;
        x->len += 1;
        return OK;
    } else {
        x->type = RDX_TYPE_TUPLE;
        x->cformat = RDX_FORMAT_JDR_PIN;
        x->len += 2;
        $null(x->plex);
        return NEXT;
    }
}
fun ok64 JDRonColon(utf8cs tok, rdxp x) {
    if (x->format == RDX_FORMAT_JDR_PIN) return JDRonInlineColon(tok, x);
    x->type = RDX_TYPE_TUPLE;
    x->cformat = RDX_FORMAT_JDR_PIN;
    x->len += 2;
    $null(x->plex);
    return NEXT;
}
fun ok64 JDRonInlineOpen(utf8cs tok, rdxp x) { return NOTIMPLYET; }
fun ok64 JDRonOpen(utf8cs tok, rdxp x) {
    if (x->format == RDX_FORMAT_JDR_PIN) {
        if (x->len == 0) return RDXBAD;
    }
    x->type = RDX_TYPE_BRACKET_REV[**tok];
    x->plex[0] = tok[1];
    x->plex[1] = x->data[1];
    x->cformat = RDX_FORMAT_JDR;
    return NEXT;
}
fun ok64 JDRonInlineClose(utf8cs tok, rdxp x) {  // ????
    x->data[0] = tok[0];                         // backtrack
    x->type = 0;
    return END;
}
fun ok64 JDRonClose(utf8cs tok, rdxp x) {
    if (x->format == RDX_FORMAT_JDR_PIN) return RDXBADNEST;
    if (x->type != RDX_TYPE_BRACKET_REV[**tok]) return RDXBADNEST;
    x->type = 0;
    // FIXME peek?  test   1:(2, 3) vs 1:(2, 3):4:5
    return END;
}
fun ok64 JDRonInter(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonInlineFIRST(utf8cs tok, rdxp x) {
    if (x->len & 1) {
        return RDXBAD;  // ?
    }
    if (JDRPeek(tok, x) != ':') {
        x->data[1] = tok[1];
    }
    x->len += 1;
    return NEXT;
}
fun ok64 JDRonFIRST(utf8cs tok, rdxp x) {
    if (x->format == RDX_FORMAT_JDR_PIN) return JDRonInlineFIRST(tok, x);
    if (x->len & 1) {
        x->len += 2;
    } else {
        x->len += 1;
    }
    if (JDRPeek(tok, x) == ':') {
        x->data[0] = tok[0];
        x->type = RDX_TYPE_TUPLE;
        x->cformat = RDX_FORMAT_JDR_PIN;
        x->plex[0] = tok[0];
        x->plex[1] = x->data[1];
    }
    return NEXT;
}
fun ok64 JDRonRoot(utf8cs tok, rdxp x) { return OK; }
