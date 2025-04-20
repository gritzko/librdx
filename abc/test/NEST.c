//
// Created by gritzko on 8 Dec 24.
//
#include "NEST.h"

#include "PRO.h"
#include "TEST.h"

ok64 NESTtest1() {
    sane(1);
    aBpad(u8, ct, 128);
    NESTreset(ct);
    static const ok64 mood = 0xc73ce8;
    a$strc(templ, "Hello $mood world!");
    call(NESTfeed, ct, templ);
    call(NESTsplice, ct, mood);
    a$strc(good, "beautiful");
    call($u8feed, NESTidle(ct), good);

    aBpad2(u8, res, 128);
    call(NESTrender, residle, ct);
    a$strc(correct, "Hello beautiful world!");
    $testeq(correct, resdata);

    done;
}

ok64 NESTtest2() {
    sane(1);
    aBpad(u8, ct, 128);
    NESTreset(ct);
    static const ok64 a = 0x25;
    a$strc(templ, "1. $a 2. ${a} 3. a");
    call(NESTfeed, ct, templ);
    call(NESTspliceall, ct, a);
    a$strc(good, "A");
    call($u8feed, NESTidle(ct), good);

    aBpad2(u8, res, 128);
    call(NESTrender, residle, ct);
    a$strc(correct, "1. A 2. A 3. a");
    $testeq(correct, resdata);

    done;
}

ok64 NESTtest() {
    sane(1);
    call(NESTtest1);
    call(NESTtest2);
    done;
}

TEST(NESTtest);
