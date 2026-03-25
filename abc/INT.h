//
// Created by gritzko on 5/10/24.
//

#ifndef ABC_INT_H
#define ABC_INT_H

#include <stdint.h>

#include "01.h"
#include "B.h"
#include "BUF.h"

// Slice from static array: a_u64cs(nums, MY_ARRAY)
#define a_u64cs(n, arr) u64c *n[2] = {(arr), (arr) + sizeof(arr)/sizeof(arr[0])}
#define a_u32cs(n, arr) u32c *n[2] = {(arr), (arr) + sizeof(arr)/sizeof(arr[0])}
#define a_i64cs(n, arr) i64c *n[2] = {(arr), (arr) + sizeof(arr)/sizeof(arr[0])}
#define a_i32cs(n, arr) i32c *n[2] = {(arr), (arr) + sizeof(arr)/sizeof(arr[0])}

con ok64 INTBAD = 0x49774b28d;

#define I64_MAX INT64_MAX
#define I64_MIN INT64_MIN
#define I64_MIN_ABS 0x8000000000000000

fun int u16cmp(const u16 *a, const u16 *b) { return (int)*a - (int)*b; }
fun int u32cmp(const u32 *a, const u32 *b) {
    // return (i64)*a - (i64)*b;
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

fun int u256cmp(u256 const *a, u256 const *b) {
    int ret = u64cmp(&a->_64[3], &b->_64[3]);
    if (ret == 0) {
        ret = u64cmp(&a->_64[2], &b->_64[2]);
        if (ret == 0) {
            ret = u64cmp(&a->_64[1], &b->_64[1]);
            if (ret == 0) {
                ret = u64cmp(&a->_64[0], &b->_64[0]);
            }
        }
    }
    return ret;
}

fun int u512cmp(u512 const *a, u512 const *b) {
    for (int i = 7; i >= 0; --i) {
        if (a->_64[i] == b->_64[i]) continue;
        return a->_64[i] < b->_64[i] ? -1 : 1;
    }
    return 0;
}

#define X(M, name) M##u16##name
#include "Bx.h"
#undef X

#define X(M, name) M##u32##name
#include "Bx.h"
#undef X

#define X(M, name) M##u64##name
#include "Bx.h"
#undef X

fun int u64bcmp(u64b const *a, u64b const *b) {
    if ((*a)[0] == (*b)[0]) return 0;
    return (*a)[0] < (*b)[0] ? -1 : 1;
}

// u64bcp needed by Bx-generated u64bbIdx; ABC_X_$ skips it in Sx.h
typedef u64b const *u64bcp;

#define X(M, name) M##u64b##name
#define ABC_X_$
#include "Bx.h"
#undef ABC_X_$
#undef X

// Bury: save data to past, push data length as sentinel.
// After bury, data region is empty, idle is available.
fun ok64 u64bBury(u64bp buf) {
    if (u64bIdleLen(buf) < 1) return BNOROOM;
    u64 dl = (u64)u64bDataLen(buf);
    u64bFeed1(buf, dl);
    ((u64 **)buf)[1] = buf[2];
    return OK;
}

// Digup: discard current data, restore buried data from past.
fun ok64 u64bDigup(u64bp buf) {
    if (u64bPastLen(buf) < 1) return BNODATA;
    u64 dl = *(buf[1] - 1);
    if (u64bPastLen(buf) < dl + 1) return BNODATA;
    ((u64 **)buf)[1] -= dl + 1;
    ((u64 **)buf)[2] = buf[1] + dl;
    return OK;
}

#define X(M, name) M##u128##name
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

#define X(M, name) M##u256##name
#include "Bx.h"
#undef X

#define Bprintf(buf, fmt, ...)           \
    {                                    \
        u8 **into = Bidle(buf);          \
        $printf(into, fmt, __VA_ARGS__); \
    }

fun ok64 put32(u8s into, u32 x) {
    if ($len(into) < 4) return BNOROOM;
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

fun ok64 u8sDrain8(u8cs from, u8 *into) {
    if ($empty(from)) return NODATA;
    *into = **from;
    *from += sizeof(u8);
    return OK;
}

fun ok64 u8sFeed8(u8s into, u8 const *what) {
    if ($empty(into)) return NOROOM;
    **into = *what;
    *into += sizeof(u8);
    return OK;
}

#ifdef ABC_ALIGN
fun ok64 u8sDrain16(u16 *into, u8sfrom) {
    if ($size(from) < sizeof(u16)) return NODATA;
    *into = **from;
    ++*from;
    *into |= u16(**from) << 8;
    ++*from;
    return OK;
}
fun ok64 u8sDrain32(u32 *into, u8sfrom) {
    if ($size(from) < sizeof(u32)) return NODATA;
    u16 lo = 0, hi = 0;
    u8sDrain16(&lo, from);
    u8sDrain16(&hi, from);
    *into = lo;
    *into |= u32(hi) << 16;
    return OK;
}
fun ok64 u8sDrain64(u64 *into, u8sfrom) {
    if ($size(from) < sizeof(u64)) return NODATA;
    u32 lo = 0, hi = 0;
    u8sDrain32(&lo, from);
    u8sDrain32(&hi, from);
    *into = lo;
    *into |= u64(hi) << 32;
    return OK;
}
#else
fun ok64 u8sDrain16(u8cs from, u16 *into) {
    if ($len(from) < sizeof(u16)) return NODATA;
    memcpy(into, *from, sizeof(u16));
    *from += sizeof(u16);
    return OK;
}
fun ok64 u8sFeed16(u8s into, u16 const *what) {
    if ($len(into) < sizeof(u16)) return NOROOM;
    memcpy(*into, what, 2);
    *into += sizeof(u16);
    return OK;
}
fun ok64 u8sDrain32(u8cs from, u32p into) {
    if ($len(from) < sizeof(u32)) return NODATA;
    memcpy(into, *from, sizeof(u32));
    *from += sizeof(u32);
    return OK;
}
fun ok64 u8sFeed32(u8s into, u32 const *what) {
    if ($len(into) < sizeof(u32)) return NOROOM;
    memcpy(*into, what, 4);
    *into += sizeof(u32);
    return OK;
}
fun ok64 u8sDrain64(u8cs from, u64p into) {
    if ($len(from) < sizeof(u64)) return NODATA;
    memcpy(into, *from, sizeof(u64));
    *from += sizeof(u64);
    return OK;
}
fun ok64 u8sFeed64(u8s into, u64 const *what) {
    if ($len(into) < sizeof(u64)) return NOROOM;
    memcpy(*into, what, 8);
    *into += sizeof(u64);
    return OK;
}
#endif

fun u8 u64bit(u64 u, u32 ndx) { return 1 & (u >> ndx); }
fun u8 u128bit(u128 u, u32 ndx) { return 1 & (u._64[ndx >> 6] >> (ndx & 63)); }

ok64 i64decdrain(i64 *i, u8csc tok);

fun u64 u64hash2(u64 low, u64 high) {
    // Murmur-inspired hashing from LLVM
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t a = (low ^ high) * kMul;
    a ^= (a >> 47);
    uint64_t b = (high ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;
    return b;
}

fun u64 u128hash(u128cp val) {
    u64 low = val->_64[0];
    u64 high = val->_64[1];
    return u64hash2(low, high);
}

#endif  // ABC_INT_H
