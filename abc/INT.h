//
// Created by gritzko on 5/10/24.
//

#ifndef ABC_INT_H
#define ABC_INT_H

#include <stdint.h>

#include "01.h"
#include "B.h"
#include "BUF.h"

static const ok64 INTbad = 0x497766968;

#define I64_MAX INT64_MAX
#define I64_MIN INT64_MIN
#define I64_MIN_ABS 0x8000000000000000

fun z32 u16z(const u16 *a, const u16 *b) {
    if (*a == *b) return z32eq;
    return *a < *b ? z32lt : z32gt;
}
fun z32 u32z(const u32 *a, const u32 *b) {
    if (*a == *b) return z32eq;
    return *a < *b ? z32lt : z32gt;
}
fun z32 u64z(const u64 *a, const u64 *b) {
    if (*a == *b) return z32eq;
    return *a < *b ? z32lt : z32gt;
}

fun z32 i8z(const i8 *a, const i8 *b) {
    if (*a == *b) return z32eq;
    return *a < *b ? z32lt : z32gt;
}
fun z32 i16z(const i16 *a, const i16 *b) {
    if (*a == *b) return z32eq;
    return *a < *b ? z32lt : z32gt;
}
fun z32 i32z(const i32 *a, const i32 *b) {
    if (*a == *b) return z32eq;
    return *a < *b ? z32lt : z32gt;
}
fun z32 i64z(const i64 *a, const i64 *b) {
    if (*a == *b) return z32eq;
    return *a < *b ? z32lt : z32gt;
}

fun z32 u128z(u128 const *a, u128 const *b) {
    z32 ret = u64z(&a->_64[1], &b->_64[1]);
    if (ret == z32eq) {
        ret = u64z(&a->_64[0], &b->_64[0]);
    }
    return ret;
}

fun z32 u256z(u256 const *a, u256 const *b) {
    z32 ret = u64z(&a->_64[3], &b->_64[3]);
    if (ret == z32eq) {
        ret = u64z(&a->_64[2], &b->_64[2]);
        if (ret == z32eq) {
            ret = u64z(&a->_64[1], &b->_64[1]);
            if (ret == z32eq) {
                ret = u64z(&a->_64[0], &b->_64[0]);
            }
        }
    }
    return ret;
}

fun z32 u512z(u512 const *a, u512 const *b) {
    for (int i = 7; i >= 0; --i) {
        if (a->_64[i] == b->_64[i]) continue;
        return a->_64[i] < b->_64[i] ? z32lt : z32gt;
    }
    return z32eq;
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

fun ok64 $u8drain8(u8 *into, $u8c from) {
    if ($empty(from)) return $nodata;
    *into = **from;
    *from += sizeof(u8);
    return OK;
}

fun ok64 $u8feed8($u8 into, u8 const *what) {
    if ($empty(into)) return $noroom;
    **into = *what;
    *into += sizeof(u8);
    return OK;
}

#ifdef ABC_ALIGN
fun ok64 $u8drain16(u16 *into, $u8 from) {
    if ($size(from) < sizeof(u16)) return $nodata;
    *into = **from;
    ++*from;
    *into |= u16(**from) << 8;
    ++*from;
    return OK;
}
fun ok64 $u8drain32(u32 *into, $u8 from) {
    if ($size(from) < sizeof(u32)) return $nodata;
    u16 lo = 0, hi = 0;
    $u8drain16(&lo, from);
    $u8drain16(&hi, from);
    *into = lo;
    *into |= u32(hi) << 16;
    return OK;
}
fun ok64 $u8drain64(u64 *into, $u8 from) {
    if ($size(from) < sizeof(u64)) return $nodata;
    u32 lo = 0, hi = 0;
    $u8drain32(&lo, from);
    $u8drain32(&hi, from);
    *into = lo;
    *into |= u64(hi) << 32;
    return OK;
}
#else
fun ok64 $u8drain16(u16 *into, $u8c from) {
    if ($len(from) < sizeof(u16)) return $nodata;
    memcpy(into, *from, sizeof(u16));
    *from += sizeof(u16);
    return OK;
}
fun ok64 $u8feed16($u8 into, u16 const *what) {
    if ($len(into) < sizeof(u16)) return $noroom;
    memcpy(*into, what, 2);
    *into += sizeof(u16);
    return OK;
}
fun ok64 $u8drain32(u32 *into, $u8c from) {
    if ($len(from) < sizeof(u32)) return $nodata;
    memcpy(into, *from, sizeof(u32));
    *from += sizeof(u32);
    return OK;
}
fun ok64 $u8feed32($u8 into, u32 const *what) {
    if ($len(into) < sizeof(u32)) return $noroom;
    memcpy(*into, what, 4);
    *into += sizeof(u32);
    return OK;
}
fun ok64 $u8drain64(u64 *into, $u8c from) {
    if ($len(from) < sizeof(u64)) return $nodata;
    memcpy(into, *from, sizeof(u64));
    *from += sizeof(u64);
    return OK;
}
fun ok64 $u8feed64($u8 into, u64 const *what) {
    if ($len(into) < sizeof(u64)) return $noroom;
    memcpy(*into, what, 8);
    *into += sizeof(u64);
    return OK;
}
#endif

fun u8 u64bit(u64 u, u32 ndx) { return 1 & (u >> ndx); }
fun u8 u128bit(u128 u, u32 ndx) { return 1 & (u._64[ndx >> 6] >> (ndx & 63)); }

ok64 i64decdrain(i64 *i, $u8c tok);

#endif  // ABC_INT_H
