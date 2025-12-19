
#include "ZINT.h"

#include <stdint.h>
#include <unistd.h>

#include "PRO.h"
#include "S.h"
#include "TEST.h"

pro(ZINTTest1) {
    sane(1);
    i64 i[] = {
        -126446, 65536, 0, INT64_MAX, INT64_MIN, -1, 1,
    };
    a$(i64, is, i);
    $eat(is) {
        u64 u = i64Zig(**is);
        i64 i2 = u64Zag(u);
        sane(**is == i2);
    }

    done;
}

pro(ZINTTest2) {
    sane(1);
    i64 i[] = {
        0,          0,

        126446,     65536,

        0,          UINT64_MAX,

        UINT64_MAX, 0,

        0xffff,     0xff,

        0xff,       0xffff,

        0xffffff,   0,

        0,          0xffffff,

        0xffffff,   0xffff,

        0xffff,     0xffffff,

        1,          1,
    };
    a$(i64, tozip, i);
    $eat(tozip) {
        u128 a;
        a._64[0] = **tozip;
        ++*tozip;
        a._64[1] = **tozip;
        aBpad(u8, pad, 16);
        u8$ into = u8bIdle(pad);
        u8c$ data = u8bDataC(pad);
        ZINTu128feed(into, &a);
        u128 b = {};
        ZINTu128drain(&b, data);
        sane(0 == u128cmp(&a, &b));
    }

    done;
}

ok64 ZINTTest3() {
    sane(1);
    a_pad(u64, words, 8);
    a_pad(u8, buf, 64);
    a_pad(u64, words2, 8);
    u64bFeed1(words, 1);
    u64bFeed1(words, 0xff);
    u64bFeed1(words, 0x123);
    u64bFeed1(words, 0x12345);
    u64bFeed1(words, 1);
    u64bFeed1(words, 0xffffffffffffffff);
    a_dup(u64c, w, words_datac);
    call(ZINTu8sFeedBlocked, buf_idle, w);
    call(ZINTu8sDrainBlocked, buf_datac, words2_idle);
    $testeq(words_data, words2_data);
    done;
}

ok64 ZINTTest4() {
    sane(1);
    a_pad(u64, words, 8);
    a_pad(u64, words2, 8);
    u64bFeed1(words, 1);
    u64bFeed1(words, 0xff);
    u64bFeed1(words, 0x123);
    u64bFeed1(words, 0x12345);
    u64bFeed1(words, 0x1234567);
    u64bFeed1(words, 0xffffffffffffffff);
    u64bFeed(words2, words_datac);
    $testeq(words_data, words2_data);
    call(ZINTu64sDelta, words_data, 1);
    testeq(**words_data, 0);
    call(ZINTu64sUndelta, words_data, 1);
    $testeq(words_data, words2_data);
    done;
}

pro(ZINTTest) {
    sane(1);
    call(ZINTTest1);
    call(ZINTTest2);
    call(ZINTTest3);
    call(ZINTTest4);
    done;
}

TEST(ZINTTest);
