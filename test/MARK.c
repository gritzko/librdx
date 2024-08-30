
#include "MARK.h"

#include <unistd.h>

#include "$.h"
#include "01.h"
#include "FILE.h"
#include "INT.h"
#include "LEX.h"
#include "MARK.h"
#include "PRO.h"
#include "TEST.h"

pro(MARKparsetest) {
    sane(YES);
    MARKstate state = {};
    aBpad(u8, into, 1024);
    aBpad(u64, divs, 32);
    aBpad(u8cp, lines, 32);
    state.lines = (u8cpB)lines;
    state.divs = (u64B)divs;
    a$strc(mark,
           "  # Header\n"
           " 1. list of\n"
           "\n"
           "    two lines\n"
           " 2. two entries\n"
           "...and some text\n");
    call(MARKlexer, &state);
    testeqv(0L, $len(state.text), "%li");
    testeqv(7L, Bdatalen(state.lines), "%li");
    a$str(line1, " 1. list of\n");
    u8c$ l1 = MARKline(&state, 1);
    $testeq(line1, l1);
    testeqv(6L, Bdatalen(state.divs), "%li");
    testeqv((u64)MARK_H1, Bat(state.divs, 0), "%lu");
    testeqv((u64)MARK_OLIST, Bat(state.divs, 1), "%lu");
    testeqv((u64)MARK_INDENT, Bat(state.divs, 3), "%lu");
    testeqv((u64)MARK_OLIST, Bat(state.divs, 4), "%lu");
    testeqv(0L, Bat(state.divs, 5), "%lu");
    testeqv(12L, $len(state.lines[0] + 1), "%lu");
    nedo(Bu64free(divs); Bu8cpfree(lines););
}

void debugdivs($cu64c $divs) {
    $for(u64c, p, $divs) {
        printf("|%c%c%c%c%c%c%c%c|\n", u64byte(*p, 0), u64byte(*p, 1),
               u64byte(*p, 2), u64byte(*p, 3), u64byte(*p, 4), u64byte(*p, 5),
               u64byte(*p, 6), u64byte(*p, 7));
    }
}

pro(MARKtest1) {
    sane(YES);
#define MARK1cases 8
    $u8c cases[MARK1cases][2] = {

        {$u8str("Good morning!\n"),
         $u8str("<p><span>Good morning!\n</span></p>\n")},

        {$u8str("Good morning!\nHave a good day!\n"),
         $u8str("<p><span>Good morning!\n</span><span>Have a good "
                "day!\n</span></p>\n")},

        {$u8str("Good morning!\n\nHave a good day!\n"),
         $u8str("<p><span>Good morning!\n</span></p>\n<p><span>Have a good "
                "day!\n</span></p>\n")},

        {$u8str("#   Good morning!\nHave a good day!\n"),
         $u8str("<h1><span>Good morning!\n</span></h1>\n<p><span>Have a good "
                "day!\n</span></p>\n")},

        {$u8str("#   Good morning!\n"
                " 1. Take\n"
                " 2. a list,\n"
                "buy things\n"),
         $u8str("<h1><span>Good morning!\n</span></h1>\n"
                "<ol><li><p><span>Take\n"
                "</span></p>\n"
                "</li><li><p><span>a list,\n"
                "</span></p>\n"
                "</li></ol>\n"
                "<p><span>buy things\n</span></p>\n")},

        {$u8str("Hello *world*!\n"),
         $u8str("<p><span>Hello </span><span class='"
                "strong'>*world*</span><span>!\n</span></p>\n")},

        {$u8str("#   Hello *world*!\n"),
         $u8str("<h1><span>Hello </span><span class='"
                "strong'>*world*</span><span>!\n</span></h1>\n")},

        {$u8str("Hello _beautiful world_!\n"),
         $u8str("<p><span>Hello </span><span class='emph'>_beautiful "
                "world_</span><span>!\n</span></p>\n")},

    };

    a$str(hline, "---\n");
    MARKstate state = {};
    aBpad(u64, divs, 32);
    aBpad(u8cp, lines, 32);
    state.lines = (u8cpB)lines;
    state.divs = (u64B)divs;
    for (int i = 0; i < MARK1cases; i++) {
        aBpad(u8, into, 1024);
        Bu8cpreset(lines);
        Bu64reset(divs);
        call(MARKlexer, &state);
        call(MARKHTML, Bu8idle(into), &state);

        $print(hline);
        debugdivs(Bu64cdata(state.divs));
        $print(Bu8cdata(into));
        $print(hline);
        test($eq(cases[i][1], Bu8cdata(into)), TESTfail);
    }
    nedo(Bu64free(divs); Bu8cpfree(lines););
};

pro(MARKtest) {
    call(MARKparsetest);
    call(MARKtest1);
    done;
}

TEST(MARKtest);
