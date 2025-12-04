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
    if (a->type == RDX_TYPE_TUPLE) {  // FIXME mess
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

fun ok64 rdxWinZ(rdxcp a, rdxcp b) {
    sane(a && b && a->ptype == b->ptype);
    u64 aseq = a->id.seq & id128SeqMask;
    u64 bseq = b->id.seq & id128SeqMask;
    if (aseq != bseq) return aseq < bseq;
    u64 asrc = a->id.src & id128SrcMask;
    u64 bsrc = b->id.src & id128SrcMask;
    if (asrc != bsrc) return asrc < bsrc;
    if (a->type != b->type) return u8Z(&a->type, &b->type);
    if (!rdxTypePlex(a)) return rdx1Z(a, b);
    return NO;
}

fun ok64 rdxpWinZ(rdxpcp a, rdxpcp b) { return rdxWinZ(*a, *b); }

/*
ok64 rdxMerge(rdxp into, rdxbp ins) {
    sane(into && rdxbOK(ins));
    a_pad(rdxp, heap, RDX_MAX_INPUTS);
    $for(rdx, i, rdxbData(ins)) call(HEAPrdxpPush1Z, heap, i, rdxpZ);
    while (rdxpbDataLen(heap)) {
        rdxps eqs = {};
        HEAPrdxpEqsZ(rdxpbData(heap), eqs, rdxpZ);
        a_dup(rdxp, wins, eqs);
        if (rdxpsLen(eqs) > 1) {
            HEAPrdxpMakeZ(eqs, rdxpWinZ);
            HEAPrdxpEqsZ(wins, eqs, rdxpWinZ);
        }
        if ((**wins)->type < RDX_TYPE_PLEX_LEN) {
            a_dup(rdx, old, rdxbData(ins));
            rdxsAte(rdxbData(ins));
            rdx c = {};
            call(rdxInto, &c, into);
            $for(rdxp, q, wins) {
                rdxp p = 0;
                call(rdxbPushed, ins, &p);
                p->type = 0;
                call(rdxInto, p, *q);
            }
            call(rdxMerge, into, ins);
            call(rdxOuto, &c, into);
            $mv(rdxbData(ins), old);
        } else {
            rdxMv(into, **wins);
            call(rdxNext, into);
        }
        $rof(rdxp, p, eqs) {
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
*/

ok64 rdxMergeIn(rdxp into, rdxs ins, rdxs idle) {
    sane(into && rdxsOK(ins) && rdxsOK(idle) && !$empty(ins));
    rdxz Z = ZTABLE[(**ins).ptype];
    call(rdxsHeapZ, ins, Z);
    while (rdxsLen(ins)) {
        rdxs eqs = {};
        rdxsTopsZ(ins, eqs, Z);
        a_dup(rdx, wins, eqs);
        if (rdxsLen(eqs) > 1) {
            rdxsHeapZ(eqs, rdxWinZ);
            rdxsTopsZ(wins, eqs, rdxWinZ);
        }
        if ((**wins).type < RDX_TYPE_PLEX_LEN) {
            test($len(idle) >= $len(wins), RDXNOROOM);
            a_gauge(rdx, sub, idle);
            rdx c = {};
            call(rdxInto, &c, into);
            $for(rdx, q, wins) {
                (**sub_idle).type = 0;
                call(rdxInto, *sub_idle, q);
                ++sub_idle;
            }
            call(rdxMergeIn, &c, sub_data, sub_idle);
            call(rdxOuto, &c, into);
            $for(rdx, q, wins) {
                call(rdxOuto, NULL, q);  // c is optional
            }
        } else {
            rdxMv(into, *wins);
            call(rdxNext, into);
        }
        $rof(rdx, p, eqs) {
            ok64 o = rdxNext(p);
            if (o == OK) {
                rdxsDownAt(ins, p - *ins);
            } else if (o == END) {
                *p = *rdxsLast(ins);
                --ins[1];
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
