#include "ast/CSS.h"
#include "ast/BAST.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"
#include "json/BASON.h"

// Helper: check if text contains a C string
static b8 CSSContainsStr(u8cs text, const char *needle) {
    size_t nlen = strlen(needle);
    size_t tlen = (size_t)$len(text);
    if (nlen > tlen) return NO;
    u8cp end = text[0] + tlen - nlen + 1;
    u8cp p = text[0];
    while (p < end) {
        if (memcmp(p, needle, nlen) == 0) return YES;
        p++;
    }
    return NO;
}

// Helper: parse C source into BASON tree
static ok64 CSSBuildBAST(u8bp buf, u64bp idx, const char *src) {
    sane(buf != NULL);
    u8csc source = {(u8cp)src, (u8cp)src + strlen(src)};
    u8cs ext = $u8str(".c");
    return BASTParse(buf, idx, source, ext);
}

// ---- Test 1: Parse selector "fn" ----
ok64 CSStestParseKind() {
    sane(1);
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);

    a_cstr(sel, "fn");
    call(CSSParse, qbuf, qidx, sel);

    // Walk the query tree: expect A root -> T leaf with val='E'
    u8cs qdata = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};
    test(!$empty(qdata), FAILSANITY);

    aBpad(u64, stk, 64);
    call(BASONOpen, stk, qdata);
    u8 type = 0;
    u8cs key = {}, val = {};
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'A');
    call(BASONInto, stk, qdata, val);

    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'T');
    testeq((size_t)$len(val), (size_t)1);
    testeq(val[0][0], (u8)'E');

    done;
}

// ---- Test 2: Parse selector "fn.main" ----
ok64 CSStestParseName() {
    sane(1);
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);

    a_cstr(sel, "fn.main");
    call(CSSParse, qbuf, qidx, sel);

    u8cs qdata = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};
    test(!$empty(qdata), FAILSANITY);

    aBpad(u64, stk, 64);
    call(BASONOpen, stk, qdata);
    u8 type = 0;
    u8cs key = {}, val = {};
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'A');
    call(BASONInto, stk, qdata, val);

    // First child: T val='E' (fn)
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'T');
    testeq(val[0][0], (u8)'E');

    // Second child: F val="main"
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'F');
    testeq((size_t)$len(val), (size_t)4);
    testeq(0, memcmp(val[0], "main", 4));

    done;
}

// ---- Test 3: Parse ":has()" ----
ok64 CSStestParseHas() {
    sane(1);
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);

    a_cstr(sel, "fn:has(malloc)");
    call(CSSParse, qbuf, qidx, sel);

    u8cs qdata = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};
    aBpad(u64, stk, 64);
    call(BASONOpen, stk, qdata);
    u8 type = 0;
    u8cs key = {}, val = {};
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'A');
    call(BASONInto, stk, qdata, val);

    // T val='E' (fn)
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'T');
    testeq(val[0][0], (u8)'E');

    // E container (:has)
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'E');
    call(BASONInto, stk, qdata, val);

    // Inside :has: S val="malloc" (text match — "malloc" not in kind table)
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'S');
    testeq((size_t)$len(val), (size_t)6);
    testeq(0, memcmp(val[0], "malloc", 6));

    done;
}

// ---- Test 4: Parse ":not()" ----
ok64 CSStestParseNot() {
    sane(1);
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);

    a_cstr(sel, ":not(fn)");
    call(CSSParse, qbuf, qidx, sel);

    u8cs qdata = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};
    aBpad(u64, stk, 64);
    call(BASONOpen, stk, qdata);
    u8 type = 0;
    u8cs key = {}, val = {};
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'A');
    call(BASONInto, stk, qdata, val);

    // E container with key '!' (negation)
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'E');
    test(!$empty(key), FAILSANITY);
    testeq(key[0][0], (u8)'!');

    call(BASONInto, stk, qdata, val);

    // T val='E' inside :not()
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'T');
    testeq(val[0][0], (u8)'E');

    done;
}

// ---- Test 5: Parse combinators ----
ok64 CSStestParseCombinators() {
    sane(1);
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);

    a_cstr(sel, "cmt > fn");
    call(CSSParse, qbuf, qidx, sel);

    u8cs qdata = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};
    aBpad(u64, stk, 64);
    call(BASONOpen, stk, qdata);
    u8 type = 0;
    u8cs key = {}, val = {};
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'A');
    call(BASONInto, stk, qdata, val);

    // T val='D' (cmt)
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'T');
    testeq(val[0][0], (u8)'D');

    // P val='>' (child combinator)
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'P');
    testeq((size_t)$len(val), (size_t)1);
    testeq(val[0][0], (u8)'>');

    // T val='E' (fn)
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'T');
    testeq(val[0][0], (u8)'E');

    done;
}

// ---- Test 6: Parse line range ----
ok64 CSStestParseLine() {
    sane(1);
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);

    a_cstr(sel, "L10-20");
    call(CSSParse, qbuf, qidx, sel);

    u8cs qdata = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};
    aBpad(u64, stk, 64);
    call(BASONOpen, stk, qdata);
    u8 type = 0;
    u8cs key = {}, val = {};
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'A');
    call(BASONInto, stk, qdata, val);

    // N leaf with 8-byte val: start=10, end=20
    call(BASONDrain, stk, qdata, &type, key, val);
    testeq(type, 'N');
    testeq((size_t)$len(val), (size_t)8);
    u32 start = ((u32)val[0][0] << 24) | ((u32)val[0][1] << 16) |
                ((u32)val[0][2] << 8) | ((u32)val[0][3]);
    u32 end = ((u32)val[0][4] << 24) | ((u32)val[0][5] << 16) |
              ((u32)val[0][6] << 8) | ((u32)val[0][7]);
    testeq(start, (u32)10);
    testeq(end, (u32)20);

    done;
}

// ---- Test 7: CSSMatch with type selector ----
ok64 CSStestMatchType() {
    sane(1);

    // Build BASON from C source
    a_pad(u8, pad, 65536);
    u64 _idx[1024];
    u64b idx = {_idx, _idx, _idx, _idx + 1024};

    static const char src[] =
        "int foo(int x) { return x; }\n"
        "int bar(int y) { return y; }\n";

    call(CSSBuildBAST, pad, idx, src);
    u8cs bason_data = {u8bDataHead(pad), u8bIdleHead(pad)};
    test(!$empty(bason_data), FAILSANITY);

    // Parse selector "fn"
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);
    a_cstr(sel, "fn");
    call(CSSParse, qbuf, qidx, sel);
    u8cs query = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};

    // Match into BASON buffer
    aBpad(u8, fbuf, 65536);
    call(CSSMatch, fbuf, bason_data, query);
    u8cp fd0 = u8bDataHead(fbuf), fd1 = u8bIdleHead(fbuf);
    u8cs filtered = {fd0, fd1};
    test(!$empty(filtered), FAILSANITY);

    // Export to text and check
    a_pad(u8, out, 65536);
    call(CSSExport, out_idle, filtered);
    u8cs result = {u8bDataHead(out), u8bIdleHead(out)};
    test(!$empty(result), FAILSANITY);

    done;
}

// ---- Test 8: CSSMatch with line range ----
ok64 CSStestMatchLine() {
    sane(1);

    a_pad(u8, pad, 65536);
    u64 _idx[1024];
    u64b idx = {_idx, _idx, _idx, _idx + 1024};

    static const char src[] =
        "line1\n"
        "line2\n"
        "line3\n"
        "line4\n"
        "line5\n";

    u8csc source = {(u8cp)src, (u8cp)src + strlen(src)};
    u8cs ext = $u8str(".txt");
    call(BASTParse, pad, idx, source, ext);
    u8cs bason_data = {u8bDataHead(pad), u8bIdleHead(pad)};

    // Parse selector "L2-3"
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);
    a_cstr(sel, "L2-3");
    call(CSSParse, qbuf, qidx, sel);
    u8cs query = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};

    // Match into BASON buffer
    aBpad(u8, fbuf, 65536);
    call(CSSMatch, fbuf, bason_data, query);
    u8cp fd0 = u8bDataHead(fbuf), fd1 = u8bIdleHead(fbuf);
    u8cs filtered = {fd0, fd1};

    // Export to text
    a_pad(u8, out, 65536);
    call(CSSExport, out_idle, filtered);
    u8cs result = {u8bDataHead(out), u8bIdleHead(out)};
    // Should have lines 2 and 3
    test(!$empty(result), FAILSANITY);
    test(CSSContainsStr(result, "line2"), FAILSANITY);
    test(CSSContainsStr(result, "line3"), FAILSANITY);

    done;
}

// ---- Test 9: Parse error (unclosed paren) ----
ok64 CSStestParseError() {
    sane(1);
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);

    a_cstr(sel, "fn:has(");
    ok64 o = CSSParse(qbuf, qidx, sel);
    test(o != OK, FAILSANITY);

    done;
}

// ---- Test 10: CSSMatch with text selector ----
ok64 CSStestMatchText() {
    sane(1);

    a_pad(u8, pad, 65536);
    u64 _idx[1024];
    u64b idx = {_idx, _idx, _idx, _idx + 1024};

    static const char src[] =
        "int foo(int x) { return x; }\n"
        "// a comment here\n"
        "int bar(int y) { return y; }\n";

    call(CSSBuildBAST, pad, idx, src);
    u8cs bason_data = {u8bDataHead(pad), u8bIdleHead(pad)};

    // Parse text selector "comment"
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);
    a_cstr(sel, "comment");
    call(CSSParse, qbuf, qidx, sel);
    u8cs query = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};

    // Match into BASON buffer
    aBpad(u8, fbuf, 65536);
    call(CSSMatch, fbuf, bason_data, query);
    u8cp fd0 = u8bDataHead(fbuf), fd1 = u8bIdleHead(fbuf);
    u8cs filtered = {fd0, fd1};

    // Export to text
    a_pad(u8, out, 65536);
    call(CSSExport, out_idle, filtered);
    u8cs result = {u8bDataHead(out), u8bIdleHead(out)};
    // "comment" is not a known kind, so it becomes a text search (S)
    // Should find "comment" in the comment line
    test(!$empty(result), FAILSANITY);
    test(CSSContainsStr(result, "comment"), FAILSANITY);

    done;
}

// ---- Test 11: CSSMatch with name selector (fn.foo) ----
ok64 CSStestMatchName() {
    sane(1);

    a_pad(u8, pad, 65536);
    u64 _idx[1024];
    u64b idx = {_idx, _idx, _idx, _idx + 1024};

    static const char src[] =
        "int foo(int x) { return x; }\n"
        "int bar(int y) { return y; }\n";

    call(CSSBuildBAST, pad, idx, src);
    u8cs bason_data = {u8bDataHead(pad), u8bIdleHead(pad)};
    test(!$empty(bason_data), FAILSANITY);

    // Parse selector "fn.foo"
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);
    a_cstr(sel, "fn.foo");
    call(CSSParse, qbuf, qidx, sel);
    u8cs query = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};

    // Match into BASON buffer
    aBpad(u8, fbuf, 65536);
    call(CSSMatch, fbuf, bason_data, query);
    u8cp fd0 = u8bDataHead(fbuf), fd1 = u8bIdleHead(fbuf);
    u8cs filtered = {fd0, fd1};

    // Export to text
    a_pad(u8, out, 65536);
    call(CSSExport, out_idle, filtered);
    u8cs result = {u8bDataHead(out), u8bIdleHead(out)};
    // Should match only foo, not bar
    test(!$empty(result), FAILSANITY);
    test(CSSContainsStr(result, "foo"), FAILSANITY);
    test(!CSSContainsStr(result, "bar"), FAILSANITY);

    done;
}

// ---- Test 12: CSSMatch with comment type selector ----
ok64 CSStestMatchCmt() {
    sane(1);

    a_pad(u8, pad, 65536);
    u64 _idx[1024];
    u64b idx = {_idx, _idx, _idx, _idx + 1024};

    static const char src[] =
        "int foo(int x) { return x; }\n"
        "// a comment here\n"
        "int bar(int y) { return y; }\n";

    call(CSSBuildBAST, pad, idx, src);
    u8cs bason_data = {u8bDataHead(pad), u8bIdleHead(pad)};

    // Parse selector "cmt"
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);
    a_cstr(sel, "cmt");
    call(CSSParse, qbuf, qidx, sel);
    u8cs query = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};

    // Match into BASON buffer
    aBpad(u8, fbuf, 65536);
    call(CSSMatch, fbuf, bason_data, query);
    u8cp fd0 = u8bDataHead(fbuf), fd1 = u8bIdleHead(fbuf);
    u8cs filtered = {fd0, fd1};

    // Export to text
    a_pad(u8, out, 65536);
    call(CSSExport, out_idle, filtered);
    u8cs result = {u8bDataHead(out), u8bIdleHead(out)};
    test(!$empty(result), FAILSANITY);
    test(CSSContainsStr(result, "comment"), FAILSANITY);
    // Should not include function bodies
    test(!CSSContainsStr(result, "return"), FAILSANITY);

    done;
}

// ---- Main test aggregator ----

ok64 CSStest() {
    sane(1);
    call(CSStestParseKind);
    call(CSStestParseName);
    call(CSStestParseHas);
    call(CSStestParseNot);
    call(CSStestParseCombinators);
    call(CSStestParseLine);
    call(CSStestMatchType);
    call(CSStestMatchLine);
    call(CSStestParseError);
    call(CSStestMatchText);
    call(CSStestMatchName);
    call(CSStestMatchCmt);
    done;
}

TEST(CSStest);
