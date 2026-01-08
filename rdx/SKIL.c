#include "RDX.h"
#include "abc/01.h"
#include "abc/PRO.h"
#include "abc/ZINT.h"

fun u64 SKILBlock(u64 pos) { return (pos + 0x7f) >> 7; }
u64 SKILRank(u64 pos) {
    u64 b = SKILBlock(pos);
    return b ^ (b - 1);
}
#define SKIL_MIN_RANK 7

con ok64 SKILBAD = 0x1c51254b28d;

ok64 rdxNextSKIL(rdxp x) {
    sane(x);
    if (!u8csEmpty(x->data) && SKIL_LIT == (**x->data & ~TLVaA)) {
        u8 lit;
        u8cs val;
        call(TLVu8sDrain, x->data, &lit, val);
    }
    return rdxNextTLV(x);
}

ok64 rdxIntoSKIL(rdxp c, rdxp p) {
    sane(c && p);
    a_dup(u8c, data, p->plexc);
    p->cformat = RDX_FMT_SKIL;
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

    // Iterate backwards through skip pointers
    // for (u64c *sp = u64csTerm(skipb_datac) - 1; sp >= *skipb_datac; --sp) {
    $for(u64c, sp, skipb_datac) {
        u64 pos = *sp;
        a_rest(u8c, back, data, pos);
        if (!u8csLen(back)) continue;
        u8 lit;
        u8cs blocks2 = {};
        if (**back == (SKIL_LIT | TLVaA) || **back == SKIL_LIT) {
            pos += u8csLen(back);
            call(TLVu8sDrain, back, &lit, blocks2);
            pos -= u8csLen(back);
        }
        if (!u8csLen(back)) continue;
        rdx rec = {.format = RDX_FMT_SKIL};
        $mv(rec.data, back);
        call(rdxNextTLV, &rec);
        if (Z(&rec, c)) {
            // rec < c: target is after this position, keep searching
            from = pos;
            // printf("<%lu(%lu)", pos, SKILRank(pos));
            continue;
        } else if (Z(c, &rec)) {
            // c < rec: target is before this position
            // Load sub-skip-list and restart search in narrower range
            if (*blocks2) {
                u64bReset(skipb);
                call(ZINTu8sDrainBlocked, blocks2, skipb_idle);
                if (u64csLen(skipb_datac) == 0) break;  // mishap?
                // sp = u64csTerm(skipb_datac);
                sp = u64csHead(skipb_datac) - 1;
                // printf(">%lu(%lu)", pos, SKILRank(pos));
                continue;
            } else {
                // No sub-skip-list available, use current position
                // printf("x%lu(%lu)", pos, SKILRank(pos));
                break;
            }
        } else {
            // Equal: found exact match
            from = pos;
            // printf("=");
            break;
        }
    }
    // printf(" (%lu)\n", from);

    a_rest(u8c, rest, data, from);
    c->format = p->cformat;
    u8csMv(c->data, rest);
    c->ptype = p->type;
    c->cformat = 0;
    rdx c2 = *c;
    ok64 o = OK;
    do {
        o = rdxNextSKIL(c);
    } while (o == OK && rdxZ(c, &c2));
    if (o == END || rdxZ(&c2, c)) o = NONE;
    return o;
}

ok64 rdxOutoSKIL(rdxp c, rdxp p) { return rdxOutoTLV(c, p); }

ok64 rdxWriteSKIL(rdxp x, u64 len) {
    sane(x && len && x->extra);
    u64bp skipb = (u64bp)x->extra;
    u64sp skips = u64bData(skipb);
    // test(u64sLen(skips), BADARG);
    if (!u64sLen(skips)) {
        call(u64bFeed1, skipb, 0);
    }
    u64 prev = **skips;
    u64 pos = prev + len;
    **skips = pos;
    if (SKILBlock(prev) == SKILBlock(pos)) done;
    u64 rank = SKILRank(pos);
    a_tailf(u64c, drain, skips, (SKILRank(**drain) < rank));
    size_t dl = u64csLen(drain);
    if (rank >= SKIL_MIN_RANK && dl > 0) {
        a_pad(u8, rec, 256);
        call(ZINTu8sFeedBlocked, rec_idle, drain);
        u64 l = u8sLen(x->into);
        call(TLVu8sFeed, x->into, SKIL_LIT, rec_datac);
        l -= u8sLen(x->into);
        **skips += l;
        u64sShed(skips, dl);
    }
    call(u64bFeed1, skipb, pos);
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
    u64sFed1(u64bData(skips));
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
