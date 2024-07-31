#ifndef LIBRDX_TLV_H
#define LIBRDX_TLV_H
#include "$.h"
#include "PRO.h"
#include "INT.h"

con ok64 TLVbadrec = 0x27a76a2599f55d;
con ok64 TLVnodata = 0x25e25a33c9f55d;
con ok64 TLVbadtype = 0xa74f78a2599f55d;
con ok64 TLVnospace = 0xa67974df3c9f55d;
con ok64 TLVoverflo = 0xcf0ab6a7acdf55d;
con ok64 TLVbadcall = 0xc30967a2599f55d;

const u8 TLVaa = 0x20;

fun int TLVtiny(u8 t) { return t >= '0' && t <= '9'; }
fun int TLVlong(u8 t) { return t >= 'A' && t <= 'Z'; }
fun int TLVshort(u8 t) { return t >= 'a' && t <= 'z'; }

fun ok64 TLVprobe(u8* t, u32* hlen, u32* blen, $u8c data) {
    if ($empty(data)) return TLVnodata;
    if (TLVtiny(**data)) {
        *t = '0';
        *hlen = 1;
        *blen = **data - '0';
    } else if (TLVshort(**data)) {
        if ($len(data) < 2) return TLVnodata;
        *t = **data - TLVaa;
        *hlen = 2;
        *blen = (*data)[1];
    } else if (TLVlong(**data)) {
        if ($len(data) < 5) return TLVnodata;
        *t = **data;
        *hlen = 5;
        *blen = $at(data, 1) | ($at(data, 2) << 8) | ($at(data, 3) << 16) |
                ($at(data, 4) << 24);
    } else {
        return TLVbadrec;
    }
    // trace("%sTLVprobe: %u %u\n", PROindent, *hlen, *blen);
    return (*hlen + *blen) <= $len(data) ? OK : TLVnodata;
}

pro(TLVtake, u8 t, $u8c value, $u8c from) {
    sane(value != NULL && from != NULL);
    u32 hlen = 0;
    u32 blen = 0;
    u8 fact = 0;
    call(TLVprobe, &fact, &hlen, &blen, from);
    test(fact == '0' || fact == t, TLVbadtype);
    value[0] = from[0] + hlen;
    value[1] = from[0] + hlen + blen;
    from[0] += hlen + blen;
    done;
}

fun void TLVhead($u8 into, u8 type, u32 len) {
    if (len <= 0xff) {
        **into = type + TLVaa;
        ++*into;
        **into = len;
        ++*into;
    } else {
        **into = type;
        ++*into;
        put32(into, len);
    }
}

fun pro(TLVput, $u8 into, u8 type, $u8c value) {
    sane(TLVlong(type) && into != NULL && value != NULL);
    u32 len = $len(value);
    test($len(into) >= len + 5, TLVnospace);
    TLVhead(into, type, len);
    $u8copy(into, value);
    *into += len;
    done;
}

fun void TLVtinyhead($u8 into, u8 type, u32 len) {
    if (len > 9) return TLVhead(into, type, len);
    **into = '0' + len;
    ++*into;
}

fun pro(TLVtinyput, $u8 into, u8 type, $u8c value) {
    test($len(into) >= $len(value) + 1, TLVnospace);  // todo
    TLVtinyhead(into, type, $len(value));
    $u8copy(into, value);
    *into += $len(value);
    done;
}

#define TLVmaxnest 15
typedef u32 TLVstack[TLVmaxnest + 1];

fun pro(TLVopen, Bu8 tlv, u8 type, TLVstack stack) {
    sane(tlv != NULL && stack != NULL && TLVlong(type));
    u8** into = Bu8idle(tlv);
    test($len(into) >= 5, TLVnospace);
    u32 pos = $len(Bpast(tlv)) + $len(Bdata(tlv));
    test(*stack < TLVmaxnest, TLVoverflo);
    ++*stack;
    stack[*stack] = pos;
    **into = type;
    ++*into;
    put32(into, 0);
    done;
}

fun pro(TLVclose, Bu8 tlv, u8 type, TLVstack stack) {
    sane(tlv != NULL && TLVlong(type) && *stack > 0 && *stack <= TLVmaxnest);
    u32 pos = stack[*stack];
    --*stack;
    test(pos >= $len(Bpast(tlv)), TLVbadcall);
    u32 curpos = $len(Bpast(tlv)) + $len(Bdata(tlv));
    test(curpos >= pos + 5, TLVbadcall);
    u8* p = tlv[0] + pos;
    test(*p == type, TLVbadcall);
    u32 len = curpos - pos - 5;
    if (len >= 0x40) {
        $u8 l = {p + 1, tlv[3]};
        put32(l, len);
    } else {
        *p += TLVaa;
        ++p;
        *p = len;
        ++p;
        memmove(p, p + 3, len);
        *Bu8idle(tlv) -= 3;
    }
    done;
}

#endif
