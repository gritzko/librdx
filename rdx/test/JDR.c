#include <stdio.h>
#include <test/JDRTLV.h>

#include "RDX.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// Helper to compare JDR outputs with diagnostic output
ok64 JDRu8csTestEq(u8cs a, u8cs b) {
    sane(u8csOK(a) && u8csOK(b));
    if ($eq(a, b)) return OK;
    fprintf(stderr, "  want: %.*s\n", (int)$len(a), (char*)*a);
    fprintf(stderr, "  fact: %.*s\n", (int)$len(b), (char*)*b);
    fail(RDXBAD);
}

// Helper: run roundtrip test on a table with optional inline mode
ok64 JDRTestRoundTripTable(u8cs* tests, u64 x64, char const* name) {
    sane(tests);
    for (int i = 0; $ok(tests[i]); i++) {
        a_dup(u8c, jdrA, tests[i]);
        a_pad(u8, tlv, 1024);
        a_pad(u8, jdrB, 1024);

        // JDR -> TLV
        rdx jdr1 = {.format = RDX_FMT_JDR};
        jdr1.next = jdrA[0];
        jdr1.opt = (u8p)jdrA[1];
        rdx tlv1 = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
        tlv1.bulk = tlv;

        ok64 o = rdxCopy(&tlv1, &jdr1);
        if (o != OK) {
            fprintf(stderr, "%s test %d: JDR->TLV failed: %s\n", name, i,
                    ok64str(o));
            fprintf(stderr, "  orig: %.*s\n", (int)$len(tests[i]),
                    (char*)*tests[i]);
            fail(o);
        }

        // TLV -> JDR
        rdx tlv2 = {.format = RDX_FMT_TLV};
        tlv2.next = tlv[1];      // data start
        tlv2.opt = (u8p)tlv[2];  // idle start = data end
        tlv2.bulk = NULL;
        rdx jdr2 = {};
        rdxWriteInit(&jdr2, RDX_FMT_JDR, jdrB);
        jdr2.opt = (u8p)(uintptr_t)x64;

        o = rdxCopy(&jdr2, &tlv2);
        if (o != OK) {
            fprintf(stderr, "%s test %d: TLV->JDR failed: %s\n", name, i,
                    ok64str(o));
            fprintf(stderr, "  orig: %.*s\n", (int)$len(tests[i]),
                    (char*)*tests[i]);
            fail(o);
        }

        // Compare
        o = JDRu8csTestEq(tests[i], jdrB_datac);
        if (o != OK) {
            fprintf(stderr, "%s test %d: roundtrip mismatch\n", name, i);
            fprintf(stderr, "  orig: %.*s\n", (int)$len(tests[i]),
                    (char*)*tests[i]);
            fail(o);
        }
    }
    done;
}

// Table-driven property test: JDR -> TLV -> JDR roundtrip (bracket mode)
ok64 JDRTestRoundTrip() {
    sane(1);
    call(JDRTestRoundTripTable, JDRTLV_TESTS, 0, "bracket");
    done;
}

// Table-driven property test: JDR -> TLV -> JDR roundtrip (inline mode)
ok64 JDRTestRoundTripInline() {
    sane(1);
    call(JDRTestRoundTripTable, JDRTLV_INLINE_TESTS, RON_i << 12, "inline");
    done;
}

ok64 JDRTestEulerNorm() {
    sane(1);
    a_cstr(input, "{b:2,a:1}");

    a_pad(rdx, inputs, 128);
    rdxp inp = 0;
    call(rdxbFedP, inputs, &inp);
    inp->format = RDX_FMT_JDR;
    inp->next = *input;
    inp->opt = (u8p)input[1];

    a_pad(u8, out, 4096);
    rdx w = {};
    rdxWriteInit(&w, RDX_FMT_JDR, out);

    call(rdxMerge, &w, rdxbDataIdle(inputs));

    fprintf(stderr, "Output: %.*s\n", (int)$len(out_datac), (char*)*out_datac);
    a_cstr(expect, "{(a,1),(b,2)}");
    test(0 == $cmp(out_datac, expect), RDXBAD);

    done;
}

// Test nested tuple iteration (reproduces SYNC pattern)
// Structure: euler containing tuples, each tuple has content that may be a plex
ok64 JDRTestNestedIteration() {
    sane(1);

    // Simulate FS tree: {("a",1,2,txt,()),("b",3,4,txt,())}
    a_cstr(jdr, "{(\"a\",1,2,txt,()),(\"b\",3,4,txt,())}");

    rdx root = {.format = RDX_FMT_JDR};
    root.next = jdr[0];
    root.opt = (u8p)jdr[1];

    // Parse outer euler
    call(rdxNext, &root);
    test(root.type == RDX_TYPE_EULER, RDXBAD);
    fprintf(stderr, "JDRTestNestedIteration: got euler\n");

    rdx it = {};
    call(rdxInto, &it, &root);

    int count = 0;
    ok64 r;
    while ((r = rdxNext(&it)) == OK) {
        if (it.type != RDX_TYPE_TUPLE) continue;

        // Parse tuple like fmetaParse does
        rdx child = {};
        call(rdxInto, &child, &it);

        // Get key (string)
        call(rdxNext, &child);
        test(child.type == RDX_TYPE_STRING, RDXBAD);
        fprintf(stderr, "  entry %d: key='%.*s'\n", count, (int)$len(child.s), (char*)*child.s);

        // Get int1
        call(rdxNext, &child);
        test(child.type == RDX_TYPE_INT, RDXBAD);

        // Get int2
        call(rdxNext, &child);
        test(child.type == RDX_TYPE_INT, RDXBAD);

        // Get term
        call(rdxNext, &child);
        test(child.type == RDX_TYPE_TERM, RDXBAD);

        // Get content (empty tuple)
        call(rdxNext, &child);
        test(child.type == RDX_TYPE_TUPLE, RDXBAD);
        fprintf(stderr, "    content type=%d (expect tuple=%d)\n", child.type, RDX_TYPE_TUPLE);

        test(END==rdxNext(&child), RDXBAD);

        call(rdxOuto, &child, &it);
        count++;
    }

    fprintf(stderr, "JDRTestNestedIteration: count=%d (expect 2)\n", count);
    test(count == 2, NOEQ);

    done;
}

// Regression: fuzz crash on invalid \u escape (negative-size memcpy)
ok64 JDRTestFuzzRegr() {
    sane(1);
    // "\u3Y31" has invalid hex in unicode escape; must not crash
    u8cs bad_inputs[] = {
        u8csOf("\"\\u3Y31\""),
        {},
    };
    for (int i = 0; $ok(bad_inputs[i]); i++) {
        a_pad(u8, tlv, 1024);
        rdx jdr = {.format = RDX_FMT_JDR};
        jdr.next = bad_inputs[i][0];
        jdr.opt = (u8p)bad_inputs[i][1];
        rdx tw = {};
        rdxWriteInit(&tw, RDX_FMT_TLV, tlv);
        ok64 o = rdxCopy(&tw, &jdr);
        test(o != OK, RDXBAD);  // must reject, not crash
    }
    done;
}

ok64 JDRtest() {
    sane(1);
    call(JDRTestRoundTrip);
    call(JDRTestRoundTripInline);
    call(JDRTestEulerNorm);
    call(JDRTestNestedIteration);
    call(JDRTestFuzzRegr);
    done;
}

TEST(JDRtest);
