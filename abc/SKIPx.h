#include <stdint.h>

#include "B.h"
#include "abc/S.h"
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
    return TLVlen(X(SKIP, len)(pos) * sizeof(T));
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

    u8cs w = {(u8c*)(k->off), (u8c*)(k->off + X(SKIP, len)(pos))};
    must($ok(w));
    must($ok(Bu8idle(buf)));
    call(TLVfeed, Bu8idle(buf), SKIP_TLV_TYPE, w);

    k->pos = pos;

    done;
}

fun pro(X(SKIP, drain), X(SKIP, tab) * hop, Bu8 buf, size_t pos) {
    sane(hop != nil && Bok(buf) && pos > 0);
    a$(T, into, hop->off);
    a$tail(u8c, data,Bu8cdata(buf), pos);
    u8cs w = {};
    u8 t = 0;
    call(TLVdrain, &t, w, data);
    test(t == SKIP_TLV_TYPE, SKIPbad);
    test(X(SKIP, len)(pos) * sizeof(T) <= $len(w) &&
             X(SKIP, top)(pos) * sizeof(T) >= $len(w),
         SKIPbad);
    $copy(into, w);
    hop->pos = pos;

    done;
}

fun pro(X(SKIP, finish), Bu8 buf, X(SKIP, tab) * k) {
    sane(Bok(buf) && k != nil && k->pos < Bdatalen(buf));
    size_t pos = Bdatalen(buf);
    if (k->pos != 0 && X(SKIP, blk)(pos) == X(SKIP, blk)(k->pos)) {
        u8cs lastk = {};
        a$tail(u8, tail, Bu8data(buf), k->pos);
        a$dup(u8c, rest, tail);
        call(TLVdrain$, lastk, rest);
        call($u8move, tail, rest);
        call($u8retract,Bu8cdata(buf), $len(lastk));
    }
    a$raw(w, k->off);
    a$head(u8c, wl, w, X(SKIP, top)(pos));
    call(TLVfeed, Bu8idle(buf), SKIP_TLV_TYPE, wl);
    done;
}

fun pro(X(SKIP, load), X(SKIP, tab) * k, Bu8 buf) {
    sane(k != nil && Bok(buf));
    zerop(k);
    size_t len = Bdatalen(buf);
    if (X(SKIP, blk)(len) == 0) done;

    memset(k->off, 0xff, sizeof(k->off));

    u64 termlen = 2 + X(SKIP, top)(len) * sizeof(T);
    u64 len2 = len - termlen;
    u64 top = X(SKIP, top)(len2);
    u64 termlen2 = 2 + top * sizeof(T);
    len2 = len - termlen2;
    call(X(SKIP, drain), k, buf, len2);
    u8 l = termlen2 / sizeof(T) - 2;

    X(SKIP, tab) pre = *k;

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

fun pro(X(SKIP, find), u8c$ range, Bu8 hay, u8cs needle, $cmpfn cmp) {
    sane(range != nil && Bok(hay) && $ok(needle) && cmp != nil);
    X(SKIP, tab) k = {};
    call(X(SKIP, load), &k, hay);
    u64 from = 0;
    for (int h = X(SKIP, top)(k.pos) - 1; h >= 0; --h) {
        size_t pos = X(SKIP, pos)(&k, h);
        if (from >= pos) continue;  // been there
        X(SKIP, tab) hop = {};
        call(X(SKIP, hop), &hop, hay, &k, h);
        u64 b = hop.pos + X(SKIP, tlvlen)(hop.pos);
        a$tail(u8c, sub, Bu8data(hay), b);
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
    a$tail(u8c, sub, Bu8data(hay), from);
    $mv(range, sub);
    done;
}

fun ok64 X(SKIP, findTLV)(u8c$ rec, Bu8 buf, u8cs x, $cmpfn cmp) {
    u8cs gap = {};
    ok64 o = X(SKIP, find)(gap, buf, x, cmp);
    if (o != OK) return o;
    u8cs r = {};
    while (!$empty(gap) && OK == (o = TLVdrain$(r, gap))) {
        if ((**r & ~TLVaA) == SKIP_TLV_TYPE) continue;
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
