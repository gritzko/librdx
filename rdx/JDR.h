//
// Created by gritzko on 11/20/25.
//
#include "RDX.h"
#include "abc/01.h"

typedef rdx JDRstate;

con ok64 JDRBAD = 0x4cd6cb28d;
con ok64 JDRBADNEST = 0x4cd6cb28d5ce71d;

fun u8 JDRPeek(utf8cs tok, rdxp x) { return tok[1] < x->opt ? *tok[1] : 0; }

// user functions (callbacks) for the parser
fun ok64 JDRonNL(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonUtf8cp1(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonUtf8cp2(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonUtf8cp3(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonUtf8cp4(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonInt(utf8cs tok, rdxp x) {
    if ($len(tok) > 20) return JDRBAD;
    x->type = RDX_TYPE_INT;
    i64decdrain(&x->i, tok);
    return OK;
}
fun ok64 JDRonFloat(utf8cs tok, rdxp x) {
    if ($len(tok) > 24) return JDRBAD;
    utf8sDrainFloat(tok, &x->f);
    ok64 o = rdxVerifyFloat(x->f);
    if (o != OK) return o;
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
    ok64 o = RDXutf8sDrainID(tok, &x->r);
    if (o != OK) return o;
    return rdxVerifyRef(x->r);
}
fun ok64 JDRonString(utf8cs tok, rdxp x) {
    ++tok[0];  // "
    --tok[1];  // "
    $mv(x->s, tok);
    x->type = RDX_TYPE_STRING;
    x->flags = RDX_UTF_ENC_UTF8_ESC;
    return OK;
}
fun ok64 JDRonMLString(utf8cs tok, rdxp x) {
    $mv(x->s, tok);
    x->type = RDX_TYPE_STRING;
    x->flags = RDX_UTF_ENC_UTF8_ESC_ML;  // ?
    return OK;
}
fun ok64 JDRonStamp(utf8cs tok, rdxp x) {
    ++*tok;  // @
    ok64 o = RDXutf8sDrainID(tok, &x->id);
    if (o != OK) return o;
    return rdxVerifyId(x->id);
}
fun ok64 JDRonNoStamp(utf8cs tok, rdxp x) {
    zero(x->id);
    return OK;
}
fun ok64 JDRonInlineComma(utf8cs tok, rdxp x) {
    // End of PIN tuple value - backtrack, end of tuple
    x->type = 0;
    zero(x->r);
    x->next = (u8p)tok[0];  // back up before the comma
    return BACK;
}
fun ok64 JDRonComma(utf8cs tok, rdxp x) {
    if (x->format == RDX_FMT_JDR_PIN) return JDRonInlineComma(tok, x);
    if (x->loc & 1) {
        x->type = 0;
        x->loc += 1;
        return OK;
    } else {
        x->type = RDX_TYPE_TUPLE;
        x->flags = RDX_FMT_JDR_PIN;
        x->loc += 2;
        x->plexc[0] = tok[1];     // After the comma
        x->plexc[1] = x->opt;     // End of data
        return NEXT;
    }
}
fun ok64 JDRonInlineColon(utf8cs tok, rdxp x) {
    if (x->loc & 1) {
        x->type = 0;
        x->loc += 1;
        return OK;
    } else {
        x->type = RDX_TYPE_TUPLE;
        x->flags = RDX_FMT_JDR_PIN;
        x->loc += 2;
        x->plexc[0] = tok[1];     // After the colon
        x->plexc[1] = x->opt;     // End of data
        return NEXT;
    }
}
fun ok64 JDRonColon(utf8cs tok, rdxp x) {
    if (x->format == RDX_FMT_JDR_PIN) return JDRonInlineColon(tok, x);
    $null(x->plexc);
    x->type = 0;
    return ok64sub(RDXBAD, RON_k);  // : can legally occur either after FIRST key or in PIN mode
}
fun ok64 JDRonInlineOpen(utf8cs tok, rdxp x) { return NOTIMPLYET; }
fun ok64 JDRonOpen(utf8cs tok, rdxp x) {
    if (x->format == RDX_FMT_JDR_PIN) {
        if (x->loc == 0) return ok64sub(RDXBAD, RON_l);
    }
    // Increment len like JDRonFIRST does
    if (x->loc & 1) {
        x->loc += 2;
    } else {
        x->loc += 1;
    }
    x->type = RDX_TYPE_BRACKET_REV[**tok];
    x->plexc[0] = tok[1];
    x->plexc[1] = x->opt;
    x->flags = RDX_FMT_JDR;
    return NEXT;
}
fun ok64 JDRonInlineClose(utf8cs tok, rdxp x) {
    x->type = 0;
    x->next = (u8p)tok[0];  // back up before the close bracket
    return BACK;
}
fun ok64 JDRonClose(utf8cs tok, rdxp x) {
    if (x->format == RDX_FMT_JDR_PIN) {
        return JDRonInlineClose(tok, x);
    }
    if (x->ptype != RDX_TYPE_BRACKET_REV[**tok]) {
        return RDXBADNEST;
    }
    x->type = 0;
    x->loc = u32max;
    return END;
}
fun ok64 JDRonInter(utf8cs tok, rdxp x) { return OK; }
fun ok64 JDRonInlineFIRST(utf8cs tok, rdxp x) {
    if (x->loc & 1) {
        return ok64sub(RDXBAD, RON_m);  // ?
    }
    if (JDRPeek(tok, x) != ':') {
        x->opt = (u8p)tok[1];
    }
    x->loc += 1;
    return NEXT;
}
fun ok64 JDRonFIRST(utf8cs tok, rdxp x) {
    if (x->format == RDX_FMT_JDR_PIN) return JDRonInlineFIRST(tok, x);
    if (x->loc & 1) {
        x->loc += 2;
    } else {
        x->loc += 1;
    }
    if (JDRPeek(tok, x) == ':') {
        x->next = (u8p)tok[0];
        x->type = RDX_TYPE_TUPLE;
        x->flags = RDX_FMT_JDR_PIN;
        x->plexc[0] = tok[0];
        x->plexc[1] = x->opt;
    }
    return NEXT;
}
fun ok64 JDRonRoot(utf8cs tok, rdxp x) { return OK; }
