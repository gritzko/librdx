#include "CT.h"
#include "GOT.h"
#include "PYT.h"
#include "JST.h"
#include "RST.h"
#include "JAT.h"
#include "CPPT.h"
#include "CST.h"
#include "HTMT.h"
#include "CSST.h"
#include "JSONT.h"
#include "SHT.h"
#include "RBT.h"
#include "HST.h"
#include "MLT.h"
#include "JLT.h"
#include "PHPT.h"
#include "AGDT.h"
#include "VERT.h"
#include "TOK.h"

#include "abc/TEST.h"

typedef struct {
    u8 tags[256];
    int count;
} TOK01ctx;

static ok64 TOK01cb(u8 tag, u8cs tok, void *ctx) {
    TOK01ctx *c = (TOK01ctx *)ctx;
    if (c->count < 256) {
        c->tags[c->count] = tag;
    }
    ++c->count;
    return OK;
}

typedef struct {
    const char *input;
    const char *tags;
} TOK01Case;

// Helper macro: run table-driven test for a given lexer
#define RUN_CASES(LEXER, TYPE, cases, ncases) do { \
    for (int i = 0; i < ncases; ++i) { \
        TOK01ctx ctx = {}; \
        TYPE##state st = { \
            .data = {(u8c *)cases[i].input, \
                     (u8c *)cases[i].input + strlen(cases[i].input)}, \
            .cb = TOK01cb, \
            .ctx = &ctx, \
        }; \
        ok64 o = LEXER(&st); \
        if (o != OK) { \
            fprintf(stderr, "FAIL case %d: '%s' error %s\n", i, \
                    cases[i].input, ok64str(o)); \
            fail(TESTFAIL); \
        } \
        int elen = strlen(cases[i].tags); \
        if (ctx.count != elen) { \
            fprintf(stderr, \
                    "FAIL case %d: '%s' count %d != expected %d\n", \
                    i, cases[i].input, ctx.count, elen); \
            fprintf(stderr, "  got tags: "); \
            for (int j = 0; j < ctx.count && j < 256; ++j) \
                fputc(ctx.tags[j], stderr); \
            fputc('\n', stderr); \
            fail(TESTFAIL); \
        } \
        for (int j = 0; j < elen; ++j) { \
            if (ctx.tags[j] != (u8)cases[i].tags[j]) { \
                fprintf(stderr, \
                        "FAIL case %d: '%s' tag[%d]='%c' expected '%c'\n", \
                        i, cases[i].input, j, ctx.tags[j], \
                        cases[i].tags[j]); \
                fprintf(stderr, "  got tags: "); \
                for (int k = 0; k < ctx.count && k < 256; ++k) \
                    fputc(ctx.tags[k], stderr); \
                fputc('\n', stderr); \
                fail(TESTFAIL); \
            } \
        } \
    } \
} while(0)

ok64 CTBasicTest() {
    sane(1);

    TOK01Case cases[] = {
        {"int x;", "RSSP"},
        {"if (x)", "RSPSP"},
        {"return 0;", "RSLP"},
        {"// comment\n", "DS"},
        {"/* block */", "D"},
        {"\"hello\"", "G"},
        {"'c'", "G"},
        {"42", "L"},
        {"3.14", "L"},
        {"0xff", "L"},
        {"0b101", "L"},
        {"#include <stdio.h>", "HSPSPSP"},
        {"int main(void) { return 0; }",
         "RSSPRPSPSRSLPSP"},
        {"a + b", "SSPSS"},
        {"  \t\n", "S"},
        {"{}", "PP"},
        {"x=1;", "SPLP"},
        {"while (i < n) {", "RSPSSPSSPSP"},
        {"struct foo {", "RSSSP"},
        {"#define X 1\n", "HSSSLS"},
        {"1e10", "L"},
        {"1.0f", "L"},
        {"100UL", "L"},
        {"0644", "L"},
        {"0x1p10", "L"},
        {".5f", "L"},
        {"1'000'000", "L"},
        {"0xDEAD'BEEF", "L"},
        {"1ULL", "L"},
        {"L\"wide\"", "G"},
        {"u8\"utf8\"", "G"},
        {"U'x'", "G"},
        {"'\\n'", "G"},
        {"'\\xff'", "G"},
        {"\"esc\\t\\n\"", "G"},
        {"#define FOO \\\nbar", "HSSSPSS"},
        {"// end", "D"},
        {"a->b", "SPS"},
        {"a++", "SP"},
        {"x <<= 1", "SSPSL"},
        {"...", "P"},
        {"a != b", "SSPSS"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(CTLexer, CT, cases, ncases);
    done;
}

ok64 CTKeywordTest() {
    sane(1);

    TOK01Case cases[] = {
        {"auto", "R"},
        {"break", "R"},
        {"const", "R"},
        {"continue", "R"},
        {"default", "R"},
        {"do", "R"},
        {"double", "R"},
        {"else", "R"},
        {"enum", "R"},
        {"extern", "R"},
        {"float", "R"},
        {"for", "R"},
        {"goto", "R"},
        {"if", "R"},
        {"inline", "R"},
        {"int", "R"},
        {"long", "R"},
        {"register", "R"},
        {"return", "R"},
        {"short", "R"},
        {"sizeof", "R"},
        {"static", "R"},
        {"struct", "R"},
        {"switch", "R"},
        {"typedef", "R"},
        {"union", "R"},
        {"unsigned", "R"},
        {"void", "R"},
        {"volatile", "R"},
        {"while", "R"},
        {"bool", "R"},
        {"true", "R"},
        {"false", "R"},
        {"nullptr", "R"},
        {"myvar", "S"},
        {"printf", "S"},
        {"foo_bar", "S"},
        {"_private", "S"},
        {"x1", "S"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(CTLexer, CT, cases, ncases);
    done;
}

ok64 TOKDispatchTest() {
    sane(1);

    const char *input = "int x;";
    TOK01ctx ctx = {};
    TOKstate state = {
        .data = {(u8c *)input, (u8c *)input + strlen(input)},
        .cb = TOK01cb,
        .ctx = &ctx,
    };
    u8csc ext = {(u8c *)"c", (u8c *)"c" + 1};
    call(TOKLexer, &state, ext);
    want(ctx.count == 4);
    want(ctx.tags[0] == 'R');  // int
    want(ctx.tags[1] == 'S');  // space
    want(ctx.tags[2] == 'S');  // x
    want(ctx.tags[3] == 'P');  // ;
    done;
}

ok64 GOTBasicTest() {
    sane(1);
    TOK01Case cases[] = {
        {"func main() {", "RSSPPSP"},
        {"var x int", "RSSSS"},
        {"// comment\n", "DS"},
        {"/* block */", "D"},
        {"\"hello\"", "G"},
        {"42", "L"},
        {"0xFF", "L"},
        {"0b101", "L"},
        {"0o77", "L"},
        {"3.14", "L"},
        {"1e10", "L"},
        {":=", "P"},
        {"<-", "P"},
        {"if else", "RSR"},
        {"package main", "RSS"},
        {"x := 5", "SSPSL"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(GOTLexer, GOT, cases, ncases);
    done;
}

ok64 PYTBasicTest() {
    sane(1);
    TOK01Case cases[] = {
        {"def foo():", "RSSPPP"},
        {"# comment\n", "DS"},
        {"\"hello\"", "G"},
        {"'hello'", "G"},
        {"42", "L"},
        {"0xFF", "L"},
        {"3.14", "L"},
        {"if else", "RSR"},
        {"import os", "RSS"},
        {"True False None", "RSRSR"},
        {"x = 5", "SSPSL"},
        {"@decorator", "H"},
        {"**=", "P"},
        {"//", "P"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(PYTLexer, PYT, cases, ncases);
    done;
}

ok64 JSTBasicTest() {
    sane(1);
    TOK01Case cases[] = {
        {"function foo() {", "RSSPPSP"},
        {"// comment\n", "DS"},
        {"/* block */", "D"},
        {"\"hello\"", "G"},
        {"'hello'", "G"},
        {"42", "L"},
        {"0xFF", "L"},
        {"3.14", "L"},
        {"const let var", "RSRSR"},
        {"===", "P"},
        {"!==", "P"},
        {"=>", "P"},
        {"x = 5", "SSPSL"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(JSTLexer, JST, cases, ncases);
    done;
}

ok64 RSTBasicTest() {
    sane(1);
    TOK01Case cases[] = {
        {"fn main() {", "RSSPPSP"},
        {"// comment\n", "DS"},
        {"/* block */", "D"},
        {"\"hello\"", "G"},
        {"42", "L"},
        {"0xFF", "L"},
        {"3.14", "L"},
        {"let mut", "RSR"},
        {"pub fn", "RSR"},
        {"::=", "PP"},
        {"x = 5", "SSPSL"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(RSTLexer, RST, cases, ncases);
    done;
}

ok64 JATBasicTest() {
    sane(1);
    TOK01Case cases[] = {
        {"public class Foo {", "RSRSSSP"},
        {"// comment\n", "DS"},
        {"/* block */", "D"},
        {"\"hello\"", "G"},
        {"'c'", "G"},
        {"42", "L"},
        {"3.14", "L"},
        {"int long", "RSR"},
        {"@Override", "H"},
        {"x = 5;", "SSPSLP"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(JATLexer, JAT, cases, ncases);
    done;
}

ok64 JSONTBasicTest() {
    sane(1);
    TOK01Case cases[] = {
        {"42", "L"},
        {"3.14", "L"},
        {"-1", "L"},
        {"\"hello\"", "G"},
        {"true", "R"},
        {"false", "R"},
        {"null", "R"},
        {"{}", "PP"},
        {"[]", "PP"},
        {":", "P"},
        {",", "P"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(JSONTLexer, JSONT, cases, ncases);
    done;
}

ok64 SHTBasicTest() {
    sane(1);
    TOK01Case cases[] = {
        {"# comment\n", "DS"},
        {"\"hello\"", "G"},
        {"'hello'", "G"},
        {"42", "L"},
        {"if then fi", "RSRSR"},
        {"$HOME", "S"},
        {"||", "P"},
        {"&&", "P"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(SHTLexer, SHT, cases, ncases);
    done;
}

ok64 HSTBasicTest() {
    sane(1);
    TOK01Case cases[] = {
        {"-- comment\n", "DS"},
        {"\"hello\"", "G"},
        {"42", "L"},
        {"3.14", "L"},
        {"module where", "RSR"},
        {"let in", "RSR"},
        {"::", "P"},
        {"->", "P"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(HSTLexer, HST, cases, ncases);
    done;
}

ok64 MLTBasicTest() {
    sane(1);
    TOK01Case cases[] = {
        {"(* comment *)", "D"},
        {"\"hello\"", "G"},
        {"42", "L"},
        {"3.14", "L"},
        {"let in", "RSR"},
        {"match with", "RSR"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(MLTLexer, MLT, cases, ncases);
    done;
}

ok64 JLTBasicTest() {
    sane(1);
    TOK01Case cases[] = {
        {"# comment\n", "DS"},
        {"\"hello\"", "G"},
        {"42", "L"},
        {"3.14", "L"},
        {"function end", "RSR"},
        {"if else", "RSR"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    RUN_CASES(JLTLexer, JLT, cases, ncases);
    done;
}

ok64 TOK01test() {
    sane(1);
    call(CTBasicTest);
    call(CTKeywordTest);
    call(TOKDispatchTest);
    call(GOTBasicTest);
    call(PYTBasicTest);
    call(JSTBasicTest);
    call(RSTBasicTest);
    call(JATBasicTest);
    call(JSONTBasicTest);
    call(SHTBasicTest);
    call(HSTBasicTest);
    call(MLTBasicTest);
    call(JLTBasicTest);
    done;
}

TEST(TOK01test);
