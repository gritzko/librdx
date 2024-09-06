#ifndef LIBSODIUM_HEX_H
#define LIBSODIUM_HEX_H

#include "INT.h"
#include "OK.h"

con ok64 HEXnoroom = 0x113a1cb3db3cf1;
con ok64 HEXbad = 0xa259a1391;

$u8 BASE16 = $u8str("0123456789abcdef");

fun ok64 HEXfeed($u8 hex, $u8c bin) {
    while (!$empty(bin) && $len(hex) >= 2) {
        **hex = $at(BASE16, **bin >> 4);
        ++*hex;
        **hex = $at(BASE16, **bin & 0xf);
        ++*hex;
        ++*bin;
    }
    return OK;
}

fun ok64 HEXput($u8 hex, $cu8c bin) {
    a$dup(u8c, copy, bin);
    return HEXfeed(hex, copy);
}

const u8 BASE16rev[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x0,  0x1,  0x2,  0x3,  0x4,  0x5,  0x6,  0x7,  0x8,  0x9,  0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xa,  0xb,  0xc,  0xd,  0xe,  0xf,  0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xa,  0xb,  0xc,  0xd,  0xe,  0xf,  0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
};

fun ok64 HEXdrain($u8 bin, $u8c hex) {
    if ($len(hex) & 1) return HEXbad;
    while (!$empty(hex) && !$empty(bin)) {
        u8 u = BASE16rev[**hex];
        if (unlikely(u == 0xff)) return HEXbad;
        ++*hex;
        u8 l = BASE16rev[**hex];
        if (unlikely(u == 0xff)) return HEXbad;
        ++*hex;
        **bin = (u << 4) | l;
        ++*bin;
    }
    return OK;
}

fun ok64 HEXfeedall($u8 hex, $cu8c bin) {
    if ($len(bin) * 2 > $len(hex)) return HEXnoroom;
    $u8c dup = {bin[0], bin[1]};
    return HEXfeed(hex, dup);
}

fun ok64 HEXdrainall($u8 bin, $cu8c hex) {
    if ($len(hex) > $len(bin) * 2) return HEXnoroom;
    $u8c dup = {hex[0], hex[1]};
    return HEXdrain(bin, dup);
}

fun ok64 u64hexfeed($u8 hex, u64 val) {
    if ($empty(hex)) return HEXnoroom;
    u8 tmp[16];
    $u8 h = {tmp + 16, tmp + 16};
    do {
        *--*h = $at(BASE16, val & 0xf);
        val >>= 4;
    } while (val != 0);
    return $u8feed(hex, (u8c**)h);
}

fun ok64 u64hexdrain(u64* res, $u8c hex) {
    if ($len(hex) > sizeof(u64) * 2) return HEXbad;
    u64 t = 0;
    $for(u8c, p, hex) {
        u8 u = BASE16rev[*p];
        if (unlikely(u == 0xff)) return HEXbad;
        t <<= 4;
        t |= u;
    }
    *res = t;
    return OK;
}

#endif
