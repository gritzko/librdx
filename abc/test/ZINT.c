#include "ZINT.h"

#include <stdint.h>
#include <unistd.h>

#include "PRO.h"
#include "S.h"
#include "TEST.h"

ok64 ZINTTest1() {
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

ok64 ZINTTest2() {
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

ok64 ZINTTestDelta() {
    sane(1);

    // Empty slice - should be no-op
    {
        u64 empty[1];
        u64s s = {empty, empty};
        call(ZINTu64sDelta, s, 0);
        call(ZINTu64sUndelta, s, 0);
    }

    // Single element
    {
        u64 one[] = {100};
        a$(u64, s, one);
        call(ZINTu64sDelta, s, 0);
        testeq(one[0], 100);  // delta from 0 is 100
        call(ZINTu64sUndelta, s, 0);
        testeq(one[0], 100);
    }

    // Start=0, ascending from zero
    {
        u64 vals[] = {0, 1, 2, 3, 10, 100};
        u64 orig[] = {0, 1, 2, 3, 10, 100};
        a$(u64, s, vals);
        call(ZINTu64sDelta, s, 0);
        // deltas: 0-0=0, 1-0=1, 2-1=1, 3-2=1, 10-3=7, 100-10=90
        testeq(vals[0], 0);
        testeq(vals[1], 1);
        testeq(vals[2], 1);
        testeq(vals[3], 1);
        testeq(vals[4], 7);
        testeq(vals[5], 90);
        call(ZINTu64sUndelta, s, 0);
        for (int i = 0; i < 6; i++) testeq(vals[i], orig[i]);
    }

    // Duplicates - delta=0 is valid
    {
        u64 vals[] = {5, 5, 5, 10, 10};
        u64 orig[] = {5, 5, 5, 10, 10};
        a$(u64, s, vals);
        call(ZINTu64sDelta, s, 0);
        testeq(vals[0], 5);   // 5-0
        testeq(vals[1], 0);   // 5-5
        testeq(vals[2], 0);   // 5-5
        testeq(vals[3], 5);   // 10-5
        testeq(vals[4], 0);   // 10-10
        call(ZINTu64sUndelta, s, 0);
        for (int i = 0; i < 5; i++) testeq(vals[i], orig[i]);
    }

    // Non-zero start
    {
        u64 vals[] = {100, 200, 300};
        a$(u64, s, vals);
        call(ZINTu64sDelta, s, 50);
        testeq(vals[0], 50);   // 100-50
        testeq(vals[1], 100);  // 200-100
        testeq(vals[2], 100);  // 300-200
        call(ZINTu64sUndelta, s, 50);
        testeq(vals[0], 100);
        testeq(vals[1], 200);
        testeq(vals[2], 300);
    }

    // Error: descending values
    {
        u64 vals[] = {100, 50, 200};
        a$(u64, s, vals);
        ok64 o = ZINTu64sDelta(s, 0);
        testeq(o, ZINTBAD);
    }

    // Error: first value less than start
    {
        u64 vals[] = {10, 20, 30};
        a$(u64, s, vals);
        ok64 o = ZINTu64sDelta(s, 100);
        testeq(o, ZINTBAD);
    }

    // Large values near u64 max
    {
        u64 vals[] = {UINT64_MAX - 100, UINT64_MAX - 50, UINT64_MAX};
        u64 orig[] = {UINT64_MAX - 100, UINT64_MAX - 50, UINT64_MAX};
        a$(u64, s, vals);
        call(ZINTu64sDelta, s, 0);
        call(ZINTu64sUndelta, s, 0);
        for (int i = 0; i < 3; i++) testeq(vals[i], orig[i]);
    }

    // Roundtrip with blocked encoding
    {
        u64 vals[] = {1000, 1001, 1005, 1100, 2000, 10000};
        a_pad(u64, orig, 8);
        u64bFeed(orig, ((u64cs){vals, vals + 6}));
        a_pad(u64, work, 8);
        u64bFeed(work, orig_datac);

        // Delta encode
        call(ZINTu64sDelta, work_data, 0);

        // Serialize to bytes
        a_pad(u8, buf, 64);
        a_dup(u64c, w, work_datac);
        call(ZINTu8sFeedBlocked, buf_idle, w);

        // Deserialize
        a_pad(u64, result, 8);
        call(ZINTu8sDrainBlocked, buf_datac, result_idle);

        // Undelta
        call(ZINTu64sUndelta, result_data, 0);

        // Compare
        $testeq(result_data, orig_data);
    }

    done;
}

ok64 ZINTTest() {
    sane(1);
    call(ZINTTest1);
    call(ZINTTest2);
    call(ZINTTest3);
    call(ZINTTest4);
    call(ZINTTestDelta);
    done;
}

TEST(ZINTTest);
