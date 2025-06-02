#ifndef RDX_Z_H
#define RDX_Z_H
#include "RDX.h"

typedef z32 (*RDXZfn)(const RDX* a, const RDX* b);

fun z32 Zlit(const RDX* a, const RDX* b) {
    if (a->lit == b->lit) return z32eq;
    b8 ap = RDXisPLEX(a->lit);
    b8 bp = RDXisPLEX(b->lit);
    if (ap == bp) return u8z(&a->lit, &b->lit);
    return bp ? z32lt : z32gt;
}

fun z32 Zauthor(const RDX* a, const RDX* b) {
    return u64z(&id128src(a->id), &id128src(b->id));
}

fun z32 Zrevision(const RDX* a, const RDX* b) {
    return u64z(&id128seq(a->id), &id128seq(b->id));
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
fun z32 Zvalue(const RDX* a, const RDX* b) {
    // FIXME empty
    if (a->lit == RDX_TUPLE) {
        RDX aa = {};
        a$dup(u8c, dup, a->val);
        RDXparse(&aa, dup);
        return Zvalue(&aa, b);
    }
    if (b->lit == RDX_TUPLE) {
        RDX bb = {};
        a$dup(u8c, dup, b->val);
        RDXparse(&bb, dup);
        return Zvalue(a, &bb);
    }
    if (a->lit != b->lit) return Zlit(a, b);
    switch (a->lit) {
        case RDX_FLOAT:
            return ZINTf64z(a->val, b->val);
        case RDX_INT:
            return ZINTi64z(a->val, b->val);
        case RDX_REF:
            return ZINTu128z(a->val, b->val);
        case RDX_STRING:
            return $u8cz(&a->val, &b->val);
        case RDX_TERM:
            return $u8cz(&a->val, &b->val);
        case RDX_TUPLE: {
            RDX aa = {};
            RDX bb = {};
            RDXparse$(&aa, a->val);
            RDXparse$(&bb, b->val);
            return Zvalue(&aa, &bb);
        }
        case RDX_LINEAR:
        case RDX_EULER:
        case RDX_MULTIX:
            return id128z(&a->id, &b->id);
        default:
            return id128z(&a->id, &b->id);
    }
}

fun z32 Zlww(const RDX* a, const RDX* b) {
    z32 ret = Zrevision(a, b);
    if (ret == 0) ret = Zauthor(a, b);
    if (ret == 0) ret = Zlit(a, b);
    if (ret == 0) ret = Zvalue(a, b);
    return ret;
}

fun z32 Zlinear(const RDX* a, const RDX* b) {
    u64 al = id128seq(a->id) >> 6;
    u64 bl = id128seq(b->id) >> 6;
    z32 ret = u64z(&al, &bl);
    if (ret == z32eq) ret = Zlww(a, b) / 2;
    return ret;
}

fun z32 Zlinear$($cu8c* a, $cu8c* b) {
    a$dupc(u8c, aa, a);
    a$dupc(u8c, bb, b);
    RDX ra = {};
    RDX rb = {};
    RDXparse(&ra, aa);
    RDXparse(&rb, bb);
    return Zlinear(&ra, &rb);
}

fun z32 Zeuler(const RDX* a, const RDX* b) {
    z32 ret = Zvalue(a, b);
    if (ret == z32eq) ret = Zlww(a, b) / 2;
    return ret;
}

fun z32 Zeuler$($cu8c* a, $cu8c* b) {
    RDX ra = {};
    RDX rb = {};
    RDXparse$(&ra, *a);
    RDXparse$(&rb, *b);
    return Zeuler(&ra, &rb);
}

fun z32 Zmultix(const RDX* a, const RDX* b) {
    z32 ret = Zauthor(a, b);
    if (ret == z32eq) ret = Zlww(a, b) / 2;
    fprintf(stderr, "Zmultix %i (%lu-%lu z %lu-%lu)\n", ret, id128src(a->id),
            id128seq(a->id), id128src(b->id), id128seq(b->id));
    return ret;
}

fun z32 Zmultix$($cu8c* a, $cu8c* b) {
    RDX ra = {};
    RDX rb = {};
    RDXparse$(&ra, *a);
    RDXparse$(&rb, *b);
    return Zmultix(&ra, &rb);
}

fun z32 Zsst($cu8c* a, $cu8c* b) {
    RDX ra = {};
    RDX rb = {};
    RDXparse$(&ra, *a);
    RDXparse$(&rb, *b);
    z32 ret = Zlww(&ra, &rb);
    if (ret == z32eq) ret = Zlit(&ra, &rb);
    return ret;
}

#endif
