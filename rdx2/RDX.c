#include "RDX.h"

#include "abc/01.h"
#include "abc/BUF.h"
#include "abc/PRO.h"

ok64 rdxEulerTupleZ(rdxcp a, rdxcp b) {
    sane(a && b);
    rdx ac = {};
    rdx bc = {};
    call(rdxInto, &ac, (rdxp)a);
    rdxNext(&ac);
    call(rdxInto, &bc, (rdxp)b);
    rdxNext(&bc);
    return rdxEulerZ(&ac, &bc);
}

ok64 rdxEulerZ(rdxcp a, rdxcp b) {
    sane(a && b);
    rdx ac = {}, bc = {};
    u8cs acd = {}, bcd = {};
    if (a->type == RDX_TYPE_TUPLE) {
        ac.type = 0;
        call(rdxInto, &ac, (rdxp)a);
        $mv(acd, a->plex);
        ac.data = acd;
        rdxNext(&ac);
        a = &ac;
    }
    if (b->type == RDX_TYPE_TUPLE) {
        bc.type = 0;
        call(rdxInto, &bc, (rdxp)b);
        $mv(bcd, b->plex);
        bc.data = bcd;
        rdxNext(&bc);
        b = &bc;
    }
    if (a->type != b->type) return u8Z(&a->type, &b->type);
    if (a->type < RDX_TYPE_PLEX_LEN) {
        if (a->type == 0) return NO;
        return id128Z(&a->id, &b->id);
    }
    return rdx1Z(a, b);
}

ok64 rdxCopy(rdxp into, rdxp from) {
    sane(into && from);
    scan(rdxNext, from) {
        rdxMv(into, from);
        call(rdxNext, into);
        // todo PLEX
    }
    seen(END);
    into->type = 0;
    call(rdxNext, into);
    done;
}

ok64 rdxbCopy(rdxbp into, rdxbp from) {
    sane(rdxbOK(into) && rdxbOK(from) && rdxbWritable(into) &&
         !rdxbWritable(from));
    rdxp i = rdxbLast(into);
    rdxp f = rdxbLast(from);
    scan(rdxNext, f) {
        rdxMv(i, f);
        call(rdxNext, i);
        if (rdxTypePlex(f)) {
            test(rdxbIdleLen(into) && rdxbIdleLen(from), NOROOM);
            call(rdxbInto, into);
            call(rdxbInto, from);
            call(rdxbCopy, into, from);
            call(rdxbOuto, into);
            call(rdxbOuto, from);
        }
    }
    seen(END);
    i->type = 0;
    f->type = 0;
    done;
}

ok64 rdxbNext(rdxb b) {
    sane(rdxbOK(b));
    rdxp p = rdxbLast(b);
    call(VTABLE_NEXT[p->format], p);
    done;
}

ok64 rdxbInto(rdxb b) {
    sane(rdxbOK(b) && rdxbDataLen(b));
    rdxp p = rdxbLast(b);
    call(rdxbFed1, b);
    rdxp c = rdxbLast(b);
    call(VTABLE_INTO[p->format], c, p);
    done;
}

ok64 rdxbOuto(rdxb its) {
    sane(rdxbOK(its) && rdxbDataLen(its));
    rdxp c = rdxbLast(its);
    rdxbPop(its);
    rdxp p = rdxbLast(its);
    call(VTABLE_OUTO[p->format], c, p);
    done;
}

ok64 rdx1Z(rdxcp a, rdxcp b) {
    sane(a->type == b->type && a->type >= RDX_TYPE_PLEX_LEN);
    switch (a->type) {
        case RDX_TYPE_FLOAT:
            return f64Z(&a->f, &b->f);
        case RDX_TYPE_INT:
            return i64Z(&a->i, &b->i);
        case RDX_TYPE_REF:
            return id128Z(&a->r, &b->r);
        case RDX_TYPE_STRING:            // fixme
            return u8csZ(&a->s, &b->s);  // fixme
        case RDX_TYPE_TERM:
            return u8csZ(&a->t, &b->t);
        default:
            return NO;  // fixme
    }
}

fun ok64 rdxpV(rdxpcp a, rdxpcp b) {
    sane(a && b && (**a).ptype == (**b).ptype);
    return NOTIMPLYET;
}

ok64 rdxsSpotMerge(rdxp into, rdxbp from) {
    a_dup(rdx, next, rdxbIdle(from));
    // scan, find eqs rdxsFeedP(next, eq)
    // $mv(mydata, rdxbDataC(from));
    // $mv(rdxbDataC(from), next);
}

ok64 rdxsHeapMerge(rdxp into, rdxbp ins) {
    sane(into && rdxbOK(ins));
    a_pad(rdxp, heap, RDX_MAX_INPUTS);
    $for(rdx, i, rdxbData(ins)) call(HEAPrdxpPush1Z, heap, i, rdxpZ);
    while (rdxpbDataLen(heap)) {
        rdxps eqs = {};
        HEAPrdxpEqsZ(rdxpbData(heap), eqs, rdxpZ);
        HEAPrdxpMakeZ(eqs, rdxpV);
        rdxps wins = {};
        HEAPrdxpEqsZ(wins, eqs, rdxpV);
        if ((**wins)->type < RDX_TYPE_PLEX_LEN) {
            a_dup(rdx, old, rdxbData(ins));
            rdxsAte(rdxbData(ins));
            rdx c = {};
            call(rdxInto, &c, into);
            $for(rdxp, q, eqs) {
                rdxp p;
                call(rdxbPushed, ins, &p);
                call(rdxInto, p, *q);
            }
            call(rdxsHeapMerge, into, ins);
            call(rdxOuto, &c, into);
            $mv(rdxbData(ins), old);
        } else {
            rdxMv(into, **wins);
            call(rdxNext, into);
        }
        $rof(rdxp, p, eqs) {  // fixme
            ok64 o = rdxNext(*p);
            if (o == OK) {
                HEAPrdxpDownAt(rdxpbData(heap), p - *eqs);
            } else if (o == END) {
                *p = *rdxpbLast(heap);
                rdxpbPop(heap);
            } else {
                fail(o);
            }
        }
    }
    done;
}

ok64 rdxHash(sha256p hash, rdxp root) {
    sane(hash && root);
    done;
}
