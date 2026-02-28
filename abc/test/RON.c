#include "OK.h"
#include "POL.h"
#include "TEST.h"

ok64 RONTestFromTm() {
    sane(1);
    struct tm t = {
        .tm_year = 126,  // 2026
        .tm_mon = 0,     // January (0-based)
        .tm_mday = 9,
        .tm_hour = 12,
        .tm_min = 0,
        .tm_sec = 0,
    };
    ron60 r = 0;
    call(RONOfTime, &r, &t);
    // Verify round-trip
    struct tm t2 = {};
    call(RONToTime, r, &t2);
    testeq(t.tm_year, t2.tm_year);
    testeq(t.tm_mon, t2.tm_mon);
    testeq(t.tm_mday, t2.tm_mday);
    testeq(t.tm_hour, t2.tm_hour);
    testeq(t.tm_min, t2.tm_min);
    testeq(t.tm_sec, t2.tm_sec);
    done;
}

ok64 RONTestInvalid() {
    sane(1);
    struct tm t = {};
    // Invalid month (14 in RON = 13 in tm_mon after -1)
    ron60 bad_month = 0;
    bad_month |= ((u64)2 << (9 * 6));   // year tens
    bad_month |= ((u64)6 << (8 * 6));   // year ones
    bad_month |= ((u64)14 << (7 * 6));  // month 14 (invalid)
    bad_month |= ((u64)1 << (6 * 6));   // day
    want(RONToTime(bad_month, &t) == BADARG);

    // Invalid hour (25)
    ron60 bad_hour = 0;
    bad_hour |= ((u64)2 << (9 * 6));
    bad_hour |= ((u64)6 << (8 * 6));
    bad_hour |= ((u64)1 << (7 * 6));   // month 1
    bad_hour |= ((u64)1 << (6 * 6));   // day 1
    bad_hour |= ((u64)25 << (5 * 6));  // hour 25 (invalid)
    want(RONToTime(bad_hour, &t) == BADARG);

    // Invalid minute (60)
    ron60 bad_min = 0;
    bad_min |= ((u64)2 << (9 * 6));
    bad_min |= ((u64)6 << (8 * 6));
    bad_min |= ((u64)1 << (7 * 6));
    bad_min |= ((u64)1 << (6 * 6));
    bad_min |= ((u64)0 << (5 * 6));
    bad_min |= ((u64)60 << (4 * 6));  // min 60 (invalid)
    want(RONToTime(bad_min, &t) == BADARG);

    done;
}

ok64 RONTestRoundTrip() {
    sane(1);
    u8 buf[16];
    for (u64 i = 0; i <= 4096; ++i) {
        u8* p = buf;
        u8* end = buf + sizeof(buf);
        u8* into[2] = {p, end};
        call(RONutf8sFeed, into, i);
        u8c* from[2] = {buf, into[0]};
        ok64 back = 0;
        call(RONutf8sDrain, &back, from);
        testeq(i, back);
    }
    done;
}

fun ron60 _r60(const char* s) {
    u8c* p = (u8c*)s;
    u8c* sl[2] = {p, p + strlen(s)};
    ron60 r = 0;
    RONutf8sDrain(&r, sl);
    return r;
}

ok64 RONTestNormInc() {
    sane(1);
    con char* cases[][2] = {
        {"1000000000", "1000000001"},
        {"7000000000", "7000000001"},
        {"8000000000", "8000000010"},
        {"A000000000", "A000000010"},
        {"G000000000", "G000000100"},
        {"O000000000", "O000001000"},
        {"a000000000", "a000010000"},
        {"i000000000", "i000100000"},
        {"q000000000", "q001000000"},
        {"~000000000", "~010000000"},
        // trailing digits don't affect octant
        {"1a00000000", "1a00000001"},
        {"a100000000", "a100010000"},
    };
    u8 count = sizeof(cases) / sizeof(cases[0]);
    for (u8 i = 0; i < count; i++) {
        ron60 got = ron60NormInc(_r60(cases[i][0]));
        testeq(got, _r60(cases[i][1]));
    }
    done;
}

ok64 RONTestInc() {
    sane(1);
    con char* cases[][2] = {
        {"1", "1000000001"},
        {"7", "7000000001"},
        {"8", "800000001"},
        {"A", "A00000001"},
        {"G", "G0000001"},
        {"O", "O000001"},
        {"a", "a00001"},
        {"i", "i0001"},
        {"q", "q001"},
        {"~", "~01"},
        {"a1", "a10001"},
        {"11", "1100000001"},
    };
    u8 count = sizeof(cases) / sizeof(cases[0]);
    for (u8 i = 0; i < count; i++) {
        ron60 got = ron60Inc(_r60(cases[i][0]));
        testeq(got, _r60(cases[i][1]));
    }
    done;
}

ok64 RONTestNormInk() {
    sane(1);
    con char* cases[][2] = {
        {"1000000000", "2000000000"},
        {"7000000000", "8000000000"},
        {"8000000000", "8100000000"},
        {"A000000000", "A100000000"},
        {"G000000000", "G010000000"},
        {"O000000000", "O001000000"},
        {"a000000000", "a000100000"},
        {"i000000000", "i000010000"},
        {"q000000000", "q000001000"},
        {"~000000000", "~000000100"},
        // trailing digits don't affect octant
        {"1a00000000", "2a00000000"},
        {"a100000000", "a100100000"},
    };
    u8 count = sizeof(cases) / sizeof(cases[0]);
    for (u8 i = 0; i < count; i++) {
        ron60 got = ron60NormInk(_r60(cases[i][0]));
        testeq(got, _r60(cases[i][1]));
    }
    done;
}

ok64 RONTestInk() {
    sane(1);
    con char* cases[][2] = {
        {"1", "2"},
        {"7", "8"},
        {"8", "81"},
        {"A", "A1"},
        {"G", "G01"},
        {"O", "O001"},
        {"a", "a0001"},
        {"i", "i00001"},
        {"q", "q000001"},
        {"~", "~0000001"},
        {"a1", "a1001"},
        {"11", "21"},
    };
    u8 count = sizeof(cases) / sizeof(cases[0]);
    for (u8 i = 0; i < count; i++) {
        ron60 got = ron60Ink(_r60(cases[i][0]));
        testeq(got, _r60(cases[i][1]));
    }
    done;
}

ok64 RONTestNowMonotone() {
    sane(1);
    con int N = 1000;
    ron60 ts[N];
    ts[0] = RONNow();
    for (int i = 1; i < N; i++) {
        ts[i] = RONNow();
    }
    for (int i = 1; i < N; i++) {
        test(ts[i] >= ts[i - 1], FAIL);
    }
    test(ts[N - 1] > ts[0], FAIL);
    done;
}

ok64 RONTestFeedPad() {
    sane(1);
    con char* cases[][3] = {
        {"0", "1", "0"},
        {"0", "2", "00"},
        {"0", "3", "000"},
        {"1", "2", "01"},
        {"~", "1", "~"},
        {"10", "2", "10"},
        {"~~", "2", "~~"},
        {"100", "3", "100"},
    };
    u8 count = sizeof(cases) / sizeof(cases[0]);
    for (u8 i = 0; i < count; i++) {
        ok64 val = _r60(cases[i][0]);
        u8 width = (u8)atoi(cases[i][1]);
        const char* expected = cases[i][2];
        u8 buf[16] = {};
        u8s into = {buf, buf + sizeof(buf)};
        call(RONu8sFeedPad, into, val, width);
        u8c* exp_s = (u8c*)expected;
        u8c* exp_sl[2] = {exp_s, exp_s + strlen(expected)};
        u8c* got_sl[2] = {buf, buf + width};
        want($cmp(got_sl, exp_sl) == 0);
    }
    // overflow: val=64 ("10"), width=1 should fail
    u8 buf2[16];
    u8s into2 = {buf2, buf2 + sizeof(buf2)};
    want(RONu8sFeedPad(into2, 64, 1) != OK);
    done;
}

ok64 RONTestSpliceBase() {
    sane(1);
    ok64 base = 0;
    u8 width = 0;
    // prob=1000, n=100: need=200000, 64^3=262144 -> width >= 3
    call(RONSpliceBase, &base, &width, 12345, 1000, 100);
    want(width >= 3);
    u64 space = 1;
    for (u8 i = 0; i < width; i++) space *= 64;
    want(base + 100 <= space);
    // prob=64, n=1: need=128, 64^2=4096 -> width >= 2
    call(RONSpliceBase, &base, &width, 99999, 64, 1);
    want(width >= 2);
    // different rand -> different base
    ok64 base1 = 0, base2 = 0;
    u8 w1 = 0, w2 = 0;
    call(RONSpliceBase, &base1, &w1, 111, 1000, 10);
    call(RONSpliceBase, &base2, &w2, 999, 1000, 10);
    want(base1 != base2);
    // n=0 -> error
    want(RONSpliceBase(&base, &width, 0, 1000, 0) != OK);
    // prob=0 -> error
    want(RONSpliceBase(&base, &width, 0, 0, 10) != OK);
    done;
}

ok64 RONTestSpliceKeyOrder() {
    sane(1);
    con ok64 n = 50;
    ok64 base = 0;
    u8 width = 0;
    call(RONSpliceBase, &base, &width, 42, 1000, n);
    u8 keys[50][12] = {};
    for (ok64 i = 0; i < n; i++) {
        u8s into = {keys[i], keys[i] + sizeof(keys[i])};
        call(RONu8sFeedPad, into, base + i, width);
    }
    for (ok64 i = 0; i + 1 < n; i++) {
        u8c* a[2] = {keys[i], keys[i] + width};
        u8c* b[2] = {keys[i + 1], keys[i + 1] + width};
        want($cmp(a, b) < 0);
    }
    done;
}

ok64 RONTestSpliceIsolation() {
    sane(1);
    con ok64 n = 10;
    con u64 prob = 1000;
    con int trials = 10000;
    ok64 bases[trials];
    u8 width = 0;
    for (int i = 0; i < trials; i++) {
        // simple LCG for deterministic pseudo-random
        u64 rand = (u64)i * 6364136223846793005UL + 1442695040888963407UL;
        call(RONSpliceBase, &bases[i], &width, rand, prob, n);
    }
    int overlaps = 0;
    // check first 1000 pairs for overlaps
    for (int i = 0; i < 1000; i++) {
        for (int j = i + 1; j < 1000; j++) {
            ok64 lo1 = bases[i], hi1 = bases[i] + n;
            ok64 lo2 = bases[j], hi2 = bases[j] + n;
            if (lo1 < hi2 && lo2 < hi1) overlaps++;
        }
    }
    // expect overlap rate < 2/prob ~ 0.2% of pairs
    // 1000*999/2 = 499500 pairs, 2/prob = 0.002, so expect < ~999 overlaps
    want(overlaps < (int)(2 * 499500 / prob));
    done;
}

ok64 RONtest() {
    sane(1);
    call(RONTestFromTm);
    call(RONTestInvalid);
    call(RONTestRoundTrip);
    call(RONTestNormInc);
    call(RONTestInc);
    call(RONTestNormInk);
    call(RONTestInk);
    call(RONTestNowMonotone);
    call(RONTestFeedPad);
    call(RONTestSpliceBase);
    call(RONTestSpliceKeyOrder);
    call(RONTestSpliceIsolation);
    done;
}

TEST(RONtest);
