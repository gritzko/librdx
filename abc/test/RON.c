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
    done;
}

TEST(RONtest);
