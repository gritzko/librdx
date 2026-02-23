#ifndef LIBRDX_OK_H
#define LIBRDX_OK_H
#include <errno.h>
#include <stdint.h>

#include "01.h"
#include "S.h"
#include "RON.h"

typedef uint64_t ok64;

#define OK 0
#define FAIL 0xffffffffffffffffUL

con ok64 FAILsanity = 0x3ca495de5cade3d;
con ok64 waitnotyet = 0xee5b78cb3e3da78;
con ok64 OKnoroom = 0x614cb3db3cf1;
con ok64 OKbadtext = 0x18526968e29f38;
con ok64 faileq = 0xaa5b70a75;
con ok64 FAILeq = 0x3ca495a75;
con ok64 RONbad	= 0x6d85e6968;
con ok64 NEXT	= 0x5ce85d;

// Rotating buffers for ok64str to allow multiple calls in one expression
static char _ok64_tmp[4][16];
static int _ok64_idx = 0;

fun ok64 OKprint(ok64 o, uint8_t **into) {
    if (o == 0) {
        if (into[1] < into[0] + 2) return OKnoroom;
        **into = 'O';
        ++*into;
        **into = 'K';
        ++*into;
        return OK;
    } else {
        return RONutf8sFeed(into, o);
    }
}

fun ok64 RONValid(u8c **data) {
    return OK;  // todo
}

fun ok64 OKscan(ok64 *o, uint8_t const **from) {
    ok64 oo = RONutf8sDrain(o, from);
    if (oo == OK && *o == 0x518) *o = 0;
    return oo;
}

fun const char *ok64str(ok64 o) {
    int idx = _ok64_idx;
    _ok64_idx = (_ok64_idx + 1) & 3;
    char *tmp[2] = {_ok64_tmp[idx], _ok64_tmp[idx] + 16};
    OKprint(o, (uint8_t **)tmp);
    **tmp = 0;
    return _ok64_tmp[idx];
}

fun const char *okstr(ok64 o) { return ok64str(o); }

fun int ok64is(ok64 val, ok64 root) {
    if (root == 0) return val == 0;
    u8 bits = (64 - clz64(root) + 5) / 6 * 6;
    if (bits >= 64) return val == root;
    return (val & ((1UL << bits) - 1)) == root;
}

fun ok64 errnok() {
    ok64 e = errno;
    e &= 63;
    e <<= 54;
    return e;
}

#endif
