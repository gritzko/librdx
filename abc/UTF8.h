//
// (c) Victor Grishchenko, 2020-2023
//

#ifndef ABC_UTF8_H
#define ABC_UTF8_H

#include "BUF.h"

typedef u32 cp32;

con u8 UTF8CONT = 128;
con u8 UTF8LEAD2 = 128 | 64;
con u8 UTF8LEAD3 = 128 | 64 | 32;
con u8 UTF8LEAD4 = 128 | 64 | 32 | 16;

con u32 MAX_UNICODE_CODEPOINT = 0x10FFFF;
con u8 MAX_UTF8_SIZE = 4;

con ok64 UTF8nodata = 0x79d3c8cb3a25e25;
con ok64 UTF8noroom = 0x79d3c8cb3db3cf1;
con ok64 UTF8bad = 0x1e74f226968;

ok64 _UTF8feed1($u8 into, u32 cp);

fun ok64 UTF8feed1($u8 into, u32 cp) {
    return cp < 0x80 ? u8s_feed1(into, cp) : _UTF8feed1(into, cp);
}

ok64 _UTF8drain1(u32 *cp, $u8c data);

fun ok64 UTF8drain1(u32 *cp, $u8c utf8) {
    if ($empty(utf8)) return UTF8nodata;
    if (**utf8 >= 0x80) return _UTF8drain1(cp, utf8);
    *cp = **utf8;
    ++*utf8;
    return OK;
}

fun ok64 UTF8valid($u8c utf8) {
    if (!$ok(utf8)) return badarg;
    u32 cp;
    ok64 o = OK;
    while (!$empty(utf8) && o == OK) o = UTF8drain1(&cp, utf8);
    return o;
}

#endif  // CT_UTF8_H
