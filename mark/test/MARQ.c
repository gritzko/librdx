
#include "MARQ.h"

#include <unistd.h>

#include "abc/$.h"
#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/INT.h"
#include "abc/LEX.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

pro(MARQANSItest) {
    sane(1);
    MARQstate state = {};
#define MARQANSIcases 2
#define CLR "[0m"
#define MRK "[90m"
#define BLD "[1m"
    $u8c QA[MARQANSIcases][2] = {
        {$u8str("some text\n"), $u8str("some text\n")},
        {$u8str("some *bold* text\n"),
         $u8str("some " MRK BLD "*" CLR BLD "bold" CLR MRK BLD "*" CLR
                " text\n")},
    };
    for (int c = 0; c < MARQANSIcases; ++c) {
        aBpad(u8, into, 1024);
        aBpad(u8, pfmt, PAGESIZE);
        Bzero(pfmt);
        $mv(state.text, QA[c][0]);
        $mv(state.fmt, Bu8idle(pfmt));

        call(MARQlexer, &state);
        call(MARQANSI, Bu8idle(into), state.text, (u8c**)state.fmt);

        test($eq(QA[c][1], Bu8cdata(into)), TESTfail);
    }
    nedo($print(state.text););
}

pro(MARQHTMLtest) {
    sane(1);
    MARQstate state = {};
#define MARQHTMLcases 2
    $u8c QA[MARQHTMLcases][2] = {
        {$u8str("some text\n"), $u8str("<span>some text\n</span>")},
        {$u8str("some *bold* text\n"),
         $u8str("<span>some </span>"
                "<span class='mark strong'>*</span>"
                "<span class='strong'>bold</span>"
                "<span class='mark strong'>*</span>"
                "<span> text\n</span>")},
    };
    for (int c = 0; c < MARQHTMLcases; ++c) {
        aBpad(u8, into, 1024);
        aBpad(u8, pfmt, PAGESIZE);
        Bzero(pfmt);
        $mv(state.text, QA[c][0]);
        $mv(state.fmt, Bu8idle(pfmt));

        call(MARQlexer, &state);
        call(MARQHTML, Bu8idle(into), state.text, (u8c**)state.fmt);

        test($eq(QA[c][1], Bu8cdata(into)), TESTfail);
    }
    nedo($print(state.text););
}

pro(MARQtest) {
    sane(1);
    call(MARQANSItest);
    call(MARQHTMLtest);
    done;
}

TEST(MARQtest);
