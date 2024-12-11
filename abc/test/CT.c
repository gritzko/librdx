//
// Created by gritzko on 8 Dec 24.
//
#include "CT.h"

#include "PRO.h"
#include "TEST.h"

pro(CTtest1) {
    sane(1);
    aBpad(u8, ct, 128);
    CTreset(ct);
    con ok64 mood = 0xa33cf1;
    a$strc(templ, "Hello $mood world!");
    call(CTfeed, ct, templ);
    call(CTsplice, ct, mood);
    a$strc(good, "beautiful");
    call($u8feed, CTidle(ct), good);

    aBpad2(u8, res, 128);
    call(CTrender, residle, ct);
    a$strc(correct, "Hello beautiful world!");
    $testeq(correct, resdata);

    done;
}

pro(CTtest2) {
    sane(1);
    aBpad(u8, ct, 128);
    CTreset(ct);
    con ok64 a = 0x25;
    a$strc(templ, "1. $a 2. ${a} 3. a");
    call(CTfeed, ct, templ);
    call(CTspliceall, ct, a);
    a$strc(good, "A");
    call($u8feed, CTidle(ct), good);

    aBpad2(u8, res, 128);
    call(CTrender, residle, ct);
    a$strc(correct, "1. A 2. A 3. a");
    $testeq(correct, resdata);

    done;
}

pro(CTtest) {
    sane(1);
    call(CTtest1);
    call(CTtest2);
    done;
}

TEST(CTtest);
