//
// STRIP.c - Iterator that strips tombstones and IDs
//
// Wraps an underlying iterator of any format, skipping deleted elements
// (tombstone bit set) and zeroing out IDs.
//
// Also strips trailing empty tuples from TUPLE containers.
//
// Uses rdxg (via x->ins) to manage the underlying iterator stack,
// similar to Y.c pattern. The first element in the stack is the source.
//

#include "RDX.h"
#include "abc/PRO.h"

// Forward declarations (defined in RDX.c)
b8 rdxIsEmptyTuple(rdxcp x);
b8 rdxOnlyEmptyTuplesRemain(rdxcp from);

ok64 rdxNextSTRIP(rdxp x) {
    sane(x && rdxgLeftLen(x->ins) > 0);
    rdxsp ins = rdxgLeft(x->ins);
    rdxp src = *ins;
    b8 in_tuple = (x->ptype == RDX_TYPE_TUPLE);
    ok64 o;
    scan(rdxNext, src) {
        // Skip tombstones (bit 0 of seq is the tombstone flag)
        if (src->id.seq & 1) continue;
        // In TUPLE: skip trailing empty tuples
        if (in_tuple && rdxIsEmptyTuple(src) && rdxOnlyEmptyTuplesRemain(src)) {
            return END;  // stop, trailing empties omitted
        }
        // Copy data but zero out the ID
        rdxMv(x, src);
        x->id.src = 0;
        x->id.seq = 0;
        done;
    }
    return __;
}

ok64 rdxIntoSTRIP(rdxp c, rdxp p) {
    sane(c && p && rdxTypePlex(p) && rdxgLeftLen(p->ins) > 0);
    rdxsp pins = rdxgLeft(p->ins);
    rdxp psrc = *pins;

    c->format = RDX_FMT_STRIP;
    c->ptype = p->type;

    // Allocate child from parent's rdxg pool
    rdxsGauge(rdxgRest(p->ins), c->ins);
    rdxp csrc = 0;
    call(rdxgFedP, c->ins, &csrc);
    call(rdxInto, csrc, psrc);
    done;
}

ok64 rdxOutoSTRIP(rdxp c, rdxp p) {
    sane(c && p);
    // Outo is optional for reads
    done;
}

// Write path: strip IDs and skip tombstones
// For writer, STRIP uses rdxg (via ins) for underlying writer pool
// Note: Trailing empty tuple stripping is handled by STRIP reader, not writer
ok64 rdxWriteNextSTRIP(rdxp x) {
    sane(x && rdxgLeftLen(x->ins) > 0);
    // Skip tombstones (don't write them to underlying)
    // Clear type to signal skip to caller (e.g., rdxCopy1)
    if (x->id.seq & 1) {
        x->type = 0;
        done;
    }

    rdxsp ins = rdxgLeft(x->ins);
    rdxp dst = *ins;
    // Copy data but zero out the ID in the underlying
    rdxMv(dst, x);
    dst->id.src = 0;
    dst->id.seq = 0;
    call(rdxNext, dst);  // rdxNext for write is rdxWriteNext via vtable
    done;
}

ok64 rdxWriteIntoSTRIP(rdxp c, rdxp p) {
    sane(c && p && rdxTypePlex(p) && rdxgLeftLen(p->ins) > 0);
    rdxsp pins = rdxgLeft(p->ins);
    rdxp pdst = *pins;

    c->format = RDX_FMT_STRIP | RDX_FMT_WRITE;
    c->ptype = p->type;

    // Allocate child from parent's rdxg pool
    rdxsGauge(rdxgRest(p->ins), c->ins);
    rdxp cdst = 0;
    call(rdxgFedP, c->ins, &cdst);
    // Zero the child struct before Into
    zero(*cdst);
    call(rdxInto, cdst, pdst);
#ifdef ABC_TRACE
    fprintf(stderr, "WriteIntoSTRIP: pdst->bulk=%p opt=%p cdst->bulk=%p opt=%p\n",
            (void*)pdst->bulk, (void*)pdst->opt, (void*)cdst->bulk, (void*)cdst->opt);
#endif
    done;
}

ok64 rdxWriteOutoSTRIP(rdxp c, rdxp p) {
#ifdef ABC_TRACE
    fprintf(stderr, "WriteOutoSTRIP entered: c->format=%d p->format=%d\n", c->format, p->format);
#endif
    sane(c && p && rdxgLeftLen(c->ins) > 0 && rdxgLeftLen(p->ins) > 0);
    rdxsp cins = rdxgLeft(c->ins);
    rdxsp pins = rdxgLeft(p->ins);
    rdxp cdst = *cins;
    rdxp pdst = *pins;
#ifdef ABC_TRACE
    fprintf(stderr, "WriteOutoSTRIP: pdst->bulk=%p opt=%p cdst->bulk=%p opt=%p\n",
            (void*)pdst->bulk, (void*)pdst->opt, (void*)cdst->bulk, (void*)cdst->opt);
#endif
    call(rdxOuto, cdst, pdst);
    done;
}
