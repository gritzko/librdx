#ifndef RDX_VLT_H
#define RDX_VLT_H

#include "abc/01.h"
#include "abc/B.h"
#include "abc/INT.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"

con ok64 VLTbad = 0xa2599d55f;
con ok64 VLTnoroom = 0x31cf3db3c9d55f;

typedef struct {
    u64 pos;
    // u8 rec[24];
    u8 key[32 - 8 - 2];
    u8 lit;
    u8 keylen;
} backmark;

fun ok64 VLTopen(Bu8 buf, Bu256 stack, u64 lit) {
    backmark mark = {
        .pos = Busylen(buf),
        .lit = lit,
    };
    return Bu256feedp(stack, (u256*)&mark);
}

fun ok64 VLTsetid(Bu8 vlt, Bu256 stack, $cu8c id) {
    sane(Bok(vlt) && Bok(stack) && !Bempty(stack));
    backmark* mark = (backmark*)Bu256top(stack);
    if (mark->keylen != 0) return VLTbad;
    memcpy(mark->key, id[0], $len(id));
    mark->keylen = $len(id);  // FIXME
    return OK;
}

fun ok64 VLTclose(Bu8 vlt, Bu256 stack, u64 lit) {
    u8$ into = Bu8idle(vlt);
    backmark* mark = (backmark*)Bu256top(stack);
    if ($len(into) < mark->keylen + 1 + 4 + 1) return VLTnoroom;
    memcpy(*into, mark->key, mark->keylen);
    into[0] += mark->keylen;
    **into = mark->keylen;
    ++*into;
    size_t blen = Bdatalen(vlt) - mark->pos;
    if (blen < 0x100) {
        **into = blen;
        ++*into;
        **into = lit | TLVaa;
        ++*into;
    } else {
        *(u32*)*into = (u32)blen;
        *into += sizeof(u32);
        **into = lit & ~TLVaa;
        ++*into;
    }
    return OK;
}

ok64 VLTreverse(Bu8 vlt, u64 deep);

ok64 VLTfeedTLV($u8 tlv, $u8c vlt, u64 deep);

#endif
