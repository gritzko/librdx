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
con ok64 SKIPnoroom = 0xc73cf6cf265251c;

#define SKIP_MASK 0xffff
#define SKIP_TERM_LEN 3
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

ok64 SKIPfeed(Bu8 buf, SKIPs* k);

fun ok64 SKIPdrain(SKIPs* hop, Bu8 buf, size_t pos) {
    if (pos == 0) return SKIPbof;
    aB$(u8c, sub, buf, pos, Bdatalen(buf));
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
    if (k->off[height] == SKIP_MASK) return SKIPnone;
    size_t now = k->pos >> k->gap;
    hop->pos = SKIPpos(k, height);
    hop->gap = k->gap;
    return SKIPdrain(hop, buf, hop->pos);
}

fun ok64 SKIPmayfeed(Bu8 buf, SKIPs* skips) {
    size_t pos = Bdatalen(buf);
    size_t was = skips->pos >> skips->gap;
    size_t now = pos >> skips->gap;
    if (now == was) return OK;
    return SKIPfeed(buf, skips);
}

fun ok64 SKIPterm(Bu8 buf, SKIPs const* k) {
    u8$ idle = (u8$)Bidle(buf);
    if (SKIP_TERM_LEN > $len(idle)) return SKIPnoroom;
    if ((Bdatalen(buf) & ~SKIP_MASK) != (k->pos & ~SKIP_MASK)) return SKIPnone;
    size_t off = k->pos & SKIP_MASK;
    idle[0][0] = '2';
    idle[0][1] = off;
    idle[0][2] = off >> 8;
    return OK;
}

#define SKIPcall(buf, skips, feed, ...)    \
    call(feed, Bu8idle(buf), __VA_ARGS__); \
    ++(skips)->ndx;                        \
    call(SKIPmayfeed, buf, (skips));

ok64 SKIPload(SKIPs* k, Bu8 buf);

// Finds the interval containing the first record >= x.
ok64 SKIPfind(u8c$ gap, Bu8 buf, $u8c x, SKIPcmpfn cmp);

fun ok64 SKIPTLVfind(u8c$ rec, Bu8 buf, $u8c x, SKIPcmpfn cmp) {
    $u8c gap = {};
    ok64 o = SKIPfind(gap, buf, x, cmp);
    if (o != OK) return o;
    $u8c r = {};
    while (!$empty(gap) && OK == (o = TLVdrain$(r, gap))) {
        if (cmp((const $u8c*)&x, &r) <= 0) {
            $mv(rec, r);
            return OK;
        }
    }
    return SKIPnone;
}

#endif  // ABC_SKIP_H
