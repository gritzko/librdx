#include "RDX.h"

#include "abc/01.h"
#include "abc/BUF.h"
#include "abc/NACL.h"
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
    rdx ac = {}, bc = {};             // todo
    if (a->type == RDX_TYPE_TUPLE) {  // FIXME mess
        ac.type = 0;
        call(rdxInto, &ac, (rdxp)a);
        rdxNext(&ac);
        a = &ac;
    }
    if (b->type == RDX_TYPE_TUPLE) {
        bc.type = 0;
        call(rdxInto, &bc, (rdxp)b);
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
    sane(into && from && rdxWritable(into) && !rdxWritable(from));
    scan(rdxNext, from) {
        rdxMv(into, from);
        call(rdxNext, into);
        if (rdxTypePlex(from)) {
            rdx cinto = {};
            rdx cfrom = {};
            call(rdxInto, &cinto, into);
            call(rdxInto, &cfrom, from);
            call(rdxCopy, &cinto, &cfrom);
            call(rdxOuto, &cinto, into);
            call(rdxOuto, &cfrom, from);
        }
    }
    seen(END);
    into->type = 0;
    done;
}

ok64 rdxCopyF(rdxp into, rdxp from, voidf f, voidp p) {
    sane(into && from && rdxWritable(into) && !rdxWritable(from));
    scan(rdxNext, from) {
        rdxMv(into, from);
        call(rdxNext, into);
        if (rdxTypePlex(from)) {
            rdx cinto = {};
            rdx cfrom = {};
            call(rdxInto, &cinto, into);
            call(rdxInto, &cfrom, from);
            call(rdxCopy, &cinto, &cfrom);
            call(rdxOuto, &cinto, into);
            call(rdxOuto, &cfrom, from);
            call(f, p);
        }
    }
    seen(END);
    into->type = 0;
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

fun ok64 rdxpWinZ(rdxpcp a, rdxpcp b) { return rdxWinZ(*a, *b); }

ok64 rdxNorm(rdxg inputs) {
    sane(rdxgOK(inputs));
    $for(rdx, p, rdxgLeft(inputs)) {
        rdxz Z = ZTABLE[p->ptype];
        rdx copy = *p;
        if (OK != rdxNext(&copy)) break;
        rdx c = copy;
        while (OK == rdxNext(&c)) {
            if (Z(&c, &copy)) {
                p->data[1] = copy.data[0];
                rdxp pp = 0;
                call(rdxgFedP, inputs, &pp);
                zerop(pp);
                pp->format = p->format;
                pp->ptype = p->ptype;
                pp->len = 0;
                $mv(pp->data, copy.data);
                break;
            }
            copy = c;
        }
    }
    done;
}

ok64 rdxMerge(rdxp into, rdxg inputs) {
    sane(into && rdxgOK(inputs) && !rdxgEmpty(inputs));
    rdxz Z = ZTABLE[(**inputs).ptype];
    // todo norm run
    a_dup(rdx, eqs, inputs);
    a_gauge(rdx, sub, rdxgRest(inputs));
    while (rdxsLen(inputs)) {
        $rof(rdx, p, eqs) {
            ok64 o = rdxNext(p);
            if (o == OK) {
            } else if (o == END) {
                rdxSwap(p, rdxsLast(inputs));
                rdxgShed1(inputs);
            } else {
                fail(o);
            }
            if (p < inputs[1]) rdxsDownAtZ(inputs, p - inputs[0], Z);
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
        if (rdxTypePlex(*wins)) {
            test(rdxgRestLen(inputs) >= rdxsLen(wins), RDXNOROOM);
            rdx c = {};
            rdxgShedAll(sub);
            call(rdxInto, &c, into);
            $for(rdx, q, wins) {
                (**sub_idle).type = 0;
                call(rdxInto, *sub_idle, q);
                rdxgFed1(sub);
            }
            call(rdxNorm, sub);
            a_dup(rdx, fc, sub_data);
            call(rdxMerge, &c, sub);
            call(rdxOuto, &c, into);
        }
    }
    done;
}

ok64 rdxStringLength(rdxp str, u32p len) {
    sane(str && str->type == RDX_TYPE_STRING && u8csOK(str->s));
    a_pad(u8, pad, 256);
    a_dup(u8c, from, str->s);
    UTFRecode re = UTABLE[str->cformat][UTF8_DECODER_ALL];
    ok64 o = OK;
    do {
        o = re(pad_idle, from);
        *len += u8csLen(pad_datac);
        Breset(pad);
    } while (o == NOROOM);  // todo ok64Is()
    return o;
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

ok64 rdxHashBlake(rdxp of, blake256* hash);

ok64 rdxHashBlake1(rdxp of, blake256* hash) {
    sane(of && hash);
    blake0 state = {};
    NACLBlakeInit(&state);
    w64 head = {};
    a_rawc(raw, head);
    head._32[0] = of->type;
    a_rawc(rawid, of->id);
    switch (of->type) {
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX: {
            head._32[1] = 32;
            NACLBlakeUpdate(&state, rawid);
            NACLBlakeUpdate(&state, raw);
            rdx c = {.format = of->format};
            call(rdxInto, &c, of);
            blake256 chash;
            a_rawc(craw, chash);
            call(rdxHashBlake, &c, &chash);
            NACLBlakeUpdate(&state, craw);
            break;
        }
        case RDX_TYPE_INT:
        case RDX_TYPE_FLOAT: {
            a_rawc(raw, of->f);
            head._32[1] = 8;
            NACLBlakeUpdate(&state, rawid);
            NACLBlakeUpdate(&state, raw);
            break;
        }
        case RDX_TYPE_REF: {
            a_rawc(raw, of->r);
            head._32[1] = 16;
            NACLBlakeUpdate(&state, rawid);
            NACLBlakeUpdate(&state, raw);
            break;
        }
        case RDX_TYPE_STRING:
        case RDX_TYPE_TERM: {
            head._32[1] = $len(of->s);
            NACLBlakeUpdate(&state, rawid);
            NACLBlakeUpdate(&state, of->s);
            // FIXME this ignores escaping issues
            break;
        }
        default:
            fail(NOTIMPLYET);
    }
    NACLBlakeFinal(&state, hash);
    done;
}

ok64 rdxHashBlake(rdxp of, blake256* hash) {
    sane(of && hash);
    blake256 pad[32] = {};
    scan(rdxNext, of) {
        blake256 one;
        call(rdxHashBlake1, of, hash);
        u8 hi = ctz64(one._64[0] | (1UL << 31));
        blake0 state = {};
        NACLBlakeInit(&state);
        u8cs raw = {(u8*)pad, ((hi + 1) * sizeof(blake256)) + (u8*)pad};
        NACLBlakeUpdate(&state, raw);
        a_rawc(oneraw, one);
        NACLBlakeUpdate(&state, oneraw);
        NACLBlakeFinal(&state, &pad[hi]);
        for (int i = 0; i < hi; ++i) zero(pad[i]);
    }
    blake0 outer;
    NACLBlakeInit(&outer);
    u8cs raw = {(u8*)pad, (32 * sizeof(blake256)) + (u8*)pad};
    NACLBlakeUpdate(&outer, raw);
    NACLBlakeFinal(&outer, hash);
    done;
}
