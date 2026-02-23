#include "RDX.h"

#include "abc/01.h"
#include "abc/BUF.h"
#include "abc/NACL.h"
#include "abc/PRO.h"
#include "abc/SHA.h"

static u8cs EXT_JDR = {(u8cp)".jdr", (u8cp)".jdr" + 4};
static u8cs EXT_TLV = {(u8cp)".tlv", (u8cp)".tlv" + 4};
static u8cs EXT_SKIL = {(u8cp)".skil", (u8cp)".skil" + 5};
static u8cs EXT_SLIK = {(u8cp)".slik", (u8cp)".slik" + 5};
static u8cs EXT_WAL = {(u8cp)".wal", (u8cp)".wal" + 4};
static u8cs EXT_EMPTY = {(u8cp)"", (u8cp)""};

u8 RDXFmtFromExt(u8cs ext) {
    if (u8csHasSuffix(ext, EXT_JDR)) return RDX_FMT_JDR;
    if (u8csHasSuffix(ext, EXT_TLV)) return RDX_FMT_TLV;
    if (u8csHasSuffix(ext, EXT_SKIL)) return RDX_FMT_SKIL;
    if (u8csHasSuffix(ext, EXT_SLIK)) return RDX_FMT_SLIK;
    if (u8csHasSuffix(ext, EXT_WAL)) return RDX_FMT_WAL;
    return RDX_FMT_JDR;  // default
}

u8csp RDXExtFromFmt(u8 fmt) {
    u8 base = fmt & ~RDX_FMT_WRITE;
    switch (base) {
        case RDX_FMT_JDR:
        case RDX_FMT_JDR_PIN:
            return EXT_JDR;
        case RDX_FMT_TLV:
            return EXT_TLV;
        case RDX_FMT_SKIL:
            return EXT_SKIL;
        case RDX_FMT_SLIK:
            return EXT_SLIK;
        case RDX_FMT_WAL:
            return EXT_WAL;
        default:
            return EXT_EMPTY;
    }
}

ok64 rdxParseVal(rdxp x, u8cs val) {
    sane(x != NULL);
    switch (x->type) {
        case RDX_TYPE_FLOAT:
            call(ZINTu8sDrainFloat, &x->f, val);
            break;
        case RDX_TYPE_INT:
            call(ZINTu8sDrainInt, &x->i, val);
            break;
        case RDX_TYPE_REF:
            call(ZINTu8sDrain128, val, &x->r.seq, &x->r.src);
            break;
        case RDX_TYPE_STRING:
            u8csMv(x->s, val);
            break;
        case RDX_TYPE_TERM:
            u8csMv(x->t, val);
            break;
        default:
            break;
    }
    done;
}

ok64 rdxDrainTLV(rdxp x, u8cs stream) {
    sane(x != NULL && $ok(stream));
    u8 lit = 0;
    u8cs id = {}, val = {};
    call(TLVDrainKeyVal, &lit, id, val, stream);
    x->type = RDX_TYPE_LIT_REV[lit];
    if (!$empty(id)) {
        call(ZINTu8sDrain128, id, &x->id.seq, &x->id.src);
    } else {
        zero(x->id);
    }
    if (!rdxTypePlex(x)) {
        call(rdxParseVal, x, val);
    }
    done;
}

ok64 rdxConvert(rdxp into, rdxp from) {
    sane(into && from);
    a_pad(rdx, inputs, 64);
    *rdxbAtP(inputs, 0) = *from;
    rdxbFed1(inputs);
    return rdxMerge(into, rdxbDataIdle(inputs));
}

// Compare tuples by key to determine winner (greater key wins)
b8 rdxTupleKeyWinZ(rdxcp a, rdxcp b) {  // fixme this mess is correct
    if (!a || !b || a->type != RDX_TYPE_TUPLE || b->type != RDX_TYPE_TUPLE)
        return NO;
    rdx ac = {}, bc = {};
    b8 a_empty = NO, b_empty = NO;
    if (rdxInto(&ac, (rdxp)a) != OK) return NO;
    if (rdxNext(&ac) != OK) a_empty = YES;
    if (rdxInto(&bc, (rdxp)b) != OK) return NO;
    if (rdxNext(&bc) != OK) b_empty = YES;
    // Empty tuples: winner by id
    if (a_empty && b_empty) return id128RevZ(&b->id, &a->id);  // greater wins
    if (a_empty) return NO;   // a (empty) loses
    if (b_empty) return YES;  // a wins (b empty)
    if (ac.type != bc.type || ac.type < RDX_TYPE_PLEX_LEN)
        return u8Z(&ac.type, &bc.type);
    // rdx1Z returns YES if ac < bc, so !rdx1Z means ac >= bc (a wins)
    return rdx1Z(&bc, &ac);
}

b8 rdxEulerZ(rdxcp a, rdxcp b) {
    if (!a || !b) return NO;
    b8 result;
    if (a->type == RDX_TYPE_TUPLE) {
        rdx ac = {};
        rdxInto(&ac, (rdxp)a);
        rdxNext(&ac);
        result = rdxEulerZ(&ac, b);
        if (a->format!=RDX_FMT_JDR && a->format!=RDX_FMT_JDR_PIN)
            rdxOuto(&ac, (rdxp)a);  // FIXmE
    } else if (b->type == RDX_TYPE_TUPLE) {
        rdx bc = {};
        rdxInto(&bc, (rdxp)b);
        rdxNext(&bc);
        result = rdxEulerZ(a, &bc);
        if (b->format!=RDX_FMT_JDR && b->format!=RDX_FMT_JDR_PIN)
            rdxOuto(&bc, (rdxp)b); // FIXME
    } else if (a->type != b->type) {
        result = u8Z(&a->type, &b->type);
    } else if (a->type < RDX_TYPE_PLEX_LEN) {
        result = id128RevZ(&a->id, &b->id);
    } else {
        result = rdx1Z(a, b);
    }
    return result;
}

// Check if current element is an empty tuple (iterator level, format-agnostic)
// Must be called when iterator is positioned at the element (after rdxNext)
b8 rdxIsEmptyTuple(rdxcp x) {
    if (x->type != RDX_TYPE_TUPLE) return NO;
    // Check if tuple has no children by entering and checking for END
    rdx child = {};
    rdx parent = *x;  // copy to avoid modifying original
    ok64 into_o = rdxInto(&child, &parent);
    if (into_o != OK) return NO;
    ok64 o = rdxNext(&child);
    return o == END;
}

// Check if remaining elements (after stripping tombstones) are all empty tuples
// Clone iterator and advance through rest. Returns YES if only empty tuples
// remain.
b8 rdxOnlyEmptyTuplesRemain(rdxcp from) {
    rdx clone = *from;
    ok64 o;
    while ((o = rdxNext(&clone)) == OK) {
        if (clone.id.seq & 1) continue;  // skip tombstones
        if (!rdxIsEmptyTuple(&clone)) return NO;
    }
    return YES;
}

ok64 rdxSkipChildren(rdxp from);  // forward declaration

ok64 rdxStrip(rdxp into, rdxp from) {
    sane(into && from && rdxWritable(into) && !rdxWritable(from));
    b8 in_tuple = (from->ptype == RDX_TYPE_TUPLE);
    scan(rdxNext, from) {
        if (from->id.seq & 1) {
            // Tombstone: skip for most containers, but in TUPLE emit ()
            if (in_tuple) {
                // Check if only tombstones/empty tuples remain - if so, omit
                if (rdxOnlyEmptyTuplesRemain(from)) {
                    // Skip this and all remaining elements
                    if (rdxTypePlex(from)) call(rdxSkipChildren, from);
                    break;
                }
                into->type = RDX_TYPE_TUPLE;
                into->flags = 0;
                zero(into->id);
                call(rdxNext, into);
                rdx cinto = {};
                call(rdxInto, &cinto, into);
                call(rdxOuto, &cinto, into);  // empty tuple
            }
            // Skip tombstone's content if it's a container
            if (rdxTypePlex(from)) {
                call(rdxSkipChildren, from);
            }
            continue;
        }
        // Skip empty tuples outside of tuples (they're meaningless)
        if (!in_tuple && rdxIsEmptyTuple(from)) {
            continue;  // skip - empty tuple only valid as null in tuple
        }
        // For TUPLE: check if this is an empty tuple and only empty tuples
        // remain
        if (in_tuple && rdxIsEmptyTuple(from) &&
            rdxOnlyEmptyTuplesRemain(from)) {
            break;  // stop writing, trailing empty tuples omitted
        }
        into->type = from->type;
        into->flags = from->flags;
        // MULTIX children keep src (it's the slot key), zero seq only
        if (from->ptype == RDX_TYPE_MULTIX) {
            into->id.src = from->id.src;
            into->id.seq = 0;
        } else {
            zero(into->id);
        }
        into->r = from->r;
        // Clear ignored bits on REF values (ms 4 bits of src/seq, tombstone
        // bit)
        if (from->type == RDX_TYPE_REF) {
            into->r.src &= RON60_MASK;
            into->r.seq &= RON60_MASK;
        }
        call(rdxNext, into);
        if (rdxTypePlex(from)) {
            rdx cinto = {};
            rdx cfrom = {};
            call(rdxInto, &cinto, into);
            call(rdxInto, &cfrom, from);
            call(rdxStrip, &cinto, &cfrom);
            call(rdxOuto, &cinto, into);
            call(rdxOuto, &cfrom, from);
        }
    }
    seen(END);
    into->type = 0;
    done;
}

// Skip children of a plex element without writing
ok64 rdxSkipChildren(rdxp from) {
    sane(from && rdxTypePlex(from));
    rdx cfrom = {};
    call(rdxInto, &cfrom, from);
    ok64 o;
    scan(rdxNext, &cfrom) {
        if (rdxTypePlex(&cfrom)) {
            call(rdxSkipChildren, &cfrom);
        }
    }
    seen(END);
    call(rdxOuto, &cfrom, from);
    done;
}

// Copy current element (already positioned in from) and its contents
ok64 rdxCopy1(rdxp into, rdxp from) {
    sane(into && from && rdxWritable(into) && !rdxWritable(from) &&
         from->type != 0);  // Must have a current element
    rdxMv(into, from);
    call(rdxNext, into);
    if (rdxTypePlex(from)) {
        // Check if into->type is 0 (element was skipped by writer, e.g., STRIP)
        if (into->type == 0) {
            // Skip children from source without writing
            call(rdxSkipChildren, from);
        } else {
            rdx cinto = {};
            rdx cfrom = {};
            call(rdxInto, &cinto, into);
            call(rdxInto, &cfrom, from);
            call(rdxCopy, &cinto, &cfrom);
            call(rdxOuto, &cinto, into);
            call(rdxOuto, &cfrom, from);
        }
    }
    done;
}

ok64 rdxCopy(rdxp into, rdxp from) {
    sane(into && from && rdxWritable(into) && !rdxWritable(from));
    scan(rdxNext, from) { call(rdxCopy1, into, from); }
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

fun b8 rdxStringZ(rdxcp a, rdxcp b) {
    if (!a || !b) return NO;
    u8 ae = a->flags & RDX_UTF_ENC_BITS;
    u8 be = b->flags & RDX_UTF_ENC_BITS;
    if (ae == RDX_UTF_ENC_UTF8 && be == RDX_UTF_ENC_UTF8)
        return u8csZ(&a->s, &b->s);
    UTFRecode are = UTABLE[ae][UTF8_DECODER_ONE];
    UTFRecode bre = UTABLE[be][UTF8_DECODER_ONE];
    a_pad(u8, autf, 16);
    a_pad(u8, butf, 16);
    a_dup(u8c, as, a->s);
    a_dup(u8c, bs, b->s);
    while (u8csLen(as) && u8csLen(bs)) {
        if (are(autf_idle, as) != OK) return NO;
        if (bre(butf_idle, bs) != OK) return NO;
        int z = $cmp(autf_datac, butf_datac);
        if (z != 0) return z < 0;
        u8bReset(autf);
        u8bReset(butf);
    }
    return u8csLen(as) < u8csLen(bs);
}

b8 rdx1Z(rdxcp a, rdxcp b) {
    assert(a->type == b->type && a->type >= RDX_TYPE_PLEX_LEN);
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

fun b8 rdxpWinZ(rdxpcp a, rdxpcp b) { return rdxWinZ(*a, *b); }

ok64 rdxNorm(rdxg inputs) {
    sane(rdxgOK(inputs));
    $for(rdx, p, rdxgLeft(inputs)) {
        rdxz Z = ZTABLE[p->ptype];
        rdx prev = *p;
        ok64 o = rdxNext(&prev);
        if (OK != o) continue;
        if (rdxTypePlex(&prev)) {
            call(rdxSkipChildren, &prev);
        }
        rdx next = prev;
        b8 split = NO;
        scan(rdxNext, &next) {
            if (!Z(&prev, &next)) {
                p->opt = (u8p)prev.next;
                rdxp pp = 0;
                call(rdxgFedP, inputs, &pp);
                *pp = prev;
                split = YES;
                break;
            }
            prev = next;
            if (rdxTypePlex(&prev)) {
                call(rdxSkipChildren, &prev);
            }
        }
        if (split) continue;
        seen(END);
        test(next.ptype <= RDX_TYPE_TUPLE || next.loc == u32max, RDXBADNEST);
    }
    done;
}

ok64 rdxMerge(rdxp into, rdxg inputs) {
    sane(into && rdxgOK(inputs) && !rdxgEmpty(inputs));
    rdxz Z = ZTABLE[(**inputs).ptype];
    u8 ptype = (**inputs).ptype;
    u32 pos = 0;  // position within container (for TUPLE key handling)
    a_dup(rdx, eqs, inputs);
    while (!rdxgEmpty(inputs)) {
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
            rdxMv(into, *wins);
            // OR tombstone bits only for REF (r.seq overlaps with s[1] for
            // STRING)
            if ((**wins).type == RDX_TYPE_REF) {
                $for(rdx, q, wins) { into->r.seq |= (q->r.seq & 1); }
            }
        } else {
            rdxMv(into, *wins);
        }
        // TUPLE key (position 0) should have zero ID after merge
        if (ptype == RDX_TYPE_TUPLE && pos == 0) {
            into->id.src = 0;
            into->id.seq = 0;
        }
        call(rdxNext, into);
        pos++;
        if (rdxTypePlex(*wins)) {
            if (rdxgRestLen(inputs) < rdxsLen(wins)) {
                printf("oops\n");
            }
            test(rdxgRestLen(inputs) >= rdxsLen(wins), NOROOM);
            rdx c = {};
            rdxg sub;
            rdxsGauge(rdxgRest(inputs), sub);
            $for(rdx, q, wins) {
                rdxp slot = NULL;
                call(rdxgFedP, sub, &slot);
                slot->type = 0;
                call(rdxInto, slot, q);
                if (q->type == RDX_TYPE_REF) {
                    // OR tombstone bits for refs (delete wins)
                    into->r.seq |= (q->r.seq & 1);  // fixme nonsense
                }
            }
            call(rdxInto, &c, into);
            if ((**wins).type != RDX_TYPE_TUPLE &&
                (**wins).type != RDX_TYPE_LINEAR) {
                call(rdxNorm, sub);
            }
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
    UTFRecode re = UTABLE[str->flags & RDX_UTF_ENC_BITS][UTF8_DECODER_ALL];
    ok64 o = OK;
    do {
        o = re(pad_idle, from);
        *len += u8csLen(pad_datac);
        u8bReset(pad);
    } while (o == NOROOM);  // todo ok64Is()
    return o;
}

fun ok64 LenCB(u8cs chunk, voidp ctx) {
    *(u32*)ctx += $len(chunk);
    return OK;
}

fun ok64 SHACB(u8cs chunk, voidp ctx) {
    SHAFeed(ctx, chunk);
    return OK;
}

ok64 rdxHash1(rdxp of, SHAstate* state) {
    sane(of && state);
    w64 head = {};
    a_rawc(raw, head);
    head._32[0] = of->type;
    a_rawc(rawid, of->id);
    switch (of->type) {
        case RDX_TYPE_INT:
        case RDX_TYPE_FLOAT: {
            a_rawc(rval, of->f);
            head._32[1] = 8;
            SHAFeed(state, rawid);
            SHAFeed(state, raw);
            SHAFeed(state, rval);
            break;
        }
        case RDX_TYPE_REF: {
            a_rawc(rval, of->r);
            head._32[1] = 16;
            SHAFeed(state, rawid);
            SHAFeed(state, raw);
            SHAFeed(state, rval);
            break;
        }
        case RDX_TYPE_STRING:
        case RDX_TYPE_TERM: {
            SHAFeed(state, rawid);
            u8 enc = of->flags & RDX_UTF_ENC_BITS;
            if (enc == RDX_UTF_ENC_UTF8) {
                head._32[1] = $len(of->s);
                SHAFeed(state, raw);
                SHAFeed(state, of->s);
            } else {
                u32 len = 0;
                call(UTFRecodeCB, of->s, enc, UTF8_DECODER_ALL, LenCB, &len);
                head._32[1] = len;
                SHAFeed(state, raw);
                call(UTFRecodeCB, of->s, enc, UTF8_DECODER_ALL, SHACB,
                     state);
            }
            break;
        }
        default:
            fail(NOTIMPLYET);
    }
    done;
}

ok64 rdxHash(sha256p hash, rdxp of) {
    sane(hash && of);
    SHAstate state = {};
    SHAOpen(&state);
    scan(rdxNext, of) {
        if (rdxTypePlex(of)) {
            w64 head = {};
            head._32[0] = of->type;
            head._32[1] = 32;
            a_rawc(raw, head);
            a_rawc(rawid, of->id);
            SHAFeed(&state, rawid);
            SHAFeed(&state, raw);
            rdx c = {};
            call(rdxInto, &c, of);
            sha256 chash;
            a_rawc(craw, chash);
            call(rdxHash, &chash, &c);
            call(rdxOuto, &c, of);
            SHAFeed(&state, craw);
        } else {
            call(rdxHash1, of, &state);
        }
    }
    seen(END);
    SHAClose(&state, hash);
    done;
}

fun ok64 BlakeCB(u8cs chunk, voidp ctx) {
    NACLBlakeUpdate(ctx, chunk);
    return OK;
}

ok64 rdxHashBlake1(rdxp of, blake0* state) {
    sane(of && state);
    w64 head = {};
    a_rawc(raw, head);
    head._32[0] = of->type;
    a_rawc(rawid, of->id);
    switch (of->type) {
        case RDX_TYPE_INT:
        case RDX_TYPE_FLOAT: {
            a_rawc(rval, of->f);
            head._32[1] = 8;
            NACLBlakeUpdate(state, rawid);
            NACLBlakeUpdate(state, raw);
            NACLBlakeUpdate(state, rval);
            break;
        }
        case RDX_TYPE_REF: {
            a_rawc(rval, of->r);
            head._32[1] = 16;
            NACLBlakeUpdate(state, rawid);
            NACLBlakeUpdate(state, raw);
            NACLBlakeUpdate(state, rval);
            break;
        }
        case RDX_TYPE_STRING:
        case RDX_TYPE_TERM: {
            NACLBlakeUpdate(state, rawid);
            u8 enc = of->flags & RDX_UTF_ENC_BITS;
            if (enc == RDX_UTF_ENC_UTF8) {
                head._32[1] = $len(of->s);
                NACLBlakeUpdate(state, raw);
                NACLBlakeUpdate(state, of->s);
            } else {
                u32 len = 0;
                call(UTFRecodeCB, of->s, enc, UTF8_DECODER_ALL, LenCB, &len);
                head._32[1] = len;
                NACLBlakeUpdate(state, raw);
                call(UTFRecodeCB, of->s, enc, UTF8_DECODER_ALL, BlakeCB, state);
            }
            break;
        }
        default:
            fail(NOTIMPLYET);
    }
    done;
}

ok64 rdxHashBlake(rdxp of, blake256* hash) {
    sane(of && hash);
    blake0 state = {};
    NACLBlakeInit(&state);
    scan(rdxNext, of) {
        if (rdxTypePlex(of)) {
            w64 head = {};
            head._32[0] = of->type;
            head._32[1] = 32;
            a_rawc(raw, head);
            a_rawc(rawid, of->id);
            NACLBlakeUpdate(&state, rawid);
            NACLBlakeUpdate(&state, raw);
            rdx c = {};
            call(rdxInto, &c, of);
            blake256 chash;
            a_rawc(craw, chash);
            call(rdxHashBlake, &c, &chash);
            call(rdxOuto, &c, of);
            NACLBlakeUpdate(&state, craw);
        } else {
            call(rdxHashBlake1, of, &state);
        }
    }
    seen(END);
    NACLBlakeFinal(&state, hash);
    done;
}
