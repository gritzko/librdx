#ifndef RDX_VLT_H
#define RDX_VLT_H

#include "abc/01.h"
#include "abc/B.h"
#include "abc/INT.h"
#include "abc/OK.h"
#include "abc/TLV.h"

con ok64 VLTbad = 0xa2599d55f;
con ok64 VLTnoroom = 0x31cf3db3c9d55f;
con ok64 VLTbadnest = 0xe37a72a2599d55f;
con ok64 VLTbadkey = 0x3da6fa2599d55f;

fun ok64 VLTopen(Bu8 buf, Bu64 stack, u64 lit) {
    u64 mark = Busylen(buf) | (lit << B_MAX_LEN_BITS);
    return Bu64feed1(stack, mark);
}

fun ok64 VLTreopen(Bu8 buf, Bu64 stack, u64 lit) {
    u64 pos = *(stack[2]);
    if (pos == 0) return VLTbadnest;
    pos &= B_MAX_LEN - 1;
    u64 mark = pos | (lit << B_MAX_LEN_BITS);
    return Bu64feed1(stack, mark);
}

fun u64 VLTpos(u64 v) { return v & (B_MAX_LEN - 1); }

fun ok64 VLTrename(Bu8 buf, Bu64 stack, u64 old, u64 lit) {
    if (Bempty(stack)) return FAILsanity;
    u64* top = Btop(stack);
    if ((*top >> B_MAX_LEN_BITS) != old) return FAILsanity;
    *top = (((u64)lit) << B_MAX_LEN_BITS) | (*top & (B_MAX_LEN - 1));
    return OK;
}

fun u8 VLTtop(Bu64 stack) {
    if (Bempty(stack)) return 0;
    return (*Bu64top(stack) >> B_MAX_LEN_BITS) & ~TLVaa;
}

fun b8 VLTis1(Bu64 stack) {
    u64$ data = Bu64data(stack);
    if (Bempty(stack)) return NO;
    if ($empty(Bidle(stack))) return NO;
    if (VLTpos(*$term(data)) == VLTpos(*$last(data)) && *$term(data) != 0)
        return YES;
    return NO;
}

fun ok64 VLTclose(Bu8 vlt, Bu64 stack, u64 lit) {
    u8$ into = Bu8idle(vlt);
    u64 pos = *Bu64top(stack) & (B_MAX_LEN - 1);
    u8 toplit = VLTtop(stack);
    if (lit != 0 && lit != toplit) return VLTbadnest;
    if (Busylen(vlt) < pos) return VLTbad;
    size_t blen = Busylen(vlt) - pos;
    if (blen < 0x100) {
        **into = blen;
        ++*into;
        **into = toplit | TLVaa;
        ++*into;
    } else {
        *(u32*)*into = (u32)blen;  // TODO feed
        *into += sizeof(u32);
        **into = toplit & ~TLVaa;
        ++*into;
    }
    Bu64pop(stack);
    return OK;
}

ok64 VLTreverseTLKV(Bu8 vlt, u64 deep);

ok64 VLTfeedTLKV($u8 tlv, $u8c vlt, u64 deep);

fun void VLTdump(Bu64 stack) {
    for (u64c* p = stack[1]; p < stack[2]; ++p) {
        fprintf(stderr, "%lu.\t%c %lu\n", p - stack[1],
                (u8)(*p >> B_MAX_LEN_BITS), (*p & (B_MAX_LEN - 1)));
    }
}

#endif
