//
// STRIP iterator tests - table-driven property tests using JDR
//
// Tests:
// 1. Deleted elements (tombstoned primitives) - skipped
// 2. Deleted containers (tombstoned plex elements) - skipped
// 3. IDs are zeroed on all returned elements
//
// Uses JDR input/output strings for clarity.
// In JDR, IDs are encoded as (id,value) pairs. The id format is src-seq.
// Tombstone is when seq has bit 0 set (odd seq numbers).
//
#include "RDX.h"
#include "abc/01.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// Test case: JDR input, expected JDR output after stripping
typedef struct {
    char const* input;
    char const* expect;
    char const* desc;
} STRIPcase;

con STRIPcase STRIP_CASES[] = {
    // === EMPTY CONTAINERS ===
    {"[]", "[]", "empty linear"},
    {"{}", "{}", "empty euler"},
    {"()", "()", "empty tuple"},

    // === PRIMITIVE TOMBSTONES ===
    // JDR stamp format: value@seq or value@src-seq
    // Odd seq = tombstoned, even seq = live
    // No stamp = no ID (live, will remain without stamp after strip)

    // One live element with stamp (seq=0, even = not tombstone)
    {"[42@1-0]", "[42]", "one non-tombstone int"},

    // One tombstoned element (seq=1, odd = tombstone)
    {"[42@1]", "[]", "one tombstone int"},

    // Mixed: live, tomb, live
    {"[10@0,20@1,30@2]", "[10,30]", "mixed tomb non-tomb"},

    // All tombstoned
    {"[10@1,20@3]", "[]", "all tombstoned"},

    // All live (even seq numbers)
    {"[10@0,20@2]", "[10,20]", "all non-tombstoned"},

    // === CONTAINER TOMBSTONES ===
    // Container stamps go right after opening bracket: [@stamp content]

    // Live container with live content (stamp @1-0: src=1, seq=0 even = live)
    {"([@1-0 10])", "([10])", "live container with content"},

    // Tombstoned container (stamp @1: seq=1 odd = tombstone)
    {"([@1 10])", "()", "tombstoned container skipped"},

    // Mixed containers: first live @0, second tombstoned @1, third live @2
    {"([@0 10],[@1 20],[@2 30])", "([10],[30])", "mixed containers"},

    // === ID VERIFICATION ===
    // IDs should be stripped (zeroed) in output

    // Large ID should become empty (no stamp in output)
    {"[42@100-200]", "[42]", "large ID stripped"},

    // Multiple elements with different IDs
    {"[1@5-10,2@7-20,3@9-30]", "[1,2,3]", "multiple IDs stripped"},

    // === NESTED STRUCTURES ===
    // Nested tombstones

    // Euler (set) with mixed tombstones
    {"{10@0,20@1,30@2}", "{10,30}", "euler set mixed tomb"},

    // Tombstoned tuple vs tombstoned value:
    // (@1 key,val) = tombstoned tuple -> skipped entirely
    // key:val@1 = tombstoned value -> (key) remains (b: == (b))
    {"((@1 a,10),(b,20))", "((b,20))", "tombstoned tuple skipped"},
    {"{a:10@1,b:20@0}", "{(a),(b,20)}", "tombstoned value leaves key"},

    // Deeply nested - inner element tombstoned
    {"[[1@0,2@1]]", "[[1]]", "nested with tombstone"},

    // === TRAILING EMPTY TUPLES ===
    // Trailing empty tuples should be stripped from TUPLE containers
    {"(1,2,3,(),())", "(1,2,3)", "trailing empty tuples"},
    {"(1,())", "(1)", "one trailing empty"},
    {"((),(),())", "()", "all empty tuples"},
    {"(1,(),2)", "(1,(),2)", "middle empty preserved"},
    {"((),(1,2,()))", "((),(1,2))", "nested trailing"},
};

con u64 STRIP_CASES_LEN = sizeof(STRIP_CASES) / sizeof(STRIP_CASES[0]);

ok64 test_strip_jdr(STRIPcase const* tc) {
    sane(tc);

    // Set up iterator pool for readers
    a_pad0(rdx, readers, 128);

    // Set up JDR reader as source
    rdxp jdr_reader = 0;
    call(rdxbFedP, readers, &jdr_reader);
    jdr_reader->format = RDX_FMT_JDR;
    u8cs input = {(u8cp)tc->input, (u8cp)tc->input + strlen(tc->input)};
    jdr_reader->next = (u8p)input[0]; jdr_reader->opt = (u8p)input[1];

    // Set up STRIP reader wrapping JDR reader
    rdxp strip = 0;
    call(rdxbFedP, readers, &strip);
    strip->format = RDX_FMT_STRIP;
    // Point STRIP's ins to JDR reader as source
    // Rest of pool is for child iterators
    strip->ins[0] = jdr_reader;
    strip->ins[1] = readers[2];     // Child pool starts at idle
    strip->ins[2] = readers[3];     // Child pool ends at term

    // Set up output buffer and JDR writer
    a_pad(u8, out, 4096);
    rdx jdr_writer = {};
    rdxWriteInit(&jdr_writer, RDX_FMT_JDR, out);

    // Copy from STRIP reader to JDR writer
    call(rdxCopy, &jdr_writer, strip);

    // Compare output
    u8cs expect = {(u8cp)tc->expect, (u8cp)tc->expect + strlen(tc->expect)};
    fprintf(stderr, "    got: %.*s\n", (int)$len(out_datac), (char*)out_datac[0]);
    test(0 == $cmp(out_datac, expect), RDXBAD);

    done;
}

ok64 test_strip_all() {
    sane(1);
    for (u64 i = 0; i < STRIP_CASES_LEN; i++) {
        STRIPcase const* tc = &STRIP_CASES[i];
        fprintf(stderr, "  case %lu: %s\n", i, tc->desc);
        ok64 o = test_strip_jdr(tc);
        if (o != OK) {
            fprintf(stderr, "FAIL: case %lu (%s): %s\n", i, tc->desc, ok64str(o));
            fprintf(stderr, "  input:  %s\n", tc->input);
            fprintf(stderr, "  expect: %s\n", tc->expect);
            fail(o);
        }
    }
    done;
}

// Test rdxStrip directly (JDR->TLV strip->JDR) for trailing empty tuples
ok64 test_strip_trailing_empty() {
    sane(1);
    fprintf(stderr, "Testing trailing empty tuple stripping (rdxStrip):\n");

    // Test cases specifically for trailing empty tuples
    con STRIPcase cases[] = {
        {"(1,2,3,(),())", "(1,2,3)", "trailing empty tuples"},
        {"(1,())", "(1)", "one trailing empty"},
        {"((),(),())", "()", "all empty tuples"},
        {"(1,(),2)", "(1,(),2)", "middle empty preserved"},
        {"((),(1,2,()))", "((),(1,2))", "nested trailing"},
    };

    for (u64 i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        STRIPcase const* tc = &cases[i];
        fprintf(stderr, "  %s\n", tc->desc);

        // Parse JDR input to TLV
        a_pad(u8, tlv_in, 4096);
        {
            rdx jdr = {.format = RDX_FMT_JDR};
            u8cs input = {(u8cp)tc->input, (u8cp)tc->input + strlen(tc->input)};
            jdr.next = *input;
            jdr.opt = (u8p)input[1];
            rdx tlv = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
            tlv.bulk = tlv_in;
            call(rdxCopy, &tlv, &jdr);
        }

        // Strip TLV -> TLV
        a_pad(u8, tlv_out, 4096);
        {
            rdx from = {.format = RDX_FMT_TLV};
            from.next = tlv_in[1];
            from.opt = (u8p)tlv_in[2];
            from.bulk = NULL;
            rdx into = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
            into.bulk = tlv_out;
            call(rdxStrip, &into, &from);
        }

        // Convert TLV output to JDR for comparison
        a_pad(u8, jdr_out, 4096);
        {
            rdx tlv = {.format = RDX_FMT_TLV};
            tlv.next = tlv_out[1];
            tlv.opt = (u8p)tlv_out[2];
            tlv.bulk = NULL;
            rdx jdr = {};
            rdxWriteInit(&jdr, RDX_FMT_JDR, jdr_out);
            call(rdxCopy, &jdr, &tlv);
            (void)0;
        }

        u8cs expect = {(u8cp)tc->expect, (u8cp)tc->expect + strlen(tc->expect)};
        fprintf(stderr, "    got: %.*s\n", (int)$len(jdr_out_datac), (char*)jdr_out_datac[0]);
        if ($cmp(jdr_out_datac, expect) != 0) {
            fprintf(stderr, "FAIL: %s\n", tc->desc);
            fprintf(stderr, "  input:  %s\n", tc->input);
            fprintf(stderr, "  expect: %s\n", tc->expect);
            fail(RDXBAD);
        }
    }
    done;
}

ok64 STRIPtest() {
    sane(1);
    call(test_strip_trailing_empty);  // Test rdxStrip first (with trailing empty fix)
    call(test_strip_all);             // STRIP writer tests (need separate fix)
    done;
}

TEST(STRIPtest);
