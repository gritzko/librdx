//
// Created by gritzko on 11/20/25.
//
#include "RDX.h"

#include <strings.h>

#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "abc/TEST.h"

ok64 u8csTestEq(u8cs a, u8cs b) {
    sane(u8csOK(a) && u8csOK(b));
#if NDEBUG
    if ($eq(a, b)) return OK;
#endif
    a_pad(u8, msg, PAGESIZE);
    a_cstr(wantmsg, "want:\t");
    a_cstr(factmsg, "fact:\t");
    call(u8bFeed, msg, wantmsg);
    call(HEXPut, msg_idle, a);
    call(u8bFeed1, msg, '\n');
    call(u8bFeed, msg, factmsg);
    call(HEXPut, msg_idle, b);
    call(u8bFeed1, msg, '\n');

    call(u8bFeed, msg, wantmsg);
    call(u8bFeed, msg, a);
    call(u8bFeed1, msg, '\n');
    call(u8bFeed, msg, factmsg);
    call(u8bFeed, msg, b);
    call(u8bFeed1, msg, '\n');

    $print(msg_datac);

    return $eq(a, b) ? OK : NOEQ;
}

ok64 RDXTestBasics() {
    sane(1);
    testeq(sizeof(rdx), 64);
    rdx a;
    rdxgp g = a.ins;
    done;
}

ok64 RDXid128test() {
    sane(1);
#define RDXIDINLEN 3
    id128 inputs[RDXIDINLEN] = {{0, 0},      //
                                {100, 200},  //
                                {ron60Max, ron60Max}};
    for (int i = 0; i < RDXIDINLEN; ++i) {
        aBpad(u8, ron, 64);
        zerob(ron);
        call(RDXutf8sFeedID, u8bIdle(ron), &inputs[i]);
        id128 in2 = {};
        call(RDXutf8sDrainID, u8bDataC(ron), &in2);
        want(id128Eq(&inputs[i], &in2));
    }
    done;
}

/*   tlv1 --> jdr1 --> tlv4
 *    |
 *   tlv2 --> jdr2
 *    |
 *   tlv3
 */
ok64 RDXTestTLV() {
    sane(1);
    a_u8cs(uno, 'i', 2, 0, 2);
    a_u8cs(duos, 's', 12, 0, 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l',
           'd');
    a_u8cs(tres, 'r', 3, 2, 1, 2);
    a_u8cs(tuple, 'p', 5, 0, 'i', 2, 0, 8);
    a_u8cs(euler, 'e', 18, 0, 'p', 5, 0, 'i', 2, 0, 8, 'p', 8, 2, 2, 3, 'i', 3,
           0, 1, 2);
    u8csp inputs[] = {
        tuple, uno, duos, tres, euler, NULL,
    };
    int i = 0;
    while (inputs[i]) {
        a_dup(u8c, in, inputs[i]);
        a_pad(u8, tlv2, 256);
        // Reader: next=position, opt=range_end, bulk=NULL
        rdx itc = {.format = RDX_FMT_TLV};
        itc.next = in[0];
        itc.opt = (u8p)in[1];
        itc.bulk = NULL;
        // Writer: bulk points to buffer
        rdx it = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
        it.bulk = tlv2;

        call(rdxCopy, &it, &itc);

        call(u8csTestEq, inputs[i], tlv2_datac);

        i++;
    }

    done;
}

ok64 RDXTestJDR() {
    sane(1);
    a_cstr(oneint, "1");
    a_cstr(oneterm, "a1");
    a_cstr(oneid, "a-1");
    a_cstr(tuple, "(1,two,3E0)");
    u8csp inputs[] = {
        oneint, oneterm, oneid, tuple, NULL,
    };
    int i = 0;
    while (inputs[i]) {
        a_dup(u8c, jdrA, inputs[i]);
        a_pad(u8, tlv, 256);
        a_pad(u8, jdrB, 256);
        // 1. a_rdx(jdr1, in, ...) no rdxb
        // 2. u8csFork(orig, copy), u8csJoin(orig, copy)
        // 3. u8csIn(outer, inner) u8csIs(outer, inner)
        // 4. RDX_FMT_SKIP ((()))
        // 5. RDX_FMT_VEC 40b (diff, use len)
        // 6. RDX_FMT_MEM (prepared, threshold, ext str)
        // 7. g is s*2, no u8g API, a_gauge and .

        rdx jdr1 = {.format = RDX_FMT_JDR};
        jdr1.next = jdrA[0];
        jdr1.opt = (u8p)jdrA[1];
        // TLV writer: bulk points to buffer's DataIdle gauge
        rdx tlv1 = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
        tlv1.bulk = tlv;

        call(rdxCopy, &tlv1, &jdr1);

        // TLV reader: next=position, opt=range_end, bulk=NULL
        a_dup(u8c, tlv2_bounds, tlv_datac);
        rdx tlv2 = {.format = RDX_FMT_TLV};
        tlv2.next = tlv2_bounds[0];
        tlv2.opt = (u8p)tlv2_bounds[1];
        tlv2.bulk = NULL;
        rdx jdr2 = {};
        rdxWriteInit(&jdr2, RDX_FMT_JDR, jdrB);

        call(rdxCopy, &jdr2, &tlv2);

        call(u8csTestEq, inputs[i], jdrB_datac);

        i++;
    }

    done;
}

#include <test/ZE.h>

ok64 RDXTestZE() {
    sane(1);
    a_pad(rdx, elems, 64);
    a_cstr(in, EULERZ_TEST);
    rdx it = {.format = RDX_FMT_JDR};
    it.next = in[0]; it.opt = (u8p)in[1];
    call(rdxNext, &it);
    test(it.type == RDX_TYPE_EULER, RDXBAD);
    rdx c = {};
    call(rdxInto, &c, &it);

    scan(rdxNext, &c) call(rdxbFeedP, elems, &c);
    seen(END);

    fprintf(stderr, "Stored %lu elements\n", rdxbDataLen(elems));

    // N**N compare
    rdxsp data = rdxbData(elems);
    ok64 eu = rdxEulerZ(*data + 30, *data + 31);
    for (rdxp i = data[0]; i < data[1]; ++i) {
        for (rdxp j = i + 1; j < data[1]; ++j) {
            printf("%lu %lu\n", i - *data, j - *data);
            test(YES == rdxEulerZ(i, j), RDXBADORDR);
        }
    }

    done;
}

#include <test/Y1.h>
#include <test/YE.h>
#include <test/YL.h>
#include <test/YP.h>
#include <test/YSYS.h>
#include <test/YX.h>

ok64 RDXTestY(u8cs test[][8]) {
    sane(1);
    for (int i = 0; test[i][0][0]; i++) {
        a_pad(rdx, inputs, 16);
        rdxbZero(inputs);
        a_dup(u8c, correct, test[i][0]);
        for (int j = 0; test[i][j][0]; j++) {
            rdxp it = 0;
            call(rdxbFedP, inputs, &it);
            it->format = RDX_FMT_JDR;
            it->next = test[i][j][0]; it->opt = (u8p)test[i][j][1];
        }
        rdxsFed1(rdxbData(inputs));
        a_pad(u8, res, PAGESIZE);
        u8bZero(res);
        rdx w = {};
        rdxWriteInit(&w, RDX_FMT_JDR, res);
        call(rdxMerge, &w, rdxbDataIdle(inputs));

        $println(res_datac);
        $println(correct);
        test(0 == $cmp(res_datac, correct), RDXBAD);
    }
    done;
}

ok64 RDXTestY2(u8cs test[][8]) {
    sane(1);
    for (int i = 0; test[i][0][0]; i++) {
        a_pad(rdx, inputs, 16);
        rdxbZero(inputs);
        a_dup(u8c, correct, test[i][0]);
        for (int j = 0; test[i][j][0]; j++) {
            rdxp it = 0;
            call(rdxbFedP, inputs, &it);
            it->format = RDX_FMT_JDR;
            it->next = test[i][j][0]; it->opt = (u8p)test[i][j][1];
        }
        rdxsFed1(rdxbData(inputs));
        rdx y = {.format = RDX_FMT_Y};
        rdxgMv(y.ins, rdxbDataIdle(inputs));
        a_pad(u8, res, PAGESIZE);
        rdx w = {};
        rdxWriteInit(&w, RDX_FMT_JDR, res);
        call(rdxCopy, &w, &y);

        $println(res_datac);
        test(0 == $cmp(res_datac, correct), RDXBAD);
    }
    done;
}

#include <test/YSKIP.h>

ok64 RDXTestSkipJDR() {
    sane(1);
    for (int i = 0; $ok(SKIP_TESTS[i].jdr); i++) {
        a_dup(u8c, jdr, SKIP_TESTS[i].jdr);
        u32 expected = SKIP_TESTS[i].count;
        rdx r = {.format = RDX_FMT_JDR};
        r.next = *jdr;
        r.opt = (u8p)jdr[1];

        // Parse root element
        ok64 o = rdxNext(&r);
        if (o != OK) {
            fprintf(stderr, "test %d: rdxNext root failed: %s\n    input: ", i,
                    ok64str(o));
            $println(SKIP_TESTS[i].jdr);
            fail(o);
        }

        // Must be a plex type
        if (!rdxTypePlex(&r)) {
            fprintf(stderr, "test %d: root is not plex, type=%d\n", i, r.type);
            fail(RDXBAD);
        }

        // Go into the plex and count children
        rdx c = {};
        o = rdxInto(&c, &r);
        if (o != OK) {
            fprintf(stderr, "test %d: rdxInto failed: %s\n    input: ", i,
                    ok64str(o));
            $println(SKIP_TESTS[i].jdr);
            fail(o);
        }

        u32 count = 0;
        while ((o = rdxNext(&c)) == OK) {
            count++;
        }
        if (o != END) {
            fprintf(stderr,
                    "test %d: iteration failed at elem %u: %s\n    input: ", i,
                    count, ok64str(o));
            $println(SKIP_TESTS[i].jdr);
            fail(o);
        }

        if (count != expected) {
            fprintf(stderr,
                    "test %d: expected %u elements, got %u\n    input: ", i,
                    expected, count);
            $println(SKIP_TESTS[i].jdr);
            fail(RDXBAD);
        }
    }
    done;
}

ok64 RDXTestEulerSort() {
    sane(1);
    // Create array 0..100
    int nums[101];
    for (int i = 0; i <= 100; i++) nums[i] = i;

    // Shuffle using Fisher-Yates
    srandom(42);
    for (int i = 100; i > 0; i--) {
        int j = random() % (i + 1);
        int tmp = nums[i];
        nums[i] = nums[j];
        nums[j] = tmp;
    }

    // Build JDR Euler set string: {n0,n1,n2,...}
    a_pad(u8, jdr, PAGESIZE);
    call(u8bFeed1, jdr, '{');
    for (int i = 0; i <= 100; i++) {
        if (i > 0) call(u8bFeed1, jdr, ',');
        i64 n = nums[i];
        call(utf8sFeedInt, jdr_idle, &n);
    }
    call(u8bFeed1, jdr, '}');

    // Normalize using rdxMerge (gauge needs space for sorting work)
    a_pad(rdx, inputs, 128);
    rdxp inp = 0;
    call(rdxbFedP, inputs, &inp);
    inp->format = RDX_FMT_JDR;
    inp->next = jdr_datac[0]; inp->opt = (u8p)jdr_datac[1];
    a_pad(u8, out, PAGESIZE);
    rdx w = {};
    rdxWriteInit(&w, RDX_FMT_JDR, out);
    call(rdxMerge, &w, rdxbDataIdle(inputs));


    // Parse result and verify order
    rdx res = {.format = RDX_FMT_JDR};
    res.next = out_datac[0]; res.opt = (u8p)out_datac[1];
    call(rdxNext, &res);
    test(res.type == RDX_TYPE_EULER, RDXBAD);

    rdx c = {};
    call(rdxInto, &c, &res);
    i64 prev = -1;
    int count = 0;
    ok64 o;
    while ((o = rdxNext(&c)) == OK) {
        test(c.type == RDX_TYPE_INT, RDXBAD);
        test(c.i > prev, RDXBADORDR);
        prev = c.i;
        count++;
    }
    test(o == END, o);
    test(count == 101, RDXBAD);

    done;
}

// Test TLV string to JDR escaping (tricky strings with quotes, newlines, etc.)
ok64 RDXTestTLVtoJDREscape() {
    sane(1);
    // TLV body length = 1 (idlen byte) + idlen + value_len
    // String with newline: "hello\nworld" (11 chars), body_len = 1+0+11 = 12
    a_u8cs(s_newline, 's', 12, 0, 'h', 'e', 'l', 'l', 'o', '\n', 'w', 'o', 'r',
           'l', 'd');
    // String with quote: say "hi" (8 chars), body_len = 1+0+8 = 9
    a_u8cs(s_quote, 's', 9, 0, 's', 'a', 'y', ' ', '"', 'h', 'i', '"');
    // String with backslash: path\file (9 chars), body_len = 1+0+9 = 10
    a_u8cs(s_bslash, 's', 10, 0, 'p', 'a', 't', 'h', '\\', 'f', 'i', 'l', 'e');

    // Expected JDR outputs (escaped)
    a$str(expect_newline, "\"hello\\nworld\"");
    a$str(expect_quote, "\"say \\\"hi\\\"\"");
    a$str(expect_bslash, "\"path\\\\file\"");

    u8csp inputs[] = {s_newline, s_quote, s_bslash, NULL};
    u8csp expects[] = {expect_newline, expect_quote, expect_bslash, NULL};

    for (int i = 0; inputs[i]; i++) {
        a_dup(u8c, in, inputs[i]);
        a_pad(u8, jdr, 256);

        // TLV reader: next=position, opt=range_end, bulk=NULL
        rdx tlv_read = {.format = RDX_FMT_TLV};
        tlv_read.next = in[0];
        tlv_read.opt = (u8p)in[1];
        tlv_read.bulk = NULL;

        rdx jdr_write = {};
        rdxWriteInit(&jdr_write, RDX_FMT_JDR, jdr);

        call(rdxCopy, &jdr_write, &tlv_read);


        $println(jdr_datac);
        call(u8csTestEq, expects[i], jdr_datac);
    }

    done;
}

/*
// Multimap test: Euler set with tuples having same key but different IDs
// Tests that:
// 1. Tuples are ordered by (id, key) in Euler sets
// 2. Multiple tuples with same key but different IDs are preserved
// 3. Merge handles multimap semantics correctly
typedef struct {
    char const* name;
    char const* inputs[4];  // up to 3 inputs + NULL terminator
    char const* expected;   // expected result after merge
} multimap_test;

static multimap_test MULTIMAP_TESTS[] = {
    // Same key, different IDs -> multimap (both preserved)
    {
        .name = "same_key_diff_id",
        .inputs = {"{(@0-2 \"k\",\"v1\")}", "{(@0-4 \"k\",\"v2\")}", NULL},
        .expected = "{(@2 \"k\",\"v1\"),(@4 \"k\",\"v2\")}",
    },
    // Same key, same ID, different values -> higher seq wins
    {
        .name = "same_key_same_id",
        .inputs = {"{(@0-2 \"k\",\"v1\")}", "{(@0-4 \"k\",\"v2\")}", NULL},
        .expected = "{(@2 \"k\",\"v1\"),(@4 \"k\",\"v2\")}",
    },
    // Different keys -> regular map behavior (both preserved, sorted)
    {
        .name = "diff_keys",
        .inputs = {"{(@0-2 \"a\",1)}", "{(@0-4 \"b\",2)}", NULL},
        .expected = "{(@2 \"a\",1),(@4 \"b\",2)}",
    },
    // No IDs -> regular map (tuples merged by key)
    {
        .name = "no_ids_same_key",
        .inputs = {"{(\"k\",1)}", "{(\"k\",2)}", NULL},
        .expected = "{(\"k\",2)}",  // latest value wins
    },
    // No IDs, different keys -> regular map
    {
        .name = "no_ids_diff_keys",
        .inputs = {"{(\"a\",1)}", "{(\"b\",2)}", NULL},
        .expected = "{(\"a\",1),(\"b\",2)}",
    },
    // Three tuples with same key, different IDs
    {
        .name = "three_same_key",
        .inputs = {"{(@0-2 \"k\",\"a\")}", "{(@0-4 \"k\",\"b\")}",
                   "{(@0-6 \"k\",\"c\")}", NULL},
        .expected = "{(@2 \"k\",\"a\"),(@4 \"k\",\"b\"),(@6 \"k\",\"c\")}",
    },
    {.name = NULL},  // sentinel
};

ok64 RDXTestMultimap() {
    sane(1);
    fprintf(stderr, "Multimap tests:\n");
    for (int t = 0; MULTIMAP_TESTS[t].name; t++) {
        multimap_test* test = &MULTIMAP_TESTS[t];
        fprintf(stderr, "  %s\n", test->name);

        a_pad(rdx, inputs, 16);
        rdxbZero(inputs);
        a_cstr(expected, test->expected);

        // Add all inputs
        for (int i = 0; test->inputs[i]; i++) {
            rdxp inp = 0;
            call(rdxbFedP, inputs, &inp);
            inp->format = RDX_FMT_JDR;
            a_cstr(input_jdr, test->inputs[i]);
            inp->next = input_jdr[0]; inp->opt = (u8p)input_jdr[1];
        }

        // Merge
        a_pad(u8, result, PAGESIZE);
        u8bZero(result);
        rdx w = {};
        rdxWriteInit(&w, RDX_FMT_JDR, result);
        call(rdxMerge, &w, rdxbDataIdle(inputs));


        // Compare
        if ($cmp(result_datac, expected) != 0) {
            fprintf(stderr, "    MISMATCH:\n");
            fprintf(stderr, "      expected: %s\n", test->expected);
            fprintf(stderr, "      got:      %.*s\n", (int)$len(result_datac),
                    (char*)*result_datac);
            fail(RDXBAD);
        }
    }
    done;
}
*/

// Tuple replacement test: merging tuples with same key, higher seq wins
typedef struct {
    char const* name;
    char const* inputs[4];  // up to 3 inputs + NULL terminator
    char const* expected;   // expected result after merge
} tuple_replace_test;

static tuple_replace_test TUPLE_REPLACE_TESTS[] = {
    // Higher seq tuple replaces lower seq tuple (same key)
    {
        .name = "higher_seq_wins",
        .inputs = {"(1,2,3,4)", "(@2 1,2)", NULL},
        .expected = "(@2 1,2)",
    },
    // Same seq, same key - values merge position-wise
    {
        .name = "same_seq_merge",
        .inputs = {"(1,2,3)", "(1,2,4)", NULL},
        .expected = "(1,2,4)",  // last value wins at each position
    },
    // Three-way merge, highest seq wins
    {
        .name = "three_way_highest_wins",
        .inputs = {"(1,2)", "(@2 1,3)", "(@4 1,4)", NULL},
        .expected = "(@4 1,4)",
    },
    // Different keys - higher seq still wins
    {
        .name = "diff_keys_higher_seq",
        .inputs = {"(1,2)", "(@2 2,3)", NULL},
        .expected = "(@2 2,3)",  // higher seq wins
    },
    // Same seq/src, different keys - tests the rdxWinZ TODO
    // With same seq/src, tuple with greater key should win
    {
        .name = "same_seq_diff_keys",
        .inputs = {"(1,\"val\")", "(2,\"val\")", NULL},
        .expected = "(2,\"val\")",  // key 2 > key 1
    },
    // Reversed order - should still pick greater key
    {
        .name = "same_seq_diff_keys_rev",
        .inputs = {"(2,\"val\")", "(1,\"val\")", NULL},
        .expected = "(2,\"val\")",  // key 2 > key 1 regardless of order
    },
    // Same seq/src, different keys - tuple with greater key wins entirely
    {
        .name = "same_seq_key_wins",
        .inputs = {"(2,\"A\")", "(1,\"B\")", NULL},
        .expected = "(2,\"A\")",  // key 2 > key 1, so (2,"A") wins entirely
    },
    // Bug test: key ID should be zero after merge (stripped form)
    {
        .name = "key_id_zero_after_merge",
        .inputs = {"(a@2,1)", "(a@4,2)", NULL},
        .expected = "(a,2)",  // key 'a' should have no ID, value 2 wins
    },
    // Bug test: TUPLE children position must not shift during merge
    {
        .name = "tuple_position_preserved",
        .inputs = {"(1@2,2@2,3@2,4@2)", "(1@1,2@1,3@1,4@1)", NULL},
        .expected = "(1,2@2,3@2,4@2)",  // key (pos 0) ID zeroed, other
                                        // positions preserve higher seq
    },
    // Bug test: position-based merge with mixed types at same positions
    {
        .name = "tuple_position_types",
        .inputs = {"(1,\"a\",())", "(@2 1,\"a\",())", NULL},
        .expected =
            "(@2 1,\"a\",())",  // higher seq wins, types preserved at positions
    },
    // Empty tuple in tuple preserved (null placeholder)
    {
        .name = "empty_tuple_in_tuple_kept",
        .inputs = {"(1,(),3)", NULL},
        .expected = "(1,(),3)",  // empty tuple = null, preserved in tuple
    },
    // Multiple empty tuples in tuple (sparse array)
    {
        .name = "multiple_nulls_in_tuple",
        .inputs = {"(1,(),(),4)", NULL},
        .expected = "(1,(),(),4)",  // multiple nulls preserved
    },
    {.name = NULL},  // sentinel
};

// Normalization tests for empty tuple stripping
typedef struct {
    char const* name;
    char const* input;
    char const* expected;
} normalize_test;

static normalize_test NORMALIZE_TESTS[] = {
    // Empty tuple in euler gets stripped (not valid in euler)
    {
        .name = "empty_tuple_in_euler_stripped",
        .input = "{1,(),2}",
        .expected = "{1,2}",
    },
    // Multiple empty tuples in euler all stripped
    {
        .name = "multiple_empty_tuples_in_euler_stripped",
        .input = "{(),1,(),2,()}",
        .expected = "{1,2}",
    },
    // Empty tuple in linear gets stripped
    {
        .name = "empty_tuple_in_linear_stripped",
        .input = "[1,(),2]",
        .expected = "[1,2]",
    },
    // Nested: empty tuple inside tuple inside euler - kept
    {
        .name = "nested_empty_tuple_kept",
        .input = "{(1,(),3)}",
        .expected = "{(1,(),3)}",
    },
    // All empty tuples stripped leaves empty euler
    {
        .name = "all_empty_tuples_leaves_empty",
        .input = "{(),()}",
        .expected = "{}",
    },
    // Euler set with string-keyed tuples sorted by key
    {
        .name = "euler_tuples_sorted_by_key",
        .input = "{(\"Version\",\"1.0\"),(\"Architecture\",\"amd64\"),"
                 "(\"Section\",\"games\"),(\"Depends\",\"libc6\"),"
                 "(\"Maintainer\",\"Debian\"),(\"Priority\",\"optional\")}",
        .expected = "{(\"Architecture\",\"amd64\"),(\"Depends\",\"libc6\"),"
                    "(\"Maintainer\",\"Debian\"),(\"Priority\",\"optional\"),"
                    "(\"Section\",\"games\"),(\"Version\",\"1.0\")}",
    },
    {.name = NULL},
};

ok64 RDXTestNormalize() {
    sane(1);
    fprintf(stderr, "Normalization tests:\n");
    for (int t = 0; NORMALIZE_TESTS[t].name; t++) {
        normalize_test* test = &NORMALIZE_TESTS[t];
        fprintf(stderr, "  %s\n", test->name);

        a_pad(rdx, inputs, 16);
        rdxbZero(inputs);
        a_cstr(expected, test->expected);

        rdxp inp = 0;
        call(rdxbFedP, inputs, &inp);
        inp->format = RDX_FMT_JDR;
        a_cstr(input_jdr, test->input);
        inp->next = input_jdr[0]; inp->opt = (u8p)input_jdr[1];

        // Step 1: Normalize (ordering, preserve tombstones)
        a_pad(u8, merged, PAGESIZE);
        u8bZero(merged);
        rdx w = {};
        rdxWriteInit(&w, RDX_FMT_JDR, merged);
        ok64 norm_result = rdxNormalize(&w, inp);
        if (norm_result != OK) {
            fprintf(stderr, "    NORMALIZE FAILED: %s\n", ok64str(norm_result));
            fail(norm_result);
        }


        // Step 2: Strip (remove empty tuples from LEX containers)
        a_pad(u8, result, PAGESIZE);
        u8bZero(result);
        rdx r = {.format = RDX_FMT_JDR};
        r.next = merged_datac[0]; r.opt = (u8p)merged_datac[1];
        rdx w2 = {};
        rdxWriteInit(&w2, RDX_FMT_JDR, result);
        call(rdxStrip, &w2, &r);


        if ($cmp(result_datac, expected) != 0) {
            fprintf(stderr, "    MISMATCH:\n");
            fprintf(stderr, "      expected: %s\n", test->expected);
            fprintf(stderr, "      got:      %.*s\n", (int)$len(result_datac),
                    (char*)*result_datac);
            fail(RDXBAD);
        }
    }
    done;
}

ok64 RDXTestTupleReplace() {
    sane(1);
    fprintf(stderr, "Tuple replacement tests:\n");
    for (int t = 0; TUPLE_REPLACE_TESTS[t].name; t++) {
        tuple_replace_test* test = &TUPLE_REPLACE_TESTS[t];
        fprintf(stderr, "  %s\n", test->name);

        a_pad(rdx, inputs, 16);
        rdxbZero(inputs);
        a_cstr(expected, test->expected);

        // Add all inputs
        for (int i = 0; test->inputs[i]; i++) {
            rdxp inp = 0;
            call(rdxbFedP, inputs, &inp);
            inp->format = RDX_FMT_JDR;
            a_cstr(input_jdr, test->inputs[i]);
            inp->next = input_jdr[0]; inp->opt = (u8p)input_jdr[1];
        }

        // Merge
        a_pad(u8, result, PAGESIZE);
        u8bZero(result);
        rdx w = {};
        rdxWriteInit(&w, RDX_FMT_JDR, result);
        ok64 merge_result = rdxMerge(&w, rdxbDataIdle(inputs));
        if (merge_result != OK) {
            fprintf(stderr, "    MERGE FAILED: %s\n", ok64str(merge_result));
            fail(merge_result);
        }


        // Compare
        if ($cmp(result_datac, expected) != 0) {
            fprintf(stderr, "    MISMATCH:\n");
            fprintf(stderr, "      expected: %s\n", test->expected);
            fprintf(stderr, "      got:      %.*s\n", (int)$len(result_datac),
                    (char*)*result_datac);
            fail(RDXBAD);
        }
    }
    done;
}

ok64 RDXtest() {
    sane(1);
    call(RDXTestBasics);
    call(RDXid128test);
    call(RDXTestTLV);
    call(RDXTestTLVtoJDREscape);
    call(RDXTestJDR);
    call(RDXTestSkipJDR);
    call(RDXTestZE);
    call(RDXTestEulerSort);
    call(RDXTestY, FIRSTY_TEST);
    call(RDXTestY, YP_TEST);
    call(RDXTestY, YL_TEST);
    call(RDXTestY, YE_TEST);
    call(RDXTestY, YX_TEST);
    call(RDXTestY, YSYS_TEST);

    call(RDXTestY2, FIRSTY_TEST);
    call(RDXTestY2, YP_TEST);
    call(RDXTestY2, YL_TEST);
    call(RDXTestY2, YE_TEST);
    call(RDXTestY2, YX_TEST);
    call(RDXTestY2, YSYS_TEST);
    // call(RDXTestMultimap);
    call(RDXTestTupleReplace);
    call(RDXTestNormalize);
    done;
}

TEST(RDXtest);
