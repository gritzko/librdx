//
// Created by gritzko on 5/10/24.
//

#ifndef LIBRDX_U_H
#define LIBRDX_U_H

#include <stdint.h>

#include "B.h"

fun int u8cmp(const u8 *a, const u8 *b) { return (int)*a - (int)*b; }
fun int u16cmp(const u16 *a, const u16 *b) { return (int)*a - (int)*b; }
fun int u32cmp(const u32 *a, const u32 *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}
fun int u64cmp(const u64 *a, const u64 *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

fun int i8cmp(const i8 *a, const i8 *b) { return (int)*a - (int)*b; }
fun int i16cmp(const i16 *a, const i16 *b) { return (int)*a - (int)*b; }
fun int i32cmp(const i32 *a, const i32 *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}
fun int i64cmp(const i16 *a, const i16 *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

fun int u128cmp(u128 const *a, u128 const *b) {
    int ret = u64cmp(&a->_64[1], &b->_64[1]);
    if (ret == 0) {
        ret = u64cmp(&a->_64[0], &b->_64[0]);
    }
    return ret;
}

fun int u8cpcmp(u8 const *const *a, u8 const *const *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

fun int $u8cmp(u8 const *const *a, u8 const *const *b) { return $cmp(a, b); }

#define X(M, name) M##u8##name
#include "Bx.h"
#undef X

#define X(M, name) M##u8cp##name
#include "Bx.h"
#undef X

#define X(M, name) M##u16##name
#include "Bx.h"
#undef X

#define X(M, name) M##u32##name
#include "Bx.h"
#undef X

#define X(M, name) M##u64##name
#include "Bx.h"
#undef X

#define X(M, name) M##i8##name
#include "Bx.h"
#undef X

#define X(M, name) M##i16##name
#include "Bx.h"
#undef X

#define X(M, name) M##i32##name
#include "Bx.h"
#undef X

#define X(M, name) M##i64##name
#include "Bx.h"
#undef X

fun int u8c$cmp(u8$ const *a, u8$ const *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

#define X(M, name) M##u8c$##name
#include "Bx.h"
#undef X

#define $u8raw(v) \
    { (u8 *)&(v), (u8 *)(&v) + sizeof(v) }

#define $u8str(c) \
    { (u8 *)c, (u8 *)c + strlen(c) }

#define Bprintf(buf, fmt, ...)           \
    {                                    \
        u8 **into = Bidle(buf);          \
        $printf(into, fmt, __VA_ARGS__); \
    }

fun ok64 put32($u8 into, u32 x) {
    if ($len(into) < 4) return Bnoroom;
    **into = x;
    ++*into;
    **into = x >> 8;
    ++*into;
    **into = x >> 16;
    ++*into;
    **into = x >> 24;
    ++*into;
    return OK;
}

fun u128 u128xor(u128 a, u128 b) {
    u128 x = {._64 = {a._64[0] ^ b._64[0], a._64[1] ^ b._64[1]}};
    return x;
}

#endif  // LIBRDX_U_H
