#ifndef ABC_RDX_C
#define ABC_RDX_C
#include "RDX.h"

fun ok64 RDXCdrainF(RDXfloat* c, id128* id, $u8c tlv) {
    sane(c != nil && id != nil && $ok(tlv));
    u8 t = 0;
    $u8c value = {};
    u64 bits = 0;
    call(RDXdrain, &t, id, value, tlv);
    test(t == RDX_FLOAT, RDXbad);
    call(ZINTu64drain, &bits, value);
    *(u64*)c = flip64(bits);
    done;
}

fun ok64 RDXCfeedF($u8 tlv, RDXfloat c, u128 time) {
    sane($ok(tlv));
    u64 bits = *(u64*)&c;
    aBpad(u8, pad, 8);
    call(ZINTu64feed, Bu8idle(pad), flip64(bits));
    call(RDXfeed, tlv, RDX_FLOAT, time, Bu8cdata(pad));
    done;
}

fun ok64 RDXCdrainI(RDXint* c, id128* id, $u8c tlv) {
    sane(c != nil && id != nil && $ok(tlv));
    u8 t = 0;
    u64 bits = 0;
    $u8c value = {};
    call(RDXdrain, &t, id, value, tlv);
    call(ZINTu64drain, &bits, value);
    *c = ZINTzagzig(bits);
    done;
}

fun ok64 RDXCfeedI($u8 tlv, RDXint c, u128 time) {
    sane($ok(tlv));
    aBpad(u8, pad, 8);
    u64 bits = ZINTzigzag(c);
    ZINTu64feed(Bu8idle(pad), bits);
    call(RDXfeed, tlv, RDX_INT, time, Bu8cdata(pad));
    done;
}

fun ok64 RDXCdrainR(RDXref* c, id128* id, $u8c rdx) {
    sane(c != nil && id != nil && $ok(rdx));
    u8 t = 0;
    $u8c value = {};
    call(RDXdrain, &t, id, value, rdx);
    call(ZINTu128drain, c, value);
    done;
}

fun ok64 RDXCfeedR($u8 tlv, RDXref c, u128 time) {
    sane($ok(tlv));
    aBpad(u8, pad, 16);
    ZINTu128feed(Bu8idle(pad), &c);
    call(RDXfeed, tlv, RDX_REF, time, Bu8cdata(pad));
    done;
}

fun ok64 RDXCdrainS(u8c$ str, id128* id, $u8c rdx) {
    u8 t = 0;
    id128 _;
    if (id == nil) id = &_;
    ok64 o = RDXdrain(&t, id, str, rdx);
    return t == RDX_STRING ? o : RDXwrong;
}

fun ok64 RDXCfeedS($u8 tlv, $cu8c c, u128 time) {
    sane($ok(tlv));
    call(RDXfeed, tlv, RDX_STRING, time, c);
    done;
}

fun ok64 RDXCdrainT(u8c$ str, id128* id, $u8c rdx) {
    u8 t = 0;
    ok64 o = RDXdrain(&t, id, str, rdx);
    return t == RDX_TERM ? o : RDXwrong;
}

#endif
