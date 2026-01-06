//
// Created by gritzko on 11/19/25.
//
#include "RDX.h"
#include "abc/PRO.h"

ok64 rdxNextTLV(rdxp x) {
    sane(x);
    if (!$empty(x->data) && SKIL_LIT == (**x->data & ~TLVaA)) {
        u8 lit;
        u8cs val;
        call(TLVu8sDrain, x->data, &lit, val);
    }
    u8csp data = x->data;
    if ($empty(data)) {
        x->type = 0;
        return END;
    }
    u8cs idbody = {};
    u8cs value = {};
    u8 lit = 0;
    call(TLVDrainKeyVal, &lit, idbody, value, data);
    x->type = RDX_TYPE_LIT_REV[lit];
    call(ZINTu8sDrain128, idbody, &x->id.seq, &x->id.src);
    switch (x->type) {  //
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX: {
            u8csMv(x->plexc, value);
            x->cformat = RDX_FMT_TLV;
            break;
        }
        case RDX_TYPE_FLOAT:
            call(ZINTu8sDrainFloat, &x->f, value);
            break;
        case RDX_TYPE_INT:
            call(ZINTu8sDrainInt, &x->i, value);
            break;
        case RDX_TYPE_REF:
            call(ZINTu8sDrain128, value, &x->r.seq, &x->r.src);
            break;
        case RDX_TYPE_STRING:
            $mv(x->s, value);
            break;
        case RDX_TYPE_TERM:
            $mv(x->t, value);
            break;
        case 0:
            fail(END);
        default:
            fail(RDXBAD);
    }
    done;
}

ok64 rdxIntoTLV(rdxp c, rdxp p) {
    sane(c && p && p->type && rdxTypePlex(p));
    c->format = p->cformat;
    u8csMv(c->data, p->plexc);
    c->ptype = p->type;
    c->cformat = 0;
    if (!c->type) {
        zero(c->r);
        done;
    }
    rdx c2 = *c;
    ok64 o = OK;
    do {
        o = rdxNextTLV(c);
    } while (o == OK && rdxZ(c, &c2));
    if (o == END || rdxZ(&c2, c)) o = NONE;
    return o;
}

ok64 rdxOutoTLV(rdxp c, rdxp p) { return OK; }

ok64 rdxWriteTLV1(rdxp x) { return NOTIMPLYET; }

ok64 rdxWriteNextTLV(rdxp x) {
    sane(x);
    u8 lit = RDX_TYPE_LIT[x->type];
    a_pad(u8, id, 16);
    call(ZINTu8sFeed128, id_idle, x->id.seq, x->id.src);
    a_pad(u8, val, 16);
    u8sp into = x->into;
    switch (x->type) {
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX: {
            u8sp plex = (u8sp)x->plex;
            call(TLVu8sStart, x->into, plex, lit);
            call(u8sFeed1, plex, $len(id_data));
            call(u8sFeed, plex, id_datac);
            x->cformat = RDX_FMT_TLV | RDX_FMT_WRITE;
            done;
        }
        case RDX_TYPE_FLOAT:
            call(ZINTu8sFeedFloat, val_idle, &x->f);
            call(TLVFeedKeyVal, into, lit, id_datac, val_datac);
            break;
        case RDX_TYPE_INT:
            call(ZINTu8sFeedInt, val_idle, &x->i);
            call(TLVFeedKeyVal, into, lit, id_datac, val_datac);
            break;
        case RDX_TYPE_REF:
            call(ZINTu8sFeed128, val_idle, x->r.seq, x->r.src);
            call(TLVFeedKeyVal, into, lit, id_datac, val_datac);
            break;
        case RDX_TYPE_STRING: {
            u8s str = {};
            call(TLVu8sStart, x->into, str, lit);
            call(u8sFeed1, str, $len(id_data));
            call(u8sFeed, str, id_datac);
            UTABLE[x->cformat][UTF8_DECODER_ALL](str, x->s);
            call(TLVu8sEnd, x->into, str, lit);
            break;
        }
        case RDX_TYPE_TERM:
            call(TLVFeedKeyVal, into, lit, id_datac, x->t);
            break;
        default:
            fail(RDXBAD);
    }
    done;
}

ok64 rdxWriteIntoTLV(rdxp c, rdxp p) {
    sane(c && p && p->type);
    c->format = p->cformat;
    u8sFork(p->plex, c->into);
    c->ptype = p->type;
    c->type = 0;
    c->cformat = 0;
    zero(c->r);
    done;
}

ok64 rdxWriteOutoTLV(rdxp c, rdxp p) {
    sane(c && p);
    u8 plit = RDX_TYPE_LIT[c->ptype];
    call(TLVu8sEnd, p->into, c->into, plit);
    done;
}
