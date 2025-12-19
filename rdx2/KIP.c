#include "KIP.h"

#include "RDX.h"
#include "abc/PRO.h"

/*

// [   [ ]|   ]
fun ok64 skip8csBack(skip8csp skip, u8cg data, u8 hi) {
    sane(u8cgOK(data) && u8cgIn(data, skip));
    u64 pos = skip[0] - data[0];
    if (pos == 0 || hi > skip8Top(pos)) return 0;
    u64 mask = (1 << hi) - 1;
    u64 was = (skip8Block(pos) - 1) & ~mask;
    u64 off = u8csAt(skip, hi + 2);
    if (off == 0xff) fail(SKIPNONE);
    u8cs p = {data[0] + skip8BlockOffset(was) + off, u8cgTerm(data)};
    test(u8csOK(p) && u8csLen(p) > 2 && *u8csHead(p) == SKIP_LIT &&
             u8csLen(p) >= 2 + u8csAt(p, 1),
         SKIPBADREC);
    p[1] = p[0] + u8csAt(p, 1);
    done;
}

fun ok64 skip8sFeed(skip8sp last, u8g into) {
    sane(u8gOK(into) && u8pIn(last, u8gDoneC(into)));
    a_pad(u8, sk, 64);
    // block ndx happy 0s
    // read
    u8cs tail = {last, into[1]};
    u8cs prev = {};
    u8 lit = 0;
    call(TLVu8sDrain, tail, &lit, prev);
    // collect the necessary length
    // test len
    call(u8bFeed, sk, prev);
    // blockdiff, new height
    // off x k
    // copy x k
    // ff x k
    // feed
    done;
}

fun ok64 u8gSkips(u8cg data, skip8csp skips) {  // ????????
    sane(u8cgOK(data) && data[1] + 1 < data[2] && *data[1] == SKIP_LIT);
    u8 top = skip8Top(u8cgDoneLen(data));
    test(data[1][1] <= top, BADSKIP);
    skips[0] = data[1] + 2;
    skips[1] = skips[0] + data[1][1];
    test(skips[1] < data[2], BADSKIP);
    done;
}
*/

// === === === === === === === === === === ===

ok64 rdxNextKIP(rdxp x) {
    sane(x);
    if (!$empty(x->data) && SKIP_LIT == (**x->data & ~TLVaA)) {
        u8 lit;
        u8cs val;
        call(TLVu8sDrain, x->data, &lit, val);
    }
    return rdxNextTLV(x);
}

ok64 rdxIntoKIP(rdxp c, rdxp p) {
    sane(c && p && p->extra);
    a_dup(u8c, data, p->plexc);
    if ($len(data) <= 0xff || c->type == 0) return rdxIntoTLV(c, p);
    skip512 skip = {.cur = $len(data)};
    u8 tall = skip512Tallness(&skip);
    skip.cur -= tall + 2;
    // might be off by 1
    if (skip512Tallness(&skip) != tall) skip.cur++;
    u8cs offs = {};
    u8 lit;
    a_rest(u8c, tail, data, skip.cur);
    call(TLVu8sDrain, tail, &lit, offs);
    test(lit == SKIP_LIT, SKIPBAD);
    a$raw(rawoff, skip.offs);
    u8sCopy(rawoff, offs);
    u64 from = 0;

    for (int h = top - 1; h >= 0; --h) {
        skip512 step = skip;
        u8s offs = {};
        call(skip512Back, &skip, h, offs);
        if (from >= skip.pos) continue;  // been there
        // read tlv
        // copy offs
        rdx rec = {.data = {}};
        call(rdxNextTLV, &rec);
        if (rdxZ(&rec, c)) {
            from = skip.pos;  //...
        } else if (rdxZ(c, &rec)) {
            skip = step;
            top;
        } else {
            from = skip.pos;  //...
            break;
        }
    }
    p->plex[0] += skip.pos;
    return rdxIntoTLV(c, p);
}
ok64 rdxOutoKIP(rdxp c, rdxp p) { return rdxOutoTLV(c, p); }

ok64 rdxWriteNextKIP(rdxp x) {
    sane(x);
    skip512p skip = 0;
    call(skip512bTop, (skip512bp)x->extra, &skip);
    u64 l = u8sLen(x->into);
    call(rdxWriteNextTLV, x);
    skip->pos += l - u8sLen(x->into);
    u8cs offs = {};
    ok64 o = skip512Next(skip, pos, offs);
    if (o == SKIPNONE) done;
    call(TLVu8sFeed, x->into, SKIP_LIT, offs);
    done;
}
ok64 rdxWriteIntoKIP(rdxp c, rdxp p) {
    sane(c && p);
    c->extra = p->extra;
    skip512bp sb = (skip512bp)p->extra;
    skip512p skip = 0;
    call(skip512bPushed, sb, &skip);
    c->extra = p->extra;
    // todo skip512
    return rdxWriteIntoTLV(c, p);
}
ok64 rdxWriteOutoKIP(rdxp c, rdxp p) {
    sane(c && p);
    i64 l = u8sLen(p->into);  // fixme all bs
    skip512p skip = 0;
    call(skip512bTop, (skip512bp)p->extra, &skip);
    // fixme double
    i64 pos = skip->pos + u8sLen(p->into) - l;  //?
    u8cs offs;
    ok64 o = skip512TallNext(skip, pos, offs);
    // fixme if next block
    call(TLVu8sFeed, c->into, SKIP_LIT, offs);
    // use len
    call(skip512bPop, (skip512bp)p->extra);
    return rdxWriteOutoTLV(c, p);
}
