#ifndef ABC_BUF_H
#define ABC_BUF_H

#include "B.h"

fun int u8cmp(const u8 *a, const u8 *b) { return (int)*a - (int)*b; }

fun int u8cpcmp(u8 const *const *a, u8 const *const *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

fun int $u8cmp(u8 const *const *a, u8 const *const *b) { return $cmp(a, b); }

fun int $u8cz($cu8c a, $cu8c b) { return $cmp(a, b); }

#define X(M, name) M##u8##name
#include "Bx.h"
#undef X

#define X(M, name) M##u8cp##name
#include "Bx.h"
#undef X

fun int $u8ccmp($u8c const *a, $u8c const *b) { return $cmp(*a, *b); }

#define X(M, name) M##$u8c##name
#define ABC_X_$
#include "Bx.h"
#undef ABC_X_$
#undef X

fun int Bu8cmp(Bu8 const *a, Bu8 const *b) {
    return u8cpcmp((u8cp *)((*a)[0]), (u8cp *)((*b)[0]));
}

#define X(M, name) M##Bu8##name
#define ABC_X_$
#include "Bx.h"
#undef ABC_X_$
#undef X

#define $u8raw(v) \
    { (u8 *)&(v), (u8 *)(&v) + sizeof(v) }

#define a$raw(n, v) $u8 n = {(u8 *)&(v), (u8 *)(&v) + sizeof(v)}
#define a$rawc(n, v) $u8c n = {(u8 *)&(v), (u8 *)(&v) + sizeof(v)}

fun b8 Bitat(Bu8 buf, size_t ndx) {
    // FIXME bounds
    return ((buf[0][ndx >> 3]) >> (ndx & 7)) & 1;
}

fun b8 Bitset(Bu8 buf, size_t ndx) {
    return (buf[0][ndx >> 3]) |= 1 << (ndx & 7);
}

fun b8 Bitunset(Bu8 buf, size_t ndx) {
    return (buf[0][ndx >> 3]) &= ~(1 << (ndx & 7));
}

#endif
