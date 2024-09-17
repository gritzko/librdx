#include "SKIP.h"

pro(SKIPfeed, Bu8 buf, SKIPs* k) {
    sane(Bok(buf) && k != nil);
    size_t pos = Bdatalen(buf);
    size_t was = k->pos >> k->gap;
    size_t now = pos >> k->gap;
    if (was == now) return OK;
    u8 height = ctz64(now);
    if (now != was + 1) {
        u8 topflip = 63 - clz64(now ^ was);
        for (u8 h = 0; h < topflip; ++h) k->off[h] = SKIP_MASK;
    }
    u8 len = height + 1;
    size_t sz = sizeof(u16) * len;
    $u8c w = {};
    w[0] = (u8c*)k->off;
    w[1] = w[0] + sz;
    call(TLVtinyfeed, Bu8idle(buf), SKIP_TLV_TYPE, w);
    u64 mask = (1 << k->gap) - 1;
    u16 myoff = pos & mask;
    for (u8 h = 0; h <= height; ++h) k->off[h] = myoff;
    k->pos = pos;
    if (len > k->len) k->len = len;
    k->tlvlen = Bdatalen(buf) - pos;
    done;
}

pro(SKIPload, SKIPs* k, Bu8 buf) {
    sane(k != nil && Bok(buf));
    done;
}

pro(SKIPfind, u8c$ gap, Bu8 buf, $u8c x, SKIPcmpfn cmp) {
    sane(gap != nil && Bok(buf) && $ok(x) && cmp != nil);
    SKIPs k = {};
    call(SKIPload, &k, buf);
    for (int h = k.len - 1; h >= 0; --h) {
        SKIPs hop = {};
        call(SKIPhop, &hop, buf, &k, h);
        aB$(u8c, sub, buf, hop.pos + hop.tlvlen, k.pos);
        if (cmp((const $u8c*)&x, &sub) <= 0) {
            k = hop;
            h = k.len;
            continue;
        }
    }
    SKIPs prev = {};
    call(SKIPhop, &prev, buf, &k, 0);
    gap[0] = buf[0] + prev.pos + prev.tlvlen;
    gap[1] = buf[0] + k.pos;
    done;
}
