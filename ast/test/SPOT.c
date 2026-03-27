#include "ast/SPOT.h"
#include "ast/BAST.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"
#include "json/BASON.h"

// Helper: parse C source into BASON tree
static ok64 SPOTBuildBAST(u8bp buf, u64bp idx, const char *src) {
    sane(buf != NULL);
    u8csc source = {(u8cp)src, (u8cp)src + strlen(src)};
    u8cs ext = $u8str(".c");
    return BASTParse(buf, idx, source, ext);
}

// Helper: init SPOT from C source strings
static ok64 SPOTSetup(SPOTstate *st,
                       u8bp nbuf, u64bp nidx,
                       u8bp hbuf, u64bp hidx,
                       const char *needle, const char *haystack) {
    sane(st != NULL);
    call(SPOTBuildBAST, hbuf, hidx, haystack);
    u8cs hay = {u8bDataHead(hbuf), u8bIdleHead(hbuf)};
    test(!$empty(hay), FAILSANITY);

    u8csc nsrc = {(u8cp)needle, (u8cp)needle + strlen(needle)};
    u8cs ext = $u8str(".c");
    call(SPOTInit, st, nbuf, nidx, nsrc, ext, hay);
    done;
}

// Helper: check if text contains a C string
static b8 SPOTContainsStr(u8cs text, const char *needle) {
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

// Helper: flatten BASON subtree starting at byte offset pos
static ok64 SPOTFlattenRec(u8s out, u64bp stk, u8csc data) {
    sane(1);
    u8 type = 0;
    u8cs key = {}, val = {};
    while (BASONDrain(stk, data, &type, key, val) == OK) {
        if (!BASONCollection(type)) {
            call(u8sFeed, out, val);
        } else {
            call(BASONInto, stk, data, val);
            call(SPOTFlattenRec, out, stk, data);
            call(BASONOuto, stk);
        }
    }
    done;
}

// Flatten the element at byte offset pos within data.
// Seeks to pos level-by-level, drains the element, flattens.
static ok64 SPOTFlattenAt(u8s out, u8csc data, u64 pos) {
    sane($ok(out));
    aBpad(u64, stk, 256);
    call(BASONOpen, stk, data);
    // Descend to the level containing pos
    u8 type = 0;
    u8cs key = {}, val = {};
    u8cp base = data[0];
    for (;;) {
        ok64 o = BASONDrain(stk, data, &type, key, val);
        if (o != OK) break;
        if (!BASONCollection(type)) continue;
        u64 vstart = (u64)(val[0] - base);
        u64 vend = (u64)(val[1] - base);
        if (pos >= vstart && pos < vend) {
            call(BASONInto, stk, data, val);
            continue;
        }
    }
    // Position cursor at pos and drain the target element
    *u64bLast(stk) = pos;
    ok64 o = BASONDrain(stk, data, &type, key, val);
    if (o != OK) done;
    if (!BASONCollection(type)) {
        call(u8sFeed, out, val);
    } else {
        call(BASONInto, stk, data, val);
        call(SPOTFlattenRec, out, stk, data);
        call(BASONOuto, stk);
    }
    done;
}

// ---- Test: error codes ----
ok64 SPOTtestCodes() {
    sane(1);
    test(0 == strcmp(ok64str(SPOTEND), "SPOTEND"), FAILSANITY);
    test(0 == strcmp(ok64str(SPOTBAD), "SPOTBAD"), FAILSANITY);
    done;
}

// ---- Table-driven match/no-match tests ----

typedef struct {
    const char *name;
    const char *needle;
    const char *haystack;
    ok64 expect;            // OK = match expected, SPOTEND = no match
    const char *must_have;  // if non-NULL, output text must contain this
    const char *must_lack;  // if non-NULL, output text must NOT contain this
} SPOTcase;

static const SPOTcase SPOT_CASES[] = {

    // --- Basic exact matching ---

    {
        "ExactReturn0",
        "return 0;",
        "int foo() { return 0; }\n",
        OK, "return", NULL
    },
    {
        "ExactReturn1",
        "return 1;",
        "int foo() { return 0; }\n",
        SPOTEND, NULL, NULL
    },
    {
        "NoMatch_goto",
        "goto x;",
        "int foo() { return 0; }\n",
        SPOTEND, NULL, NULL
    },

    // --- Lowercase placeholder: name binding ---

    {
        "PlaceholderName",
        "return x;",
        "int foo() { return counter; }\n",
        OK, "counter", NULL
    },

    // --- Lowercase placeholder consistency ---
    //   x = x + 1: x binds to same name on both occurrences

    {
        "ConsistencyMatch",
        "x = x + 1",
        "void f() { y = y + 1; }\n",
        OK, NULL, NULL
    },
    {
        "ConsistencyFail",
        "x = x + 1",
        "void f() { y = z + 1; }\n",
        SPOTEND, NULL, NULL
    },

    // --- Uppercase placeholder: subtree binding ---

    {
        "SubtreePlaceholder",
        "return X;",
        "int foo() { return a + b; }\n",
        OK, "return", NULL
    },

    // --- Skip comments on haystack side ---

    {
        "SkipComment",
        "a = b",
        "void f() { a /* comment */ = b; }\n",
        OK, NULL, NULL
    },

    // --- Gap handling: SKIP_SIBLINGS (2 spaces in needle) ---
    //   "int  x" parses to [T]"int" [S]"  " [V]"x"
    //   2 spaces → SKIP_SIBLINGS: scan same-level siblings

    {
        "SkipSiblings_intDecl",
        "int  x",
        "void f() { int foo, bar, baz; }\n",
        OK, "int", NULL
    },
    {
        "SkipSiblings_noType",
        "int  x",
        "void f() { char foo, bar; }\n",
        SPOTEND, NULL, NULL
    },
    {
        "SkipSiblings_return",
        "return  x",
        "int f() { int q = 1; return q; }\n",
        OK, "return", NULL
    },

    // --- Gap handling: SKIP_DFS (3+ spaces in needle) ---
    //   "int   x" parses to [T]"int" [S]"   " [V]"x"
    //   3 spaces → SKIP_DFS: depth-first search from current position
    //   DFS can find things inside nested containers

    {
        "SkipDFS_finds",
        "int   x",
        "void f() { int val; }\n",
        OK, "int", NULL
    },
    {
        "SkipDFS_noType",
        "int   x",
        "void f() { char arr[10]; }\n",
        SPOTEND, NULL, NULL
    },

    // --- Literal keyword matching ---

    {
        "KeywordWhile",
        "while (x)",
        "void f() { while (cond) { foo(); } }\n",
        OK, "while", NULL
    },
    {
        "KeywordIf",
        "if (x)",
        "void f() { if (ready) { go(); } }\n",
        OK, "if", NULL
    },

    // --- Mixed: literal + placeholder in expression ---

    {
        "AssignLiteral",
        "x = 0",
        "void f() { count = 0; }\n",
        OK, "count", NULL
    },

    // --- Two-letter names are NOT placeholders (only single-letter) ---

    {
        "TwoLetterLiteral",
        "return ab;",
        "int f() { return ab; }\n",
        OK, "ab", NULL
    },
    {
        "TwoLetterNoMatch",
        "return ab;",
        "int f() { return cd; }\n",
        SPOTEND, NULL, NULL
    },

    // --- Gap: 2-space SKIP_SIBLINGS with multiple declarations ---
    //   Needle skips sibling leaves to find a match

    {
        "SkipSibl_multiDecl",
        "int  x",
        "void f() { int aaa, bbb, ccc; }\n",
        OK, "int", NULL
    },

    // --- Gap: 3-space SKIP_DFS with init_declarator (nested container) ---
    //   DFS descends into child [A] containers

    {
        "SkipDFS_initDecl",
        "int   x",
        "void f() { int val = 42; }\n",
        OK, "val", NULL
    },

    // --- Gap: 4-space also DFS (3+ all map to DFS) ---

    {
        "SkipDFS_4space",
        "int    x",
        "void f() { int val = 42; }\n",
        OK, "val", NULL
    },

    // --- Gap: 1-space is EXACT (default, like normal code) ---
    //   Only matches the immediately next sibling

    {
        "ExactGap_1space",
        "return x;",
        "int f() { return val; }\n",
        OK, "val", NULL
    },

    // --- SKIP_SIBLINGS: return with extra tokens between ---

    {
        "SkipSibl_returnExpr",
        "return  x;",
        "int f() { return 42; }\n",
        OK, "42", NULL
    },

    // --- SKIP_DFS: finds target past nested blocks ---

    {
        "SkipDFS_pastBlock",
        "return   x;",
        "int f() { if (1) {} return 99; }\n",
        OK, "99", NULL
    },
};

#define SPOT_NCASES (sizeof(SPOT_CASES) / sizeof(SPOT_CASES[0]))

ok64 SPOTtestTable() {
    sane(1);

    for (size_t i = 0; i < SPOT_NCASES; i++) {
        const SPOTcase *tc = &SPOT_CASES[i];
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             tc->needle, tc->haystack);

        ok64 o = SPOTNext(&st);

        if (o != tc->expect) {
            fprintf(stderr, "FAIL [%s]: expected %s got %s\n",
                    tc->name, ok64str(tc->expect), ok64str(o));
            fail(FAILEQ);
        }

        if (o == OK && tc->must_have) {
            a_pad(u8, txt, 65536);
            call(SPOTFlattenAt, txt_idle, st.hay, st.match);
            u8cs flat = {u8bDataHead(txt), u8bIdleHead(txt)};
            if (!SPOTContainsStr(flat, tc->must_have)) {
                fprintf(stderr, "FAIL [%s]: output missing \"%s\"\n",
                        tc->name, tc->must_have);
                fail(FAILEQ);
            }
        }

        if (o == OK && tc->must_lack) {
            a_pad(u8, txt, 65536);
            call(SPOTFlattenAt, txt_idle, st.hay, st.match);
            u8cs flat = {u8bDataHead(txt), u8bIdleHead(txt)};
            if (SPOTContainsStr(flat, tc->must_lack)) {
                fprintf(stderr, "FAIL [%s]: output has unwanted \"%s\"\n",
                        tc->name, tc->must_lack);
                fail(FAILEQ);
            }
        }
    }
    done;
}

// ---- Test: reentrant multiple matches ----
ok64 SPOTtestReentrant() {
    sane(1);
    aBpad(u8, nbuf, 4096);
    aBpad(u64, nidx, 256);
    aBpad(u8, hbuf, 65536);
    aBpad(u64, hidx, 1024);

    SPOTstate st = {};
    call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
         "return x;",
         "int a() { return 1; }\n"
         "int b() { return 2; }\n"
         "int c() { return 3; }\n");

    int matches = 0;
    for (int i = 0; i < 10; i++) {
        ok64 o = SPOTNext(&st);
        if (o == OK)
            matches++;
        else if (o == SPOTEND)
            break;
        else
            fail(o);
    }
    testeq(matches, 3);
    done;
}

// ---- Test: gap handling specifics ----

// SKIP_SIBLINGS: 2-space gap in needle skips siblings at the same level
ok64 SPOTtestGapSkipSiblings() {
    sane(1);

    // Needle: "int  x" → [T]"int" [S]"  " [V]"x"
    // 2 spaces → SKIP_SIBLINGS: scan through sibling nodes for matching "x"
    // Haystack has int declaration with multiple declarators: foo, bar, x
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "int  x",
             "void f() { int alpha, beta, gamma; }\n");

        ok64 o = SPOTNext(&st);
        // "x" placeholder with SKIP_SIBLINGS should match any of the
        // declarator names (alpha, beta, gamma)
        testeq(o, OK);
    }

    // Needle "return  x": 2 spaces between return and x
    // [R]"return" [S]"  " [S]"x"
    // Should skip sibling nodes to find the value
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "return  x",
             "int f() { return 42; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, OK);
    }

    done;
}

// SKIP_DFS: 3-space gap in needle does depth-first search
ok64 SPOTtestGapSkipDFS() {
    sane(1);

    // Needle: "int   x" → [T]"int" [S]"   " [V]"x"
    // 3 spaces → SKIP_DFS: finds placeholder deeply nested
    // Haystack: int declaration with an init_declarator (nested [A])
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "int   x",
             "void f() { int val = 5; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, OK);

        // Verify match contains the declaration
        a_pad(u8, txt, 65536);
        call(SPOTFlattenAt, txt_idle, st.hay, st.match);
        u8cs flat = {u8bDataHead(txt), u8bIdleHead(txt)};
        test(SPOTContainsStr(flat, "int"), FAILSANITY);
        test(SPOTContainsStr(flat, "val"), FAILSANITY);
    }

    // DFS should fail when type doesn't match
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "int   x",
             "void f() { char *p = 0; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, SPOTEND);
    }

    done;
}

// EXACT: 0-1 space gap — match next non-ws sibling strictly
ok64 SPOTtestGapExact() {
    sane(1);

    // 1 space (normal): must match the immediately next sibling
    // "return x;" → exact matching, x binds to the next identifier
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "return x;",
             "int f() { return val; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, OK);

        a_pad(u8, txt, 65536);
        call(SPOTFlattenAt, txt_idle, st.hay, st.match);
        u8cs flat = {u8bDataHead(txt), u8bIdleHead(txt)};
        test(SPOTContainsStr(flat, "val"), FAILSANITY);
    }

    done;
}

// ---- Test: multi-statement needle with reentrant matching ----
// Needle spans multiple siblings at the same level, connected by gaps.
// Must find all matches by trying each starting position.
ok64 SPOTtestMultiStmt() {
    sane(1);

    // Needle: "int x;  x = A;" — declaration + 2-space gap + assignment
    // Haystack: "int a; int b; a = 1; b = 2;"
    // Expected: 2 matches — (x→a, A→1) and (x→b, A→2)
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "int x;  x = A;",
             "int a; int b; a = 1; b = 2;\n");

        int matches = 0;
        for (int i = 0; i < 10; i++) {
            ok64 o = SPOTNext(&st);
            if (o == OK)
                matches++;
            else if (o == SPOTEND)
                break;
            else
                fail(o);
        }
        if (matches != 2) {
            fprintf(stderr, "FAIL [MultiStmt]: expected 2 matches, got %d\n",
                    matches);
            fail(FAILEQ);
        }
    }

    // Single match: needle "int x;  x = A;" with only one matching pair
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "int x;  x = A;",
             "int a; a = 1;\n");

        ok64 o1 = SPOTNext(&st);
        testeq(o1, OK);

        ok64 o2 = SPOTNext(&st);
        testeq(o2, SPOTEND);
    }

    // No match: declaration exists but no matching assignment
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "int x;  x = A;",
             "int a; b = 1;\n");

        ok64 o = SPOTNext(&st);
        testeq(o, SPOTEND);
    }

    done;
}

// ---- Test: consistency across SKIP_SIBLINGS (greedy works) ----
// When a placeholder is already bound, the skip scan correctly rejects
// non-matching siblings and finds the right one. No backtracking needed.
ok64 SPOTtestConsistencySkip() {
    sane(1);

    // Needle: "x = A;  x = B;" — same x, SKIP between the two
    // Haystack: a = 1; b = 2; a = 3;
    // Greedy binds x→"a" at first stmt. Scanning for "x = B;" with x="a":
    // - "b = 2;" rejected (b≠a) → skip
    // - "a = 3;" accepted (a=a, B→"3") → match
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = A;  x = B;",
             "void f() { a = 1; b = 2; a = 3; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, OK);
    }

    // Needle: "x = A;  x = B;" but no second occurrence of same variable
    // Haystack: a = 1; b = 2; c = 3;
    // x→"a" from first, then need "a = ?" but no more "a =" → no match
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = A;  x = B;",
             "void f() { a = 1; b = 2; c = 3; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, SPOTEND);
    }

    done;
}

// ---- Test: swap pattern (greedy works) ----
// "x = y;  y = x;" — cross-reference, greedy scans past non-matching
ok64 SPOTtestSwapPattern() {
    sane(1);

    // Haystack has: a = b; c = d; b = a;
    // x→"a", y→"b" at first. SKIP: need "y = x;" = "b = a;" → found!
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = y;  y = x;",
             "void f() { a = b; c = d; b = a; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, OK);
    }

    // No swap exists: a = b; c = d; e = f;
    // No pair (x,y) has both "x=y" and "y=x"
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = y;  y = x;",
             "void f() { a = b; c = d; e = f; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, SPOTEND);
    }

    done;
}

// ---- Test: backtracking required ----
// Greedy picks wrong sibling at a skip point, later constraint fails.
// With backtracking, an alternative choice would succeed.
ok64 SPOTtestBacktrack() {
    sane(1);

    // Needle: "x = A;  y = B;  x = y;"
    //   Three segments connected by SKIP_SIBLINGS (2-space gaps):
    //   1. "x = A;" binds x and A
    //   2. "y = B;" binds y and B (greedily picks first match)
    //   3. "x = y;" requires x and y to be consistent
    //
    // Haystack: "void f() { a = 1; c = 3; b = 2; a = b; }"
    //   Siblings: a=1, c=3, b=2, a=b
    //
    // Greedy path:
    //   x→"a", A→"1" at "a = 1;"
    //   SKIP: y→"c", B→"3" at "c = 3;" (first match for "y = B;")
    //   SKIP: need "x = y;" = "a = c;" → not found → FAIL
    //
    // Backtracking path:
    //   Retry step 2: skip past "c = 3;", try "b = 2;"
    //   y→"b", B→"2"
    //   SKIP: need "x = y;" = "a = b;" → found! → SUCCESS
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = A;  y = B;  x = y;",
             "void f() { a = 1; c = 3; b = 2; a = b; }\n");

        ok64 o = SPOTNext(&st);
        if (o != OK) {
            fprintf(stderr, "FAIL [Backtrack]: expected OK, got %s "
                    "(backtracking not implemented)\n", ok64str(o));
            fail(FAILEQ);
        }
    }

    // Greedy-friendly case: needle "x = A;  y = B;  x = y;"
    // Haystack where greedy works (no backtracking needed):
    // "void f() { a = 1; b = 2; a = b; }"
    // Greedy: x→"a", then y→"b" (first scan hit), then "a = b;" found.
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = A;  y = B;  x = y;",
             "void f() { a = 1; b = 2; a = b; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, OK);
    }

    // No possible match at all
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = A;  y = B;  x = y;",
             "void f() { a = 1; b = 2; c = 3; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, SPOTEND);
    }

    done;
}

// ---- Test: backtracking with DFS skip ----
// Similar to above but using SKIP_DFS (3+ spaces) for the gap.
ok64 SPOTtestBacktrackDFS() {
    sane(1);

    // Needle: "x = A;   y = B;   x = y;"  (3-space gaps = DFS)
    // Haystack: nested blocks with the matching statements scattered
    // "void f() { a = 1; if(1) { c = 3; } b = 2; a = b; }"
    //
    // DFS finds a=1 (x→"a"), then DFS for "y=B;" greedily picks c=3 (y→"c"),
    // then DFS for "x=y;" = "a=c;" not found → needs backtracking to y→"b"
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = A;   y = B;   x = y;",
             "void f() { a = 1; if(1) { c = 3; } b = 2; a = b; }\n");

        ok64 o = SPOTNext(&st);
        if (o != OK) {
            fprintf(stderr, "FAIL [BacktrackDFS]: expected OK, got %s "
                    "(backtracking not implemented)\n", ok64str(o));
            fail(FAILEQ);
        }
    }

    done;
}

// ---- Test: backtracking with reentrant multiple matches ----
// Backtracking should work correctly across multiple SPOTNext calls.
ok64 SPOTtestBacktrackReentrant() {
    sane(1);

    // Needle: "x = A;  y = B;  x = y;"
    // Haystack has two valid match sets:
    //   a=1; c=3; b=2; a=b;  → (x→a, y→b) via backtracking
    //   d=4; e=5; d=e;       → (x→d, y→e) greedy works
    // But they're in different functions → 2 matches total
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = A;  y = B;  x = y;",
             "void f() { a = 1; c = 3; b = 2; a = b; }\n"
             "void g() { d = 4; e = 5; d = e; }\n");

        int matches = 0;
        for (int i = 0; i < 10; i++) {
            ok64 o = SPOTNext(&st);
            if (o == OK)
                matches++;
            else if (o == SPOTEND)
                break;
            else
                fail(o);
        }
        if (matches != 2) {
            fprintf(stderr, "FAIL [BacktrackReentrant]: expected 2 matches, "
                    "got %d\n", matches);
            fail(FAILEQ);
        }
    }

    done;
}

// ---- Test: submatch offsets ----
ok64 SPOTtestSubs() {
    sane(1);

    // Needle: "x = A;  y = B;  x = y;" — 3 gap-separated segments
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = A;  y = B;  x = y;",
             "void f() { a = 1; b = 2; a = b; }\n");

        ok64 o = SPOTNext(&st);
        if (o != OK) {
            fprintf(stderr, "FAIL [Subs3]: expected OK, got %s\n", ok64str(o));
            fail(FAILEQ);
        }
        if (st.nsubs != 3) {
            fprintf(stderr, "FAIL [Subs3]: nsubs=%d, expected 3\n", st.nsubs);
            fail(FAILEQ);
        }
        // All segment offsets should be non-zero (inside function body)
        test(st.subs[0] > 0, FAILSANITY);
        test(st.subs[1] > st.subs[0], FAILSANITY);
        test(st.subs[2] > st.subs[1], FAILSANITY);
    }

    // Single-segment needle: "return x;" — 1 segment (no gaps)
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "return x;",
             "int f() { return 42; }\n");

        ok64 o = SPOTNext(&st);
        if (o != OK) {
            fprintf(stderr, "FAIL [Subs1]: expected OK, got %s\n", ok64str(o));
            fail(FAILEQ);
        }
        if (st.nsubs != 1) {
            fprintf(stderr, "FAIL [Subs1]: nsubs=%d, expected 1\n", st.nsubs);
            fail(FAILEQ);
        }
        test(st.subs[0] > 0, FAILSANITY);
    }

    // DFS needle: "int   x" — gap is inside the declaration container,
    // so at the top (TU) level there is only 1 segment.
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "int   x",
             "void f() { int val = 5; }\n");

        ok64 o = SPOTNext(&st);
        if (o != OK) {
            fprintf(stderr, "FAIL [Subs_dfs]: expected OK, got %s\n", ok64str(o));
            fail(FAILEQ);
        }
        if (st.nsubs != 2) {
            fprintf(stderr, "FAIL [Subs_dfs]: nsubs=%d, expected 2\n", st.nsubs);
            fail(FAILEQ);
        }
    }

    // Two top-level segments: "x = A;  return x;"
    // Two statements separated by 2-space gap = 2 segments
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = A;  return x;",
             "int f() { a = 1; return a; }\n");

        ok64 o = SPOTNext(&st);
        if (o != OK) {
            fprintf(stderr, "FAIL [Subs2]: expected OK, got %s\n", ok64str(o));
            fail(FAILEQ);
        }
        if (st.nsubs != 2) {
            fprintf(stderr, "FAIL [Subs2]: nsubs=%d, expected 2\n", st.nsubs);
            fail(FAILEQ);
        }
    }

    done;
}

// ---- Test: match/alias logs ----
ok64 SPOTtestLogs() {
    sane(1);

    // Needle: "x = A;  y = B;  x = y;" — 3 gap-separated segments
    // Haystack: "void f() { a = 1; b = 2; a = b; }"
    // Bindings: x→a, A→1, y→b, B→2
    // 3 match log entries (one per segment), 4 alias log entries
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "x = A;  y = B;  x = y;",
             "void f() { a = 1; b = 2; a = b; }\n");

        // Set up log buffers
        aBpad(u64, mlog, 64);
        aBpad(u64, alog, 64);
        st.mlog[0] = mlog[0]; st.mlog[1] = mlog[1];
        st.mlog[2] = mlog[2]; st.mlog[3] = mlog[3];
        st.alog[0] = alog[0]; st.alog[1] = alog[1];
        st.alog[2] = alog[2]; st.alog[3] = alog[3];

        ok64 o = SPOTNext(&st);
        if (o != OK) {
            fprintf(stderr, "FAIL [Logs]: expected OK, got %s\n", ok64str(o));
            fail(FAILEQ);
        }

        // Check match log: 3 entries (one per gap-separated segment)
        size_t mlen = u64bDataLen(st.mlog);
        if (mlen != 3) {
            fprintf(stderr, "FAIL [Logs]: mlog entries=%zu, expected 3\n", mlen);
            fail(FAILEQ);
        }

        // Check alias log: 4 entries (x, A, y, B — each bound once)
        size_t alen = u64bDataLen(st.alog);
        if (alen != 4) {
            fprintf(stderr, "FAIL [Logs]: alog entries=%zu, expected 4\n", alen);
            fail(FAILEQ);
        }

        // Verify match log entries have valid offsets
        u64p mp = u64bDataHead(st.mlog);
        u64 hay_len = (u64)$len(st.hay);
        u64 ndl_len = (u64)$len(st.ndl);
        for (size_t i = 0; i < mlen; i++) {
            u32 hpos = SPOTLogHay(mp[i]);
            u16 npos = SPOTLogNdl(mp[i]);
            test(hpos > 0 && hpos < hay_len, FAILSANITY);
            test(npos < ndl_len, FAILSANITY);
        }

        // Verify alias_len is cumulative (non-decreasing)
        u16 prev_alen = 0;
        for (size_t i = 0; i < mlen; i++) {
            u16 al = SPOTLogExtra(mp[i]);
            test(al >= prev_alen, FAILSANITY);
            prev_alen = al;
        }

        // Verify alias log entries have valid offsets
        u64p ap = u64bDataHead(st.alog);
        for (size_t i = 0; i < alen; i++) {
            u32 hpos = SPOTLogHay(ap[i]);
            u16 npos = SPOTLogNdl(ap[i]);
            test(hpos > 0 && hpos < hay_len, FAILSANITY);
            test(npos < ndl_len, FAILSANITY);
        }
    }

    // Without logs: backward compatible (no crash)
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "return x;",
             "int f() { return 42; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, OK);
    }

    done;
}

// ---- Test: bracket-constrained matching (no cross-function matches) ----
ok64 SPOTtestBrackets() {
    sane(1);

    // Needle: "ok64 f(P){   ++*a;   }" — match entire function
    // Haystack: two functions, only second has ++*
    // Must NOT match across function boundaries
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "   ok64 f(P){   ++*a;   }",
             "ok64 func1(int x) { x += 1; return x; }\n"
             "ok64 func2(int *y) { ++*y; return 0; }\n");

        int matches = 0;
        u32 first_lo = 0, first_hi = 0;
        for (int i = 0; i < 10; i++) {
            ok64 o = SPOTNext(&st);
            if (o == OK) {
                if (matches == 0) {
                    first_lo = st.src_lo;
                    first_hi = st.src_hi;
                }
                matches++;
            } else if (o == SPOTEND) {
                break;
            } else {
                fail(o);
            }
        }
        if (matches != 1) {
            fprintf(stderr, "FAIL [Brackets]: expected 1 match, got %d\n",
                    matches);
            fail(FAILEQ);
        }
        // Match must be within func2, not spanning func1
        // func1 starts at byte 0; func2 starts somewhere after func1's '}'
        if (first_lo == 0) {
            fprintf(stderr, "FAIL [Brackets]: match starts at byte 0 "
                    "(spans from func1)\n");
            fail(FAILEQ);
        }
    }

    // No match: single function without ++*
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "   ok64 f(P){   ++*a;   }",
             "ok64 func1(int x) { x += 1; return x; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, SPOTEND);
    }

    // Closing bracket matches function scope, not inner scope
    {
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTSetup, &st, nbuf, nidx, hbuf, hidx,
             "   void f(P){   ++*a;   }",
             "void g(int *p) { while(1) { ++*p; } return; }\n");

        ok64 o = SPOTNext(&st);
        testeq(o, OK);
        // src_hi should be at the end of the function (past 'return;')
        // not at the while's '}'
        u8cp src = (u8cp)"void g(int *p) { while(1) { ++*p; } return; }\n";
        u32 func_end = (u32)strlen((char *)src) - 1; // before \n
        if (st.src_hi < func_end - 1) {
            fprintf(stderr, "FAIL [BracketScope]: src_hi=%u, expected >= %u "
                    "(matched inner } not function })\n",
                    st.src_hi, func_end - 1);
            fail(FAILEQ);
        }
    }

    done;
}

// ---- Main test aggregator ----

ok64 SPOTtest() {
    sane(1);
    call(SPOTtestCodes);
    call(SPOTtestTable);
    call(SPOTtestReentrant);
    call(SPOTtestGapExact);
    call(SPOTtestGapSkipSiblings);
    call(SPOTtestGapSkipDFS);
    call(SPOTtestMultiStmt);
    call(SPOTtestConsistencySkip);
    call(SPOTtestSwapPattern);
    // Backtracking tests — need backtracking in SPOTMatchChildren
    call(SPOTtestBacktrack);
    call(SPOTtestBacktrackDFS);
    call(SPOTtestBacktrackReentrant);
    call(SPOTtestSubs);
    call(SPOTtestLogs);
    call(SPOTtestBrackets);
    done;
}

TEST(SPOTtest);
