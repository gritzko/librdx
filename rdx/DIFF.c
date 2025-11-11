//
// Created by gritzko on 11/5/25.
//
#include "RDX2.h"
#include "abc/KV.h"
#include "abc/PRO.h"

typedef enum {
    DIFF_ACTION_EQ = 0,
    DIFF_ACTION_RM = 1,
    DIFF_ACTION_IN = 2,
    DIFF_ACTION_UP = 3,
} DIFF_ACTION;

typedef enum {
    REL2_EQUAL = 0,
    REL2_LESSER = 1,
    REL2_GREATER = 2,
    REL2_INCOMP = 3,
} REL2;

typedef struct {
    u32 adv, rev;
    u32 cost, rcost;
} diff128;

fun int diff128cmp(diff128 const* a, diff128 const* b) { return 0; }

#define X(M, name) M##diff128##name
#include "abc/Bx.h"
#include "abc/HEAPx.h"
#undef X

fun int diff128pcmp(diff128p const* a, diff128p const* b) { return 0; }

#define X(M, name) M##diff128p##name
#include "abc/Bx.h"
#include "abc/HEAPx.h"
#undef X

typedef struct {
    u8cs a, b;        // raw
    u32b atoc, btoc;  // off+2
    diff128b adv;
    diff128pb heap;
} differ;

u8 DIFFz(u32 andx, u32 bndx) {}  // enc shit V
u8 DIFFType(u32 ndx) {}

ok64 DIFFMakeToC(u8cs x, kv32bp toc) { return notimplyet; }

rdxZ RDXTypeZ(u8 type) {
    switch (type) {
        case RDX_TUPLE:
            return rdxTupleZ;
        case RDX_LINEAR:
            return rdxLinearZ;
        case RDX_EULER:
            return rdxEulerZ;
        case RDX_MULTIX:
            return rdxMultixZ;
        default:
            return NULL;
    }
}

ok64 TLVkv32Peek(kv32p range, u8cs tlv) {}
ok64 TLVkv32Set(kv32 range, u8csp set, u8csc tlv) {}
ok64 TLVkv32Get(kv32p range, u8csc get, u8csc tlv) {}

ok64 DIFFTerm(u8cs tlv, u32p term, u32 off) {  // todo ranges
    sane(tlv != NULL && $len(tlv) >= off && term != NULL);
    u8cs t2 = {tlv[0] + off, tlv[1]};
    u8cs rec = {};
    call(TLVDrain$, rec, t2);
    *term = rec[1] - tlv[0];
    done;
}

ok64 DIFFStep(rdxp r, u32 pos) {}

ok64 DIFFNext(differ* d) {
    sane(d != NULL);
    diff128p top = **d->heap;
    u32 terma, termb;
    u32 posa, posb;
    call(DIFFTerm, d->a, &terma, posa);
    call(DIFFTerm, d->b, &termb, posb);
    a_pad(diff128, next, 4);
    u32 anext, bnext;
    rdx a, b;
    rdx pa, pb;
    u8cs aval, bval;
    u8 at, bt;
    u8 ptype;
    // noa, nob => outo
    rdxZ z = RDXTypeZ(ptype);
    ok64 zab = !bnext || z(&a, &b);
    ok64 zba = !anext || z(&b, &a);
    // z?
    // neq? rdxEmpty(a b)?
    if (zab) {  // rm
    }
    if (zba) {  // in
    }
    if (!zab && !zba) {                                        // eq? up?
        if (at == bt && RDXisFIRST(at) && $cmp(aval, bval)) {  //      eq

        } else {  //      into
        }
    }
    // over? break
    done;
}

ok64 DIFFRun(differ* d) {
    sane(d != NULL);
    // step 0
    scan(DIFFNext, d);
    seen(nodata);
    return notimplyet;
}

ok64 DIFFDerive(differ* d, u8bp diff) { return notimplyet; }

ok64 RDXu8bDiff(u8b diff, u8cs a, u8cs b) {
    differ d = {};
    // set up  todo alloc/mmap
    u8csDup(d.a, a);
    u8csDup(d.b, b);

    diff128bAllocate(d.adv, ($len(a) + $len(b)) * 2);  // todo
    u32bAllocate(d.atoc, $len(a));
    u32bAllocate(d.btoc, $len(b));
    diff128pbAllocate(d.heap, DIFF_MAX_STEPS);

    // run
    ok64 o = DIFFRun(&d);
    // derive?
    if (o == OK) o = DIFFDerive(&d, diff);

    diff128pbFree(d.heap);
    u32bFree(d.btoc);
    u32bFree(d.atoc);
    diff128bFree(d.adv);

    return notimplyet;
}
