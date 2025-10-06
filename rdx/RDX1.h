#ifndef ABC_RDX1_H
#define ABC_RDX1_H
#include <stdio.h>

#include "abc/01.h"
#include "RDX.h"
#include "abc/ZINT.h"

fun pro(YmergeFIRST, $u8 into, u8css from) {
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

fun pro(RDXFdtlv, $u8 dtlv, $cu8c oldtlv, RDXfloat c, u128* clock) {
    sane($ok(oldtlv) && clock != nil);
    u64 bits;
    *(RDXfloat*)&bits = c;
    aBpad(u8, pad, 8);
    ZINTu64feed(Bu8idle(pad), flip64(bits));
    call(RDX1dtlv, dtlv, oldtlv, RDX_FLOAT, clock,Bu8cdata(pad));
    done;
}

// I

fun pro(RDXIdtlv, $u8 dtlv, $cu8c oldtlv, RDXint c, u128* clock) {
    sane($ok(oldtlv) && clock != nil);
    u64 bits = ZINTzigzag(c);
    aBpad(u8, pad, 8);
    ZINTu64feed(Bu8idle(pad), bits);
    call(RDX1dtlv, dtlv, oldtlv, RDX_INT, clock,Bu8cdata(pad));
    done;
}

// R

fun pro(RDXRdtlv, $u8 dtlv, $cu8c oldtlv, RDXref c, u128* clock) {
    sane($ok(oldtlv) && clock != nil);
    aBpad(u8, pad, 16);
    ZINTu128feed(Bu8idle(pad), c);
    call(RDX1dtlv, dtlv, oldtlv, RDX_REF, clock,Bu8cdata(pad));
    done;
}

// S

fun pro(RDXSdtlv, $u8 dtlv, $cu8c oldtlv, $cu8c c, u128* clock) {
    sane($ok(oldtlv) && clock != nil);
    call(RDX1dtlv, dtlv, oldtlv, RDX_STRING, clock, c);
    done;
}

// T

#define RDXTtlv2c RDXStlv2c
#define RDXTc2tlv RDXSc2tlv
#define RDXTdtlv RDXSdtlv
#define RDXTtxt2tlv RDXStxt2tlv
#define RDXTtlv2txt RDXStlv2txt

#endif
