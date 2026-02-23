#include "JSON.h"

#include "B.h"
#include "TEST.h"

typedef struct {
    const char *input;
    const char *expected;
} JSONRoundtripCase;

ok64 JSONRoundtripTest() {
    sane(1);

    JSONRoundtripCase cases[] = {
        {"42", "42"},
        {"\"hello\"", "\"hello\""},
        {"true", "true"},
        {"false", "false"},
        {"null", "null"},
        {"{}", "{}"},
        {"[]", "[]"},
        {"[1,2,3]", "[1,2,3]"},
        {"{\"a\":1}", "{\"a\":1}"},
        {"{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2}"},
        {"{\"a\":[1,2]}", "{\"a\":[1,2]}"},
        {"[{\"x\":1},{\"y\":2}]", "[{\"x\":1},{\"y\":2}]"},
        {"{\"a\":{\"b\":{\"c\":3}}}", "{\"a\":{\"b\":{\"c\":3}}}"},
        {"\"hello\\nworld\"", "\"hello\\nworld\""},
        {"\"tab\\there\"", "\"tab\\there\""},
        {"\"quote\\\"here\"", "\"quote\\\"here\""},
        {" { \"a\" : 1 , \"b\" : 2 } ", "{\"a\":1,\"b\":2}"},
        {"[true,false,null]", "[true,false,null]"},
        {"-42", "-42"},
        {"3.14", "3.14"},
        {"1e10", "1e10"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < ncases; ++i) {
        a_pad(u8, buf, 1 << 16);
        a_pad(u64, stk, 1 << 12);
        u8cs json = {(u8c *)cases[i].input,
                     (u8c *)cases[i].input + strlen(cases[i].input)};
        call(JSONParse, buf, stk, json);

        a_pad(u8, out, 1 << 16);
        u8cs bdata = {buf[1], buf[2]};
        call(JSONExport, out_idle, bdata);

        u8cs result = {out[1], out_idle[0]};
        u8cs expect = {(u8c *)cases[i].expected,
                       (u8c *)cases[i].expected + strlen(cases[i].expected)};

        if (!u8csEq(result, expect)) {
            fprintf(stderr, "FAIL case %d: '%s'\n", i, cases[i].input);
            fprintf(stderr, "  expected: '%.*s'\n", (int)u8csLen(expect),
                    expect[0]);
            fprintf(stderr, "  got:      '%.*s'\n", (int)u8csLen(result),
                    result[0]);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 JSONEscapeTest() {
    sane(1);

    typedef struct {
        const char *unescaped;
        const char *escaped;
    } EscCase;

    EscCase cases[] = {
        {"hello", "hello"},
        {"a\tb", "a\\tb"},
        {"a\nb", "a\\nb"},
        {"a\"b", "a\\\"b"},
        {"a\\b", "a\\\\b"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < ncases; ++i) {
        a_pad(u8, buf, 1024);
        u8cs from = {(u8c *)cases[i].unescaped,
                     (u8c *)cases[i].unescaped + strlen(cases[i].unescaped)};
        call(JSONEscapeAll, buf_idle, from);
        u8cs result = {buf[1], buf_idle[0]};
        u8cs expect = {(u8c *)cases[i].escaped,
                       (u8c *)cases[i].escaped + strlen(cases[i].escaped)};
        want(u8csEq(result, expect));
    }
    done;
}

ok64 JSONtest() {
    sane(1);
    call(JSONEscapeTest);
    call(JSONRoundtripTest);
    done;
}

TEST(JSONtest);
