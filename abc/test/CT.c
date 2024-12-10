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

pro(CTtest) {
    sane(1);
    call(CTtest1);
    done;
}

TEST(CTtest);
