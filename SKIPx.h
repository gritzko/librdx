#include <stdint.h>

#include "INT.h"
#include "SKIP.h"
#define T X(, )
#define GAP ctz64(sizeof(T))

#ifndef SKIPoff_t
#define SKIPoff_t u16
#endif

#define SKIP_TERM_LEN (1 + sizeof(SKIPoff_t))
#define SKIP_TERM_LIT ('0' + sizeof(SKIPoff_t))

typedef struct {
    // The last skip record.
    u64 pos;
    u8 len;
    u8 tlvlen;
    SKIPoff_t off[40];
} X(SKIP, );

fun u8 X(SKIP, height)(size_t pos, X(SKIP, ) const* skips) {
    size_t now = pos >> GAP;
    return ctz64(now);
}

fun size_t X(SKIP, pos)(X(SKIP, ) const* skips, u8 height) {
    size_t now = skips->pos >> GAP;
    size_t step = 1 << height;
    if (step >= now) return 0;
    size_t was = (now - step) & ~(step - 1);
    size_t off = skips->off[height];
    assert(off != (SKIPoff_t)u64max);
    return (was << GAP) + off;
}

fun pro(X(SKIP, feed), Bu8 buf, X(SKIP, ) * k) {
    sane(Bok(buf) && k != nil);
    size_t pos = Bdatalen(buf);
    size_t was = k->pos >> GAP;
    size_t now = pos >> GAP;
    if (was == now) return OK;
    u8 height = ctz64(now);
    if (now != was + 1) {
        u8 topflip = 63 - clz64(now ^ was);
        for (u8 h = 0; h < topflip; ++h) k->off[h] = (SKIPoff_t)u64max;
    }
    u8 len = height + 1;
    if (len > k->len) len = k->len;
    size_t sz = sizeof(SKIPoff_t) * len;
    $u8c w = {};
    w[0] = (u8c*)k->off;
    w[1] = w[0] + sz;
    call(TLVtinyfeed, Bu8idle(buf), SKIP_TLV_TYPE, w);
    u64 mask = (1 << GAP) - 1;
    SKIPoff_t myoff = pos & mask;
    for (u8 h = 0; h <= height; ++h) k->off[h] = myoff;
    k->pos = pos;
    if (height + 1 > k->len) k->len = height + 1;
    k->tlvlen = Bdatalen(buf) - pos;
    done;
}

fun ok64 X(SKIP, drain)(X(SKIP, ) * hop, Bu8 buf, size_t pos);

// k->gap must be set
fun pro(X(SKIP, load), X(SKIP, ) * k, Bu8 buf) {
    sane(k != nil && Bok(buf));
    u8c$ data = Bu8cdata(buf);
    test($len(data) >= SKIP_TERM_LEN, SKIPbad);
    a$last(u8c, term, data, SKIP_TERM_LEN);
    u8 lit = 0;
    $u8drain8(&lit, term);
    test(lit == SKIP_TERM_LIT, SKIPbad);
    SKIPoff_t off = 0;
    a$raw(bits, off);
    $u8drain(bits, term);
    test($len(data) >= SKIP_TERM_LEN + off, SKIPbad);
    size_t pos = $len(data) - SKIP_TERM_LEN - off;
    size_t mask = (1 << GAP) - 1;
    while (pos != 0) {
        X(SKIP, ) hop = {};
        call(X(SKIP, drain), &hop, buf, pos);
        while (k->len < hop.len) {
            k->off[k->len] = pos & mask;
            ++k->len;
        }
        if (hop.off[hop.len - 1] == (SKIPoff_t)u64max) break;  // TODO
        pos = X(SKIP, pos)(&hop, hop.len - 1);
        if (pos > hop.pos) {
            fprintf(stderr, "POS %lx->%lx(%i)\n", hop.pos, pos, hop.len - 1);
        }
    }
    k->pos = pos;
    done;
}

fun ok64 X(SKIP, drain)(X(SKIP, ) * hop, Bu8 buf, size_t pos) {
    if (pos == 0) return SKIPbof;
    a$dup(u8c, sub, Bdata(buf));
    if (pos > $len(sub)) return SKIPmiss;
    sub[0] += pos;
    SKIPoff_t const* w[2] = {};
    u8 t = 0;
    size_t l = $len(sub);
    ok64 o = TLVdrain(&t, (u8c$)w, sub);
    if (o != OK) return o;
    a$(SKIPoff_t, into, hop->off);
    if ((t != TLV_TINY_TYPE && t != SKIP_TLV_TYPE)) return SKIPbad;
    $copy(into, w);
    hop->len = $len(w);
    hop->pos = pos;
    hop->tlvlen = l - $len(sub);
    return OK;
}

fun ok64 X(SKIP, hop)(X(SKIP, ) * hop, Bu8 buf, X(SKIP, ) const* k, u8 height) {
    if (height >= k->len) return SKIPtoofar;
    if (k->off[height] == (SKIPoff_t)u64max) return SKIPnone;
    size_t now = k->pos >> GAP;
    hop->pos = X(SKIP, pos)(k, height);
    return X(SKIP, drain)(hop, buf, hop->pos);
}

fun ok64 X(SKIP, term)(Bu8 buf, X(SKIP, ) const* k) {
    u8$ idle = (u8$)Bidle(buf);
    if (SKIP_TERM_LEN > $len(idle)) return SKIPnoroom;
    SKIPoff_t off = Bdatalen(buf) - k->pos;
    a$rawc(bits, off);
    $u8feed1(idle, SKIP_TERM_LIT);
    $u8feed(idle, bits);
    return OK;
}

fun ok64 X(SKIP, mayfeed)(Bu8 buf, X(SKIP, ) * skips) {
    size_t pos = Bdatalen(buf);
    size_t was = skips->pos >> GAP;
    size_t now = pos >> GAP;
    if (now == was) return OK;  // X(SKIP, term)(buf, skips);
    return X(SKIP, feed)(buf, skips);
}

ok64 X(SKIP, load)(X(SKIP, ) * k, Bu8 buf);

fun pro(X(SKIP, find), u8c$ gap, Bu8 buf, $u8c x, SKIPcmpfn cmp) {
    sane(gap != nil && Bok(buf) && $ok(x) && cmp != nil);
    X(SKIP, ) k = {};
    call(X(SKIP, load), &k, buf);
    for (int h = k.len - 1; h >= 0; --h) {
        X(SKIP, ) hop = {};
        call(X(SKIP, hop), &hop, buf, &k, h);
        aB$(u8c, sub, buf, hop.pos + hop.tlvlen, k.pos);
        if (cmp((const $u8c*)&x, &sub) <= 0) {
            k = hop;
            h = k.len;
            continue;
        }
    }
    X(SKIP, ) prev = {};
    call(X(SKIP, hop), &prev, buf, &k, 0);
    gap[0] = buf[0] + prev.pos + prev.tlvlen;
    gap[1] = buf[0] + k.pos;
    done;
}

fun ok64 X(SKIPTLV, find)(u8c$ rec, Bu8 buf, $u8c x, SKIPcmpfn cmp) {
    $u8c gap = {};
    ok64 o = X(SKIP, find)(gap, buf, x, cmp);
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

#undef SKIPoff_t
