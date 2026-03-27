//
// (c) Victor Grishchenko, 2020-2023
//
#include "UTF8.h"

#include "01.h"
#include "PRO.h"
#include "ryu/ryu.h"

ok64 _utf8sFeed32(utf8s into, u32 cp) {
    sane($ok(into));
    if (cp < 0x800) {
        test($len(into) >= 2, UTFNOROOM);
        $feed1(into, (u8)(cp >> 6) | UTF8LEAD2);
        $feed1(into, (u8)(cp & 63) | UTF8CONT);
    } else if (cp < 0x10000) {
        test($len(into) >= 3, UTFNOROOM);
        $feed1(into, (u8)(cp >> 12) | UTF8LEAD3);
        $feed1(into, (u8)((cp >> 6) & 63) | UTF8CONT);
        $feed1(into, (u8)(cp & 63) | UTF8CONT);
    } else if (cp <= MAX_UNICODE_CODEPOINT) {
        test($len(into) >= 4, UTFNOROOM);
        $feed1(into, (u8)(cp >> 18) | UTF8LEAD4);
        $feed1(into, (u8)((cp >> 12) & 63) | UTF8CONT);
        $feed1(into, (u8)((cp >> 6) & 63) | UTF8CONT);
        $feed1(into, (u8)(cp & 63) | UTF8CONT);
    } else {
        fail(UTFBAD)
    }
    done;
}

ok64 _utf8sDrain32(u32 *cp, utf8cs data) {
    sane($ok(data) && cp != NULL);
    const u8 *utf8 = *data;
    unsigned char byte = utf8[0];
    u32 code_point = 0;
    if (byte < 0b10000000) {
        ++*data;
        code_point = byte;
    } else if ((byte & 0b11100000) == 0b11000000) {
        test(2 <= $len(data), UTF8NODATA);
        if ((utf8[1] & 0b11000000) != 0b10000000) fail(UTFBAD);
        code_point = (byte & 0b00011111) << 6 | (utf8[1] & 0b00111111);
        if (code_point < 0x80 || 0x7ff < code_point) fail(UTFBAD);
        *data += 2;
    } else if ((byte & 0b11110000) == 0b11100000) {
        test(3 <= $len(data), UTF8NODATA);
        if ((utf8[1] & 0b11000000) != 0b10000000) fail(UTFBAD);
        if ((utf8[2] & 0b11000000) != 0b10000000) fail(UTFBAD);
        code_point = (byte & 0b00001111) << 12 | (utf8[1] & 0b00111111) << 6 |
                     (utf8[2] & 0b00111111);
        if (code_point < 0x800 || 0xffff < code_point ||
            (0xd7ff < code_point && code_point < 0xe000))
            fail(UTFBAD);
        *data += 3;
    } else if ((byte & 0b11111000) == 0b11110000) {  // 0b11110000
        test(4 <= $len(data), UTF8NODATA);
        if ((utf8[1] & 0b11000000) != 0b10000000) fail(UTFBAD);
        if ((utf8[2] & 0b11000000) != 0b10000000) fail(UTFBAD);
        if ((utf8[3] & 0b11000000) != 0b10000000) fail(UTFBAD);
        code_point = (byte & 0b00000111) << 18 | (utf8[1] & 0b00111111) << 12 |
                     (utf8[2] & 0b00111111) << 6 | (utf8[3] & 0b00111111);
        if (code_point < 0xffff || 0x10ffff < code_point) fail(UTFBAD);
        *data += 4;
    } else {
        // we may have a continuation
        fail(UTFBAD);
    }
    *cp = code_point;
    done;
}

ok64 utf8sDrainFloat(utf8cs txt, f64p f) {
    sane($ok(txt) && f != NULL);
    size_t tl = $len(txt);
    if (tl >= 32) tl = 31;
    char str[32];
    memcpy(str, *txt, tl);
    str[tl] = 0;
    char *ep = NULL;
    *f = strtod((char *)str, &ep);
    test(ep > str, UTFBADNUM);
    *txt += ep - str;
    done;
}

ok64 utf8sFeedFloat(utf8s txt, f64cp f) {
    sane($ok(txt) && f != NULL);
    u8 res[32];
    int len = d2s_buffered_n(*f, (char *)res);
    u8cs $res = {res, res + len};
    call(u8sFeed, txt, $res);
    done;
}

ok64 utf8sDrainInt(utf8cs txt, i64p i) {
    sane($ok(txt) && !$empty(txt) && i != NULL);
    u64 u = 0;
    b8 neg = NO;
    if (**txt == '-') {
        ++*txt;
        neg = YES;
    }
    call(u64decdrain, &u, txt);
    if (neg) {
        test(u <= i64MinAbsValue, UTFBADNUM);
        *i = -u;
    } else {
        test(u <= i64MaxValue, UTFBADNUM);
        *i = u;
    }
    done;
}

ok64 utf8sFeedInt(utf8s txt, i64cp i) {
    sane($ok(txt) && i != NULL);
    u64 u;
    if (*i < 0) {
        call(u8sFeed1, txt, '-');
        u = -*i;
    } else {
        u = *i;
    }
    call(utf8sFeed10, txt, u);
    done;
}
