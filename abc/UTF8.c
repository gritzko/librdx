//
// (c) Victor Grishchenko, 2020-2023
//
#include "UTF8.h"

#include "PRO.h"

pro(_UTF8feed1, $u8 into, u32 cp) {
    sane($ok(into));
    if (cp < 0x800) {
        test($len(into) >= 2, UTF8noroom);
        $feed1(into, (u8)(cp >> 6) | UTF8LEAD2);
        $feed1(into, (u8)(cp & 63) | UTF8CONT);
    } else if (cp < 0x10000) {
        test($len(into) >= 3, UTF8noroom);
        $feed1(into, (u8)(cp >> 12) | UTF8LEAD3);
        $feed1(into, (u8)((cp >> 6) & 63) | UTF8CONT);
        $feed1(into, (u8)(cp & 63) | UTF8CONT);
    } else if (cp <= MAX_UNICODE_CODEPOINT) {
        test($len(into) >= 4, UTF8noroom);
        $feed1(into, (u8)(cp >> 18) | UTF8LEAD4);
        $feed1(into, (u8)((cp >> 12) & 63) | UTF8CONT);
        $feed1(into, (u8)((cp >> 6) & 63) | UTF8CONT);
        $feed1(into, (u8)(cp & 63) | UTF8CONT);
    } else {
        fail(UTF8bad)
    }
    done;
}

pro(_UTF8drain1, u32 *cp, $u8c data) {
    sane($ok(data) && cp != nil);
    const u8 *utf8 = *data;
    unsigned char byte = utf8[0];
    u32 code_point = 0;
    if (byte < 0b10000000) {
        ++*data;
        code_point = byte;
    } else if ((byte & 0b11100000) == 0b11000000) {
        test(2 <= $len(data), UTF8nodata);
        if ((utf8[1] & 0b11000000) != 0b10000000) fail(UTF8bad);
        code_point = (byte & 0b00011111) << 6 | (utf8[1] & 0b00111111);
        if (code_point < 0x80 || 0x7ff < code_point) fail(UTF8bad);
        *data += 2;
    } else if ((byte & 0b11110000) == 0b11100000) {
        test(3 <= $len(data), UTF8nodata);
        if ((utf8[1] & 0b11000000) != 0b10000000) fail(UTF8bad);
        if ((utf8[2] & 0b11000000) != 0b10000000) fail(UTF8bad);
        code_point = (byte & 0b00001111) << 12 | (utf8[1] & 0b00111111) << 6 |
                     (utf8[2] & 0b00111111);
        if (code_point < 0x800 || 0xffff < code_point ||
            (0xd7ff < code_point && code_point < 0xe000))
            fail(UTF8bad);
        *data += 3;
    } else if ((byte & 0b11111000) == 0b11110000) {  // 0b11110000
        test(4 <= $len(data), UTF8nodata);
        if ((utf8[1] & 0b11000000) != 0b10000000) fail(UTF8bad);
        if ((utf8[2] & 0b11000000) != 0b10000000) fail(UTF8bad);
        if ((utf8[3] & 0b11000000) != 0b10000000) fail(UTF8bad);
        code_point = (byte & 0b00000111) << 18 | (utf8[1] & 0b00111111) << 12 |
                     (utf8[2] & 0b00111111) << 6 | (utf8[3] & 0b00111111);
        if (code_point < 0xffff || 0x10ffff < code_point) fail(UTF8bad);
        *data += 4;
    } else {
        // we may have a continuation
        fail(UTF8bad);
    }
    *cp = code_point;
    done;
}
