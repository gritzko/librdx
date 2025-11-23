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
    u8cs rest;
    $mv(rest, top->data);
    rest[0] += top->pos + top->len;
    if ($empty(rest)) fail(NOdata);
    u64 ol = u8csLen(rest);
    u8 type;
    call(TLVDrainKeyVal, &type, idbody, value, rest);
    top->type = RDX_TYPE_LIT_REV[type];
    call(ZINTu8sDrain128, idbody, &top->id.seq, &top->id.src);
    top->pos += top->len;
    top->len = ol - u8csLen(rest);
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
ok64 RDXIntoTLV(rdxb x) {
    sane(rdxbOK(x));
    if (rdxbEmpty(x)) {
        test(u8csOK(rdxbAtP(x, 0)->data), BADarg);
    } else {
        test(!$empty(rdxbIdle(x)), NOroom);
        rdxp last = $last(rdxbData(x));
        rdxp term = $head(rdxbIdle(x));
        $mv(term->data, (u8**)last->plex);
    }
    rdxsFed1(rdxbIdle(x));
    done;
}
ok64 RDXOutoTLV(rdxb x) { return rdxsPuked(rdxbData(x), 1); }
ok64 RDXSeekTLV(rdxb x) {
    // todo id <> id?!!  rdxbPending(x)
    return NOTimplyet;
}

ok64 RDXWriteBuffer(rdxcp r, u8bp buf) {
    u8c** b = (u8c**)buf;
    b[0] = r->data[0];
    b[3] = r->data[1];
    b[1] = b[0] + r->pos;
    b[2] = b[0] + r->pos + r->len;
    return b[3] >= b[2] ? OK : BADpos;
}

ok64 RDXWriteFoobar(rdxp r, u8b buf) {
    r->data[0] = buf[0];
    r->data[1] = buf[3];
    r->pos = buf[1] - buf[0];
    if (unlikely(buf[2] > buf[1] + u32max)) return FAILmiss;
    r->len = buf[2] - buf[1];
    return OK;
}

ok64 RDXWriteNextTLV(rdxb x) {
    sane(rdxbOK(x) && !rdxbEmpty(x));
    rdxp top = rdxbLast(x);
    u8b buf;
    call(RDXWriteBuffer, top, buf);
    if (top->len) {
        call(TLVu8bOuto, buf, 0);
    }
    RDX_TYPE t = rdxType(top);
    a_pad(u8, id, 16);
    call(ZINTu8sFeed128, id_idle, top->id.seq, top->id.src);
    u8 type = RDX_TYPE_LIT[t];

    a_pad(u8, val, 16);
    switch (t) {
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX:
            call(TLVu8bInto, buf, type);
            call(u8bFeed1, buf, $len(id_data));
            call(u8bFeed, buf, id_datac);
            call(RDXWriteFoobar, top, buf);
            done;
        case RDX_TYPE_FLOAT:
            call(ZINTu8sFeedFloat, val_idle, &top->f);
            call(TLVFeedKeyVal, u8bIdle(buf), type, id_datac, val_datac);
            break;
        case RDX_TYPE_INT:
            call(ZINTu8sFeedInt, val_idle, &top->i);
            call(TLVFeedKeyVal, u8bIdle(buf), type, id_datac, val_datac);
            break;
        case RDX_TYPE_REF:
            call(ZINTu8sFeed128, val_idle, top->r.seq, top->r.src);
            call(TLVFeedKeyVal, u8bIdle(buf), type, id_datac, val_datac);
            break;
        case RDX_TYPE_STRING: {
            call(TLVu8bInto, buf, type);
            call(u8bFeed1, buf, $len(id_data));
            call(u8bFeed, buf, id_datac);
            UTABLE[top->enc][3](u8bIdle(buf), top->s);
            call(TLVu8bOuto, buf, type);
            break;
        }
        case RDX_TYPE_TERM:
            call(TLVFeedKeyVal, u8bIdle(buf), type, id_datac, top->t);
            break;
        default:
            fail(BADarg);
    }
    ((u8**)buf)[1] = buf[2]; // fixme
    call(RDXWriteFoobar, top, buf);
    done;
}

ok64 RDXWriteIntoTLV(rdxb x) {
    sane(rdxbOK(x));
    if (rdxbEmpty(x)) {
        rdxp first = rdxbAtP(x, 0);
        test(u8sOK(first->into), BADarg);
        call(rdxbFed1, x);
    } else {
        rdxp hi = rdxbLast(x);
        u8b buf;
        RDXWriteBuffer(hi, buf);
        rdx lo = {.format = hi->format};
        $mv(lo.data, u8bIdle(buf));
        call(rdxbPush, x, &lo);
    }
    done;
}

ok64 RDXWriteOutoTLV(rdxb x) {
    sane(rdxbOK(x));
    rdxp lo = rdxbLast(x);
    if (lo->len) {
        u8b buf;
        RDXWriteBuffer(lo, buf);
        call(TLVu8bOuto, buf, 0);
        RDXWriteFoobar(lo, buf);
    }
    rdxbPop(x);
    u64 pos = lo->pos;
    if (!rdxbEmpty(x)) {
        rdxp hi = rdxbLast(x);
        hi->len = pos - hi->pos;
    }
    done;
}

ok64 RDXWriteSeekTLV(rdxb x) { return NOTimplyet; }

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
