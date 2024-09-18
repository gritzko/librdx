#define T X(, )
#define GAP ctz64(sizeof(T))

fun u8 X(SKIP, height)(size_t pos, SKIPs const* skips) {
    size_t now = pos >> GAP;
    return ctz64(now);
}

fun size_t X(SKIP, pos)(SKIPs const* skips, u8 height) {
    size_t now = skips->pos >> GAP;
    size_t step = 1 << height;
    if (step >= now) return 0;
    size_t was = (now - step) & ~(step - 1);
    return (was << GAP) + skips->off[height];
}

fun pro(X(SKIP, feed), Bu8 buf, SKIPs* k) {
    sane(Bok(buf) && k != nil);
    size_t pos = Bdatalen(buf);
    size_t was = k->pos >> GAP;
    size_t now = pos >> GAP;
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
    u64 mask = (1 << GAP) - 1;
    u16 myoff = pos & mask;
    for (u8 h = 0; h <= height; ++h) k->off[h] = myoff;
    k->pos = pos;
    if (len > k->len) k->len = len;
    k->tlvlen = Bdatalen(buf) - pos;
    done;
}

fun ok64 X(SKIP, drain)(SKIPs* hop, Bu8 buf, size_t pos);

// k->gap must be set
fun pro(X(SKIP, load), SKIPs* k, Bu8 buf) {
    sane(k != nil && Bok(buf));
    u8c$ idle = Bu8cidle(buf);
    test($len(idle) >= SKIP_TERM_LEN && **idle == '2', SKIPbad);
    u16 off = idle[0][2];
    off <<= 8;
    off |= idle[0][1];
    size_t pos = (Bdatalen(buf) & ~SKIP_MASK) | off;
    while (pos != 0) {
        SKIPs hop = {};
        call(X(SKIP, drain), &hop, buf, pos);
        while (k->len < hop.len) {
            k->off[k->len] = pos & ~SKIP_MASK;
            ++k->len;
        }
        size_t pos = X(SKIP, pos)(&hop, hop.len - 1);
    }
    k->pos = pos;
    done;
}

fun ok64 X(SKIP, drain)(SKIPs* hop, Bu8 buf, size_t pos) {
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

fun ok64 X(SKIP, hop)(SKIPs* hop, Bu8 buf, SKIPs const* k, u8 height) {
    if (height >= k->len) return SKIPtoofar;
    if (k->off[height] == SKIP_MASK) return SKIPnone;
    size_t now = k->pos >> GAP;
    hop->pos = X(SKIP, pos)(k, height);
    return X(SKIP, drain)(hop, buf, hop->pos);
}

fun ok64 X(SKIP, term)(Bu8 buf, SKIPs const* k) {
    u8$ idle = (u8$)Bidle(buf);
    if (SKIP_TERM_LEN > $len(idle)) return SKIPnoroom;
    if ((Bdatalen(buf) & ~SKIP_MASK) != (k->pos & ~SKIP_MASK)) return SKIPnone;
    size_t off = k->pos & SKIP_MASK;
    idle[0][0] = '2';
    idle[0][1] = off;
    idle[0][2] = off >> 8;
    return OK;
}

fun ok64 X(SKIP, mayfeed)(Bu8 buf, SKIPs* skips) {
    size_t pos = Bdatalen(buf);
    size_t was = skips->pos >> GAP;
    size_t now = pos >> GAP;
    if (now == was) X(SKIP, term)(buf, skips);
    return X(SKIP, feed)(buf, skips);
}

ok64 X(SKIP, load)(SKIPs* k, Bu8 buf);

fun pro(X(SKIP, find), u8c$ gap, Bu8 buf, $u8c x, SKIPcmpfn cmp) {
    sane(gap != nil && Bok(buf) && $ok(x) && cmp != nil);
    SKIPs k = {};
    call(X(SKIP, load), &k, buf);
    for (int h = k.len - 1; h >= 0; --h) {
        SKIPs hop = {};
        call(X(SKIP, hop), &hop, buf, &k, h);
        aB$(u8c, sub, buf, hop.pos + hop.tlvlen, k.pos);
        if (cmp((const $u8c*)&x, &sub) <= 0) {
            k = hop;
            h = k.len;
            continue;
        }
    }
    SKIPs prev = {};
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
