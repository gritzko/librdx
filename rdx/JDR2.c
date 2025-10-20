#include "JDR2.h"

#include "RDX2.h"
#include "abc/PRO.h"

typedef struct {
    u8bp builder;
    rdx cur;
    u64 flags;
} JDRstate;

au8cs(TLV_EMPTY_TUPLE, 'p', 1, 0);

typedef enum {
    JDR_COMMA = 1,
    JDR_COLON = 2,
    JDR_TUPLE = 4,
} JDR_FLAGS;

fun ok64 JDRFlush(JDRstate* state, b8 force) {
    ok64 o = OK;
    utf8sp idle = utf8bIdle(state->builder);
    if (state->cur.type != 0) {
        o = RDXutf8sFeed1(idle, &state->cur);
        state->cur.type = 0;
    } else if (force) {
        o = utf8sFeed(idle, TLV_EMPTY_TUPLE);
    }
    return o;
}

fun ok64 JDRonNL($cu8c tok, JDRstate* state) { return OK; }
fun ok64 JDRonUtf8cp1($cu8c tok, JDRstate* state) { return OK; }
fun ok64 JDRonUtf8cp2($cu8c tok, JDRstate* state) { return OK; }
fun ok64 JDRonUtf8cp3($cu8c tok, JDRstate* state) { return OK; }
fun ok64 JDRonUtf8cp4($cu8c tok, JDRstate* state) { return OK; }
fun ok64 JDRonInt($cu8c tok, JDRstate* state) {
    return RDXutf8sDrainI(tok, &state->cur);
}
fun ok64 JDRonFloat($cu8c tok, JDRstate* state) {
    return RDXutf8sDrainF(tok, &state->cur);
}
fun ok64 JDRonTerm($cu8c tok, JDRstate* state) {
    return RDXutf8sDrainT(tok, &state->cur);
}
fun ok64 JDRonRef($cu8c tok, JDRstate* state) {
    return RDXutf8sDrainR(tok, &state->cur);
}
fun ok64 JDRonString($cu8c tok, JDRstate* state) {
    return RDXutf8sDrainS(tok, &state->cur);
}
fun ok64 JDRonMLString($cu8c tok, JDRstate* state) {
    return RDXutf8sDrainS(tok, &state->cur);
}
fun ok64 JDRonStamp($cu8c tok, JDRstate* state) {
    return RDXutf8sDrainR(tok, &state->cur);
}
fun ok64 JDRonNoStamp($cu8c tok, JDRstate* state) {
    zero(state->cur.id);
    return OK;
}
fun ok64 JDRonOpenP($cu8c tok, JDRstate* state) {
    state->cur.type = RDX_TUPLE;
    return OK;
}
fun ok64 JDRonCloseP($cu8c tok, JDRstate* state) {
    state->cur.type = RDX_TUPLE;
    return OK;
}
fun ok64 JDRonOpenL($cu8c tok, JDRstate* state) {
    state->cur.type = RDX_LINEAR;
    return OK;
}
fun ok64 JDRonCloseL($cu8c tok, JDRstate* state) {
    state->cur.type = RDX_LINEAR;
    return OK;
}
fun ok64 JDRonOpenE($cu8c tok, JDRstate* state) {
    state->cur.type = RDX_EULER;
    return OK;
}
fun ok64 JDRonCloseE($cu8c tok, JDRstate* state) {
    state->cur.type = RDX_EULER;
    return OK;
}
fun ok64 JDRonOpenX($cu8c tok, JDRstate* state) {
    state->cur.type = RDX_MULTIX;
    return OK;
}
fun ok64 JDRonCloseX($cu8c tok, JDRstate* state) {
    state->cur.type = RDX_MULTIX;
    return OK;
}
fun ok64 JDRonComma($cu8c tok, JDRstate* state) {
    ok64 o = OK;
    utf8sp idle = utf8bIdle(state->builder);
    switch (state->flags) {
        case 0:
            state->flags = JDR_COMMA;
            break;
        case JDR_COMMA:
            o = utf8sFeed(idle, TLV_EMPTY_TUPLE);
            break;
        case JDR_TUPLE:
            o = RDXu8bOuto(state->builder, NULL);
            state->flags = JDR_COMMA;
            break;
        case JDR_COLON | JDR_TUPLE:
            o = RDXu8bOuto(state->builder, NULL);
            state->flags = JDR_COMMA;
            break;
        default:
            o = JDRbad;
            break;
    }
    return o;
}

fun ok64 JDRonColon($cu8c tok, JDRstate* state) {
    ok64 o = OK;
    utf8sp idle = utf8bIdle(state->builder);
    switch (state->flags) {
        case 0: {
            rdx wat = {.type = RDX_TUPLE};
            o = RDXu8bInto(state->builder, &wat);
            state->flags = JDR_TUPLE | JDR_COLON;
            break;
        }
        case JDR_COMMA: {
            rdx wat = {.type = RDX_TUPLE};
            o = RDXu8bInto(state->builder, &wat);
            state->flags = JDR_TUPLE | JDR_COLON;
            o = utf8sFeed(idle, TLV_EMPTY_TUPLE);
            break;
        }
        case JDR_TUPLE:
            state->flags |= JDR_COLON;
            break;
        case JDR_COLON | JDR_TUPLE:
            o = utf8sFeed(idle, TLV_EMPTY_TUPLE);
            break;
        default:
            o = JDRbad;
            break;
    }
    return OK;
}
fun ok64 JDRonOpen($cu8c tok, JDRstate* state) {
    return RDXu8bInto(state->builder, &state->cur);
}
fun ok64 JDRonClose($cu8c tok, JDRstate* state) {
    return RDXu8bOuto(state->builder, &state->cur);
}
fun ok64 JDRonInter($cu8c tok, JDRstate* state) { return OK; }
fun ok64 JDRonFIRST($cu8c tok, JDRstate* state) {
    state->flags &= ~(JDR_TUPLE | JDR_COLON);
    return OK;
}
fun ok64 JDRonRoot($cu8c tok, JDRstate* state) { return OK; }
