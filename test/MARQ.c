
#include "MARQ.h"

#include <unistd.h>

#include "$.h"
#include "01.h"
#include "FILE.h"
#include "INT.h"
#include "LEX.h"
#include "PRO.h"
#include "TEST.h"

pro(MARQANSItest) {
    sane(1);
    MARQstate state = {};
#define MARQANSIcases 2
    $u8c QA[MARQANSIcases][2] = {
        {$u8str("some text\n"), $u8str("some text\n")},
        {$u8str("some *bold* text\n"),
         $u8str("some [2m[1m*[0m[1mbold[0m[2m[1m*[0m text\n")},
    };
    for (int c = 0; c < MARQANSIcases; ++c) {
        aBpad(u8, into, 1024);
        aBpad(u8, pfmt, PAGESIZE);
        $mv(state.text, QA[c][0]);
        $mv(state.fmt, Bu8idle(pfmt));

        call(MARQlexer, &state);
        call(MARQANSI, Bu8idle(into), state.text, (u8c**)state.fmt);

        a$str(hline, "---\n");
        $print(hline);
        $print(Bu8cdata(into));
        $print(hline);

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
        memset(pfmt[0], 0, PAGESIZE);
        $mv(state.text, QA[c][0]);
        $mv(state.fmt, Bu8idle(pfmt));

        call(MARQlexer, &state);
        call(MARQHTML, Bu8idle(into), state.text, (u8c**)state.fmt);

        a$str(hline, "---\n");
        $print(hline);
        $print(Bu8cdata(into));
        $print(hline);

        test($eq(QA[c][1], Bu8cdata(into)), TESTfail);
    }
    nedo($print(state.text););
}

pro(MARQtest) {
    call(MARQANSItest);
    call(MARQHTMLtest);
    done;
}

TEST(MARQtest);
