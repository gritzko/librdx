#include "abc/NUM.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

ok64 NUMTest1() {
    sane(1);
    a_pad(u8, buf, 512);

    // Zero
    call(NUMu8sFeed, buf_idle, 0);
    a_cstr(zero, "zero");
    testeq(u8bDataLen(buf), u8csLen(zero));
    u8bReset(buf);

    // Single digits
    call(NUMu8sFeed, buf_idle, 1);
    a_cstr(one, "one");
    testeq(u8bDataLen(buf), u8csLen(one));
    u8bReset(buf);

    call(NUMu8sFeed, buf_idle, 9);
    a_cstr(nine, "nine");
    testeq(u8bDataLen(buf), u8csLen(nine));

    done;
}

ok64 NUMTest2() {
    sane(1);
    a_pad(u8, buf, 512);

    // Teens
    call(NUMu8sFeed, buf_idle, 13);
    a_cstr(thirteen, "thirteen");
    testeq(u8bDataLen(buf), u8csLen(thirteen));
    u8bReset(buf);

    call(NUMu8sFeed, buf_idle, 19);
    a_cstr(nineteen, "nineteen");
    testeq(u8bDataLen(buf), u8csLen(nineteen));
    u8bReset(buf);

    // Tens
    call(NUMu8sFeed, buf_idle, 42);
    a_cstr(fortytwo, "forty two");
    testeq(u8bDataLen(buf), u8csLen(fortytwo));
    u8bReset(buf);

    call(NUMu8sFeed, buf_idle, 99);
    a_cstr(ninetynine, "ninety nine");
    testeq(u8bDataLen(buf), u8csLen(ninetynine));

    done;
}

ok64 NUMTest3() {
    sane(1);
    a_pad(u8, buf, 512);

    // Hundreds
    call(NUMu8sFeed, buf_idle, 100);
    a_cstr(hundred, "one hundred");
    testeq(u8bDataLen(buf), u8csLen(hundred));
    u8bReset(buf);

    call(NUMu8sFeed, buf_idle, 123);
    a_cstr(oneTwoThree, "one hundred twenty three");
    testeq(u8bDataLen(buf), u8csLen(oneTwoThree));
    u8bReset(buf);

    // Thousands
    call(NUMu8sFeed, buf_idle, 1000);
    a_cstr(thousand, "one thousand");
    testeq(u8bDataLen(buf), u8csLen(thousand));
    u8bReset(buf);

    call(NUMu8sFeed, buf_idle, 12345);
    a_cstr(n12345, "twelve thousand three hundred forty five");
    testeq(u8bDataLen(buf), u8csLen(n12345));

    done;
}

ok64 NUMTest4() {
    sane(1);
    a_pad(u8, buf, 512);

    // Million
    call(NUMu8sFeed, buf_idle, 1000000);
    a_cstr(million, "one million");
    testeq(u8bDataLen(buf), u8csLen(million));
    u8bReset(buf);

    // Billion
    call(NUMu8sFeed, buf_idle, 1000000000ULL);
    a_cstr(billion, "one billion");
    testeq(u8bDataLen(buf), u8csLen(billion));
    u8bReset(buf);

    // Trillion
    call(NUMu8sFeed, buf_idle, 1000000000000ULL);
    a_cstr(trillion, "one trillion");
    testeq(u8bDataLen(buf), u8csLen(trillion));
    u8bReset(buf);

    // Complex large number
    call(NUMu8sFeed, buf_idle, 1234567890ULL);
    want(u8bDataLen(buf) > 50);

    done;
}

ok64 NUMTest5() {
    sane(1);
    a_pad(u8, buf, 512);

    // Very large numbers (quadrillion, quintillion)
    call(NUMu8sFeed, buf_idle, 1000000000000000ULL);
    a_cstr(quadrillion, "one quadrillion");
    testeq(u8bDataLen(buf), u8csLen(quadrillion));
    u8bReset(buf);

    call(NUMu8sFeed, buf_idle, 1000000000000000000ULL);
    a_cstr(quintillion, "one quintillion");
    testeq(u8bDataLen(buf), u8csLen(quintillion));
    u8bReset(buf);

    // Near UINT64_MAX
    call(NUMu8sFeed, buf_idle, UINT64_MAX);
    want(u8bDataLen(buf) > 100);

    done;
}

ok64 NUMTests() {
    sane(1);
    call(NUMTest1);
    call(NUMTest2);
    call(NUMTest3);
    call(NUMTest4);
    call(NUMTest5);
    done;
}

TEST(NUMTests);
