#ifndef LIBRDX_ZINT_H
#define LIBRDX_ZINT_H

#include "INT.h"
#include "PRO.h"

con ok64 ZINTnoroom = 0xc73cf6cf27574a3;
con ok64 ZINTbadrec = 0x9e9da89667574a3;

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

fun ok64 ZINTu64feed($u8 into, u64 n) {
    if ($len(into) < 8) return ZINTnoroom;
    if (n <= B1) {
        if (n != 0) $u8feed8(into, (u8*)&n);
    } else if (n <= B2) {
        $u8feed16(into, (u16*)&n);
    } else if (n <= B4) {
        $u8feed32(into, (u32*)&n);
    } else {
        $u8feed64(into, &n);
    }
    return OK;
}

fun ok64 ZINTu64drain(u64* n, $cu8c zip) {
    a$dup(u8c, from, zip);
    switch ($len(from)) {
        case 0:
            *n = 0;
            return OK;
        case 1:
            $u8drain8((u8*)n, from);
            return OK;
        case 2:
            $u8drain16((u16*)n, from);
            return OK;
        case 3:
            return ZINTbadrec;
        case 4:
            $u8drain32((u32*)n, from);
            return OK;
        case 5:
        case 6:
        case 7:
            return ZINTbadrec;
        case 8:
            $u8drain64(n, from);
            return OK;
        default:
            return ZINTbadrec;
    }
}

// ZipUint64Pair packs a pair of uint64 into a byte string.
// The smaller the ints, the shorter the string
fun pro(ZINTu128feed, $u8 into, u128 a) {
    sane($ok(into));
    test($size(into) >= sizeof(u64) * 2, ZINTnoroom);
    u64 big = a._64[0];
    u64 lil = a._64[1];
    if (lil <= B1) {
        if (big <= B1) {
            if (big != 0 || lil != 0) $u8feed8(into, (u8*)&big);
        } else if (big <= B2) {
            $u8feed16(into, (u16*)&big);
        } else if (big <= B4) {
            $u8feed32(into, (u32*)&big);
        } else {
            $u8feed64(into, &big);
        }
        if (lil != 0 || big > B1) $u8feed8(into, (u8*)&lil);
    } else if (lil <= B2) {
        if (big <= B2) {
            $u8feed16(into, (u16*)&big);
        } else if (big <= B4) {
            $u8feed32(into, (u32*)&big);
        } else {
            $u8feed64(into, &big);
        }
        $u8feed16(into, (u16*)&lil);
    } else if (lil <= B4) {
        if (big <= B4) {
            $u8feed32(into, (u32*)&big);
        } else {
            $u8feed64(into, &big);
        }
        $u8feed32(into, (u32*)&lil);
    } else {
        $u8feed64(into, &big);
        $u8feed64(into, &lil);
    }
    done;
}

fun pro(ZINTu128drain, u128* a, $u8c from) {
    sane($ok(from) && a != nil);
    u32 len = $len(from);
    u64* big = &(a->_64[0]);
    u64* lil = &(a->_64[1]);
    *big = *lil = 0;
    switch (len) {
        case 0:
            break;
        case 1:
            $u8drain8((u8*)big, from);
            break;
        case 2:
            $u8drain8((u8*)big, from);
            $u8drain8((u8*)lil, from);
            break;
        case 3:
            $u8drain16((u16*)big, from);
            $u8drain8((u8*)lil, from);
            break;
        case 4:
            $u8drain16((u16*)big, from);
            $u8drain16((u16*)lil, from);
            break;
        case 5:
            $u8drain32((u32*)big, from);
            $u8drain8((u8*)lil, from);
            break;
        case 6:
            $u8drain32((u32*)big, from);
            $u8drain16((u16*)lil, from);
            break;
        case 7:
            return ZINTbadrec;
        case 8:
            $u8drain32((u32*)big, from);
            $u8drain32((u32*)lil, from);
            break;
        case 9:
            $u8drain64((u64*)big, from);
            $u8drain8((u8*)lil, from);
            break;
        case 10:
            $u8drain64((u64*)big, from);
            $u8drain16((u16*)lil, from);
            break;
        case 11:
            return ZINTbadrec;
        case 12:
            $u8drain64((u64*)big, from);
            $u8drain32((u32*)lil, from);
            break;
        case 13:
            return ZINTbadrec;
        case 14:
            return ZINTbadrec;
        case 15:
            return ZINTbadrec;
        case 16:
            $u8drain64((u64*)big, from);
            $u8drain64((u64*)lil, from);
            break;
        default:
            return ZINTbadrec;
    }
    done;
}

fun u64 ZINTzigzag(int64_t i) { return (u64)(i * 2) ^ (u64)(i >> 63); }

fun int64_t ZINTzagzig(u64 u) {
    u64 half = u >> 1;
    u64 mask = -(u & 1);
    return (int64_t)(half ^ mask);
}

fun ok64 ZINTi64feed($u8 into, i64 const* n) {
    return ZINTu64feed(into, ZINTzigzag(*n));
}
fun ok64 ZINTi64drain(i64* n, $cu8c zip) {
    u64 u;
    ok64 o = ZINTu64drain(&u, zip);
    if (o == OK) *n = ZINTzagzig(u);
    return o;
}

typedef double f64;

fun u64 ZINTf64bits(f64 val) { return *(u64*)&val; }

fun f64 ZINTf64from(u64 bits) { return *(f64*)&bits; }

fun ok64 ZINTf64feed($u8 into, f64 const* n) {
    u64 bits = flip64(*(u64 const*)n);
    return ZINTu64feed(into, bits);
}
fun ok64 ZINTf64drain(f64* n, $cu8c from) {
    u64 bits = 0;
    ok64 o = ZINTu64drain(&bits, from);
    if (o == OK) *(u64*)n = flip64(bits);
    return o;
}

fun int ZINTu128z($cu8c a, $cu8c b) {
    $u8c aa = $dup(a);
    $u8c bb = $dup(b);
    u128 an = {};
    u128 bn = {};
    ZINTu128drain(&an, aa);
    ZINTu128drain(&bn, bb);
    return u128cmp(&an, &bn);
}

fun int ZINTi64z($cu8c a, $cu8c b) {
    $u8c aa = $dup(a);
    $u8c bb = $dup(b);
    i64 an = {};
    i64 bn = {};
    ZINTi64drain(&an, aa);
    ZINTi64drain(&bn, bb);
    return i64cmp(&an, &bn);
}

fun int f64z(f64 const* a, f64 const* b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

fun int ZINTf64z($cu8c a, $cu8c b) {
    $u8c aa = $dup(a);
    $u8c bb = $dup(b);
    f64 an = {};
    f64 bn = {};
    ZINTf64drain(&an, aa);
    ZINTf64drain(&bn, bb);
    return f64z(&an, &bn);
}

#endif
