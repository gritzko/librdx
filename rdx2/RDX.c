#include "RDX.h"

#include "abc/01.h"
#include "abc/BUF.h"
#include "abc/PRO.h"
#include "abc/SHA.h"

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

fun ok64 rdxStringZ(rdxcp a, rdxcp b) {
    sane(a && b);
    if (a->cformat == RDX_UTF_ENC_UTF8 && b->cformat == RDX_UTF_ENC_UTF8)
        return u8csZ(&a->s, &b->s);
    test(a->cformat < RDX_UTF_ENC_LEN && b->cformat < RDX_UTF_ENC_LEN, RDXBAD);
    UTFRecode are = UTABLE[a->cformat][UTF8_DECODER_ONE];
    UTFRecode bre = UTABLE[b->cformat][UTF8_DECODER_ONE];
    a_pad(u8, autf, 16);
    a_pad(u8, butf, 16);
    a_dup(u8c, as, a->s);
    a_dup(u8c, bs, b->s);
    while (u8csLen(as) && u8csLen(bs)) {
        call(are, autf_idle, as);
        call(bre, butf_idle, bs);
        int z = $cmp(autf_datac, butf_datac);
        if (z != 0) return z < 0;
        Breset(autf);
        Breset(butf);
    }
    return u8csLen(as) < u8csLen(bs);
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
        case RDX_TYPE_STRING:
            return rdxStringZ(a, b);
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
    if (aseq != bseq) return aseq > bseq;
    u64 asrc = a->id.src & id128SrcMask;
    u64 bsrc = b->id.src & id128SrcMask;
    if (asrc != bsrc) return asrc > bsrc;
    if (a->type != b->type) return u8Z(&a->type, &b->type);
    if (!rdxTypePlex(a)) return !rdx1Z(a, b);
    return NO;
}

fun ok64 rdxpWinZ(rdxpcp a, rdxpcp b) { return rdxWinZ(*a, *b); }

ok64 rdxMerge(rdxp into, rdxg inputs) {
    sane(into && rdxgOK(inputs) && !rdxgEmpty(inputs));
    rdxz Z = ZTABLE[(**inputs).ptype];
    // todo norm run
    a_dup(rdx, eqs, inputs);
    while (rdxsLen(inputs)) {
        $rof(rdx, p, eqs) {
            ok64 o = rdxNext(p);
            if (o == OK) {
                rdxsDownAtZ(inputs, p - *inputs, Z);
            } else if (o == END) {
                *p = *rdxsLast(inputs);
                rdxgFreed1(inputs);
                if (p < inputs[1]) rdxsDownAtZ(inputs, p - inputs[0], Z);
            } else {
                fail(o);
            }
        }
        if (rdxsEmpty(inputs)) break;
        rdxsTopsZ(inputs, eqs, Z);
        a_dup(rdx, wins, eqs);
        if (rdxsLen(eqs) > 1) {
            rdxsHeapZ(eqs, rdxWinZ);
            rdxsTopsZ(eqs, wins, rdxWinZ);
        }
        rdxMv(into, *wins);
        call(rdxNext, into);
        if ((**wins).type < RDX_TYPE_PLEX_LEN) {
            test(rdxgIdleLen(inputs) >= rdxsLen(wins), RDXNOROOM);
            a_gauge(rdx, sub, rdxgIdle(inputs));
            rdx c = {};
            call(rdxInto, &c, into);
            $for(rdx, q, wins) {
                (**sub_idle).type = 0;
                call(rdxInto, *sub_idle, q);
                rdxgFed1(sub);
            }
            call(rdxMerge, &c, sub);
            call(rdxOuto, &c, into);
            $for(rdx, q, wins) {
                call(rdxOuto, NULL, q);  // c is optional
            }
        }
    }
    done;
}

ok64 rdxHashMore(SHAstate* hash, rdxp root) {
    // todo todo todo
    sane(hash && root);
    a_pad(u8, head, 8);
    u8sFeed1(head_idle, RDX_TYPE_LIT[root->type]);
    u32 len = 0;
    u8sFeed32(head_idle, &len);
    SHAfeed(hash, head_datac);
    // feed lit, len?, id, value (str enc - 2 pass)
    switch (root->type) {
        case RDX_TYPE_ROOT:
        // collection types
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX:
            rdx c = {};
            call(rdxInto, &c, root);
            scan(rdxNext, &c) { call(rdxHashMore, hash, &c); }
            seen(END);
            call(rdxOuto, &c, root);
            break;
        // primitive types
        case RDX_TYPE_FLOAT:
        case RDX_TYPE_INT: {
            a_rawc(vraw, root->i);
            SHAfeed(hash, vraw);
            break;
        }
        case RDX_TYPE_REF: {
            a_rawc(vraw, root->r);
            SHAfeed(hash, vraw);
            break;
        }
        case RDX_TYPE_STRING:
            if (root->cformat != RDX_UTF_ENC_UTF8) return NOTIMPLYET;
            break;
        case RDX_TYPE_TERM:
            SHAfeed(hash, root->t);
            break;
        case RDX_TYPE_BLOB:
            return NOTIMPLYET;
        default:
            return RDXBAD;
    }
    done;
}

ok64 rdxHash(sha256p hash, rdxp root) {
    sane(hash && root);
    SHAstate state = {};
    SHAopen(&state);
    call(rdxHashMore, &state, root);
    SHAclose(&state, hash);
    done;
}
