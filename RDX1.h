#ifndef ABC_RDX1_H
#define ABC_RDX1_H
#include <stdio.h>

#include "01.h"
#include "RDX.h"
#include "ZINT.h"
#define RYU_OPTIMIZE_SIZE
#include "ryu/ryu.h"

fun pro(RDX1merge, $u8 into, $u8cp from) {
    sane($ok(into) && $ok(from));
    u128 max = {};
    $u8c rec = {};
    u8 t;
    u128 id;
    $u8c value;
    $eat2(from) {
        u8c* p = **from;
        call(RDXdrain, &t, &id, value, *from);
        if (u128cmp(&max, &id) < 0) {
            max = id;
            rec[0] = p;
            rec[1] = **from;
        }
    }
    if (*rec != nil) call($u8feed, into, rec);
    done;
}

#define RDXFmerge RDX1merge
#define RDXImerge RDX1merge
#define RDXRmerge RDX1merge
#define RDXSmerge RDX1merge
#define RDXTmerge RDX1merge

fun pro(RDX1dtlv, $u8 dtlv, $cu8c oldtlv, u8 type, u128* clock, $cu8c newbits) {
    sane($ok(oldtlv) && clock != nil);
    a$dup(u8c, oldtlv2, oldtlv);
    u8 t = 0;
    u128 id = {};
    $u8c value = {};
    call(RDXdrain, &t, &id, value, oldtlv2);
    RDXtock(clock, id);
    RDXtick(clock);
    call(RDXfeed, dtlv, type, *clock, newbits);
    done;
}

// F

fun pro(RDXFtlv2c, RDXfloat* c, id128* id, $cu8c tlv) {
    sane(c != nil && id != nil && $ok(tlv));
    u8 t = 0;
    $u8c value = {};
    a$dup(u8c, dup, tlv);
    call(RDXdrain, &t, id, value, dup);
    u64 bits = 0;
    call(ZINTu64drain, &bits, value);
    *(u64*)c = flip64(bits);
    done;
}

fun pro(RDXFc2tlv, $u8 tlv, RDXfloat c, u128 time) {
    sane($ok(tlv));
    u64 bits = 0;
    *(RDXfloat*)&bits = c;
    aBpad(u8, pad, 8);
    ZINTu64feed(Bu8idle(pad), flip64(bits));
    call(RDXfeed, tlv, RDX_FLOAT, time, Bu8cdata(pad));
    done;
}

fun pro(RDXFdtlv, $u8 dtlv, $cu8c oldtlv, RDXfloat c, u128* clock) {
    sane($ok(oldtlv) && clock != nil);
    u64 bits;
    *(RDXfloat*)&bits = c;
    aBpad(u8, pad, 8);
    ZINTu64feed(Bu8idle(pad), flip64(bits));
    call(RDX1dtlv, dtlv, oldtlv, RDX_FLOAT, clock, Bu8cdata(pad));
    done;
}

fun pro(RDXFtxt2tlv, $u8 tlv, $cu8c txt, id128 time) {
    sane($ok(tlv) && $ok(txt));
    size_t tl = $len(txt);
    test(tl < 32, RDXbad);
    u8 str[32];
    memcpy(str, *txt, tl);
    str[tl] = 0;
    double d = strtod((char*)str, nil);
    call(RDXFc2tlv, tlv, d, time);
    done;
}

fun pro(RDXFtlv2txt, $u8 txt, $cu8c tlv) {
    sane($ok(txt) && $ok(tlv));
    u128 time;
    RDXfloat v;
    call(RDXFtlv2c, &v, &time, tlv);
    u8 res[32];
    int len = d2s_buffered_n(v, (char*)res);
    $u8c $res = {res, res + len};
    call($u8feed, txt, $res);
    done;
}

// I

fun pro(RDXItlv2c, RDXint* c, id128* id, $cu8c tlv) {
    sane(c != nil && id != nil && $ok(tlv));
    u8 t = 0;
    $u8c value = {};
    a$dup(u8c, dup, tlv);
    call(RDXdrain, &t, id, value, dup);
    u64 bits = 0;
    call(ZINTu64drain, &bits, value);
    *c = ZINTzagzig(bits);
    done;
}

fun pro(RDXIc2tlv, $u8 tlv, RDXint c, u128 time) {
    sane($ok(tlv));
    u64 bits = ZINTzigzag(c);
    aBpad(u8, pad, 8);
    ZINTu64feed(Bu8idle(pad), bits);
    call(RDXfeed, tlv, RDX_INT, time, Bu8cdata(pad));
    done;
}

fun pro(RDXIdtlv, $u8 dtlv, $cu8c oldtlv, RDXint c, u128* clock) {
    sane($ok(oldtlv) && clock != nil);
    u64 bits = ZINTzigzag(c);
    aBpad(u8, pad, 8);
    ZINTu64feed(Bu8idle(pad), bits);
    call(RDX1dtlv, dtlv, oldtlv, RDX_INT, clock, Bu8cdata(pad));
    done;
}

fun pro(RDXItxt2tlv, $u8 tlv, $cu8c txt, id128 time) {
    sane($ok(tlv) && $ok(txt));
    size_t tl = $len(txt);
    test(tl < 32, RDXbad);
    u8 str[32];
    memcpy(str, *txt, tl);
    str[tl] = 0;
    i64 i = strtol((char*)str, nil, 10);
    call(RDXIc2tlv, tlv, i, time);
    done;
}

fun pro(RDXItlv2txt, $u8 txt, $cu8c tlv) {
    sane($ok(txt) && $ok(tlv));
    RDXint v = 0;
    id128 time = {};
    call(RDXItlv2c, &v, &time, tlv);
    u8 res[32];
    int len = sprintf((char*)res, "%li", v);
    $u8c $res = {res, res + len};
    call($u8feed, txt, $res);
    done;
}

// R

fun pro(RDXRtlv2c, RDXref* c, id128* id, $cu8c tlv) {
    sane(c != nil && id != nil && $ok(tlv));
    u8 t = 0;
    $u8c value = {};
    a$dup(u8c, dup, tlv);
    call(RDXdrain, &t, id, value, dup);
    call(ZINTu128drain, c, value);
    done;
}

fun pro(RDXRc2tlv, $u8 tlv, RDXref c, u128 time) {
    sane($ok(tlv));
    aBpad(u8, pad, 16);
    ZINTu128feed(Bu8idle(pad), c);
    call(RDXfeed, tlv, RDX_REF, time, Bu8cdata(pad));
    done;
}

fun pro(RDXRdtlv, $u8 dtlv, $cu8c oldtlv, RDXref c, u128* clock) {
    sane($ok(oldtlv) && clock != nil);
    aBpad(u8, pad, 16);
    ZINTu128feed(Bu8idle(pad), c);
    call(RDX1dtlv, dtlv, oldtlv, RDX_REF, clock, Bu8cdata(pad));
    done;
}

fun pro(RDXRtxt2tlv, $u8 tlv, $cu8c txt, id128 time) {
    sane($ok(tlv) && $ok(txt));
    id128 id = {};
    call(RDXid128drain, &id, txt);
    call(RDXRc2tlv, tlv, id, time);
    done;
}

fun pro(RDXRtlv2txt, $u8 txt, $cu8c tlv) {
    sane($ok(txt) && $ok(tlv));
    id128 time;
    RDXref v = {};
    call(RDXRtlv2c, &v, &time, tlv);
    call(RDXid128feed, txt, v);
    done;
}

// S

fun pro(RDXStlv2c, u8c$ str, id128* id, $cu8c tlv) {
    sane(str != nil && id != nil && $ok(tlv));
    a$dup(u8c, dup, tlv);
    u8 t = 0;
    call(RDXdrain, &t, id, str, dup);
    done;
}

fun pro(RDXSc2tlv, $u8 tlv, $cu8c c, u128 time) {
    sane($ok(tlv));
    call(RDXfeed, tlv, RDX_STRING, time, c);
    done;
}

fun pro(RDXSdtlv, $u8 dtlv, $cu8c oldtlv, $cu8c c, u128* clock) {
    sane($ok(oldtlv) && clock != nil);
    call(RDX1dtlv, dtlv, oldtlv, RDX_STRING, clock, c);
    done;
}

fun pro(RDXStxt2tlv, $u8 tlv, $cu8c txt, id128 time) {
    sane($ok(tlv) && $ok(txt) && $len(txt) >= 2 && **txt == '"' &&
         *$last(txt) == '"');
    $u8c t = {txt[0] + 1, txt[1] - 1};
    call(RDXSc2tlv, tlv, t, time);  // FIXME escapes
    done;
}

fun pro(RDXStlv2txt, $u8 txt, $cu8c tlv) {
    // FIXME escapes
    sane(txt != nil && $ok(tlv));
    id128 time = {};
    $u8c t = {};
    call(RDXStlv2c, t, &time, tlv);
    call($u8feed1, txt, '"');
    call($u8feed, txt, t);
    call($u8feed1, txt, '"');
    done;
}

// T

#define RDXTtlv2c RDXStlv2c
#define RDXTc2tlv RDXSc2tlv
#define RDXTdtlv RDXSdtlv
#define RDXTtxt2tlv RDXStxt2tlv
#define RDXTtlv2txt RDXStlv2txt

#endif
