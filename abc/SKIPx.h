#include <stdint.h>

#include "abc/$.h"
#include "abc/01.h"
#include "abc/INT.h"
#include "abc/OK.h"
#include "abc/SKIP.h"
#include "abc/TLV.h"
#define T X(, )
#define SKIP_BLK_HI ctz64(sizeof(T))
#define SKIP_BLK_MASK (sizeof(T) - 1)

#ifndef offX
#define offX u16
#endif

#define SKIP_TERM_LEN (1 + sizeof(offX))
#define SKIP_TERM_LIT ('0' + sizeof(offX))
#define SKIP_NONE ((offX)u64max)

typedef struct {
    // The last skip record.
    u64 pos;
    offX off[40];
} X(SKIP, tab);

typedef X(SKIP, tab) X(SKIP, rtab);
typedef X(SKIP, tab) X(SKIP, wtab);

fun size_t X(SKIP, blk)(size_t pos) { return pos >> ctz64(sizeof(T)); }
fun u8 X(SKIP, hi)(size_t pos) { return ctz64(X(SKIP, blk)(pos)); }
fun u8 X(SKIP, len)(size_t pos) { return X(SKIP, hi)(pos) + 1; }  // TODO
fun u8 X(SKIP, top)(size_t pos) { return 64 - clz64(X(SKIP, blk)(pos)); }
fun offX X(SKIP, off)(size_t pos) { return pos & (sizeof(T) - 1); }
fun u32 X(SKIP, tlvlen)(size_t pos) {
    return TLVtinylen(X(SKIP, len)(pos) * sizeof(offX));
}

fun size_t X(SKIP, pos)(X(SKIP, rtab) const* k, u8 hi) {
    size_t pos = k->pos;
    if (pos == 0 || hi > X(SKIP, top)(pos)) return 0;
    size_t mask = (1 << hi) - 1;
    size_t was = (X(SKIP, blk)(pos) - 1) & ~mask;
    size_t off = k->off[hi];
    if (off == SKIP_NONE) return 0;
    return (was << SKIP_BLK_HI) + off;
}

fun pro(X(SKIP, feed), Bu8 buf, X(SKIP, wtab) * k) {
    sane(Bok(buf) && k != nil);
    size_t pos = Bdatalen(buf);
    size_t last = k->pos;
    size_t blk = X(SKIP, blk)(pos);
    size_t lastblk = X(SKIP, blk)(last);

    if (lastblk >= blk) {
        if (lastblk == blk) skip;
        fail(FAILsanity);
    } else if (blk != lastblk + 1) {
        u8 topflip = 64 - clz64(blk ^ lastblk);
        memset(k->off, 0xff, topflip * sizeof(offX));
    }

    $u8c w = {(u8c*)(k->off), (u8c*)(k->off + X(SKIP, len)(pos))};
    call(TLVtinyfeed, Bu8idle(buf), SKIP_TLV_TYPE, w);

    k->pos = pos;

    offX myoff = X(SKIP, off)(pos);
    for (u8 h = 0; h <= X(SKIP, hi)(pos); ++h) k->off[h] = myoff;

    done;
}

fun pro(X(SKIP, drain), X(SKIP, rtab) * hop, Bu8 buf, size_t pos) {
    sane(hop != nil && Bok(buf) && pos > 0);
    a$(offX, into, hop->off);
    $u8c data = {};
    call($u8tail, data, Bu8cdata(buf), pos);
    $u8c w = {};
    u8 t = 0;
    call(TLVdrain, &t, w, data);
    test(t == TLV_TINY_TYPE || t == SKIP_TLV_TYPE, SKIPbad);
    test(X(SKIP, len)(pos) * sizeof(offX) == $len(w), SKIPbad);
    $copy(into, w);
    hop->pos = pos;

    done;
}

fun pro(X(SKIP, term), Bu8 buf, X(SKIP, wtab) * k) {
    sane(Bok(buf) && k != nil && k->pos < Bdatalen(buf));
    u8$ idle = (u8$)Bidle(buf);
    size_t pos = Bdatalen(buf);
    while (X(SKIP, blk)(pos) != X(SKIP, blk)(k->pos)) {
        call(X(SKIP, feed), buf, k);
        pos = Bdatalen(buf);
    }
    u8c* offs[2] = {(u8c*)k->off, (u8c*)(k->off + X(SKIP, top)(pos))};
    call(TLVfeed, Bu8idle(buf), SKIP_TLV_TYPE, offs);
    done;
}

fun pro(X(SKIP, flip), X(SKIP, rtab) * r, X(SKIP, wtab) const* w, Bu8 buf) {
    sane(r != nil && w != nil && Bok(buf));
    *r = *w;
    X(SKIP, rtab) l = {};
    call(X(SKIP, drain), &l, buf, w->pos);
    for (u8 h = 0; h < X(SKIP, len)(w->pos); ++h) r->off[h] = l.off[h];
    done;
}

fun size_t X(_SKIP, termlen)(size_t len) {
    u8 top = X(SKIP, top)(len);  // FIXME 0
    size_t termlen = top * sizeof(offX) + 2;
    u8 top2 = X(SKIP, top)(len - termlen);
    if (top2 != top) {
        termlen = top2 * sizeof(offX) + 2;
    }
    return termlen;
}

fun pro(X(SKIP, load), X(SKIP, wtab) * k, Bu8 buf) {
    sane(k != nil && Bok(buf));
    size_t len = Bdatalen(buf);
    size_t termlen = X(_SKIP, termlen)(len);
    a$last(u8c, term, Bdata(buf), termlen);
    u8 t = 0;
    $u8c saved = {};
    call(TLVdrain, &t, saved, term);
    test(t == SKIP_TLV_TYPE, SKIPbad);
    u8* offs[2] = {(u8*)k->off, (u8*)(k->off + X(SKIP, top)(len))};
    $copy(offs, saved);
    k->pos = ((len - termlen) & ~SKIP_BLK_MASK) + k->off[0];
    done;
}

fun pro(X(SKIP, trim), X(SKIP, wtab) * k, Bu8 buf) {
    sane(k != nil && Bok(buf));
    call(X(SKIP, load), k, buf);
    size_t len = Bdatalen(buf);
    size_t termlen = X(_SKIP, termlen)(len);
    u8** b = (u8**)buf;
    b[2] -= termlen;
    done;
}

fun ok64 X(SKIP, hop)(X(SKIP, rtab) * hop, Bu8 buf, X(SKIP, rtab) const* k,
                      u8 hi) {
    size_t pos = X(SKIP, pos)(k, hi);
    if (pos == 0) return SKIPnone;
    return X(SKIP, drain)(hop, buf, pos);
}

fun ok64 X(SKIP, mayfeed)(Bu8 buf, X(SKIP, wtab) * skips) {
    size_t pos = Bdatalen(buf);
    if (X(SKIP, blk)(pos) == X(SKIP, blk)(skips->pos)) return OK;
    return X(SKIP, feed)(buf, skips);
}

ok64 X(SKIP, load)(X(SKIP, wtab) * k, Bu8 buf);

fun pro(X(SKIP, find), u8c$ gap, Bu8 buf, $u8c x, $cmpfn cmp) {
    sane(gap != nil && Bok(buf) && $ok(x) && cmp != nil);
    X(SKIP, wtab) w = {};
    call(X(SKIP, load), &w, buf);
    X(SKIP, rtab) k = {};
    call(X(SKIP, flip), &k, &w, buf);
    for (int h = X(SKIP, top)(k.pos) - 1; h >= 0; --h) {
        size_t pos = X(SKIP, pos)(&k, h);
        if (pos == 0) continue;
        X(SKIP, rtab) hop = {};
        call(X(SKIP, hop), &hop, buf, &k, h);
        aB$(u8c, sub, buf, hop.pos + X(SKIP, tlvlen)(hop.pos), k.pos);
        if (cmp((cc$)x, (cc$)sub) < 0) {
            k = hop;
            h = X(SKIP, len)(pos);
            continue;
        }
    }
    size_t pos = X(SKIP, pos)(&k, 0);
    if (pos > 0) {
        X(SKIP, rtab) prev = {};
        call(X(SKIP, hop), &prev, buf, &k, 0);  // FIXME ff
        pos += X(SKIP, tlvlen)(pos);
    }
    gap[0] = buf[1] + pos;
    gap[1] = buf[1] + k.pos;
    done;
}

fun ok64 X(SKIPTLV, find)(u8c$ rec, Bu8 buf, $u8c x, $cmpfn cmp) {
    $u8c gap = {};
    ok64 o = X(SKIP, find)(gap, buf, x, cmp);
    if (o != OK) return o;
    $u8c r = {};
    while (!$empty(gap) && OK == (o = TLVdrain$(r, gap))) {
        if (cmp((cc$)x, (cc$)r) <= 0) {
            $mv(rec, r);
            return OK;
        }
    }
    return SKIPnone;
}

#undef offX
#undef aSKIP
#undef SKIP_BLK_HI
#undef SKIP_BLK_MASK
