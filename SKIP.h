#ifndef ABC_SKIP_H
#define ABC_SKIP_H

#include "$.h"
#include "01.h"
#include "B.h"
#include "OK.h"
#include "TLV.h"

con ok64 SKIPnotyet = 0xe29f78cf265251c;
con ok64 SKIPbad = 0x2896665251c;
con ok64 SKIPmiss = 0xdf7b7165251c;
con ok64 SKIPnodata = 0x978968cf265251c;
con ok64 SKIPtoofar = 0xda5ab3cf865251c;
con ok64 SKIPbof = 0x2ace665251c;
con ok64 SKIPnone = 0xa72cf265251c;

#define SKIP_NONE 0xffff
#define SKIP_TLV_TYPE 'Z'

typedef $u8ccmpfn SKIPcmpfn;

typedef struct {
    // The last skip record.
    u64 pos;
    // Avg skip record: 2 entries, 4+1 bytes,
    // OVerhead 1% => gap 1<<9=512
    u8 gap;
    u8 len;
    u8 tlvlen;
    u16 off[40];
} SKIPs;

fun u8 SKIPheight(size_t pos, SKIPs const* skips) {
    size_t now = pos >> skips->gap;
    return ctz64(now);
}

fun size_t SKIPpos(SKIPs const* skips, u8 height) {
    size_t now = skips->pos >> skips->gap;
    size_t step = 1 << height;
    if (step >= now) return 0;
    size_t was = (now - step) & ~(step - 1);
    return (was << skips->gap) + skips->off[height];
}

fun ok64 SKIPfeed(Bu8 buf, SKIPs* k) {
    size_t pos = Busylen(buf);
    size_t was = k->pos >> k->gap;
    size_t now = pos >> k->gap;
    if (was == now) return OK;
    u8 height = ctz64(now);
    if (now != was + 1) {
        u8 topflip = 63 - clz64(now ^ was);
        for (u8 h = 0; h < topflip; ++h) k->off[h] = SKIP_NONE;
    }
    u8 len = height + 1;
    size_t sz = sizeof(u16) * len;
    $u8c w = {};
    w[0] = (u8c*)k->off;
    w[1] = w[0] + sz;
    ok64 o = TLVtinyfeed(Bu8idle(buf), SKIP_TLV_TYPE, w);
    if (o != OK) return o;
    u64 mask = (1 << k->gap) - 1;
    u16 myoff = pos & mask;
    for (u8 h = 0; h <= height; ++h) k->off[h] = myoff;
    k->pos = pos;
    if (len > k->len) k->len = len;
    k->tlvlen = Busylen(buf) - pos;
    return o;
}

fun ok64 SKIPdrain(SKIPs* hop, Bu8 buf, size_t pos) {
    if (pos == 0) return SKIPbof;
    aB$(u8c, sub, buf, pos, Busylen(buf));
    $u16c w = {};
    u8 t = 0;
    size_t l = $len(sub);
    ok64 o = TLVdrain(&t, (u8c$)w, sub);
    if (o != OK) return o;
    a$(u16, into, hop->off);
    if ((t != TLV_TINY_TYPE && t != SKIP_TLV_TYPE) || ($size(w) & 1))
        return SKIPbad;
    $copy(into, w);
    hop->len = $len(w);
    hop->pos = pos;
    hop->tlvlen = l - $len(sub);
    return OK;
}

fun ok64 SKIPhop(SKIPs* hop, Bu8 buf, SKIPs const* k, u8 height) {
    if (height >= k->len) return SKIPtoofar;
    if (k->off[height] == SKIP_NONE) return SKIPnone;
    size_t now = k->pos >> k->gap;
    hop->pos = SKIPpos(k, height);
    hop->gap = k->gap;
    return SKIPdrain(hop, buf, hop->pos);
}

fun ok64 SKIPmayfeed(Bu8 buf, SKIPs* skips) {
    size_t pos = Busylen(buf);
    size_t was = skips->pos >> skips->gap;
    size_t now = pos >> skips->gap;
    if (now == was) return OK;
    return SKIPfeed(buf, skips);
}

#define SKIPcall(buf, skips, feed, ...)    \
    call(feed, Bu8idle(buf), __VA_ARGS__); \
    ++(skips)->ndx;                        \
    call(SKIPmayfeed, buf, (skips));

// Finds the interval containing the first record >= x.
ok64 SKIPfind(u8c$ gap, Bu8 buf, $u8c x, SKIPcmpfn cmp);
ok64 SKIPTLVfind(u8c$ rec, Bu8 buf, $u8c x, SKIPcmpfn cmp) {
    // get the gap
    // iterate the gap
    return notimplyet;
}

#endif  // ABC_SKIP_H
