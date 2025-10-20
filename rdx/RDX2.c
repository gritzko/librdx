#include "RDX2.h"

#include <stddef.h>

#include "abc/01.h"
#include "abc/B.h"
#include "abc/PRO.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"

a$strc(RDX_ROOT_REC, " \x01\x00");

ok64 rdxbInit(rdxb reader, u8cs data) {
    sane(Bok(reader) && $ok(data));
    Breset(reader);
    call(rdxsFed1, Bidle(reader));
    rdxp p = Blastp(reader);
    zerop(p);
    u8csDup(p->rest, data);
    call(rdxbNext, reader);
    done;
}

ok64 rdxbNext(rdxb reader) {
    sane(Bok(reader) && Bdatalen(reader) != 0);
    rdxp p = Blastp(reader);
    test(!$empty(p->rest), RDXnodata);
    u8 type;
    u8cs id, val;
    call(TLVDrainKeyVal, &type, id, val, p->rest);
    call(ZINTu8sDrain128, id, &p->id.src, &p->id.seq);
    switch (type) {
        case RDX_FLOAT:
            call(ZINTf64drain, &p->f, val);
            break;
        case RDX_INT:
            call(ZINTi64drain, &p->i, val);
            break;
        case RDX_REF:
            call(ZINTu8sDrain128, val, &p->r.src, &p->r.seq);
            break;
        case RDX_STRING:
            u8csDup(p->s, val);
            break;
        case RDX_TERM:
            u8csDup(p->t, val);
            break;
        default:
            u8csDup(p->plex, val);
            break;
    }
    done;
}

ok64 rdxbInto(rdxb reader) {
    sane(Bok(reader) && Bdatalen(reader) > 0 && Bidlelen(reader) > 0 &&
         RDXisPLEX(rdxbType(reader)));
    rdxp up = Blastp(reader);
    call(rdxsFed1, Bidle(reader));
    rdxp p = Blastp(reader);
    zerop(p);
    u8csDup(p->rest, up->plex);
    done;
}

ok64 rdxbOuto(rdxb reader) {
    sane(Bok(reader) && Bdatalen(reader) > 0);
    --reader[2];
    done;
}

ok64 rdxpsUpAt(rdxps heap, size_t ndx, rdxZ z) {
    sane(ndx < rdxpsLen(heap));
    int a = ndx;
    while (a) {
        size_t b = (a - 1) / 2;  // parent
        if (!z($at(heap, a), $at(heap, b))) break;
        rdxpsSwap(heap, a, b);
        a = b;
    }
    done;
}

ok64 rdxpsDownAt(rdxps heap, size_t ndx, rdxZ z) {
    sane(rdxpsOK(heap) && ndx < rdxpsLen(heap) && z != NULL);
    size_t i = ndx;
    size_t n = rdxpsLen(heap);
    do {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t j = left;
        size_t right = left + 1;
        if (right < n && ($at(heap, right), $at(heap, j))) j = right;
        if (!z($at(heap, j), $at(heap, i))) break;
        rdxpsSwap(heap, i, j);
        i = j;
    } while (1);
    done;
}

ok64 rdxpsEqs(rdxps heap, u32p eqs, rdxZ z) {
    sane($ok(heap) && eqs != NULL && z != NULL);
    if ($len(heap) <= 1) {
        *eqs = $len(heap);
        done;
    }
    *eqs = 1;
    a_pad(u8, q, RDX_MAX_INPUTS);
    u8Bfeed2(q, 1, 2);
    eats(u8, n, q) {
        if (!z($at(heap, 0), $at(heap, *n))) {
            u8 j1 = 2 * *n + 1;
            u8Bfeed2(q, j1, j1 + 1);
            rdxpSwap($atp(heap, *eqs), $atp(heap, *n));
            ++*eqs;
        }
    }
    done;
}

ok64 rdxpsNexts(rdxps heap, u32 eqs, rdxZ z) {  // ejects
    sane($ok(heap) && z != NULL);
    u8 i = eqs;
    while (i > 0) {
        --i;
        call(rdxNext, $at(heap, i));
        rdxpsDownAt(heap, i, z);
    }
    done;
}

//
ok64 RDXu8ssMonoFeed(u8css spans, u8cs input, rdxZ z) {
    sane(!$empty(spans) && input != NULL && z != NULL);
    u8cs span;
    u8csDup(span, input);
    rdx fit, old;  // todo it[2]
    rdxInit(&fit, input);
    call(rdxNext, &fit);
    rdxMove(&old, &fit);
    ok64 o;
    scan(rdxNext, &fit) {
        if (!z(&old, &fit)) {  // order break
            span[1] = old.rest[0];
            call(u8cssFeed1, spans, span);
            u8csDup(span, old.rest);
        }
        rdxMove(&old, &fit);
    }
    seen($nodata);
    span[1] = old.rest[0];
    call(u8cssFeed1, spans, span);
    done;
}

ok64 RDXu8sFeed(u8s rdx, rdxcp fit) {
    a_pad(u8, val_pad, 16);
    a_pad(u8, key_pad, 16);
    u8cs val = {};
    u8 lit = fit->type;
    switch (lit) {
        case 0:
            break;
        case RDX_FLOAT:
            ZINTf64feed(val_pad_idle, &fit->f);
            $mv(val, val_pad_data);
            break;
        case RDX_INT:
            ZINTi64feed(val_pad_idle, &fit->i);
            $mv(val, val_pad_data);
            break;
        case RDX_REF:
            ZINTu8sFeed128(val_pad_idle, fit->r.src, fit->r.seq);
            $mv(val, val_pad_data);
            break;
        case RDX_STRING:
            $mv(val, fit->s);
            break;
        case RDX_TERM:
            $mv(val, fit->t);
            break;
        case RDX_TUPLE:
        case RDX_LINEAR:
        case RDX_EULER:
        case RDX_MULTIX:
            $mv(val, fit->plex);
            break;
    }
    ZINTu8sFeed128(key_pad_idle, fit->id.src, fit->id.seq);
    return TLVFeedkv(rdx, lit, key_pad_datac, val);
}

ok64 RDXu8bInto(u8b builder, rdxcp what) {
    sane(Bok(builder) && what != NULL);
    u8sp idle = Bidle(builder);
    call(u8sFeed1, idle, what->type);
    size_t dlen = Bdatalen(builder);
    test(dlen <= u32max, Bnoroom);
    call(u8sFeed32, idle, (u32*)&dlen);
    size_t ol = $len(idle);
    call(ZINTu8sFeed128, idle, what->id.src, what->id.seq);
    call(u8sFeed1, idle, ol - $len(idle));
    builder[1] = builder[2];
    done;
}

ok64 RDXu8bOuto(u8b builder, rdxcp what) {
    sane(Bok(builder) && $len(Bpast(builder)) >= 6 && what != NULL);
    u8csp past = Bu8cpast(builder);
    u8cs oldpast;
    u8csDup(oldpast, past);
    u8sp data = Bdata(builder);
    a_pad(u8, oldid, 16);
    u8 idlen = 0;
    u32 prevlen = 0;
    call(u8sPop1, past, &idlen);
    call(u8sPop, past, oldid_idle);
    call(u8sPop32, past, &prevlen);
    size_t newlen = $len(data);
    if (newlen < 0xff) {
        u8sFeed1(data, (u8)newlen);
    } else {
        u8sFeed32(data, (u32*)&newlen);
    }
    u8sFeed1(data, idlen);
    u8sFeed(data, oldid_datac);
    ptrdiff_t d = $len(past);
    if (d != $len(oldpast)) {
        past[1] = oldpast[1];
        call(u8bShift, builder, d);
    }
    // todo test(what==NULL || what->type==type, RDXbadnest);
    builder[1] = builder[0] + prevlen;
    done;
}

ok64 RDXu8bFeedDeep(u8b builder, rdxb reader) {
    sane(Bok(builder) && $ok(reader));
    scan(rdxbNext, reader) {
        if (RDXisFIRST(rdxbType(reader))) {
            call(RDXu8bFeed, builder, rdxbLast(reader));
        } else {
            rdxcp top = rdxbLast(reader);
            call(RDXu8bInto, builder, top);
            call(rdxbInto, reader);
            call(RDXu8bFeedDeep, builder, reader);
            call(rdxbOuto, reader);
            call(RDXu8bOuto, builder, top);
        }
    }
    seen($nodata);
    done;
}

ok64 RDXu8bFeedAll(u8b into, u8cs from) {
    sane(Bok(into) && $ok(from));
    a_pad(rdx, reader, RDX_MAX_NESTING);
    call(rdxbInit, reader, from);
    u8b builder = {into[0], into[0], into[0], into[1]};
    call(RDXu8bFeedDeep, builder, reader);
    into[0] = builder[2];
    done;
}

ok64 rdxNext(rdxp it) {
    sane($ok(it->rest));
    u8 lit;
    u8cs key = {}, val = {};
    size_t ol = $len(it->rest);
    call(TLVDrainKeyVal, &lit, key, val, it->rest);
    it->reclen = ol - $len(it->rest);
    switch (lit) {
        case 0:
            break;
        case RDX_FLOAT:
            call(ZINTf64drain, &it->f, val);
            break;
        case RDX_INT:
            call(ZINTi64drain, &it->i, val);
            break;
        case RDX_REF:
            call(ZINTu8sDrain128, val, &it->r.src, &it->r.seq);
            break;
        case RDX_STRING:
            $mv(it->s, val);
            break;
        case RDX_TERM:
            $mv(it->t, val);
            break;
        case RDX_TUPLE:
        case RDX_LINEAR:
        case RDX_EULER:
        case RDX_MULTIX:
            $mv(it->plex, val);
            break;
    }
    call(ZINTu8sDrain128, key, &it->id.src, &it->id.seq);
    done;
}

ok64 RDXu8bMergeLWW(u8b merged, rdxpsc eqs) {
    sane(Bok(merged) && !$empty(eqs));
    int eqlen = 1;
    rdxp toprev = **eqs;
    for (rdxp* p = *eqs + 1; p < $term(eqs); ++p) {
        if (rdxLastWriteWinsZ(**eqs, *p) == LESS) {
            rdxpSwap(*eqs, p);
            eqlen = 1;
            toprev = **eqs;
        } else if (rdxLastWriteWinsZ(*p, **eqs) != LESS) {
            rdxpSwap(*eqs + eqlen, p);
            ++eqlen;
            // todo toprev
        }
    }
    if (eqlen == 1) return RDXu8bFeed(merged, toprev);
    rdxps wins;
    b8 plex = NO;
    rdxZ z = NULL;
    switch ((**wins)->type) {
        case RDX_FLOAT: {
            f64 max = f64MinValue;
            eats(rdxp, p, wins) if ((**p).f > max) max = (**p).f;
            toprev->f = max;
            break;
        }
        case RDX_INT: {
            i64 max = i64MinValue;
            eats(rdxp, p, wins) if ((**p).i > max) max = (**p).i;
            toprev->i = max;
            break;
        }
        case RDX_REF: {
            ref128 max = {};
            eats(rdxp, p, wins) if (ref128Z(&max, &(**p).r)) max = (**p).r;
            toprev->r = max;
            break;
        }
        case RDX_STRING: {
            u8cs max = {};
            eats(rdxp, p, wins) if (u8csZ(max, (**p).s)) u8csDup(max, (**p).s);
            u8csDup(toprev->s, max);
            break;
        }
        case RDX_TERM: {
            u8cs max = {};
            eats(rdxp, p, wins) if (u8csZ(max, (**p).t)) u8csDup(max, (**p).t);
            u8csDup(toprev->t, max);
            break;
        }
        case RDX_TUPLE: {
            plex = YES;
            z = rdxTupleZ;
            break;
        }
        case RDX_LINEAR: {
            plex = YES;
            z = rdxLinearZ;
            break;
        }
        case RDX_EULER: {
            plex = YES;
            z = rdxEulerZ;
            break;
        }
        case RDX_MULTIX: {
            plex = YES;
            z = rdxMultixZ;
            break;
        }
    }
    if (!plex) {
        call(RDXu8bFeed, merged, toprev);
    } else {
        a_pad(u8cs, inners, RDX_MAX_INPUTS);
        eats(rdxp, p, wins) u8cssFeed1(inners_idle, (**p).plex);
        call(RDXu8bInto, merged, toprev);
        call(RDXu8sMergeZ, $idle(merged), inners_data, z);
        call(RDXu8bOuto, merged, toprev);
    }
    done;
}

ok64 RDXu8bMerge(u8b merged, rdxps inputs, rdxZ z) {
    sane($ok(merged) && $ok(inputs) && z != NULL);
    while (!$empty(inputs)) {
        u32 eqs;
        rdxpsEqs(inputs, &eqs, z);
        rdxps eq = {inputs[0], inputs[0] + eqs};
        call(RDXu8bMergeLWW, merged, eq);
        rdxpsNexts(inputs, eqs, z);
    }
    done;
}

ok64 RDXu8sMergeZ(u8s merged, u8css inputs, rdxZ less) {
    sane($ok(merged) && $ok(inputs));
    a_pad(rdx, its, RDX_MAX_INPUTS);
    a_pad(rdxp, ins, RDX_MAX_INPUTS);

    eats(u8cs, i, inputs) {
        rdxInit(*its_idle, (u8cspc)i);
        call(rdxpsFeed1, ins_idle, *its_idle);  // noroom
        rdxsFed1(its_idle);
        rdxpsUp(ins_data, rdxTupleZ);
    }

    call(RDXu8bMerge, merged, ins_data, rdxTupleZ);

    done;
}

b8 rdx1Z(rdxcp a, rdxcp b) {
    u8 at = a->type;
    u8 bt = b->type;
    if (at != bt) return rdxTypeZ(a, b);
    switch (at) {
        case RDX_FLOAT:
            return a->f < b->f;
        case RDX_INT:
            return a->i < b->i;
        case RDX_REF:
            return ref128Z(&a->r, &b->r);
        case RDX_STRING:
            return u8csZ(a->s, b->s);
        case RDX_TERM:
            return u8csZ(a->t, b->t);
        default:
            return ref128Z(&a->id, &b->id);
    }
}

b8 rdxTypeZ(rdxcp a, rdxcp b) {
    u8 at = a->type;
    u8 bt = b->type;
    if (at == bt) return GREQ;
    b8 aplex = rdxIsPLEX(a);
    b8 bplex = rdxIsPLEX(b);
    return aplex == bplex ? at < bt : aplex < bplex;
}

b8 rdxTupleZ(rdxcp a, rdxcp b) { return GREQ; }

b8 rdxLinearZ(rdxcp a, rdxcp b) { return ref128Z(&a->id, &b->id); }

b8 rdxEulerZ(rdxcp a, rdxcp b) {
    rdx aa;
    if (a->type == RDX_TUPLE) {
        rdxInit(&aa, a->plex);
        rdxNext(&aa);
        a = &aa;
    }
    rdx bb;
    if (b->type == RDX_TUPLE) {
        rdxInit(&bb, b->plex);
        rdxNext(&bb);
        b = &bb;
    }
    return rdx1Z(a, b);
}

b8 rdxMultixZ(rdxcp a, rdxcp b) { return u64Z(&a->id.src, &b->id.src); }

b8 rdxLastWriteWinsZ(rdxcp a, rdxcp b) {
    if (!ref128Eq(&a->id, &b->id)) return ref128Z(&a->id, &b->id);
    u8 at = a->type;
    u8 bt = b->type;
    return at == bt ? rdx1Z(a, b) : rdxTypeZ(a, b);
}
