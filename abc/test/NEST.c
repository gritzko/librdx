//
// Created by gritzko on 8 Dec 24.
//
#include "NEST.h"

#include "PRO.h"
#include "TEST.h"

pro(NESTtest1) {
    sane(1);
    aBpad(u8, ct, 128);
    NESTreset(ct);
    con ok64 mood = 0xc73ce8;
    a_cstr(templ, "Hello $mood world!");
    call(NESTFeed, ct, templ);
    call(NESTSplice, ct, mood);
    a_cstr(good, "beautiful");
    call(u8sFeed, NESTidle(ct), good);

    aBpad2(u8, res, 128);
    call(NESTRender, residle, ct);
    a_cstr(correct, "Hello beautiful world!");
    $testeq(correct, resdata);

    done;
}

pro(NESTtest2) {
    sane(1);
    aBpad(u8, ct, 128);
    NESTreset(ct);
    con ok64 a = 0x25;
    a_cstr(templ, "1. $a 2. ${a} 3. a");
    call(NESTFeed, ct, templ);
    call(NESTSpliceAll, ct, a);
    a_cstr(good, "A");
    call(u8sFeed, NESTidle(ct), good);

    aBpad2(u8, res, 128);
    call(NESTRender, residle, ct);
    a_cstr(correct, "1. A 2. A 3. a");
    $testeq(correct, resdata);

    done;
}

pro(NESTtest) {
    sane(1);
    call(NESTtest1);
    call(NESTtest2);
    done;
}

TEST(NESTtest);
