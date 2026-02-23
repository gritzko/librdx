#include "abc/PRO.h"
#include "abc/TEST.h"
#include "rdx/RDX.h"

// Test case: doc JDR, oud JDR, neu JDR, expected patch
typedef struct {
    char const* name;
    char const* doc;    // Original doc with IDs (JDR)
    char const* oud;    // Stripped doc (JDR) - for reference
    char const* neu;    // Desired stripped output (JDR)
    char const* patch;  // Expected patch (JDR)
} diff_test;

static diff_test DIFF_TESTS[] = {
    // Single element unchanged (using EULER for ID-based merge)
    {
        .name = "single_unchanged",
        .doc = "{1@0-2}",
        .oud = "{1}",
        .neu = "{1}",
        .patch = "{}",  // empty patch - no changes
    },
    // Single element changed: tombstone old + insert new
    {
        .name = "single_changed",
        .doc = "{1@0-2}",
        .oud = "{1}",
        .neu = "{2}",
        .patch = "{1@3,2@4}",  // tombstone 1@3, new value 2@4
    },
    // Two elements, second changed: tombstone old + insert new
    {
        .name = "two_second_changed",
        .doc = "{1@0-2,2@0-4}",
        .oud = "{1,2}",
        .neu = "{1,3}",
        .patch = "{2@5,3@6}",  // tombstone 2@5, new value 3@6
    },
    // EULER: all elements differ - DEL+INS not REPLACE (keys differ)
    {
        .name = "euler_all_different",
        .doc = "{1@0-2}",
        .oud = "{1}",
        .neu = "{2,3}",
        .patch = "{1@3,2@4,3@6}",  // tombstone 1, insert 2 and 3
    },
    // TUPLE: single element unchanged (key has no ID)
    {
        .name = "tuple_unchanged",
        .doc = "(1)",
        .oud = "(1)",
        .neu = "(1)",
        .patch = "(1)",  // unchanged element preserved
    },
    // TUPLE: two elements, second changed (key has no ID, value has ID)
    {
        .name = "tuple_second_changed",
        .doc = "(1,2@0-4)",
        .oud = "(1,2)",
        .neu = "(1,3)",
        .patch = "(1,3@6)",  // unchanged key + new value with seq 6 > 4
    },
    // TUPLE: trailing element deleted (strip discards trailing tombstone)
    {
        .name = "tuple_trailing_del",
        .doc = "(1,2@0-4,3@0-6)",
        .oud = "(1,2,3)",
        .neu = "(1,2)",
        .patch = "(1,2@4,3@7)",  // unchanged + unchanged + tombstone
    },
    // TUPLE: duplicate values with multiple replacements (fuzz crash)
    // Tests position-ordered comparison when same value appears twice
    {
        .name = "tuple_dup_values_replace",
        .doc = "(33,-88,33)",
        .oud = "(33,-88,33)",
        .neu = "(33,-13,-3)",
        .patch = "(33,-13@2,-3@2)",  // EQ + REPLACE + REPLACE
    },
    // EULER containing tuple: key changed = replace whole tuple
    // Note: tuple key (position 0) has no ID, only the tuple itself does
    {
        .name = "euler_tuple_key_changed",
        .doc = "{(@0-6 1,2@0-4)}",  // stamp on tuple, no stamp on key
        .oud = "{(1,2)}",
        .neu = "{(3,4)}",
        .patch = "{(@7 1,2@4),(@8 3,4)}",  // tombstone old + insert new
    },

    // LINEAR (array) tests - position-based ordering
    // Position = seq >> 1, tombstone = seq & 1
    // Use seq gaps to allow insertions between elements

    // LINEAR: single element unchanged
    {
        .name = "linear_unchanged",
        .doc = "[@0-100 1@0-100]",
        .oud = "[1]",
        .neu = "[1]",
        .patch = "[@100 ]",  // no change - empty container
    },
    // LINEAR: single element changed (tombstone + append)
    {
        .name = "linear_changed",
        .doc = "[@0-100 1@0-100]",
        .oud = "[1]",
        .neu = "[2]",
        .patch = "[@100 1@101,2@102]",  // tombstone + new at next position
    },
    // LINEAR: append element
    {
        .name = "linear_append",
        .doc = "[@0-100 1@0-100]",
        .oud = "[1]",
        .neu = "[1,2]",
        .patch = "[@100 2@102]",  // only new element
    },
    // LINEAR: delete last element
    {
        .name = "linear_delete_last",
        .doc = "[@0-100 1@0-100,2@0-200]",
        .oud = "[1,2]",
        .neu = "[1]",
        .patch = "[@100 2@201]",  // only tombstone
    },
    // LINEAR: insert between elements using seq gap
    // Using Ron60: 8=8, A=10, C=12
    // [ 1@8, 10@C ] -> [ 1@8, 6@A, 10@C ]
    {
        .name = "linear_insert_middle",
        .doc = "[@0-C 1@0-8,10@0-C]",
        .oud = "[1,10]",
        .neu = "[1,6,10]",
        .patch =
            "[@C 6@A]",  // only new element at seq A=10 (between 8 and C=12)
    },
    // LINEAR: insert at beginning (before first element)
    // Using Ron60: 2=2, 4=4, 6=6
    {
        .name = "linear_insert_front",
        .doc = "[@0-6 1@0-6]",
        .oud = "[1]",
        .neu = "[0,1]",
        .patch = "[@6 0@2]",  // only new element at seq 2 (before 6)
    },
    // LINEAR: multiple inserts using gaps
    // Using Ron60: 4=4, 8=8, C=12
    {
        .name = "linear_multi_insert",
        .doc = "[@0-4 1@0-4,3@0-C]",
        .oud = "[1,3]",
        .neu = "[1,2,3]",
        .patch = "[@4 2@8]",  // only new element at seq 8 (between 4 and C=12)
    },

    // MULTIX (counter/version vector) tests - keyed by src
    // Stripped form keeps src, zeros seq: <1@alice-2> -> <1@alice-0>
    // JDR stamp format: @src-seq (both required for MULTIX children)
    // Merge by src: only changed/new slots in patch

    // MULTIX: single element unchanged
    {
        .name = "multix_unchanged",
        .doc = "<1@alice-2>",
        .oud = "<1@alice-0>",
        .neu = "<1@alice-0>",
        .patch = "<>",  // empty patch - no changes
    },
    // MULTIX: single element value changed
    {
        .name = "multix_changed",
        .doc = "<1@alice-2>",
        .oud = "<1@alice-0>",
        .neu = "<2@alice-0>",
        .patch = "<2@alice-4>",  // new value at alice slot with higher seq
    },
    // MULTIX: two elements, second changed
    // Using 'a' and 'b' as src names - RON60('a')=36, RON60('b')=37 so a < b
    {
        .name = "multix_second_changed",
        .doc = "<1@a-2,3@b-4>",
        .oud = "<1@a-0,3@b-0>",
        .neu = "<1@a-0,5@b-0>",
        .patch = "<5@b-6>",  // only changed b slot
    },
    // MULTIX: add new slot
    {
        .name = "multix_add_slot",
        .doc = "<1@a-2>",
        .oud = "<1@a-0>",
        .neu = "<1@a-0,2@b-0>",
        .patch = "<2@b-4>",  // only new slot needed (merge by src)
    },
    // ROOT: trivial
    {
        .name = "root_append",
        .doc = "1,2",
        .oud = "1,2",
        .neu = "1,2,3",
        .patch = "1,2,3",
    },
    // ROOT: replace
    {
        .name = "root_replace",
        .doc = "1,2@2,3",
        .oud = "1,2,3",
        .neu = "1,22,33",
        .patch = "1,22@4,33@2",
    },
    // TUPLE: tombstone resurrection (fuzz crash)
    // doc has tombstone at position 1, neu resurrects it
    // In TUPLE, tombstones become () to preserve positions
    {
        .name = "tuple_tombstone_resurrect",
        .doc = "(1,2@3)",    // position 1 has seq=3 (odd = tombstone)
        .oud = "(1,)",       // tombstone stripped to () - position preserved
        .neu = "(1,3)",      // resurrect position 1 with value 3
        .patch = "(1,3@4)",  // seq=4 beats tombstone seq=3
    },
    // TUPLE: structure change - container to FIRST (fuzz crash)
    // doc has array at position 1, neu has FIRST value
    {
        .name = "tuple_container_to_first",
        .doc = "(1,[2])",
        .oud = "(1,[2])",
        .neu = "(1,3)",      // array becomes integer
        .patch = "(1,3@2)",  // REPLACE container with FIRST
    },
    // TUPLE: structure change - FIRST to container (fuzz crash variant)
    {
        .name = "tuple_first_to_container",
        .doc = "(1,2)",
        .oud = "(1,2)",
        .neu = "(1,[3])",       // integer becomes array
        .patch = "(1,[@2 3])",  // REPLACE FIRST with container
    },
    // TUPLE: empty container to FIRST (fuzz crash cc5272e1)
    // doc has empty [] at position 1, neu has value
    {
        .name = "tuple_empty_container_to_first",
        .doc = "(1,[],3)",
        .oud = "(1,[],3)",
        .neu = "(1,2,3)",      // [] becomes 2
        .patch = "(1,2@2,3)",  // REPLACE [] with 2
    },
    // TUPLE: fewer elements in neu than doc (fuzz crash)
    {
        .name = "tuple_truncate",
        .doc = "(1,2,3,4)",
        .oud = "(1,2,3,4)",
        .neu = "(1,2)",            // remove trailing elements
        .patch = "(1,2,3@1,4@1)",  // tombstones at positions 2 and 3
    },
    // TUPLE: REF value change - tombstone bit is stripped, so 2-1 == 2-0
    // Test actual REF change (different src)
    {
        .name = "tuple_ref_value_change",
        .doc = "(1-0,2-0)",
        .oud = "(1-0,2-0)",
        .neu = "(1-0,3-0)",      // 2-0 -> 3-0 (ref src changed)
        .patch = "(1-0,3-0@2)",  // REPLACE with new ref
    },
    // TUPLE: simple int change (sanity check)
    {
        .name = "tuple_simple_int_change",
        .doc = "(1,2,3)",
        .oud = "(1,2,3)",
        .neu = "(1,5,3)",  // 2 -> 5 at position 1
        .patch = "(1,5@2,3)",
    },

    // ========================================================================
    // Fuzz crash regression tests (crashes fixed in parallel iteration logic)
    // ========================================================================

    // Fuzz crash 294a84b4: trailing empty tuples in doc
    // rdxStrip strips trailing empty tuples from oud, doc keeps them
    {
        .name = "fuzz_trailing_empty_tuples",
        .doc = "(1,2,())",    // trailing empty tuple
        .oud = "(1,2)",       // stripped - empty tuple removed
        .neu = "(1,2)",       // same as oud
        .patch = "(1,2,())",  // empty tuple preserved (unchanged)
    },
    // Fuzz crash 2a3d0643: tombstoned element handling
    // In LEX containers, tombstones are removed from oud entirely
    {
        .name = "fuzz_lex_tombstone_skip",
        .doc =
            "{1@0-2,2@0-3,3@0-4}",  // middle element is tombstone (seq=3, odd)
        .oud = "{1,3}",             // tombstone 2@3 removed
        .neu = "{1,3}",             // same - no change
        .patch = "{}",              // empty patch
    },
    // Fuzz crash 1a90e06d: type mismatch during descent
    // oud and neu have same position but different types - don't descend
    {
        .name = "fuzz_type_mismatch_no_descent",
        .doc = "(1,(2,3))",
        .oud = "(1,(2,3))",
        .neu = "(1,4)",      // nested tuple replaced with int
        .patch = "(1,4@2)",  // REPLACE - don't descend into mismatched types
    },
    // Fuzz crash 6a9f6583: tombstoned nested container
    // doc has tombstoned tuple at position 2 - rdxStrip clears it
    {
        .name = "fuzz_tombstoned_nested_container",
        .doc = "(1,(@0-1 2,3))",  // nested tuple is tombstone (seq=1, odd)
        .oud = "(1)",             // tombstone cleared, trailing removed
        .neu = "(1)",             // stripped version (same as oud)
        .patch = "(1)",           // no change - both strip to (1)
    },
    // Fuzz crash: interspersed empty tuples and tombstones in trailing elements
    // rdxStrip removes both, doc keeps them - must handle in single loop
    {
        .name = "fuzz_interspersed_empty_tomb",
        .doc = "(1,(),(@0-1 2))",  // pos 1 = empty, pos 2 = tombstone
        .oud = "(1)",              // both removed by strip
        .neu = "(1)",              // same as oud
        .patch =
            "(1,())",  // empty tuple unnecessarily preserved, tombstone skipped
    },

    // ========================================================================
    // EULER insert-in-middle with nested tuples (Debian packages bug)
    // When inserting in middle, unchanged entries after insertion should NOT
    // appear in the diff
    // ========================================================================

    // EULER: insert nested tuple in middle - unchanged entry should not appear
    {
        .name = "euler_nested_insert_middle",
        .doc = "{(@0-2 \"a\",{(\"x\",1)}),(@0-6 \"c\",{(\"x\",3)})}",
        .oud = "{(\"a\",{(\"x\",1)}),(\"c\",{(\"x\",3)})}",
        .neu = "{(\"a\",{(\"x\",1)}),(\"b\",{(\"x\",2)}),(\"c\",{(\"x\",3)})}",
        .patch = "{(@4 \"b\",{(\"x\",2)})}",  // only "b" - "c" unchanged
    },
    // EULER: insert at beginning - entries after should not appear
    {
        .name = "euler_nested_insert_front",
        .doc = "{(@0-4 \"b\",{(\"x\",2)}),(@0-6 \"c\",{(\"x\",3)})}",
        .oud = "{(\"b\",{(\"x\",2)}),(\"c\",{(\"x\",3)})}",
        .neu = "{(\"a\",{(\"x\",1)}),(\"b\",{(\"x\",2)}),(\"c\",{(\"x\",3)})}",
        .patch = "{(@2 \"a\",{(\"x\",1)})}",  // only "a"
    },
    // EULER: value modified in nested EULER (Debian packages bug)
    // Same key "a", but nested value {"x",1} -> {"x",2}
    {
        .name = "euler_nested_value_modified",
        .doc = "{(@0-2 \"a\",{(@0-2 \"x\",1@0-2)})}",
        .oud = "{(\"a\",{(\"x\",1)})}",
        .neu = "{(\"a\",{(\"x\",2)})}",
        .patch = "{(@2 \"a\",{(@2 \"x\",2@4)})}",  // parent gets same ID, value updated
    },
    // EULER: multiple packages, one value modified (Debian scenario)
    {
        .name = "euler_multi_one_modified",
        .doc = "{(@0-2 \"a\",{(@0-2 \"v\",1@0-2)}),(@0-4 \"b\",{(@0-2 \"v\",2@0-2)}),(@0-6 \"c\",{(@0-2 \"v\",3@0-2)})}",
        .oud = "{(\"a\",{(\"v\",1)}),(\"b\",{(\"v\",2)}),(\"c\",{(\"v\",3)})}",
        .neu = "{(\"a\",{(\"v\",1)}),(\"b\",{(\"v\",9)}),(\"c\",{(\"v\",3)})}",
        .patch = "{(@4 \"b\",{(@2 \"v\",9@4)})}",  // only "b" changed
    },
};

#define DIFF_TEST_COUNT (sizeof(DIFF_TESTS) / sizeof(DIFF_TESTS[0]))

ok64 DIFFTestOne(diff_test const* t) {
    sane(t);
    fprintf(stderr, "  %s\n", t->name);

    // Parse and verify doc
    a_cstr(doc_jdr, t->doc);
    rdx doc = {.format = RDX_FMT_JDR};
    doc.next = *doc_jdr;
    doc.opt = (u8p)doc_jdr[1];

    // Convert to TLV and verify
    a_pad(u8, doc_tlv, PAGESIZE);
    rdx doc_write = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
    doc_write.bulk = doc_tlv;
    call(rdxCopy, &doc_write, &doc);

    rdx doc_verify = {.format = RDX_FMT_TLV};
    doc_verify.next = doc_tlv[1];
    doc_verify.opt = (u8p)doc_tlv[2];
    doc_verify.bulk = NULL;
    ok64 v = rdxVerifyAll(&doc_verify);
    if (v != OK) {
        fprintf(stderr, "doc verify failed: %s\n", ok64str(v));
        return v;
    }

    // Use TLV doc for diffing
    rdx doc_read = {.format = RDX_FMT_TLV};
    doc_read.next = doc_tlv[1];
    doc_read.opt = (u8p)doc_tlv[2];
    doc_read.bulk = NULL;

    // Parse neu
    a_cstr(neu_jdr, t->neu);
    rdx neu = {.format = RDX_FMT_JDR};
    neu.next = *neu_jdr;
    neu.opt = (u8p)neu_jdr[1];

    // Allocate patch buffer
    a_pad(u8, patch_buf, PAGESIZE);
    rdx patch = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
    patch.bulk = patch_buf;

    // Compute diff
    ok64 o = rdxDiff(&patch, &doc_read, &neu);
    if (o != OK) {
        fprintf(stderr, "rdxDiff failed: %s\n", ok64str(o));
        return o;
    }

    // Convert patch to JDR and verify against expected
    a_pad(u8, patch_jdr_buf, PAGESIZE);
    rdx patch_read = {.format = RDX_FMT_TLV};
    patch_read.next = patch_buf[1];
    patch_read.opt = (u8p)patch_buf[2];
    patch_read.bulk = NULL;
    rdx jdr_write = {};
    rdxWriteInit(&jdr_write, RDX_FMT_JDR, patch_jdr_buf);
    call(rdxCopy, &jdr_write, &patch_read);

    // Verify patch matches expected
    if (t->patch) {
        a_cstr(expect_patch, t->patch);
        if ($cmp(patch_jdr_buf_datac, expect_patch) != 0) {
            fprintf(stderr, "    PATCH MISMATCH:\n");
            fprintf(stderr, "      expected: %s\n", t->patch);
            fprintf(stderr, "      got:      %.*s\n",
                    (int)$len(patch_jdr_buf_datac),
                    (char*)*patch_jdr_buf_datac);
            return RDXBAD;
        }
    }

    // Verify invariant: strip(merge(doc, patch)) == neu
    // Step 1: merge(doc, patch)
    a_pad(u8, merged_buf, PAGESIZE);
    a_pad(rdx, inputs, 16);
    rdxp inp = NULL;

    // Add doc as input (use TLV)
    call(rdxbFedP, inputs, &inp);
    inp->format = RDX_FMT_TLV;
    inp->next = doc_tlv[1];
    inp->opt = (u8p)doc_tlv[2];
    inp->bulk = NULL;

    // Add patch as input
    call(rdxbFedP, inputs, &inp);
    inp->format = RDX_FMT_TLV;
    inp->next = patch_buf[1];
    inp->opt = (u8p)patch_buf[2];
    inp->bulk = NULL;

    rdx merged = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
    merged.bulk = merged_buf;
    o = rdxMerge(&merged, rdxbDataIdle(inputs));
    if (o != OK) {
        fprintf(stderr, "merge failed: %s\n", ok64str(o));
        return o;
    }

    // Step 2: strip(merged)
    a_pad(u8, stripped_buf, PAGESIZE);
    rdx merged_read = {.format = RDX_FMT_TLV};
    merged_read.next = merged_buf[1];
    merged_read.opt = (u8p)merged_buf[2];
    merged_read.bulk = NULL;
    rdx stripped = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
    stripped.bulk = stripped_buf;
    call(rdxStrip, &stripped, &merged_read);

    // Step 3: Convert to JDR and compare with neu
    a_pad(u8, result_jdr, PAGESIZE);
    rdx stripped_read = {.format = RDX_FMT_TLV};
    stripped_read.next = stripped_buf[1];
    stripped_read.opt = (u8p)stripped_buf[2];
    stripped_read.bulk = NULL;
    rdx result_write = {};
    rdxWriteInit(&result_write, RDX_FMT_JDR, result_jdr);
    call(rdxCopy, &result_write, &stripped_read);

    a_cstr(expect_neu, t->neu);
    if ($cmp(result_jdr_datac, expect_neu) != 0) {
        fprintf(stderr, "INVARIANT FAILED: strip(merge(doc,patch)) != neu\n");
        fprintf(stderr, "      expected: %s\n", t->neu);
        fprintf(stderr, "      got:      %.*s\n", (int)$len(result_jdr_datac),
                (char*)*result_jdr_datac);
        return RDXBAD;
    }

    done;
}

ok64 DIFFtest() {
    sane(1);
    fprintf(stderr, "DIFF tests:\n");
    for (u32 i = 0; i < DIFF_TEST_COUNT; i++) {
        call(DIFFTestOne, &DIFF_TESTS[i]);
    }
    done;
}

TEST(DIFFtest);
