#ifndef ABC_RDX1_H
#define ABC_RDX1_H
#include "01.h"
#include "RDX.h"
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

fun pro(RDXFtlv2txt, $u8 txt, id128* time, $cu8c tlv) {
    sane($ok(txt) && $ok(tlv) && time != nil);
    RDXfloat v;
    call(RDXFtlv2c, &v, time, tlv);
    u8 res[32];
    int len = d2s_buffered_n(v, (char*)res);
    $u8c $res = {res, res + len};
    call($u8feed, txt, $res);
    done;
}

#endif
