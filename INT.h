//
// Created by gritzko on 5/10/24.
//

#ifndef ABC_INT_H
#define ABC_INT_H

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
fun int i64cmp(const i64 *a, const i64 *b) {
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

fun int $u8ccmp($u8c const *a, $u8c const *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

#define X(M, name) M##$u8c##name
#include "Bx.h"
#undef X

#define $u8raw(v) \
    { (u8 *)&(v), (u8 *)(&v) + sizeof(v) }

#define a$raw(n, v) $u8 n = {(u8 *)&(v), (u8 *)(&v) + sizeof(v)}
#define a$rawc(n, v) $u8c n = {(u8 *)&(v), (u8 *)(&v) + sizeof(v)}

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

fun void $u8drain8(u8 *into, $u8c from) {
    *into = **from;
    *from += sizeof(u8);
}
fun void $u8feed8($u8 into, u8 const *what) {
    **into = *what;
    *into += sizeof(u8);
}

#ifdef ABC_ALIGN
fun void $u8drain16(u16 *into, $u8 from) {
    *into = **from;
    ++*from;
    *into |= u16(**from) << 8;
    ++*from;
}
fun void $u8drain32(u32 *into, $u8 from) {
    u16 lo = 0, hi = 0;
    $u8drain16(&lo, from);
    $u8drain16(&hi, from);
    *into = lo;
    *into |= u32(hi) << 16;
}
fun void $u8drain64(u64 *into, $u8 from) {
    u32 lo = 0, hi = 0;
    $u8drain32(&lo, from);
    $u8drain32(&hi, from);
    *into = lo;
    *into |= u64(hi) << 32;
}
#else
fun void $u8drain16(u16 *into, $u8c from) {
    memcpy(into, *from, 2);
    *from += sizeof(u16);
}
fun void $u8feed16($u8 into, u16 const *what) {
    memcpy(*into, what, 2);
    *into += sizeof(u16);
}
fun void $u8drain32(u32 *into, $u8c from) {
    memcpy(into, *from, 4);
    *from += sizeof(u32);
}
fun void $u8feed32($u8 into, u32 const *what) {
    memcpy(*into, what, 4);
    *into += sizeof(u32);
}
fun void $u8drain64(u64 *into, $u8c from) {
    memcpy(into, *from, 8);
    *from += sizeof(u64);
}
fun void $u8feed64($u8 into, u64 const *what) {
    memcpy(*into, what, 8);
    *into += sizeof(u64);
}
#endif

fun b8 Bitat(Bu8 buf, size_t ndx) {
    return ((buf[0][ndx >> 3]) >> (ndx & 7)) & 1;
}

fun b8 Bitset(Bu8 buf, size_t ndx) {
    return (buf[0][ndx >> 3]) |= 1 << (ndx & 7);
}

fun b8 Bitunset(Bu8 buf, size_t ndx) {
    return (buf[0][ndx >> 3]) &= ~(1 << (ndx & 7));
}

#endif  // ABC_INT_H
