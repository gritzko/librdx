#ifndef ABC_RDX_Z
#define ABC_RDX_Z
#include "RDX.h"
#include "abc/S.h"

fun z32 _RDXZlit(u8 const* a, u8 const* b) {
    b8 ap = RDXisPLEX(*a);
    b8 bp = RDXisPLEX(*b);
    if (ap == bp) return u8z(a, b);
    return bp ? -1 : 1;
}

fun z32 RDXZlit(u8 const* a, u8 const* b) {
    if (*a == *b) return 0;
    return _RDXZlit(a, b);
}

fun z32 RDXZauthor($u8c const* a, $u8c const* b) {
    u8 ta, tb;
    $u8c keya = {}, keyb = {};
    $u8c vala = {}, valb = {};
    $u8c aa = $dup(*a);
    $u8c bb = $dup(*b);
    ok64 oa = TLVdrainkv(&ta, keya, vala, aa);
    ok64 ob = TLVdrainkv(&tb, keyb, valb, bb);
    u128 reva = {}, revb = {};
    ZINTu128drain(&reva, keya);
    ZINTu128drain(&revb, keyb);
    int z = u64z(&reva._64[0], &revb._64[0]);
    return z;
}

fun z32 RDXZrevision($u8c const* a, $u8c const* b) {
    u8 ta, tb;
    $u8c keya = {}, keyb = {};
    $u8c vala = {}, valb = {};
    $u8c aa = $dup(*a);
    $u8c bb = $dup(*b);
    ok64 oa = TLVdrainkv(&ta, keya, vala, aa);
    ok64 ob = TLVdrainkv(&tb, keyb, valb, bb);
    u128 reva = {}, revb = {};
    ZINTu128drain(&reva, keya);
    ZINTu128drain(&revb, keyb);
    int z = u64z(&reva._64[1], &revb._64[1]);
    if (z == 0) z = u64z(&reva._64[0], &revb._64[0]);
    return z;
}

/**
 1. for values of differing types, use the type-alphabetical order
    (`F`, `I`, `R`, `S`, `T`),
 2. for values of the same type:
     1. `F` compare values numerically,
     2. `I` also numerically,
     3. `R` order by the value, then by the author,
     4. `S` alphanumeric, as in `strcmp(3)`,
     5. `T` alphanumeric.
 **/
fun z32 RDXZvalue($u8c const* a, $u8c const* b) {
    u8 ta, tb;
    $u8c keya = {}, keyb = {};
    $u8c vala = {}, valb = {};
    $u8c aa = $dup(*a);
    $u8c bb = $dup(*b);
    if ($empty(aa)) {
        a$u8c(RDX_EMPTY_TUPLE, RDX_TUPLE | TLVaA, 1, 0);
        if ($empty(bb) || $eq(bb, RDX_EMPTY_TUPLE)) return 0;
        return -1;
    }
    if ($empty(bb)) {
        return 1;
    }
    ok64 oa = TLVdrainkv(&ta, keya, vala, aa);
    if (ta == RDX_TUPLE) {
        return RDXZvalue(&vala, b);
    }
    ok64 ob = TLVdrainkv(&tb, keyb, valb, bb);
    while (tb == RDX_TUPLE) {
        return RDXZvalue(a, &valb);
    }
    if (ta != tb) {
        b8 fa = RDXisFIRST(ta);
        b8 fb = RDXisFIRST(tb);
        if (fa != fb) return fa ? -1 : 1;
        return u8z(&ta, &tb);
    }
    switch (ta) {
        case 'F':
            return ZINTf64z(vala, valb);
        case 'I':
            return ZINTi64z(vala, valb);
        case 'R':
            return ZINTu128z(vala, valb);
        case 'S':
            return $u8cz(&vala, &valb);
        case 'T':
            return $u8cz(&vala, &valb);
        case 'P':
            return 0;  // FIXME 1st
        case 'L':
        case 'E':
        case 'X':
            return ZINTu128z(keya, keyb);
        default:
            return ZINTu128z(keya, keyb);
    }
    return 0;
}

/**
 1. higher revision wins (*revision-order*),
 2. in case of a tie, use the *value-order* (higher wins),
 3. in case of a tie, use the *author-order* (higher replica id wins),
 4. in case of a tie, we look at the same value on both sides.
 **/
fun int RDXZlww($u8c const* a, $u8c const* b) {
    //
    u8 ta, tb;
    $u8c keya = {}, keyb = {};
    $u8c vala = {}, valb = {};
    $u8c aa = $dup(*a);
    $u8c bb = $dup(*b);
    ok64 oa = TLVdrainkv(&ta, keya, vala, aa);
    ok64 ob = TLVdrainkv(&tb, keyb, valb, bb);
    u128 reva = {}, revb = {};
    ZINTu128drain(&reva, keya);
    ZINTu128drain(&revb, keyb);
    int z = u64z(&reva._64[1], &revb._64[1]);  // FIXME tuples
    if (z == z32eq) {
        z = RDXZvalue(a, b);
        if (z == 0) {
            z = u64z(&reva._64[0], &revb._64[0]);
        }
    }

    return z;
}

#endif
