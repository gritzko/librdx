#include <stdint.h>

#include "B.h"
#include "abc/$.h"
#include "abc/01.h"
#include "abc/OK.h"
#include "abc/SKIP.h"
#include "abc/TLV.h"
#define T X(, )
#define SKIP_BLK_HI (sizeof(T) * 8)
#define SKIP_BLK_MASK ((1UL << SKIP_BLK_HI) - 1)

#define SKIP_TERM_LEN (1 + sizeof(T))
#define SKIP_TERM_LIT ('0' + sizeof(T))
#define SKIP_NONE ((T)u64max)

typedef struct {
    // The last skip record.
    u64 pos;
    T off[40];
} X(SKIP, tab);

fun size_t X(SKIP, blk)(size_t pos) { return pos >> SKIP_BLK_HI; }
fun u8 X(SKIP, hi)(size_t pos) {
    size_t blk = X(SKIP, blk)(pos);
    return ctz64(blk | !blk);
}
fun u8 X(SKIP, len)(size_t pos) { return X(SKIP, hi)(pos) + 1; }  // TODO
fun u8 X(SKIP, top)(size_t pos) { return 64 - clz64(X(SKIP, blk)(pos)); }
fun T X(SKIP, off)(size_t pos) { return pos & SKIP_BLK_MASK; }
fun u32 X(SKIP, tlvlen)(size_t pos) {
    return TLVtinylen(X(SKIP, len)(pos) * sizeof(T));
}

fun size_t X(SKIP, pos)(X(SKIP, tab) const* k, u8 hi) {
    size_t pos = k->pos;
    if (pos == 0 || hi > X(SKIP, top)(pos)) return 0;
    size_t mask = (1 << hi) - 1;
    size_t was = (X(SKIP, blk)(pos) - 1) & ~mask;
    size_t off = k->off[hi];
    if (off == SKIP_NONE) return 0;
    return (was << SKIP_BLK_HI) + off;
}

fun pro(X(SKIP, feed), Bu8 buf, X(SKIP, tab) * k) {
    sane(Bok(buf) && k != nil);
    size_t pos = Bdatalen(buf);
    size_t last = k->pos;
    size_t blk = X(SKIP, blk)(pos);
    size_t lastblk = X(SKIP, blk)(last);
    T off = X(SKIP, off)(pos);
    test(off != SKIP_NONE, SKIPbad);  // TODO fuzzer must find this

    T preoff = X(SKIP, off)(k->pos);
    for (u8 h = 0; h <= X(SKIP, hi)(k->pos); ++h) {
        k->off[h] = preoff;
    }

    if (lastblk >= blk) {
        if (lastblk == blk) skip;
        fail(FAILsanity);
    } else if (blk != lastblk + 1) {
        u8 topflip = 64 - clz64(blk ^ lastblk);
        memset(k->off, 0xff, topflip * sizeof(T));
    }

    $u8c w = {(u8c*)(k->off), (u8c*)(k->off + X(SKIP, len)(pos))};
    must($ok(w));
    must($ok(Bu8idle(buf)));
    call(TLVtinyfeed, Bu8idle(buf), SKIP_TLV_TYPE, w);

    k->pos = pos;

    done;
}

fun pro(X(SKIP, drain), X(SKIP, tab) * hop, Bu8 buf, size_t pos) {
    sane(hop != nil && Bok(buf) && pos > 0);
    a$(T, into, hop->off);
    a$tail(u8c, data, Bu8cdata(buf), pos);
    $u8c w = {};
    u8 t = 0;
    call(TLVdrain, &t, w, data);
    test(t == TLV_TINY_TYPE || t == SKIP_TLV_TYPE, SKIPbad);
    test(X(SKIP, len)(pos) * sizeof(T) == $len(w), SKIPbad);
    $copy(into, w);
    hop->pos = pos;

    done;
}

fun pro(X(SKIP, term), Bu8 buf, X(SKIP, tab) * k) {
    sane(Bok(buf) && k != nil && k->pos < Bdatalen(buf));
    u8$ idle = (u8$)Bidle(buf);
    size_t pos = Bdatalen(buf);
    if (X(SKIP, blk)(pos) != X(SKIP, blk)(k->pos)) {
        call(X(SKIP, feed), buf, k);
        done;
    }
    u64 tl = X(SKIP, tlvlen)(k->pos);
    $u8 tail = {buf[0] + k->pos, buf[2]};
    test($len(tail) >= tl, SKIPmiss);
    $u8c data1 = {tail[0] + tl, tail[1]};
    $u8c skip1 = {tail[0], tail[0] + tl};
    $u8 data2 = {tail[0], tail[1] - tl};
    $u8 skip2 = {tail[1] - tl, tail[1]};
    aBcpad(u8, tmp, 64);
    $u8feedall(tmpidle, skip1);
    $u8move(data2, data1);
    $u8move(skip2, tmpdata);
    // FIXME update k
    done;
}

fun size_t X(_SKIP, termlen)(size_t len) {
    u8 top = X(SKIP, top)(len);  // FIXME 0
    size_t termlen = top * sizeof(T) + 2;
    u8 top2 = X(SKIP, top)(len - termlen);
    if (top2 != top) {
        termlen = top2 * sizeof(T) + 2;
    }
    return termlen;
}

fun pro(X(SKIP, load), X(SKIP, tab) * k, Bu8 buf) {
    sane(k != nil && Bok(buf));
    zerop(k);
    size_t len = Bdatalen(buf);
    if (X(SKIP, blk)(len) == 0) done;

    u64 termlen = X(SKIP, tlvlen)(len) * sizeof(T);
    u64 len2 = len - termlen;
    u64 termlen2 = X(SKIP, tlvlen)(len2) * sizeof(T);
    len2 = len - termlen2;

    call(X(SKIP, drain), k, buf, len2);

    u8 l = X(SKIP, len)(len2);
    u8 top = X(SKIP, top)(len2);

    X(SKIP, tab) pre = *k;
    for (u8 i = l; i < top; ++i) pre.off[i] = SKIP_NONE;

    while (l < top) {
        u8 hi = X(SKIP, hi)(pre.pos);
        while (pre.off[hi] == SKIP_NONE) {
            if (hi == 0) return OK;
            --hi;
        }
        size_t prepos = X(SKIP, pos)(&pre, hi);
        u8 preoff = pre.off[hi];
        u8 ll = X(SKIP, len)(prepos);
        zero(pre);
        call(X(SKIP, drain), &pre, buf, prepos);
        while (l < ll) {
            k->off[l] = preoff;
            size_t prepos2 = X(SKIP, pos)(k, l);
            if (prepos2 != prepos) {
                k->off[l] = SKIP_NONE;
            }
            ++l;
        }
        if (k->off[l - 1] == SKIP_NONE && pre.off[l - 1] != SKIP_NONE) {
            k->off[l - 1] = pre.off[l - 1];
            size_t prepos2 = X(SKIP, pos)(k, l - 1);
            if (prepos2 != prepos) {
                k->off[l - 1] = SKIP_NONE;
            }
        }
    }

    done;
}

fun ok64 X(SKIP, hop)(X(SKIP, tab) * hop, Bu8 buf, X(SKIP, tab) const* k,
                      u8 hi) {
    size_t pos = X(SKIP, pos)(k, hi);
    if (pos == 0) return SKIPnone;
    return X(SKIP, drain)(hop, buf, pos);
}

fun ok64 X(SKIP, mayfeed)(Bu8 buf, X(SKIP, tab) * skips) {
    size_t pos = Bdatalen(buf);
    if (X(SKIP, blk)(pos) == X(SKIP, blk)(skips->pos)) return OK;
    return X(SKIP, feed)(buf, skips);
}

ok64 X(SKIP, load)(X(SKIP, tab) * k, Bu8 buf);

fun pro(X(SKIP, find), u8c$ range, Bu8 hay, $u8c needle, $cmpfn cmp) {
    sane(range != nil && Bok(hay) && $ok(needle) && cmp != nil);
    X(SKIP, tab) k = {};
    call(X(SKIP, load), &k, hay);  // FIXME pass
    u64 from = Bpastlen(hay);
    for (int h = X(SKIP, top)(k.pos) - 1; h >= 0; --h) {
        size_t pos = X(SKIP, pos)(&k, h);
        if (from >= pos) continue;  // been there
        X(SKIP, tab) hop = {};
        call(X(SKIP, hop), &hop, hay, &k, h);
        u64 b = hop.pos + X(SKIP, tlvlen)(hop.pos);
        aB$(u8c, sub, hay, b, k.pos);
        int c = cmp((cc$)needle, (cc$)sub);
        if (c < 0) {
            k = hop;
            h = X(SKIP, len)(pos);
        } else if (c > 0) {
            from = b;
        } else {
            from = b;
            break;
        }
    }
    aB$(u8c, sub, hay, from, k.pos);
    $mv(range, sub);
    done;
}

fun ok64 X(SKIP, findTLV)(u8c$ rec, Bu8 buf, $u8c x, $cmpfn cmp) {
    $u8c gap = {};
    ok64 o = X(SKIP, find)(gap, buf, x, cmp);
    if (o != OK) return o;
    $u8c r = {};
    while (!$empty(gap) && OK == (o = TLVdrain$(r, gap))) {
        if ((**r >= '0' && **r <= '9') || (**r & ~TLVaa) == SKIP_TLV_TYPE)
            continue;
        if (cmp((cc$)x, (cc$)r) <= 0) {
            $mv(rec, r);
            return OK;
        }
    }
    return SKIPnone;
}

#undef aSKIP
#undef SKIP_BLK_HI
#undef SKIP_BLK_MASK
