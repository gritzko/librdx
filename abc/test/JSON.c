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
        // unicode escapes
        {"\"\\u0041\"", "\"A\""},
        {"\"\\u00e9\"", "\"\xc3\xa9\""},
        // control chars re-escaped
        {"\"a\\u0000b\"", "\"a\\u0000b\""},
        {"\"\\b\\f\"", "\"\\b\\f\""},
        // slash passthrough
        {"\"a\\/b\"", "\"a/b\""},
        // backslash roundtrip
        {"\"\\\\\"", "\"\\\\\""},
        // empty string
        {"\"\"", "\"\""},
        // nested arrays
        {"[[[]]]", "[[[]]]"},
        // mixed nesting
        {"{\"a\":[{\"b\":1},2]}", "{\"a\":[{\"b\":1},2]}"},
        // deeply nested
        {"{\"a\":{\"b\":{\"c\":{\"d\":4}}}}", "{\"a\":{\"b\":{\"c\":{\"d\":4}}}}"},
        // whitespace variants
        {" [ 1 , 2 ] ", "[1,2]"},
        {"\t{\n\"k\"\t:\r1\n}", "{\"k\":1}"},
        // empty array in object
        {"{\"a\":[]}", "{\"a\":[]}"},
        // numbers
        {"-0", "-0"},
        {"0.5", "0.5"},
        {"-1.23e+4", "-1.23e+4"},
        {"1E10", "1E10"},
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

ok64 JSONIteratorTest() {
    sane(1);

    // Object with two keys
    {
        a_pad(u8, buf, 1 << 16);
        a_pad(u64, stk, 1 << 12);
        u8c json_str[] = "{\"a\":1,\"b\":\"hello\"}";
        u8cs json = {json_str, json_str + sizeof(json_str) - 1};
        call(JSONParse, buf, stk, json);

        a_pad(u64, rstk, 1 << 10);
        slit it = {};
        call(JSONOpen, &it, buf, rstk);

        ok64 o = JSONNext(&it);
        want(o == OK && it.lit == BASON_O);

        call(JSONInto, &it);

        o = JSONNext(&it);
        want(o == OK && it.lit == BASON_N);
        u8cs ka = {(u8c *)"a", (u8c *)"a" + 1};
        want(u8csEq(it.key, ka));
        u8cs va = {(u8c *)"1", (u8c *)"1" + 1};
        want(u8csEq(it.val, va));

        o = JSONNext(&it);
        want(o == OK && it.lit == BASON_S);
        u8cs kb = {(u8c *)"b", (u8c *)"b" + 1};
        want(u8csEq(it.key, kb));
        u8cs vb = {(u8c *)"hello", (u8c *)"hello" + 5};
        want(u8csEq(it.val, vb));

        o = JSONNext(&it);
        want(o == END);
        call(JSONOuto, &it);

        o = JSONNext(&it);
        want(o == END);
    }

    // Empty object
    {
        a_pad(u8, buf, 1 << 16);
        a_pad(u64, stk, 1 << 12);
        u8c json_str[] = "{}";
        u8cs json = {json_str, json_str + sizeof(json_str) - 1};
        call(JSONParse, buf, stk, json);

        a_pad(u64, rstk, 1 << 10);
        slit it = {};
        call(JSONOpen, &it, buf, rstk);

        ok64 o = JSONNext(&it);
        want(o == OK && it.lit == BASON_O);
        call(JSONInto, &it);
        o = JSONNext(&it);
        want(o == END);
        call(JSONOuto, &it);
    }

    // Nested: {"a":{"b":2}}
    {
        a_pad(u8, buf, 1 << 16);
        a_pad(u64, stk, 1 << 12);
        u8c json_str[] = "{\"a\":{\"b\":2}}";
        u8cs json = {json_str, json_str + sizeof(json_str) - 1};
        call(JSONParse, buf, stk, json);

        a_pad(u64, rstk, 1 << 10);
        slit it = {};
        call(JSONOpen, &it, buf, rstk);

        ok64 o = JSONNext(&it);
        want(o == OK && it.lit == BASON_O);
        call(JSONInto, &it);

        o = JSONNext(&it);
        want(o == OK && it.lit == BASON_O);
        u8cs ka = {(u8c *)"a", (u8c *)"a" + 1};
        want(u8csEq(it.key, ka));

        call(JSONInto, &it);
        o = JSONNext(&it);
        want(o == OK && it.lit == BASON_N);
        u8cs kb = {(u8c *)"b", (u8c *)"b" + 1};
        want(u8csEq(it.key, kb));
        u8cs vb = {(u8c *)"2", (u8c *)"2" + 1};
        want(u8csEq(it.val, vb));

        o = JSONNext(&it);
        want(o == END);
        call(JSONOuto, &it);

        o = JSONNext(&it);
        want(o == END);
        call(JSONOuto, &it);

        o = JSONNext(&it);
        want(o == END);
    }

    // Array [1,2,3]
    {
        a_pad(u8, buf, 1 << 16);
        a_pad(u64, stk, 1 << 12);
        u8c json_str[] = "[1,2,3]";
        u8cs json = {json_str, json_str + sizeof(json_str) - 1};
        call(JSONParse, buf, stk, json);

        a_pad(u64, rstk, 1 << 10);
        slit it = {};
        call(JSONOpen, &it, buf, rstk);

        ok64 o = JSONNext(&it);
        want(o == OK && it.lit == BASON_A);
        call(JSONInto, &it);

        o = JSONNext(&it);
        want(o == OK && it.lit == BASON_N);
        u8cs v1 = {(u8c *)"1", (u8c *)"1" + 1};
        want(u8csEq(it.val, v1));

        o = JSONNext(&it);
        want(o == OK && it.lit == BASON_N);
        u8cs v2 = {(u8c *)"2", (u8c *)"2" + 1};
        want(u8csEq(it.val, v2));

        o = JSONNext(&it);
        want(o == OK && it.lit == BASON_N);
        u8cs v3 = {(u8c *)"3", (u8c *)"3" + 1};
        want(u8csEq(it.val, v3));

        o = JSONNext(&it);
        want(o == END);
        call(JSONOuto, &it);
    }

    // Scalar root
    {
        a_pad(u8, buf, 1 << 16);
        a_pad(u64, stk, 1 << 12);
        u8c json_str[] = "42";
        u8cs json = {json_str, json_str + sizeof(json_str) - 1};
        call(JSONParse, buf, stk, json);

        a_pad(u64, rstk, 1 << 10);
        slit it = {};
        call(JSONOpen, &it, buf, rstk);

        ok64 o = JSONNext(&it);
        want(o == OK && it.lit == BASON_N);
        u8cs v = {(u8c *)"42", (u8c *)"42" + 2};
        want(u8csEq(it.val, v));

        o = JSONNext(&it);
        want(o == END);
    }

    done;
}

ok64 JSONtest() {
    sane(1);
    call(JSONEscapeTest);
    call(JSONRoundtripTest);
    call(JSONIteratorTest);
    done;
}

TEST(JSONtest);
