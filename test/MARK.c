
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
    aBpad(u64, blocks, 32);
    aBpad(u8cp, lines, 32);
    state.lineB = (u8cpB)lines;
    state.divB = (u64B)divs;
    state.pB = (u64B)blocks;
    a$strc(mark,
           "  # Header\n"
           " 1. list of\n"
           "\n"
           "    two lines\n"
           " 2. two entries\n"
           "...and some text\n");
    $mv(state.text, mark);
    call(MARKlexer, &state);
    // testeqv(0L, $len(state.text), "%li");
    testeqv(7L, Bdatalen(state.lineB), "%li");
    a$str(line1, " 1. list of\n");
    u8c$ l1 = MARKline(&state, 1);
    $testeq(line1, l1);
    testeqv(7L, Bdatalen(state.divB), "%li");
    testeqv((u64)MARK_H1, Bat(state.divB, 0), "%lu");
    testeqv((u64)MARK_OLIST, Bat(state.divB, 1), "%lu");
    testeqv((u64)MARK_INDENT, Bat(state.divB, 3), "%lu");
    testeqv((u64)MARK_OLIST, Bat(state.divB, 4), "%lu");
    testeqv(0L, Bat(state.divB, 5), "%lu");
    testeqv(12L, $len(state.lineB[0] + 1), "%lu");
    nedo($print(state.text););
}

void debugdivs($cu64c $divs) {
    $for(u64c, p, $divs) {
        printf("|%c%c%c%c%c%c%c%c|\n", u64byte(*p, 0), u64byte(*p, 1),
               u64byte(*p, 2), u64byte(*p, 3), u64byte(*p, 4), u64byte(*p, 5),
               u64byte(*p, 6), u64byte(*p, 7));
    }
}

pro(MARKANSItest) {
    sane(1);
#define MARKANSIcases 2
#define CLR "[0m"
#define MRK "[90m"
#define BLD "[1m"
    $u8c QA[MARKANSIcases][2] = {
        {$u8str("some text\n"), $u8str("some\ntext\n")},
        {$u8str("some *bold* text\n"),
         $u8str("some\n" MRK BLD "*" CLR BLD "bold" CLR MRK BLD "*" CLR
                "\ntext\n")},
    };
    for (int c = 0; c < MARKANSIcases; ++c) {
        MARKstate state = {};
        aBpad(u8, lines, 16);
        aBpad(u64, divs, 16);
        aBpad(u8, fmt, 256);
        aBpad(u8, into, 256);
        aBpad(u64, blocks, 256);
        state.lineB = (u8cpB)lines;
        state.divB = (u64B)divs;
        state.pB = (u64B)blocks;
        $mv(state.text, QA[c][0]);
        $mv(state.fmt, Bu8idle(fmt));
        Bzero(fmt);

        call(MARKlexer, &state);
        call(MARKMARQ, &state);
        call(MARKANSI, Bu8idle(into), 8, &state);

        debugdivs(Bu64cdata(state.divB));
        a$str(hline, "---\n");
        $print(hline);
        $print(Bu8cdata(into));
        $print(hline);
        test($eq(QA[c][1], Bu8cdata(into)), TESTfail);
    }
    done;
}

pro(MARKHTMLtest) {
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
         $u8str("<p><span>Hello </span><span class='mark strong'>*</span><span "
                "class='strong'>world</span><span class='mark "
                "strong'>*</span><span>!\n"
                "</span></p>\n")},

        {$u8str("#   Hello *world*!\n"),
         $u8str("<h1><span>Hello </span><span class='mark "
                "strong'>*</span><span class='strong'>world</span><span "
                "class='mark strong'>*</span><span>!\n"
                "</span></h1>\n")},

        {$u8str("Hello _beautiful world_!\n"),
         $u8str("<p><span>Hello </span><span class='emph'>_beautiful "
                "world_</span><span>!\n</span></p>\n")},

    };

    Bu64 divs = {};
    Bu64 blocks = {};
    Bu8cp lines = {};
    Bu8 fmt = {};
    call(Bu64alloc, divs, 32);
    call(Bu8cpalloc, lines, 32);
    call(Bu8alloc, fmt, PAGESIZE);
    call(Bu64alloc, blocks, 32);
    for (int i = 0; i < MARK1cases; i++) {
        MARKstate state = {};
        aBpad(u8, into, 1024);
        Bu8cpreset(lines);
        Bu64reset(divs);
        Bu64reset(blocks);
        Bu8reset(fmt);
        Bzero(fmt);
        state.lineB = (u8cpB)lines;
        state.divB = (u64B)divs;
        state.pB = (u64B)blocks;
        $mv(state.text, cases[i][0]);
        $mv(state.fmt, Bu8idle(fmt));

        call(MARKlexer, &state);
        call(MARKMARQ, &state);
        call(MARKHTML, Bu8idle(into), &state);

        test($eq(cases[i][1], Bu8cdata(into)), TESTfail);
    }
    nedo(Bu64free(blocks); Bu64free(divs); Bu8cpfree(lines); Bu8free(fmt););
};

pro(MARKtest) {
    call(MARKparsetest);
    call(MARKANSItest);
    call(MARKHTMLtest);
    done;
}

TEST(MARKtest);
