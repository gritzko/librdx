#ifndef LIBRDX_ZINT_H
#define LIBRDX_ZINT_H

#include "INT.h"
#include "PRO.h"

con ok64 ZINTnoroom = 0xc73cf6cf27574a3;
con ok64 ZINTbadrec = 0x9e9da89667574a3;

fun void $u8drain8(u8* into, $u8c from) {
    *into = **from;
    *from += sizeof(u8);
}
fun void $u8feed8($u8 into, u8 const* what) {
    **into = *what;
    *into += sizeof(u8);
}

#ifdef ABC_ALIGN
fun void $u8drain16(u16* into, $u8 from) {
    *into = **from;
    ++*from;
    *into |= u16(**from) << 8;
    ++*from;
}
fun void $u8drain32(u32* into, $u8 from) {
    u16 lo = 0, hi = 0;
    $u8drain16(&lo, from);
    $u8drain16(&hi, from);
    *into = lo;
    *into |= u32(hi) << 16;
}
fun void $u8drain64(u64* into, $u8 from) {
    u32 lo = 0, hi = 0;
    $u8drain32(&lo, from);
    $u8drain32(&hi, from);
    *into = lo;
    *into |= u64(hi) << 32;
}
#else
fun void $u8drain16(u16* into, $u8c from) {
    *into = *(u16*)*from;
    *from += sizeof(u16);
}
fun void $u8feed16($u8 into, u16 const* what) {
    *(u16*)*into = *what;
    *into += sizeof(u16);
}
fun void $u8drain32(u32* into, $u8c from) {
    *into = *(u32*)*from;
    *from += sizeof(u32);
}
fun void $u8feed32($u8 into, u32 const* what) {
    *(u32*)*into = *what;
    *into += sizeof(u32);
}
fun void $u8drain64(u64* into, $u8c from) {
    *into = *(u64*)*from;
    *from += sizeof(u64);
}
fun void $u8feed64($u8 into, u64 const* what) {
    *(u64*)*into = *what;
    *into += sizeof(u64);
}
#endif

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

fun void ZINTfeedu64($u8 into, u64 n) {
    if (n <= B1) {
        if (n != 0) $u8feed8(into, (u8*)&n);
    } else if (n <= B2) {
        $u8feed16(into, (u16*)&n);
    } else if (n <= B4) {
        $u8feed32(into, (u32*)&n);
    } else {
        $u8feed64(into, &n);
    }
}

fun ok64 ZINTdrainu64(u64* n, $u8c from) {
    switch ($len(from)) {
        case 0:
            *n = 0;
        case 1:
            $u8drain8((u8*)n, from);
        case 2:
            $u8drain16((u16*)n, from);
        case 3:
            return ZINTbadrec;
        case 4:
            $u8drain32((u32*)n, from);
        case 5:
        case 6:
        case 7:
            return ZINTbadrec;
        case 8:
            $u8drain64(n, from);
        default:
            return ZINTbadrec;
    }
}

// ZipUint64Pair packs a pair of uint64 into a byte string.
// The smaller the ints, the shorter the string
fun pro(ZINTu128feed, $u8 into, u128 const* a) {
    sane($ok(into) && a != nil);
    test($size(into) >= sizeof(u64) * 2, ZINTnoroom);
    u64 big = a->_64[0];
    u64 lil = a->_64[1];
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

typedef double f64;

fun u64 ZINTf64bits(f64 val) { return *(u64*)&val; }

fun f64 ZINTf64from(u64 bits) { return *(f64*)&bits; }

#endif
