#include "ast/SPOT.h"
#include "ast/BAST.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"
#include "json/BASON.h"

// Helper: parse C source into BASON tree
static ok64 SPOTreBuildBAST(u8bp buf, u64bp idx, const char *src) {
    sane(buf != NULL);
    u8csc source = {(u8cp)src, (u8cp)src + strlen(src)};
    u8cs ext = $u8str(".c");
    return BASTParse(buf, idx, source, ext);
}

// Helper: init SPOT from C source strings
static ok64 SPOTreSetup(SPOTstate *st,
                         u8bp nbuf, u64bp nidx,
                         u8bp hbuf, u64bp hidx,
                         const char *needle, const char *haystack) {
    sane(st != NULL);
    call(SPOTreBuildBAST, hbuf, hidx, haystack);
    u8cs hay = {u8bDataHead(hbuf), u8bIdleHead(hbuf)};
    test(!$empty(hay), FAILSANITY);

    u8csc nsrc = {(u8cp)needle, (u8cp)needle + strlen(needle)};
    u8cs ext = $u8str(".c");
    call(SPOTInit, st, nbuf, nidx, nsrc, ext, hay);
    done;
}

// ---- Table-driven replacement test cases ----
//
// Each case: (needle, replace, haystack, expected, nmatches)
//   needle:   SPOT pattern (single-letter vars = placeholders)
//   replace:  replacement template (same placeholders)
//   haystack: input C source
//   expected: expected output after all replacements
//   nmatches: expected number of SPOTNext() hits
//
// Placeholder conventions:
//   lowercase (x,y,z..): bind leaf name, must be consistent
//   uppercase (A,B,X..): bind any subtree

typedef struct {
    const char *name;
    const char *needle;
    const char *replace;
    const char *haystack;
    const char *expected;
    int         nmatches;
} SPOTreCase;

static const SPOTreCase SPOT_RE_CASES[] = {

    // ================================================================
    //  1-10: Return statement rewrites
    // ================================================================

    {   // 1. wrap simple return value
        "WrapReturn",
        "return x;",
        "return check(x);",
        "int f() { return val; }\n",
        "int f() { return check(val); }\n",
        1
    },
    {   // 2. wrap return across multiple functions
        "WrapReturnMulti",
        "return x;",
        "return check(x);",
        "int f() { return a; }\nint g() { return b; }\n",
        "int f() { return check(a); }\nint g() { return check(b); }\n",
        2
    },
    {   // 3. wrap return across three functions
        "WrapReturn3",
        "return x;",
        "return (x);",
        "int a() { return 1; }\n"
        "int b() { return 2; }\n"
        "int c() { return 3; }\n",
        "int a() { return (1); }\n"
        "int b() { return (2); }\n"
        "int c() { return (3); }\n",
        3
    },
    {   // 4. uppercase placeholder captures subtree in return
        "ReturnSubtree",
        "return X;",
        "return wrap(X);",
        "int f() { return a + b * c; }\n",
        "int f() { return wrap(a + b * c); }\n",
        1
    },
    {   // 5. return deeply nested
        "ReturnDeep",
        "return x;",
        "return safe(x);",
        "void f() { if (a) { if (b) { return val; } } }\n",
        "void f() { if (a) { if (b) { return safe(val); } } }\n",
        1
    },
    {   // 6. return sum of two vars
        "ReturnSum",
        "return x + y;",
        "return add(x, y);",
        "int f() { return a + b; }\n",
        "int f() { return add(a, b); }\n",
        1
    },
    {   // 7. no match for return in empty body
        "ReturnNoMatch",
        "return x;",
        "return wrap(x);",
        "void f() { }\n",
        "void f() { }\n",
        0
    },
    {   // 8. no match for goto (not present)
        "GotoNoMatch",
        "goto x;",
        "JUMP(x);",
        "void f() { return 0; }\n",
        "void f() { return 0; }\n",
        0
    },
    {   // 9. goto rename
        "GotoRename",
        "goto x;",
        "goto done;",
        "void f() { goto cleanup; }\n",
        "void f() { goto done; }\n",
        1
    },
    {   // 10. break to return
        "BreakToReturn",
        "break;",
        "return;",
        "void f() { while(1) { break; } }\n",
        "void f() { while(1) { return; } }\n",
        1
    },

    // ================================================================
    //  11-20: Assignment rewrites
    // ================================================================

    {   // 11. replace literal zero with NULL
        "ZeroToNull",
        "x = 0;",
        "x = NULL;",
        "void f() { ptr = 0; }\n",
        "void f() { ptr = NULL; }\n",
        1
    },
    {   // 12. zero-init multiple vars
        "ZeroToNullMulti",
        "x = 0;",
        "x = NULL;",
        "void f() { p = 0; q = 0; }\n",
        "void f() { p = NULL; q = NULL; }\n",
        2
    },
    {   // 13. increment idiom: x = x + 1 → x++
        "IncrIdiom",
        "x = x + 1;",
        "x++;",
        "void f() { n = n + 1; }\n",
        "void f() { n++; }\n",
        1
    },
    {   // 14. increment idiom across functions
        "IncrIdiomMulti",
        "x = x + 1;",
        "x++;",
        "void f() { i = i + 1; }\nvoid g() { j = j + 1; }\n",
        "void f() { i++; }\nvoid g() { j++; }\n",
        2
    },
    {   // 15. decrement idiom: x = x - 1 → x--
        "DecrIdiom",
        "x = x - 1;",
        "x--;",
        "void f() { n = n - 1; }\n",
        "void f() { n--; }\n",
        1
    },
    {   // 16. compound assignment: x = x + y → x += y
        "CompoundAdd",
        "x = x + y;",
        "x += y;",
        "void f() { total = total + delta; }\n",
        "void f() { total += delta; }\n",
        1
    },
    {   // 17. compound subtract
        "CompoundSub",
        "x = x - y;",
        "x -= y;",
        "void f() { balance = balance - cost; }\n",
        "void f() { balance -= cost; }\n",
        1
    },
    {   // 18. += idiom
        "PlusEqWrap",
        "x += A;",
        "x = add(x, A);",
        "void f() { n += 1; }\n",
        "void f() { n = add(n, 1); }\n",
        1
    },
    {   // 19. -= idiom
        "MinusEqWrap",
        "x -= A;",
        "x = sub(x, A);",
        "void f() { n -= 1; }\n",
        "void f() { n = sub(n, 1); }\n",
        1
    },
    {   // 20. pointer dereference assignment
        "PtrDeref",
        "*x = A;",
        "STORE(x, A);",
        "void f() { *p = 0; }\n",
        "void f() { STORE(p, 0); }\n",
        1
    },

    // ================================================================
    //  21-30: Multi-statement patterns (2-space = SKIP_SIBLINGS)
    // ================================================================

    {   // 21. declare then assign → init
        "DeclThenInit",
        "int x;  x = A;",
        "int x = A;",
        "void f() { int n; n = 42; }\n",
        "void f() { int n = 42; }\n",
        1
    },
    {   // 22. assign then return → direct return
        "AssignReturn",
        "x = A;  return x;",
        "return A;",
        "int f() { r = compute(); return r; }\n",
        "int f() { return compute(); }\n",
        1
    },
    {   // 23. assign then return, multiple functions
        "AssignReturnMulti",
        "x = A;  return x;",
        "return A;",
        "int f() { r = one(); return r; }\n"
        "int g() { v = two(); return v; }\n",
        "int f() { return one(); }\n"
        "int g() { return two(); }\n",
        2
    },
    {   // 24. consecutive overwrite: remove dead store
        "DeadStore",
        "x = A;  x = B;",
        "x = B;",
        "void f() { val = 0; val = compute(); }\n",
        "void f() { val = compute(); }\n",
        1
    },
    {   // 25. assign then use in condition
        "AssignThenIf",
        "x = A;  if (x)",
        "if (A)",
        "void f() { r = 1; if (r) {} }\n",
        "void f() { if (1) {} }\n",
        1
    },
    {   // 26. swap idiom detection
        "SwapIdiom",
        "x = y;  y = z;  z = x;",
        "SWAP(y, z);",
        "void f() { t = a; a = b; b = t; }\n",
        "void f() { SWAP(a, b); }\n",
        1
    },
    {   // 27. error check pattern: assign + if-return
        "ErrCheck",
        "x = A;  if (x) return x;",
        "TRY(x, A);",
        "void f() { rc = open(); if (rc) return rc; }\n",
        "void f() { TRY(rc, open()); }\n",
        1
    },
    {   // 28. three-statement with backtracking
        "ThreeStmtBT",
        "x = A;  y = B;  x = y;",
        "x = B;",
        "void f() { a = 1; b = 2; a = b; }\n",
        "void f() { a = 2; }\n",
        1
    },
    {   // 29. same var assigned then reused (consistency)
        "ConsistencyOK",
        "x = A;  x = B;",
        "x = B;",
        "void f() { m = 10; m = 20; }\n",
        "void f() { m = 20; }\n",
        1
    },
    {   // 30. consistency blocks: x = x + 1, x must match both
        "ConsistencyBlock",
        "x = x + 1;",
        "x++;",
        "void f() { i = j + 1; }\n",
        "void f() { i = j + 1; }\n",
        0
    },

    // ================================================================
    //  31-40: Control flow rewrites
    // ================================================================

    {   // 31. negate if condition
        "NegIf",
        "if (x)",
        "if (!x)",
        "void f() { if (ready) { go(); } }\n",
        "void f() { if (!ready) { go(); } }\n",
        1
    },
    {   // 32. while to do-while hint (match the while)
        "WhileMatch",
        "while (x)",
        "for (;x;)",
        "void f() { while (cond) { step(); } }\n",
        "void f() { for (;cond;) { step(); } }\n",
        1
    },
    {   // 33. if-return to early bail
        "IfReturnBail",
        "if (x) return A;",
        "BAIL_IF(x, A);",
        "void f() { if (err) return code; }\n",
        "void f() { BAIL_IF(err, code); }\n",
        1
    },
    {   // 34. for loop with increment idiom
        "ForLoopIncr",
        "for (x = 0; x < y; x = x + 1)",
        "for (x = 0; x < y; x++)",
        "void f() { for (i = 0; i < n; i = i + 1) {} }\n",
        "void f() { for (i = 0; i < n; i++) {} }\n",
        1
    },
    {   // 35. single-element initializer
        "InitSingle",
        "x = {A};",
        "x = {.val = A};",
        "void f() { s = {42}; }\n",
        "void f() { s = {.val = 42}; }\n",
        1
    },
    {   // 36. arrow member assign
        "ArrowAssign",
        "x->y = A;",
        "SET(x, y, A);",
        "void f() { s->val = 0; }\n",
        "void f() { SET(s, val, 0); }\n",
        1
    },
    {   // 37. arrow member assign, multiple
        "ArrowAssignMulti",
        "x->y = A;",
        "SET(x, y, A);",
        "void f() { s->a = 1; s->b = 2; }\n",
        "void f() { SET(s, a, 1); SET(s, b, 2); }\n",
        2
    },
    {   // 38. DFS skip: int declaration (full)
        "DFSDecl",
        "int x = A;",
        "long x = A;",
        "void f() { int val = 5; }\n",
        "void f() { long val = 5; }\n",
        1
    },
    {   // 39. DFS skip: find int across nested blocks (full)
        "DFSDeep",
        "int x = A;",
        "long x = A;",
        "void f() { if (1) { int deep = 0; } }\n",
        "void f() { if (1) { long deep = 0; } }\n",
        1
    },
    {   // 40. multiple if matches
        "IfMulti",
        "if (x)",
        "if (check(x))",
        "void f() { if (a) {} if (b) {} }\n",
        "void f() { if (check(a)) {} if (check(b)) {} }\n",
        2
    },

    // ================================================================
    //  41-50: Multi-match, complex, edge cases
    // ================================================================

    {   // 41. increment in two bodies, two per body
        "IncrTwoBodies",
        "x = x + 1;",
        "x++;",
        "void f() { i = i + 1; j = j + 1; }\n"
        "void g() { k = k + 1; }\n",
        "void f() { i++; j++; }\n"
        "void g() { k++; }\n",
        3
    },
    {   // 42. declare + assign across bodies
        "DeclInitCross",
        "int x;  x = A;",
        "int x = A;",
        "void f() { int a; a = 1; }\n"
        "void g() { int b; b = 2; }\n",
        "void f() { int a = 1; }\n"
        "void g() { int b = 2; }\n",
        2
    },
    {   // 43. backtracking: skip past distractor
        "BTSkip",
        "x = A;  y = B;  x = y;",
        "x = B;",
        "void f() { a = 1; c = 3; b = 2; a = b; }\n",
        "void f() { a = 2; }\n",
        1
    },
    {   // 44. two separate match groups in one body
        "TwoGroups",
        "x = A;  return x;",
        "return A;",
        "int f() { r = one(); return r; }\n",
        "int f() { return one(); }\n",
        1
    },
    {   // 45. backtrack reentrant: two functions
        "BTReentrant",
        "x = A;  y = B;  x = y;",
        "x = B;",
        "void f() { a = 1; c = 3; b = 2; a = b; }\n"
        "void g() { d = 4; e = 5; d = e; }\n",
        "void f() { a = 2; }\nvoid g() { d = 5; }\n",
        2
    },
    {   // 46. no match: needle requires 3 stmts, only 2 present
        "TooFew",
        "x = A;  y = B;  x = y;",
        "x = B;",
        "void f() { a = 1; b = 2; }\n",
        "void f() { a = 1; b = 2; }\n",
        0
    },
    {   // 47. no match: consistency blocks the pattern
        "ConsistencyNoMatch",
        "x = A;  y = B;  x = y;",
        "x = B;",
        "void f() { a = 1; b = 2; c = 3; }\n",
        "void f() { a = 1; b = 2; c = 3; }\n",
        0
    },
    {   // 48. simple declaration match (full)
        "DFSSimple",
        "int x;",
        "unsigned x;",
        "void f() { int val; }\n",
        "void f() { unsigned val; }\n",
        1
    },
    {   // 49. init decl with value
        "InitDeclVal",
        "int x = A;",
        "long x = A;",
        "void f() { int n = 42; }\n",
        "void f() { long n = 42; }\n",
        1
    },
    {   // 50. multi-match increment in same body
        "IncrSameBody",
        "x = x + 1;",
        "x++;",
        "void f() { a = a + 1; b = b + 1; c = c + 1; }\n",
        "void f() { a++; b++; c++; }\n",
        3
    },
};

#define SPOT_RE_NCASES (sizeof(SPOT_RE_CASES) / sizeof(SPOT_RE_CASES[0]))

// Verify match counts and replacement output for each test case.
ok64 SPOTreTestTable() {
    sane(1);

    for (size_t i = 0; i < SPOT_RE_NCASES; i++) {
        const SPOTreCase *tc = &SPOT_RE_CASES[i];
        aBpad(u8, nbuf, 4096);
        aBpad(u64, nidx, 256);
        aBpad(u8, hbuf, 65536);
        aBpad(u64, hidx, 1024);

        SPOTstate st = {};
        call(SPOTreSetup, &st, nbuf, nidx, hbuf, hidx,
             tc->needle, tc->haystack);

        // Set up log buffers
        aBpad(u64, mlog, 256);
        aBpad(u64, alog, 256);
        st.mlog[0] = mlog[0]; st.mlog[1] = mlog[1];
        st.mlog[2] = mlog[2]; st.mlog[3] = mlog[3];
        st.alog[0] = alog[0]; st.alog[1] = alog[1];
        st.alog[2] = alog[2]; st.alog[3] = alog[3];

        int matches = 0;
        for (int j = 0; j < 100; j++) {
            ok64 o = SPOTNext(&st);
            if (o == OK)
                matches++;
            else if (o == SPOTEND)
                break;
            else
                fail(o);
        }

        if (tc->nmatches >= 0 && matches != tc->nmatches) {
            fprintf(stderr,
                    "FAIL [%s]: expected %d matches, got %d\n",
                    tc->name, tc->nmatches, matches);
            fail(FAILEQ);
        }

        // When logs are active, verify structural properties
        if (matches > 0 && u64bDataLen(st.mlog) > 0) {
            u64p mp = u64bDataHead(st.mlog);
            u64 hay_len = (u64)$len(st.hay);
            size_t mlen = u64bDataLen(st.mlog);
            for (size_t k = 0; k < mlen; k++) {
                u32 hpos = SPOTLogHay(mp[k]);
                test(hpos > 0 && hpos < hay_len, FAILSANITY);
            }
        }

        // Verify replacement output
        if (tc->replace != NULL && tc->expected != NULL) {
            u8cs hay = {u8bDataHead(hbuf), u8bIdleHead(hbuf)};
            u8csc source = {(u8cp)tc->haystack,
                            (u8cp)tc->haystack + strlen(tc->haystack)};
            u8csc ndl_src = {(u8cp)tc->needle,
                             (u8cp)tc->needle + strlen(tc->needle)};
            u8csc rep_src = {(u8cp)tc->replace,
                             (u8cp)tc->replace + strlen(tc->replace)};
            u8cs ext = $u8str(".c");

            aBpad(u8, outbuf, 65536);
            u8s out = {u8bIdleHead(outbuf), outbuf[3]};

            ok64 ro = SPOTReplace(out, source, hay, ndl_src, rep_src, ext);

            if (tc->nmatches == 0) {
                // Expect SPOTEND (no matches)
                if (ro != SPOTEND) {
                    fprintf(stderr,
                            "FAIL [%s]: expected SPOTEND, got %s\n",
                            tc->name, ok64str(ro));
                    fail(FAILEQ);
                }
            } else {
                if (ro != OK) {
                    fprintf(stderr,
                            "FAIL [%s]: SPOTReplace failed: %s\n",
                            tc->name, ok64str(ro));
                    fail(ro);
                }
                u8cs result = {u8bIdleHead(outbuf), out[0]};
                size_t exp_len = strlen(tc->expected);
                if ($len(result) != exp_len ||
                    memcmp(result[0], tc->expected, exp_len) != 0) {
                    fprintf(stderr,
                            "FAIL [%s]: replacement mismatch\n"
                            "  expected (%zu): \"%.*s\"\n"
                            "  got      (%zu): \"%.*s\"\n",
                            tc->name,
                            exp_len, (int)exp_len, tc->expected,
                            $len(result), (int)$len(result),
                            (char *)result[0]);
                    fail(FAILEQ);
                }
            }
        }
    }
    done;
}

ok64 SPOTreTest() {
    sane(1);
    call(SPOTreTestTable);
    done;
}

TEST(SPOTreTest);
