//
// (c) Victor Grishchenko, 2020-2023
//

#ifndef ABC_UTF8_H
#define ABC_UTF8_H

#include "S.h"

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
con ok64 UTF8badnum = 0x79d3c89a5a32e71;

typedef unsigned char utf8;
fun int utf8cmp(utf8 const *a, utf8 const *b) { return $cmp(a, b); }
#define X(M, name) M##utf8##name
#include "Bx.h"
#undef X

ok64 _utf8sFeed32(utf8s into, u32 cp);

fun ok64 utf8sFeed32(utf8s into, u32 cp) {
    return cp < 0x80 ? utf8sFeed1(into, cp) : _utf8sFeed32(into, cp);
}

ok64 _utf8sDrain32(u32 *cp, utf8cs data);

fun ok64 utf8sDrain32(u32 *cp, utf8cs utf8) {
    if ($empty(utf8)) return UTF8nodata;
    if (**utf8 >= 0x80) return _utf8sDrain32(cp, utf8);
    *cp = **utf8;
    ++*utf8;
    return OK;
}

fun ok64 utf8sValid(utf8cs utf8) {
    if (!utf8csOK(utf8)) return badarg;
    u32 cp;
    ok64 o = OK;
    while (!$empty(utf8) && o == OK) o = utf8sDrain32(&cp, utf8);
    return o;
}

ok64 utf8sFeedInt(utf8s txt, i64cp i);

ok64 utf8sDrainInt(utf8cs txt, i64p i);

ok64 utf8sDrainFloat(utf8cs txt, f64p f);

ok64 utf8sFeedFloat(utf8s txt, f64cp f);

#endif  // CT_UTF8_H
