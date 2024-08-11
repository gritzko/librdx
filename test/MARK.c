
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
    call(Bu64alloc, state.lines, 1024);
    call(Bu64alloc, state.linedivs, 1024);
    call(Bu64alloc, state.links, 512);
    a$str(mark,
          "  # Header\n"
          " 1. list of\n"
          "    two lines\n"
          " 2. two entries\n"
          "...and some text\n");
    $mv(state.text, mark);
    call(MARKlexer, &state);
    testeqv(0L, $len(state.text), "%li");
    testeqv(5L, Bdatalen(state.lines), "%li");
    testeqv(5L, Bdatalen(state.linedivs), "%li");
    testeqv((u64)MARK_DIV_H1, *Bat(state.linedivs, 0), "%lu");
    testeqv((u64)MARK_DIV_OLIST, *Bat(state.linedivs, 1), "%lu");
    testeqv((u64)MARK_DIV_PLAIN, *Bat(state.linedivs, 2), "%lu");
    testeqv((u64)MARK_DIV_OLIST, *Bat(state.linedivs, 3), "%lu");
    testeqv(0L, *Bat(state.linedivs, 4), "%lu");
    done;
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
    call(Bu64alloc, state.lines, 1024);
    call(Bu64alloc, state.linedivs, 1024);
    call(Bu64alloc, state.links, 512);
    for (int i = 0; i < MARK1cases; i++) {
        zero(state);
        $mv(state.text, cases[i][0]);
        call(MARKparse, &state);
        call(MARKhtml, Bu8idle(into), &state);

        $print(hline);
        $print(Bu8cdata(into));
        $print(hline);
        test($eq(cases[i][1], Bu8cdata(into)), TESTfail);

        Bu64reset(state.lines);
        Bu64reset(state.linedivs);
        Bu64reset(state.links);
    }
    done;
};

pro(MARKtest) {
    call(MARKparsetest);
    // call(MARKtest1);
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
