#include "RDX.h"
#include "abc/01.h"
#include "abc/PRO.h"
#include "abc/ZINT.h"

con u8 SKIL_LIT = 'K';
con ok64 SKILBAD = 0x1c51254b28d;

ok64 rdxNextSKIL(rdxp x) {
    sane(x);
    if (!$empty(x->data) && SKIL_LIT == (**x->data & ~TLVaA)) {
        u8 lit;
        u8cs val;
        call(TLVu8sDrain, x->data, &lit, val);
    }
    return rdxNextTLV(x);
}

ok64 rdxIntoSKIL(rdxp c, rdxp p) {
    sane(c && p);
    a_dup(u8c, data, p->plexc);
    if ($len(data) <= 0xff || c->type == 0) return rdxIntoTLV(c, p);
    a_pad(u64, skipb, 248);  // fixme :)
    u8 flen = *u8csLast(data);
    test(flen + 3 < u8csLen(data), SKILBAD);
    a_tail(u8c, flush, data, flen + 3);
    u8 lit = 0;
    u8cs blocks = {};
    call(TLVu8sDrain, flush, &lit, blocks);
    test(lit == SKIL_LIT, SKILBAD);
    call(u8csShed1, blocks);
    call(ZINTu8sDrainBlocked, blocks, skipb_idle);
    u64 from = 0;
    rdxz Z = ZTABLE[p->type];

    $rof(u64c, p, skipb_datac) {
        u64 pos = *p;
        printf("try at %lu\n", pos);
        a_rest(u8c, back, data, pos);
        if (!u8csLen(back)) continue;  // fixme
        u8 lit;
        u8cs blocks2 = {};
        if (**back == (SKIL_LIT | TLVaA)) {
            pos += u8csLen(back);
            call(TLVu8sDrain, back, &lit, blocks2);
            pos -= u8csLen(back);
        }
        if (!u8csLen(back)) continue;  // fixme
        rdx rec = {.format = RDX_FMT_SKIL};
        $mv(rec.data, back);
        call(rdxNextTLV, &rec);
        if (Z(&rec, c)) {
            from = pos;  //...
        } else if (Z(c, &rec)) {
            u64bReset(skipb);
            call(ZINTu8sDrainBlocked, blocks2, skipb_idle);
            p = u64csTerm(skipb_datac);
            // before/after/none
        } else {
            from = pos;
            break;
        }
    }

    u8csMv(c->data, p->plexc);
    call(u8csUsed, c->data, from);
    call(rdxIntoTLV, c, p);
    done;
}

ok64 rdxOutoSKIL(rdxp c, rdxp p) { return rdxOutoTLV(c, p); }

fun u64 SKILBlock(u64 pos) { return (pos + 0xff) >> 8; }
fun u64 SKILRank(u64 pos) {
    u64 b = SKILBlock(pos);
    return b ^ (b - 1);
}

ok64 rdxWriteSKIL(rdxp x, u64 len) {
    sane(x && len && x->extra);
    u64bp skipb = (u64bp)x->extra;
    u64sp skips = u64bData(skipb);
    // test(u64sLen(skips), BADARG);
    if (!u64sLen(skips)) {
        // call(u64bFeed1, skipb, len);
        done;
    }
    u64 pos = *u64sLast(skips) + len;
    printf("pos %lu idle %lu sum %lu\n", pos, u8sLen(x->into),
           pos + u8sLen(x->into));
    u8 rank = SKILRank(pos);
    a_tailf(u64c, drain, skips, (SKILRank(**drain) < rank));
    if (u64csLen(drain)) {
        printf("skips len %lu\n", u64csLen(drain));
        a_pad(u8, rec, 256);
        call(ZINTu8sFeedBlocked, rec_idle, drain);
        u64 l = u8sLen(x->into);
        call(TLVu8sFeed, x->into, SKIL_LIT, rec_datac);
        l -= u8sLen(x->into);
        pos += l;
        u64sShed(skips, u64csLen(drain));

        call(u64bFeed1, skipb, pos);
    }
    done;
}

ok64 rdxWriteNextSKIL(rdxp x) {
    sane(x);
    u64 l = u8sLen(x->into);
    call(rdxWriteNextTLV, x);
    if (!rdxTypePlex(x)) {
        l -= u8sLen(x->into);
        call(rdxWriteSKIL, x, l);
    }
    done;
}

ok64 rdxWriteIntoSKIL(rdxp c, rdxp p) {
    sane(c && p);
    c->extra = p->extra;
    u64bp skipb = (u64bp)p->extra;
    p->len = u64bDataLen(skipb);
    u64sUsedAll(u64bData(skipb));
    call(u64bFeed1, skipb, 0);
    call(rdxWriteIntoTLV, c, p);
    c->format = RDX_FMT_SKIL | RDX_FMT_WRITE;
    done;
}

ok64 rdxWriteOutoSKIL(rdxp c, rdxp p) {
    sane(c && p);

    u64bp skips = (u64bp)c->extra;
    // fixme 0xff if (u64bDataLen(skips) && *u64bLast(skips))
    a_pad(u8, rec, 256);
    printf("skips flush len %lu\n", u64bDataLen(skips));
    call(ZINTu8sFeedBlocked, rec_idle, u64bDataC(skips));
    call(u8sFeed1, rec_idle, u8sLen(rec_data));
    call(TLVu8sFeed, c->into, SKIL_LIT, rec_datac);
    u64sShedAll(u64bData(skips));

    call(u64gShed, u64bPastData(skips), p->len);
    u64 l = u8sLen(p->into);
    call(rdxWriteOutoTLV, c, p);
    l -= u8sLen(p->into);
    call(rdxWriteSKIL, p, l);

    done;
}
