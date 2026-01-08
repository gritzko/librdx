#ifndef LIBRDX_ZINT_H
#define LIBRDX_ZINT_H

#include "01.h"
#include "INT.h"
#include "OK.h"

con ok64 ZINTNOROOM = 0x8d25dd5d86d8616;
con ok64 ZINTBAD = 0x2349774b28d;

con u64 B1 = 0xff;
con u64 B2 = 0xffff;
con u64 B4 = 0xffffffff;
con u64 B8 = 0xffffffffffffffff;

fun u32 ZINTlen(u64 n) {
    if (n <= 0xff) {
        if (n == 0) return 0;
        return 1;
    }
    if (n <= 0xffff) return 2;
    if (n <= 0xffffffff) return 4;
    return 8;
}

// Compute canonical length for a ZINT128 pair (big, lil)
fun u32 ZINT128len(u64 big, u64 lil) {
    if (lil <= B1) {
        u32 biglen = (big <= B1) ? (big != 0 || lil != 0 ? 1 : 0)
                   : (big <= B2) ? 2
                   : (big <= B4) ? 4 : 8;
        u32 lillen = (lil != 0 || big > B1) ? 1 : 0;
        return biglen + lillen;
    } else if (lil <= B2) {
        u32 biglen = (big <= B2) ? 2 : (big <= B4) ? 4 : 8;
        return biglen + 2;
    } else if (lil <= B4) {
        u32 biglen = (big <= B4) ? 4 : 8;
        return biglen + 4;
    } else {
        return 16;
    }
}

fun ok64 ZINTu64feed($u8 into, u64 n) {
    if ($len(into) < 8) return ZINTNOROOM;
    if (n <= B1) {
        if (n != 0) u8sFeed8(into, (u8*)&n);
    } else if (n <= B2) {
        u8sFeed16(into, (u16*)&n);
    } else if (n <= B4) {
        u8sFeed32(into, (u32*)&n);
    } else {
        u8sFeed64(into, &n);
    }
    return OK;
}

fun ok64 ZINTu64drain(u64* n, $cu8c zip) {
    *n = 0;
    a_dup(u8c, from, zip);
    u32 len = $len(from);
    switch (len) {
        case 0:
            return OK;
        case 1:
            u8sDrain8(from, (u8*)n);
            break;
        case 2:
            u8sDrain16(from, (u16*)n);
            break;
        case 3:
            return ZINTBAD;
        case 4:
            u8sDrain32(from, (u32*)n);
            break;
        case 5:
        case 6:
        case 7:
            return ZINTBAD;
        case 8:
            u8sDrain64(from, n);
            break;
        default:
            return ZINTBAD;
    }
    // Verify canonical encoding
    if (ZINTlen(*n) != len) return ZINTBAD;
    return OK;
}

// Packs a pair of uint64 into a byte string.
// The smaller the ints, the shorter the string
ok64 ZINTu8sFeed128(u8s into, u64 big, u64 lil);

ok64 ZINTu8sDrain128(u8cs from, u64* big, u64* lil);

fun ok64 ZINTu128drain(u128* a, u8cs from) {
    u64* big = &(a->_64[0]);
    u64* lil = &(a->_64[1]);
    return ZINTu8sDrain128(from, big, lil);
}

fun ok64 ZINTu128feed($u8 into, u128c* a) {
    if (!$ok(into) || $size(into) < sizeof(u64) * 2) return ZINTNOROOM;
    u64 big = a->_64[0];
    u64 lil = a->_64[1];
    return ZINTu8sFeed128(into, big, lil);
}

fun ok64 ZINTu8sFeedInt(u8s into, i64cp n) {
    return ZINTu64feed(into, i64Zig(*n));
}
fun ok64 ZINTu8sDrainInt(i64* n, $cu8c zip) {
    u64 u;
    ok64 o = ZINTu64drain(&u, zip);
    if (o == OK) *n = u64Zag(u);
    return o;
}

fun u64 ZINTf64bits(f64 val) { return *(u64*)&val; }

fun f64 ZINTf64from(u64 bits) { return *(f64*)&bits; }

fun ok64 ZINTu8sFeedFloat($u8 into, f64cp n) {
    u64 bits = flip64(*(u64 const*)n);
    return ZINTu64feed(into, bits);
}
fun ok64 ZINTu8sDrainFloat(f64* n, $cu8c from) {
    u64 bits = 0;
    ok64 o = ZINTu64drain(&bits, from);
    if (o == OK) *(u64*)n = flip64(bits);
    return o;
}

fun int ZINTu128z($cu8c a, $cu8c b) {
    u8cs aa = $dup(a);
    u8cs bb = $dup(b);
    u128 an = {};
    u128 bn = {};
    ZINTu128drain(&an, aa);
    ZINTu128drain(&bn, bb);
    return u128cmp(&an, &bn);
}

fun int ZINTi64z($cu8c a, $cu8c b) {
    u8cs aa = $dup(a);
    u8cs bb = $dup(b);
    i64 an = 0;
    i64 bn = 0;
    ZINTu8sDrainInt(&an, aa);
    ZINTu8sDrainInt(&bn, bb);
    return i64cmp(&an, &bn);
}

fun int f64z(f64 const* a, f64 const* b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

fun int ZINTf64z($cu8c a, $cu8c b) {
    u8cs aa = $dup(a);
    u8cs bb = $dup(b);
    f64 an = 0;
    f64 bn = 0;
    ZINTu8sDrainFloat(&an, aa);
    ZINTu8sDrainFloat(&bn, bb);
    return f64z(&an, &bn);
}

ok64 ZINTu64sDelta(u64s ints, u64 start);
ok64 ZINTu64sUndelta(u64s ints, u64 start);

// Converts a slice of u64 into a stream of blocked varints: first,
// a header byte contains 4 2-bit lengths (1,2,4,8). Then, the
// numbers follow.
ok64 ZINTu8sFeedBlocked(u8s into, u64cs ints);
// Restores an int slice from the blocked varint coding.
ok64 ZINTu8sDrainBlocked(u8cs from, u64s ints);

#endif
