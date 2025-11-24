//
// Created by gritzko on 11/19/25.
//
#include "RDX.h"
#include "abc/PRO.h"

ok64 RDXNextTLV(rdxb x) {
    sane(rdxbOK(x) && rdxbDataLen(x) > 0);
    rdxp top = rdxbLast(x);
    u8cs idbody = {};
    u8cs value = {};
    test(!$empty(top->data), END);
    u8 lit = 0;
    call(TLVDrainKeyVal, &lit, idbody, value, top->data);
    top->type = RDX_TYPE_LIT_REV[lit];
    call(ZINTu8sDrain128, idbody, &top->id.seq, &top->id.src);
    switch (top->type) {  //
        case RDX_TYPE_FLOAT:
            call(ZINTu8sDrainFloat, &top->f, value);
            break;
        case RDX_TYPE_INT:
            call(ZINTu8sDrainInt, &top->i, value);
            break;
        case RDX_TYPE_REF:
            call(ZINTu8sDrain128, value, &top->r.seq, &top->r.src);
            break;
        case RDX_TYPE_STRING:
            $mv(top->s, value);
            break;
        case RDX_TYPE_TERM:
            $mv(top->t, value);
            break;
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX:
            $mv(top->plex, value);
            break;
    }
    done;
}

ok64 RDXSeekTLV(rdxb x) {
    // todo id <> id?!!  rdxbPending(x)
    return NOTIMPLYET;
}

ok64 TLVu8sStart(u8sc idle, u8s inner, u8 lit) {
    sane(u8sOK(idle) && inner != NULL);
    $mv(inner, idle);
    call(u8sFeed1, inner, lit);
    u32 z = 0;
    call(u8sFeed32, inner, &z);
    done;
}

ok64 TLVu8sEnd(u8s idle, u8s inner, u8 lit) {
    sane(u8sOK(idle) && u8sOK(inner) && inner[1] == idle[1] &&
         *inner > *idle + 5 && lit == **idle);
    u64 tl = *inner - *idle;
    *(u32*)(1 + *idle) = tl - 5;
    if (tl <= 0xff + 5) {
        **idle |= TLVaA;
        memmove(*idle + 2, *idle + 5, tl - 5);
        *idle = *inner - 3;
    } else {
        *idle = *inner;
    }
    done;
}

ok64 RDXWriteNextTLV(rdxb x) {
    sane(rdxbOK(x) && !rdxbEmpty(x));
    rdxp top = rdxbLast(x);
    u8** into = top->into;
    u8** plex = (u8**)top->plex;
    if (top->mark && top->mark < RDX_TYPE_PLEX_LEN) {
        call(TLVu8sEnd, into, plex, RDX_TYPE_LIT[top->mark]);
    }
    top->mark = top->type;
    if (top->type == 0) {
        done;
    }
    u8 lit = RDX_TYPE_LIT[top->type];

    a_pad(u8, id, 16);
    call(ZINTu8sFeed128, id_idle, top->id.seq, top->id.src);
    a_pad(u8, val, 16);
    switch (top->type) {
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX: {
            call(TLVu8sStart, into, plex, lit);
            call(u8sFeed1, plex, $len(id_data));
            call(u8sFeed, plex, id_datac);
            done;
        }
        case RDX_TYPE_FLOAT:
            call(ZINTu8sFeedFloat, val_idle, &top->f);
            call(TLVFeedKeyVal, into, lit, id_datac, val_datac);
            break;
        case RDX_TYPE_INT:
            call(ZINTu8sFeedInt, val_idle, &top->i);
            call(TLVFeedKeyVal, into, lit, id_datac, val_datac);
            break;
        case RDX_TYPE_REF:
            call(ZINTu8sFeed128, val_idle, top->r.seq, top->r.src);
            call(TLVFeedKeyVal, into, lit, id_datac, val_datac);
            break;
        case RDX_TYPE_STRING: {
            u8s str = {};
            call(TLVu8sStart, into, str, lit);
            call(u8sFeed1, str, $len(id_data));
            call(u8sFeed, str, id_datac);
            UTABLE[top->enc][3](str, top->s);
            call(TLVu8sEnd, into, str, lit);
            break;
        }
        case RDX_TYPE_TERM:
            call(TLVFeedKeyVal, into, lit, id_datac, top->t);
            break;
        default:
            fail(BADARG);
    }
    done;
}

ok64 RDXWriteSeekTLV(rdxb x) { return NOTIMPLYET; }

const u8 RDX_TYPE_LIT_REV[] = {
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 0,  10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 3,  5,  10, 10, 6,  10, 10,
    2,  10, 10, 10, 1,  10, 7,  8,  9,  10, 10, 10, 4,  10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10};
