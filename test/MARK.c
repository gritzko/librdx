
#include "MARK.h"

#include <unistd.h>

#include "$.h"
#include "01.h"
#include "FILE.h"
#include "INT.h"
#include "LEX.h"
#include "PRO.h"
#include "TEST.h"

pro(MARKparsetest) {
    sane(YES);
    MARKstate state = {};
    aBpad(u8, into, 1024);
    a$strc(mark,
           "  # Header\n"
           " 1. list of\n"
           "    two lines\n"
           " 2. two entries\n"
           "...and some text\n");
    call(MARKstatealloc, &state, mark);
    $mv(state.text, mark);
    call(MARKlexer, &state);
    testeqv(0L, $len(state.text), "%li");
    testeqv(5L, Bdatalen(state.lines), "%li");
    testeqv(5L, Bdatalen(state.divs), "%li");
    testeqv((u64)MARK_H1, Bat(state.divs, 0), "%lu");
    testeqv((u64)MARK_OLIST, Bat(state.divs, 1), "%lu");
    testeqv((u64)MARK_INDENT, Bat(state.divs, 2), "%lu");
    testeqv((u64)MARK_OLIST, Bat(state.divs, 3), "%lu");
    testeqv(0L, Bat(state.divs, 4), "%lu");
    nedo(MARKstatefree(&state););
}

pro(MARKtest1) {
    sane(YES);
#define MARK1cases 7
    $u8c cases[MARK1cases][2] = {

        {$u8str("Good morning!\n"), $u8str("<p>Good morning!</p>\n")},

        {$u8str("Good morning!\nHave a good day!\n"),
         $u8str("<p>Good morning!\nHave a good day!</p>\n")},

        {$u8str("Good morning!\n\nHave a good day!\n"),
         $u8str("<p>Good morning!</p>\n\n<p>Have a good day!</p>\n")},

        {$u8str("#   Good morning!\nHave a good day!\n"),
         $u8str("<h1>Good morning!</h1>\n<p>Have a good day!</p>\n")},

        {$u8str("#   Good morning!\n"
                " 1. Take\n"
                " 2. a list,\n"
                "buy things\n"),
         $u8str("<h1>Good morning!</h1>\n"
                "<ol><li>Take</li>\n"
                "<li>a list,</li></ol>\n"
                "<p>buy things</p>\n")},

        {$u8str("Hello *world*!\n"), $u8str("Hello <b>world</b>!\n")},

        {$u8str("#   Hello *world*!\n"),
         $u8str("<h1>Hello <b>world</b>!</h1>\n")},

    };

    a$str(hline, "---\n");
    MARKstate state = {};
    aBpad(u8, into, 1024);
    call(MARKstatealloc, &state, cases[4][0]);
    for (int i = 0; i < MARK1cases; i++) {
        zero(state);
        $mv(state.text, cases[i][0]);
        call(MARKparse, &state);
        call(MARKhtml, Bu8idle(into), &state);

        $print(hline);
        $print(Bu8cdata(into));
        $print(hline);
        test($eq(cases[i][1], Bu8cdata(into)), TESTfail);

        MARKstatereset(&state);
    }
    nedo(MARKstatefree(&state););
};

pro(MARKtest) {
    call(MARKparsetest);
    call(MARKtest1);
    done;
}

TEST(MARKtest);

/*  realloc loop (maybe)

 u32 len = roundup($len(state.text), PAGESIZE);
do {
    u32 l = len / 4;
    Bu32reserve(state.lines, l);  // FIXME alloc/map
    Bu32reserve(state.linefmts, l);
    Bu64reserve(state.links, l / 4);
    l = len / 8;
    try(MARKlexer, &state);
    $print(state.text);
    $print(hline);
} while (is(LEXnoroom));
sure(OK);*/
