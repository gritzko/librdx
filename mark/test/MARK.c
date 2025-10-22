
#include "MARK.h"

#include <unistd.h>

#include "MARK.h"
#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/INT.h"
#include "abc/LEX.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "abc/TEST.h"

pro(MARKparsetest) {
    sane(YES);
    MARKstate state = {};
    aBpad(u8, into, 1024);
    aBpad(u64, divs, 32);
    aBpad(u64, blocks, 32);
    aBpad(u8cp, lines, 32);
    state.lineB = (u8cpbp)lines;
    state.divB = (u64bp)divs;
    state.pB = (u64bp)blocks;
    a_cstr(mark,
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
    done;
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
    u8cs QA[MARKANSIcases][2] = {
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
        state.lineB = (u8cpbp)lines;
        state.divB = (u64bp)divs;
        state.pB = (u64bp)blocks;
        $mv(state.text, QA[c][0]);
        $mv(state.fmt, u8bIdle(fmt));
        Bzero(fmt);

        call(MARKlexer, &state);
        call(MARKMARQ, &state);
        call(MARKANSI, u8bIdle(into), 8, &state);

        debugdivs(u64cbData(state.divB));
        a$str(hline, "---\n");
        $print(hline);
        $print(u8cbData(into));
        $print(hline);
        test($eq(QA[c][1], u8cbData(into)), TESTfail);
    }
    done;
}

pro(MARKHTMLtest) {
    sane(YES);
#define MARK1cases 8
    u8cs cases[MARK1cases][2] = {

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

    u64b divs = {};
    u64b blocks = {};
    u8cpb lines = {};
    Bu8 fmt = {};
    call(u64bAllocate, divs, 32);
    call(u8cpbAllocate, lines, 32);
    call(u8bAllocate, fmt, PAGESIZE);
    call(u64bAllocate, blocks, 32);
    ok64 o = OK;
    for (int i = 0; o == OK && i < MARK1cases; i++) {
        MARKstate state = {};
        aBpad(u8, into, 1024);
        Bu8cpreset(lines);
        Bu64reset(divs);
        Bu64reset(blocks);
        Bu8reset(fmt);
        Bzero(fmt);
        state.lineB = (u8cpbp)lines;
        state.divB = (u64bp)divs;
        state.pB = (u64bp)blocks;
        $mv(state.text, cases[i][0]);
        $mv(state.fmt, u8bIdle(fmt));

        callsafe(MARKlexer(&state), break);
        callsafe(MARKMARQ(&state), break);
        callsafe(MARKHTML(u8bIdle(into), &state), break);

        testsafe($eq(cases[i][1], u8cbData(into)), TESTfail, break);
    }
    u64bFree(blocks);
    u64bFree(divs);
    u8cpbFree(lines);
    u8bFree(fmt);
    return o;
};

pro(MARKtest) {
    sane(1);
    call(MARKparsetest);
    call(MARKANSItest);
    call(MARKHTMLtest);
    done;
}

TEST(MARKtest);
