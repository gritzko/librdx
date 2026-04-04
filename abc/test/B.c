#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "01.h"
#include "BUF.h"
#include "INT.h"
#include "PRO.h"
#include "TEST.h"

ok64 fail_test() {
    sane(1);
    fail(BADARG);
    done;
}

ok64 Bmap_test() {
    sane(1);
    Bu8 buf = {};
    call(u8bMap, buf, 1024);
    Bat(buf, 100) = 100;
    testeq(Bat(buf, 100), 100);
    call(u8bUnMap, buf);
    done;
}

ok64 B$_test() {
    sane(1);
    aBpad(u8cp, slices, 8);
    u8cs hello = $u8str("Hello");
    u8cpbFeed2(slices, hello[0], hello[1]);
    done;
}

ok64 Bndx_test() {
    sane(YES);
    Bu64 buf = {};
    u64bAllocate(buf, 1024);
    for (u64 i = 0; i < 1000; ++i) {
        call(u64bFeed1, buf, i);
        sane(Blast(buf) == i);
    }
    u64bFree(buf);
    done;
}

ok64 Breserve_test() {
    sane(1);
    Bu8 buf = {};
    call(u8bAllocate, buf, 1024);
    for (int i = 0; i < (1 << 18); i++) {
        otry(u8bFeed2, buf, '1', '2');
        ofix(BNOROOM) call(u8bReserve, buf, 1024);
        ocry();
    }
    call(u8bFree, buf);
    done;
}

ok64 B$test() {
    sane(1);
    a$$pad(pad, 128, 8);
    $$call(u8sFeedCStr, pad, "one");
    $$call(utf8sFeed10, pad, 2);
    $$call(u8sFeedCStr, pad, "three");
    a$str(templ, "First $1, then $2, then $3!");
    a$str(correct, "First one, then 2, then three!");
    aBpad2(u8, res, 128);
    $$feedf(residle, templ, pad$data);
    $testeq(correct, resdata);
    done;
}

ok64 BBtest() {
    sane(1);
    aBpad(u8b, buff, 4);
    testeq(sizeof(Bat(buff, 0)), sizeof(Bvoid));
    done;
}

ok64 u8sPrintf_test() {
    sane(1);

    // basic formatting
    a_pad(u8, buf, 128);
    call(u8sPrintf, buf_idle, "hello %d", 42);
    a$str(expect, "hello 42");
    $testeq(expect, buf_datac);

    // append more
    call(u8sPrintf, buf_idle, " %s!", "world");
    a$str(expect2, "hello 42 world!");
    $testeq(expect2, buf_datac);

    // empty format
    u8p before = buf[2];
    call(u8sPrintf, buf_idle, "");
    testeq(buf[2], before);

    // SNOROOM on overflow
    a_pad(u8, tiny, 4);
    ok64 o = u8sPrintf(tiny_idle, "toolong");
    testeq(o, SNOROOM);

    // "abc" = 3 chars fits in 4 bytes (3 + null)
    a_pad(u8, fit, 4);
    o = u8sPrintf(fit_idle, "abc");
    testeq(o, OK);
    testeq(u8bDataLen(fit), 3u);

    // "abcd" = 4 chars does NOT fit in 4 bytes (needs 5)
    a_pad(u8, fit2, 4);
    o = u8sPrintf(fit2_idle, "abcd");
    testeq(o, SNOROOM);

    // u8gPrintf: write into a gauge
    a_pad(u8, gbuf, 64);
    u8gp gg = u8aOpen(gbuf);
    call(u8gPrintf, gg, "x=%d", 99);
    u8cs gleft = {};
    u8aClose(gbuf, gleft);
    a$str(gexp, "x=99");
    $testeq(gexp, gleft);

    // u8bPrintf: write into a buffer
    a_pad(u8, bbuf, 64);
    call(u8bPrintf, bbuf, "%s=%d", "val", 7);
    a$str(bexp, "val=7");
    $testeq(bexp, bbuf_datac);

    // u8bPrintf: append
    call(u8bPrintf, bbuf, "!");
    a$str(bexp2, "val=7!");
    $testeq(bexp2, bbuf_datac);

    done;
}

ok64 Btest() {
    sane(1);
    call(Bmap_test);
    call(B$_test);
    call(Bndx_test);
    call(Breserve_test);
    call(B$test);
    call(BBtest);
    call(u8sPrintf_test);
    done;
}

TEST(Btest)
