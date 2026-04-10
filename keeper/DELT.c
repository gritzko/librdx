//  DELT: git delta instruction applier
//
#include "DELT.h"

#include "abc/PRO.h"

// Decode a size varint (7-bit continuation, used for base/result sizes)
static ok64 DELTDrainSize(u8cs from, u64 *out) {
    u64 val = 0;
    u32 shift = 0;
    u8c *p = from[0];
    for (;;) {
        if (p >= from[1]) return DELTBADFMT;
        u8 c = *p++;
        val |= (u64)(c & 0x7f) << shift;
        shift += 7;
        if (!(c & 0x80)) break;
    }
    from[0] = (u8p)p;
    *out = val;
    return OK;
}

ok64 DELTApply(u8cs delta, u8cs base, u8g out) {
    sane(u8csOK(delta) && u8csOK(base) && u8gOK(out));

    // header: base size, result size
    u64 base_sz = 0, result_sz = 0;
    call(DELTDrainSize, delta, &base_sz);
    call(DELTDrainSize, delta, &result_sz);

    if ((u64)u8gRestLen(out) < result_sz) return NOROOM;

    u8p wp = out[1];
    u8p wend = out[1] + result_sz;

    while (delta[0] < delta[1]) {
        u8 cmd = *delta[0]++;

        if (cmd & 0x80) {
            // copy from base
            u64 off = 0, sz = 0;
            if (cmd & 0x01) { off |= (u64)(*delta[0]++); }
            if (cmd & 0x02) { off |= (u64)(*delta[0]++) << 8; }
            if (cmd & 0x04) { off |= (u64)(*delta[0]++) << 16; }
            if (cmd & 0x08) { off |= (u64)(*delta[0]++) << 24; }
            if (cmd & 0x10) { sz |= (u64)(*delta[0]++); }
            if (cmd & 0x20) { sz |= (u64)(*delta[0]++) << 8; }
            if (cmd & 0x40) { sz |= (u64)(*delta[0]++) << 16; }
            if (sz == 0) sz = 0x10000;

            if (off + sz > (u64)$size(base)) return DELTBADFMT;
            if (wp + sz > wend) return DELTBADFMT;
            memcpy(wp, base[0] + off, sz);
            wp += sz;
        } else if (cmd > 0) {
            // insert literal
            u64 n = cmd;
            if (delta[0] + n > delta[1]) return DELTBADFMT;
            if (wp + n > wend) return DELTBADFMT;
            memcpy(wp, delta[0], n);
            delta[0] += n;
            wp += n;
        } else {
            return DELTBADFMT;  // cmd=0 is reserved
        }
    }

    if (wp != wend) return DELTBADFMT;
    out[1] = wp;
    done;
}
