#include "JSON.h"

#include "B.h"
#include "TEST.h"

typedef struct {
    const char *input;
    const char *expected;
} JSONCase;

// Compact roundtrip: parse → format with no indent → compare
ok64 JSONCompactTest() {
    sane(1);

    JSONCase cases[] = {
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
        {"\"\\\\\"", "\"\\\\\""},
        {"\"\"", "\"\""},
        {"[[[]]]", "[[[]]]"},
        {"{\"a\":[{\"b\":1},2]}", "{\"a\":[{\"b\":1},2]}"},
        {" [ 1 , 2 ] ", "[1,2]"},
        {"\t{\n\"k\"\t:\r1\n}", "{\"k\":1}"},
        {"{\"a\":[]}", "{\"a\":[]}"},
        {"-0", "-0"},
        {"0.5", "0.5"},
        {"-1.23e+4", "-1.23e+4"},
        {"1E10", "1E10"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    u8cs no_indent = {(u8c *)"", (u8c *)""};

    for (int i = 0; i < ncases; ++i) {
        a_pad(u8, out, 1 << 16);
        u8cs json = {(u8c *)cases[i].input,
                     (u8c *)cases[i].input + strlen(cases[i].input)};
        call(JSONFmt, out_idle, json, no_indent);

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

// Pretty-print: parse → format with indent → compare
ok64 JSONPrettyTest() {
    sane(1);

    JSONCase cases[] = {
        {"42", "42"},
        {"\"hi\"", "\"hi\""},
        {"true", "true"},
        {"{}", "{}"},
        {"[]", "[]"},
        {"{\"a\":1}", "{\n  \"a\": 1\n}"},
        {"[1,2]", "[\n  1,\n  2\n]"},
        {"{\"a\":1,\"b\":2}", "{\n  \"a\": 1,\n  \"b\": 2\n}"},
        {"{\"a\":{\"b\":1}}", "{\n  \"a\": {\n    \"b\": 1\n  }\n}"},
        {"[1,[2,3]]", "[\n  1,\n  [\n    2,\n    3\n  ]\n]"},
        {"{\"a\":[]}", "{\n  \"a\": []\n}"},
        {"{\"a\":{}}", "{\n  \"a\": {}\n}"},
    };
    int ncases = sizeof(cases) / sizeof(cases[0]);
    u8c indent_str[] = "  ";
    u8cs indent = {indent_str, indent_str + 2};

    for (int i = 0; i < ncases; ++i) {
        a_pad(u8, out, 1 << 16);
        u8cs json = {(u8c *)cases[i].input,
                     (u8c *)cases[i].input + strlen(cases[i].input)};
        call(JSONFmt, out_idle, json, indent);

        u8cs result = {out[1], out_idle[0]};
        u8cs expect = {(u8c *)cases[i].expected,
                       (u8c *)cases[i].expected + strlen(cases[i].expected)};

        if (!u8csEq(result, expect)) {
            fprintf(stderr, "FAIL pretty case %d: '%s'\n", i, cases[i].input);
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
    call(JSONCompactTest);
    call(JSONPrettyTest);
    done;
}

TEST(JSONtest);
