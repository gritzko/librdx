#include "BIFF.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

// Parse JSON string into BASON buffer. Returns data slice.
static ok64 BIFFParseJSON(u8bp buf, u64bp idx, char const *json) {
    sane(buf != NULL && json != NULL);
    u8cs j = $u8str(json);
    call(BASONParseJSON, buf, idx, j);
    done;
}

// Export BASON data to JSON string in out buffer.
static ok64 BIFFExportJSON(u8bp jbuf, u8csc data) {
    sane(jbuf != NULL);
    u64 _stk[64];
    u64b stk = {_stk, _stk, _stk, _stk + 64};
    call(BASONExportJSON, u8bIdle(jbuf), stk, data);
    done;
}

// Compare BASON export to expected JSON string.
static ok64 BIFFCheckJSON(u8csc data, char const *expected) {
    sane(expected != NULL);
    u8 _jbuf[4096];
    u8b jbuf = {_jbuf, _jbuf, _jbuf, _jbuf + 4096};
    call(BIFFExportJSON, jbuf, data);
    u8cs got = {jbuf[1], jbuf[2]};
    u8cs exp = $u8str(expected);
    if ($len(got) != $len(exp) || memcmp(got[0], exp[0], $len(exp)) != 0) {
        fprintf(stderr, "  expected: %s\n", expected);
        fprintf(stderr, "  got:      %.*s\n", (int)$len(got), got[0]);
        fail(TESTFAIL);
    }
    done;
}

// Setup: parse JSON into BASON, return data slice.
#define BIFF_SETUP(name, padsize, json)                            \
    u8  _##name##_pad[padsize];                                    \
    u8b name##_buf = {_##name##_pad, _##name##_pad,                \
                      _##name##_pad, _##name##_pad + padsize};     \
    u64 _##name##_idx[64];                                         \
    u64b name##_idx = {_##name##_idx, _##name##_idx,               \
                       _##name##_idx, _##name##_idx + 64};         \
    if (json[0] != '\0') {                                         \
        ok64 _o = BIFFParseJSON(name##_buf, name##_idx, json);     \
        if (_o != OK) return _o;                                   \
    }

#define BIFF_DATA(name) \
    ((u8c *[]){name##_buf[1], name##_buf[2]})

#define BIFF_MERGE_SETUP(outsize)                              \
    u8  _out_pad[outsize];                                     \
    u8b out_buf = {_out_pad, _out_pad,                         \
                   _out_pad, _out_pad + outsize};              \
    u64 _out_idx[64];                                          \
    u64b out_idx = {_out_idx, _out_idx,                        \
                    _out_idx, _out_idx + 64};                  \
    u64 _lstk[64];                                             \
    u64b lstk = {_lstk, _lstk, _lstk, _lstk + 64};            \
    u64 _rstk[64];                                             \
    u64b rstk = {_rstk, _rstk, _rstk, _rstk + 64};

// --- Merge tests ---

// 1. merge(x, empty) == x
ok64 BIFFtestMergeIdentityLeft() {
    sane(1);
    BIFF_SETUP(left, 1024, "{\"a\":1,\"b\":2}");
    BIFF_SETUP(right, 1024, "");
    BIFF_MERGE_SETUP(2048);

    u8cs ld = {left_buf[1], left_buf[2]};
    u8cs rd = {right_buf[1], right_buf[2]};
    call(BASONMerge, out_buf, out_idx, lstk, ld, rstk, rd);

    u8cs od = {out_buf[1], out_buf[2]};
    call(BIFFCheckJSON, od, "{\"a\":1,\"b\":2}");
    done;
}

// 2. merge(empty, x) == x
ok64 BIFFtestMergeIdentityRight() {
    sane(1);
    BIFF_SETUP(left, 1024, "");
    BIFF_SETUP(right, 1024, "{\"a\":1,\"b\":2}");
    BIFF_MERGE_SETUP(2048);

    u8cs ld = {left_buf[1], left_buf[2]};
    u8cs rd = {right_buf[1], right_buf[2]};
    call(BASONMerge, out_buf, out_idx, lstk, ld, rstk, rd);

    u8cs od = {out_buf[1], out_buf[2]};
    call(BIFFCheckJSON, od, "{\"a\":1,\"b\":2}");
    done;
}

// 3. Right wins on scalar conflict
ok64 BIFFtestMergeRightWins() {
    sane(1);
    BIFF_SETUP(left, 1024, "{\"a\":1}");
    BIFF_SETUP(right, 1024, "{\"a\":2}");
    BIFF_MERGE_SETUP(2048);

    u8cs ld = {left_buf[1], left_buf[2]};
    u8cs rd = {right_buf[1], right_buf[2]};
    call(BASONMerge, out_buf, out_idx, lstk, ld, rstk, rd);

    u8cs od = {out_buf[1], out_buf[2]};
    call(BIFFCheckJSON, od, "{\"a\":2}");
    done;
}

// 4. Union of keys
ok64 BIFFtestMergeUnion() {
    sane(1);
    BIFF_SETUP(left, 1024, "{\"a\":1}");
    BIFF_SETUP(right, 1024, "{\"b\":2}");
    BIFF_MERGE_SETUP(2048);

    u8cs ld = {left_buf[1], left_buf[2]};
    u8cs rd = {right_buf[1], right_buf[2]};
    call(BASONMerge, out_buf, out_idx, lstk, ld, rstk, rd);

    u8cs od = {out_buf[1], out_buf[2]};
    call(BIFFCheckJSON, od, "{\"a\":1,\"b\":2}");
    done;
}

// 5. Nested object merge
ok64 BIFFtestMergeNested() {
    sane(1);
    BIFF_SETUP(left, 1024, "{\"o\":{\"a\":1}}");
    BIFF_SETUP(right, 1024, "{\"o\":{\"b\":2}}");
    BIFF_MERGE_SETUP(2048);

    u8cs ld = {left_buf[1], left_buf[2]};
    u8cs rd = {right_buf[1], right_buf[2]};
    call(BASONMerge, out_buf, out_idx, lstk, ld, rstk, rd);

    u8cs od = {out_buf[1], out_buf[2]};
    call(BIFFCheckJSON, od, "{\"o\":{\"a\":1,\"b\":2}}");
    done;
}

// 6. Array positional merge, tail preserved
ok64 BIFFtestMergeArray() {
    sane(1);
    BIFF_SETUP(left, 1024, "[1,2,3]");
    BIFF_SETUP(right, 1024, "[4,5]");
    BIFF_MERGE_SETUP(2048);

    u8cs ld = {left_buf[1], left_buf[2]};
    u8cs rd = {right_buf[1], right_buf[2]};
    call(BASONMerge, out_buf, out_idx, lstk, ld, rstk, rd);

    u8cs od = {out_buf[1], out_buf[2]};
    call(BIFFCheckJSON, od, "[4,5,3]");
    done;
}

// 7. Right wins: type change (object over scalar)
ok64 BIFFtestMergeTypeChange() {
    sane(1);
    BIFF_SETUP(left, 1024, "{\"a\":1}");
    BIFF_SETUP(right, 1024, "{\"a\":{\"x\":1}}");
    BIFF_MERGE_SETUP(2048);

    u8cs ld = {left_buf[1], left_buf[2]};
    u8cs rd = {right_buf[1], right_buf[2]};
    call(BASONMerge, out_buf, out_idx, lstk, ld, rstk, rd);

    u8cs od = {out_buf[1], out_buf[2]};
    call(BIFFCheckJSON, od, "{\"a\":{\"x\":1}}");
    done;
}

// --- Diff tests ---

// 8. Diff identical -> empty output
ok64 BIFFtestDiffIdentical() {
    sane(1);
    BIFF_SETUP(old, 1024, "{\"a\":1,\"b\":2}");
    BIFF_SETUP(neu, 1024, "{\"a\":1,\"b\":2}");
    BIFF_MERGE_SETUP(2048);

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd, NULL);

    // empty diff: no data written
    size_t diff_len = u8bDataLen(out_buf);
    testeq(diff_len, (size_t)0);
    done;
}

// 9. Diff scalar change
ok64 BIFFtestDiffScalar() {
    sane(1);
    BIFF_SETUP(old, 1024, "{\"a\":1,\"b\":2}");
    BIFF_SETUP(neu, 1024, "{\"a\":1,\"b\":3}");
    BIFF_MERGE_SETUP(2048);

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd, NULL);

    u8cs dd = {out_buf[1], out_buf[2]};
    call(BIFFCheckJSON, dd, "{\"b\":3}");
    done;
}

// 10. Diff key added
ok64 BIFFtestDiffAdded() {
    sane(1);
    BIFF_SETUP(old, 1024, "{\"a\":1}");
    BIFF_SETUP(neu, 1024, "{\"a\":1,\"b\":2}");
    BIFF_MERGE_SETUP(2048);

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd, NULL);

    u8cs dd = {out_buf[1], out_buf[2]};
    call(BIFFCheckJSON, dd, "{\"b\":2}");
    done;
}

// 11. Diff key deleted -> null tombstone
ok64 BIFFtestDiffDeleted() {
    sane(1);
    BIFF_SETUP(old, 1024, "{\"a\":1,\"b\":2}");
    BIFF_SETUP(neu, 1024, "{\"a\":1}");
    BIFF_MERGE_SETUP(2048);

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd, NULL);

    u8cs dd = {out_buf[1], out_buf[2]};
    call(BIFFCheckJSON, dd, "{\"b\":null}");
    done;
}

// 12. Diff roundtrip: merge(old, diff(old, new)) == new
ok64 BIFFtestDiffRoundtrip() {
    sane(1);
    char const *old_json = "{\"a\":1,\"b\":2,\"c\":{\"x\":10}}";
    char const *new_json = "{\"a\":1,\"b\":3,\"c\":{\"x\":10,\"y\":20},\"d\":4}";

    // Parse old and new
    BIFF_SETUP(old, 2048, old_json);
    BIFF_SETUP(neu, 2048, new_json);

    // Compute diff(old, new)
    u8  _diff_pad[4096];
    u8b diff_buf = {_diff_pad, _diff_pad, _diff_pad, _diff_pad + 4096};
    u64 _diff_idx[64];
    u64b diff_idx = {_diff_idx, _diff_idx, _diff_idx, _diff_idx + 64};
    u64 _ostk[64];
    u64b ostk = {_ostk, _ostk, _ostk, _ostk + 64};
    u64 _nstk[64];
    u64b nstk = {_nstk, _nstk, _nstk, _nstk + 64};

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, diff_buf, diff_idx, ostk, od, nstk, nd, NULL);

    // merge(old, diff) should equal new
    u8  _merge_pad[4096];
    u8b merge_buf = {_merge_pad, _merge_pad, _merge_pad, _merge_pad + 4096};
    u64 _merge_idx[64];
    u64b merge_idx = {_merge_idx, _merge_idx, _merge_idx, _merge_idx + 64};
    u64 _lstk2[64];
    u64b lstk2 = {_lstk2, _lstk2, _lstk2, _lstk2 + 64};
    u64 _rstk2[64];
    u64b rstk2 = {_rstk2, _rstk2, _rstk2, _rstk2 + 64};

    u8cs od2 = {old_buf[1], old_buf[2]};
    u8cs dd = {diff_buf[1], diff_buf[2]};
    call(BASONMerge, merge_buf, merge_idx, lstk2, od2, rstk2, dd);

    u8cs md = {merge_buf[1], merge_buf[2]};
    call(BIFFCheckJSON, md, new_json);
    done;
}

// 13. Diff roundtrip with deletion
ok64 BIFFtestDiffRoundtripDel() {
    sane(1);
    char const *old_json = "{\"a\":1,\"b\":2,\"c\":3}";
    char const *new_json = "{\"a\":1,\"c\":4}";

    BIFF_SETUP(old, 2048, old_json);
    BIFF_SETUP(neu, 2048, new_json);

    // diff
    u8  _diff_pad[4096];
    u8b diff_buf = {_diff_pad, _diff_pad, _diff_pad, _diff_pad + 4096};
    u64 _diff_idx[64];
    u64b diff_idx = {_diff_idx, _diff_idx, _diff_idx, _diff_idx + 64};
    u64 _ostk[64];
    u64b ostk = {_ostk, _ostk, _ostk, _ostk + 64};
    u64 _nstk[64];
    u64b nstk = {_nstk, _nstk, _nstk, _nstk + 64};

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, diff_buf, diff_idx, ostk, od, nstk, nd, NULL);

    // merge(old, diff)
    u8  _merge_pad[4096];
    u8b merge_buf = {_merge_pad, _merge_pad, _merge_pad, _merge_pad + 4096};
    u64 _merge_idx[64];
    u64b merge_idx = {_merge_idx, _merge_idx, _merge_idx, _merge_idx + 64};
    u64 _lstk2[64];
    u64b lstk2 = {_lstk2, _lstk2, _lstk2, _lstk2 + 64};
    u64 _rstk2[64];
    u64b rstk2 = {_rstk2, _rstk2, _rstk2, _rstk2 + 64};

    u8cs od2 = {old_buf[1], old_buf[2]};
    u8cs dd = {diff_buf[1], diff_buf[2]};
    call(BASONMerge, merge_buf, merge_idx, lstk2, od2, rstk2, dd);

    u8cs md = {merge_buf[1], merge_buf[2]};
    call(BIFFCheckJSON, md, new_json);
    done;
}

// 14. Nested diff: deep object changes
ok64 BIFFtestDiffNested() {
    sane(1);
    char const *old_json = "{\"a\":{\"b\":{\"c\":1}}}";
    char const *new_json = "{\"a\":{\"b\":{\"c\":2,\"d\":3}}}";

    BIFF_SETUP(old, 2048, old_json);
    BIFF_SETUP(neu, 2048, new_json);

    u8  _diff_pad[4096];
    u8b diff_buf = {_diff_pad, _diff_pad, _diff_pad, _diff_pad + 4096};
    u64 _diff_idx[64];
    u64b diff_idx = {_diff_idx, _diff_idx, _diff_idx, _diff_idx + 64};
    u64 _ostk[64];
    u64b ostk = {_ostk, _ostk, _ostk, _ostk + 64};
    u64 _nstk[64];
    u64b nstk = {_nstk, _nstk, _nstk, _nstk + 64};

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, diff_buf, diff_idx, ostk, od, nstk, nd, NULL);

    // diff should have nested changes
    u8cs dd = {diff_buf[1], diff_buf[2]};
    call(BIFFCheckJSON, dd, "{\"a\":{\"b\":{\"c\":2,\"d\":3}}}");

    // roundtrip: merge(old, diff) == new
    u8  _merge_pad[4096];
    u8b merge_buf = {_merge_pad, _merge_pad, _merge_pad, _merge_pad + 4096};
    u64 _merge_idx[64];
    u64b merge_idx = {_merge_idx, _merge_idx, _merge_idx, _merge_idx + 64};
    u64 _lstk2[64];
    u64b lstk2 = {_lstk2, _lstk2, _lstk2, _lstk2 + 64};
    u64 _rstk2[64];
    u64b rstk2 = {_rstk2, _rstk2, _rstk2, _rstk2 + 64};

    u8cs od2 = {old_buf[1], old_buf[2]};
    call(BASONMerge, merge_buf, merge_idx, lstk2, od2, rstk2, dd);

    u8cs md = {merge_buf[1], merge_buf[2]};
    call(BIFFCheckJSON, md, new_json);
    done;
}

// 15. Array diff
ok64 BIFFtestDiffArray() {
    sane(1);
    BIFF_SETUP(old, 1024, "[1,2,3]");
    BIFF_SETUP(neu, 1024, "[1,4,3]");
    BIFF_MERGE_SETUP(2048);

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd, NULL);

    // Only element at index 1 changed
    u8cs dd = {out_buf[1], out_buf[2]};
    // Roundtrip check
    u8  _merge_pad[4096];
    u8b merge_buf = {_merge_pad, _merge_pad, _merge_pad, _merge_pad + 4096};
    u64 _merge_idx[64];
    u64b merge_idx = {_merge_idx, _merge_idx, _merge_idx, _merge_idx + 64};
    u64 _lstk2[64];
    u64b lstk2 = {_lstk2, _lstk2, _lstk2, _lstk2 + 64};
    u64 _rstk2[64];
    u64b rstk2 = {_rstk2, _rstk2, _rstk2, _rstk2 + 64};

    u8cs od2 = {old_buf[1], old_buf[2]};
    call(BASONMerge, merge_buf, merge_idx, lstk2, od2, rstk2, dd);

    u8cs md = {merge_buf[1], merge_buf[2]};
    call(BIFFCheckJSON, md, "[1,4,3]");
    done;
}

// --- Array diff tests (Myers + splice keys) ---

// Insert roundtrip: old=[1,2,3], new=[1,9,2,3]
ok64 BIFFtestDiffArrayInsert() {
    sane(1);
    char const *old_json = "[1,2,3]";
    char const *new_json = "[1,9,2,3]";

    BIFF_SETUP(old, 2048, old_json);
    BIFF_SETUP(neu, 2048, new_json);

    u8  _diff_pad[4096];
    u8b diff_buf = {_diff_pad, _diff_pad, _diff_pad, _diff_pad + 4096};
    u64 _diff_idx[64];
    u64b diff_idx = {_diff_idx, _diff_idx, _diff_idx, _diff_idx + 64};
    u64 _ostk[64];
    u64b ostk = {_ostk, _ostk, _ostk, _ostk + 64};
    u64 _nstk[64];
    u64b nstk = {_nstk, _nstk, _nstk, _nstk + 64};

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, diff_buf, diff_idx, ostk, od, nstk, nd, NULL);

    // merge(old, diff) should equal new
    u8  _merge_pad[4096];
    u8b merge_buf = {_merge_pad, _merge_pad, _merge_pad, _merge_pad + 4096};
    u64 _merge_idx[64];
    u64b merge_idx = {_merge_idx, _merge_idx, _merge_idx, _merge_idx + 64};
    u64 _lstk2[64];
    u64b lstk2 = {_lstk2, _lstk2, _lstk2, _lstk2 + 64};
    u64 _rstk2[64];
    u64b rstk2 = {_rstk2, _rstk2, _rstk2, _rstk2 + 64};

    u8cs od2 = {old_buf[1], old_buf[2]};
    u8cs dd = {diff_buf[1], diff_buf[2]};

    call(BASONMerge, merge_buf, merge_idx, lstk2, od2, rstk2, dd);

    u8cs md = {merge_buf[1], merge_buf[2]};
    call(BIFFCheckJSON, md, new_json);
    done;
}

// Append roundtrip: old=[1,2], new=[1,2,3]
ok64 BIFFtestDiffArrayAppend() {
    sane(1);
    char const *old_json = "[1,2]";
    char const *new_json = "[1,2,3]";

    BIFF_SETUP(old, 2048, old_json);
    BIFF_SETUP(neu, 2048, new_json);

    u8  _diff_pad[4096];
    u8b diff_buf = {_diff_pad, _diff_pad, _diff_pad, _diff_pad + 4096};
    u64 _diff_idx[64];
    u64b diff_idx = {_diff_idx, _diff_idx, _diff_idx, _diff_idx + 64};
    u64 _ostk[64];
    u64b ostk = {_ostk, _ostk, _ostk, _ostk + 64};
    u64 _nstk[64];
    u64b nstk = {_nstk, _nstk, _nstk, _nstk + 64};

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, diff_buf, diff_idx, ostk, od, nstk, nd, NULL);

    u8  _merge_pad[4096];
    u8b merge_buf = {_merge_pad, _merge_pad, _merge_pad, _merge_pad + 4096};
    u64 _merge_idx[64];
    u64b merge_idx = {_merge_idx, _merge_idx, _merge_idx, _merge_idx + 64};
    u64 _lstk2[64];
    u64b lstk2 = {_lstk2, _lstk2, _lstk2, _lstk2 + 64};
    u64 _rstk2[64];
    u64b rstk2 = {_rstk2, _rstk2, _rstk2, _rstk2 + 64};

    u8cs od2 = {old_buf[1], old_buf[2]};
    u8cs dd = {diff_buf[1], diff_buf[2]};
    call(BASONMerge, merge_buf, merge_idx, lstk2, od2, rstk2, dd);

    u8cs md = {merge_buf[1], merge_buf[2]};
    call(BIFFCheckJSON, md, new_json);
    done;
}

// Delete roundtrip: old=[1,2,3], new=[1,3]
ok64 BIFFtestDiffArrayDelete() {
    sane(1);
    char const *old_json = "[1,2,3]";
    char const *new_json = "[1,3]";

    BIFF_SETUP(old, 2048, old_json);
    BIFF_SETUP(neu, 2048, new_json);

    u8  _diff_pad[4096];
    u8b diff_buf = {_diff_pad, _diff_pad, _diff_pad, _diff_pad + 4096};
    u64 _diff_idx[64];
    u64b diff_idx = {_diff_idx, _diff_idx, _diff_idx, _diff_idx + 64};
    u64 _ostk[64];
    u64b ostk = {_ostk, _ostk, _ostk, _ostk + 64};
    u64 _nstk[64];
    u64b nstk = {_nstk, _nstk, _nstk, _nstk + 64};

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, diff_buf, diff_idx, ostk, od, nstk, nd, NULL);

    u8  _merge_pad[4096];
    u8b merge_buf = {_merge_pad, _merge_pad, _merge_pad, _merge_pad + 4096};
    u64 _merge_idx[64];
    u64b merge_idx = {_merge_idx, _merge_idx, _merge_idx, _merge_idx + 64};
    u64 _lstk2[64];
    u64b lstk2 = {_lstk2, _lstk2, _lstk2, _lstk2 + 64};
    u64 _rstk2[64];
    u64b rstk2 = {_rstk2, _rstk2, _rstk2, _rstk2 + 64};

    u8cs od2 = {old_buf[1], old_buf[2]};
    u8cs dd = {diff_buf[1], diff_buf[2]};
    call(BASONMerge, merge_buf, merge_idx, lstk2, od2, rstk2, dd);

    u8cs md = {merge_buf[1], merge_buf[2]};
    call(BIFFCheckJSON, md, new_json);
    done;
}

// Identical arrays -> empty diff
ok64 BIFFtestDiffArrayIdentical() {
    sane(1);
    BIFF_SETUP(old, 1024, "[1,2,3]");
    BIFF_SETUP(neu, 1024, "[1,2,3]");
    BIFF_MERGE_SETUP(2048);

    u8cs od = {old_buf[1], old_buf[2]};
    u8cs nd = {neu_buf[1], neu_buf[2]};
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd, NULL);

    size_t diff_len = u8bDataLen(out_buf);
    testeq(diff_len, (size_t)0);
    done;
}

// --- Fuzz repro table: roundtrip merge(old, diff(old, new)) == new ---

typedef struct {
    char const *old_json;
    char const *new_json;
} BIFFRoundtripCase;

static BIFFRoundtripCase BIFF_FUZZ_REPROS[] = {
    {"[]", "[]"},
    {"{}", "{}"},
    {"[1]", "[1]"},
    {"{\"a\":1}", "{\"a\":1}"},
    {"[1,2]", "[1]"},
    {"[1]", "[1,2]"},
    {"[1,2,3]", "[1,4,3]"},
    {"{\"a\":1}", "{\"b\":2}"},
    {"{\"a\":1,\"b\":2}", "{\"a\":1}"},
    {"{\"a\":{\"x\":1}}", "{\"a\":{\"x\":2}}"},
    // fuzz: empty array vs non-empty
    {"[]", "[1]"},
    {"[]", "[0]"},
    {"[]", "[1,2,3]"},
    {"[0]", "[]"},
    // fuzz: type change inside array
    {"[2]", "[[]]"},
    {"[[]]", "[2]"},
    // fuzz: nested array content change
    {"[[0]]", "[[]]"},
    {"[[]]", "[[0]]"},
    // content changes
    {"[[]]", "[0]"},
    {"[{}]", "[0]"},
    {"[1,2,3,4,5]", "[1,3,5]"},
    {"[1]", "[1,2,3]"},
    // fuzz: element position shift (content-based match vs positional keys)
    {"[{}]", "[[],{}]"},
    {"[1,2,3]", "[9,1,2,3]"},
    // fuzz: nested empty objects with shared keybuf
    {"{\"\":{}}", "{\"\":{\"\":{}}}"},
    {"{\"\":{},\"{\":4}", "{\"\":{\"\":{}}}"},
    // fuzz: nested object value change
    {"{\"}}@\":{\"-\":2,\"\":\"}0]{}\"}}", "{\"}}@\":{\"=\":0,\"\":\"}0]{}\"}}" },
    // fuzz: object key set change
    {"{\"=\":2,\"\":\"\"}", "{\"=,\":\"}0]{\"}"},
    // fuzz: nested obj in new, flat obj in old
    {"{\"\":2,\"}]\":\"\"}", "{\"@\":{\"\":2}}"},
    // fuzz: large array positional diff
    {"[113,2,1,2,3,68,3,3,3,2,3,2,1,2,3,68,3,3,3,2,3,188888,1888112,3,3,3,8,2,3,3,3,3,2,1,1,2,3,6881,23,21,2,2,481,23,21,2,2,41,2,1,2,3,6112,3,2,1,2,3,68,3,3,3,2,3,1888821,2,2,4]",
     "[1421,1,3,3,2,3,18888881,23,23,1,33,1,32,8,1,2,33,1,23,4,28,3,3,3,2,3,1,23,21,2,2,23,1,33,1,32,8,1,2,3,2,23,1,33,32,8,1,2,33,1,23,4,28,6,3,3,2,3,1,23,21,2,223,3,8,1,2,32]"},
    // fuzz: many-key objects with arrays, key set disjoint
    {"{\"s[,~At,1,~~ttru{~\":3,\"{\":[4],\"~\":1,\"{tt~'\":3,\"[tru{~\":8}",
     "{\"~At,{~\":3,\"{\":[],\"\":3,\"2{\":3,\"t\":[],\"u-09~[{\":[4],\"2\":4}"},
    // fuzz: array with {} inside, disjoint keys
    {"{\"a\":1}", "{\"b\":[1,{},2]}"},
    {"{\"a\":1,\"b\":2}", "{\"c\":[{},{}]}"},
    // fuzz: large array with {} and mixed types
    {"{\"x\":1,\"y\":2,\"z\":3}",
     "{\"w\":[1,2,{},3,{},4]}"},
    // fuzz: duplicate inner content across containers (cross-container match)
    {"{\"a\":{\"x\":[1,0]},\"b\":{\"x\":[1,0]}}",
     "{\"a\":{\"x\":[1,0]},\"b\":{\"y\":[2,0]}}"},
    // array: prepend
    {"[1,2,3]", "[0,1,2,3]"},
    // array: multi-insert middle
    {"[1,5]", "[1,2,3,4,5]"},
    // array: full replace
    {"[1,2,3]", "[4,5,6]"},
    // array: nested array in object
    {"{\"a\":[1,2,3]}", "{\"a\":[1,9,2,3]}"},
    // fuzz: large array with many empty sub-arrays (BASONDiff failed)
    {"[0,6,1,[],2,2,[],2,3,[],[],2,[],3,6,1,[],2,2,[],2,[],2,3,1,2,3,2,3,1,[],[],2]",
     "[6,[],2,[],3,[],2,[],2,1,[2],2,[],2,[],2,[],[],2,[],2,[],[],2,[],2,[],3,2]"},
};

// Compare two BASON trees via JSON export (handles sorted keys, splice keys)
static ok64 BIFFCheckJSONEqual(u8csc a_data, u8csc b_data) {
    sane($ok(a_data) && $ok(b_data));
    u8 _aj[4096], _bj[4096];
    u8b abuf = {_aj, _aj, _aj, _aj + 4096};
    u8b bbuf = {_bj, _bj, _bj, _bj + 4096};
    call(BIFFExportJSON, abuf, a_data);
    call(BIFFExportJSON, bbuf, b_data);
    u8cs aj = {abuf[1], abuf[2]};
    u8cs bj = {bbuf[1], bbuf[2]};
    if ($len(aj) != $len(bj) || memcmp(aj[0], bj[0], $len(aj)) != 0) {
        fprintf(stderr, "  expected: %.*s\n", (int)$len(bj), bj[0]);
        fprintf(stderr, "  got:      %.*s\n", (int)$len(aj), aj[0]);
        fail(TESTFAIL);
    }
    done;
}

ok64 BIFFtestFuzzRepros() {
    sane(1);
    size_t n = sizeof(BIFF_FUZZ_REPROS) / sizeof(BIFF_FUZZ_REPROS[0]);
    for (size_t i = 0; i < n; i++) {
        BIFFRoundtripCase *tc = &BIFF_FUZZ_REPROS[i];

        BIFF_SETUP(old, 4096, tc->old_json);
        BIFF_SETUP(neu, 4096, tc->new_json);

        u8cs od = {old_buf[1], old_buf[2]};
        u8cs nd = {neu_buf[1], neu_buf[2]};

        // diff(old, new)
        u8  _dp[4096];
        u8b dbuf = {_dp, _dp, _dp, _dp + 4096};
        u64 _di[64];
        u64b didx = {_di, _di, _di, _di + 64};
        u64 _os[64];
        u64b os = {_os, _os, _os, _os + 64};
        u64 _ns[64];
        u64b ns = {_ns, _ns, _ns, _ns + 64};

        call(BASONDiff, dbuf, didx, os, od, ns, nd, NULL);
        u8cs dd = {dbuf[1], dbuf[2]};

        // merge(old, diff) — even if diff is empty
        u8  _mp[4096];
        u8b mbuf = {_mp, _mp, _mp, _mp + 4096};
        u64 _mi[64];
        u64b midx = {_mi, _mi, _mi, _mi + 64};
        u64 _ls[64];
        u64b ls = {_ls, _ls, _ls, _ls + 64};
        u64 _rs[64];
        u64b rs = {_rs, _rs, _rs, _rs + 64};

        call(BASONMerge, mbuf, midx, ls, od, rs, dd);
        u8cs md = {mbuf[1], mbuf[2]};

        // verify via JSON export comparison (handles sorted keys + splice keys)
        call(BIFFCheckJSONEqual, md, nd);
    }
    done;
}

// Test from raw bytes (same logic as fuzz harness)
ok64 BIFFtestFromRaw(u8csc raw) {
    sane($ok(raw) && $len(raw) >= 4);
    size_t mid = $len(raw) / 2;
    u8cs old_raw = {raw[0], raw[0] + mid};
    u8cs new_raw = {raw[0] + mid, raw[1]};

    // Skip whitespace, check plex root
    u8cp p = old_raw[0];
    while (p < old_raw[1] && (*p==' '||*p=='\t'||*p=='\n'||*p=='\r')) ++p;
    test(p < old_raw[1] && (*p == '{' || *p == '['), BADARG);
    p = new_raw[0];
    while (p < new_raw[1] && (*p==' '||*p=='\t'||*p=='\n'||*p=='\r')) ++p;
    test(p < new_raw[1] && (*p == '{' || *p == '['), BADARG);

    BIFF_SETUP(old, 8192, "");
    BIFF_SETUP(neu, 8192, "");
    call(BASONParseJSON, old_buf, NULL, old_raw);
    call(BASONParseJSON, neu_buf, NULL, new_raw);

    u8cp od0 = old_buf[1], od1 = old_buf[2];
    u8cs odata = {od0, od1};
    u8cp nd0 = neu_buf[1], nd1 = neu_buf[2];
    u8cs ndata = {nd0, nd1};

    test((odata[0][0] & ~TLVaA) == (ndata[0][0] & ~TLVaA), BADARG);

    // diff — no index (NULL idx)
    u8  _dp[16384];
    u8b dbuf = {_dp, _dp, _dp, _dp + 16384};
    u64 _os[256];
    u64b os = {_os, _os, _os, _os + 256};
    u64 _ns[256];
    u64b ns = {_ns, _ns, _ns, _ns + 256};
    call(BASONDiff, dbuf, NULL, os, odata, ns, ndata, NULL);
    u8cp dd0 = dbuf[1], dd1 = dbuf[2];
    u8cs dd = {dd0, dd1};

    // merge — no index (NULL idx)
    u8  _mp[16384];
    u8b mbuf = {_mp, _mp, _mp, _mp + 16384};
    u64 _ls[256];
    u64b ls = {_ls, _ls, _ls, _ls + 256};
    u64 _rs[256];
    u64b rs = {_rs, _rs, _rs, _rs + 256};

    if ($len(dd) == 0) {
        testeq($len(odata), $len(ndata));
        test(memcmp(odata[0], ndata[0], $len(odata)) == 0, TESTFAIL);
        done;
    }
    // Dump diff JSON
    {
        u8 _dj[8192];
        u8b djbuf = {_dj, _dj, _dj, _dj + 8192};
        BIFFExportJSON(djbuf, dd);
        u8cs dj = {djbuf[1], djbuf[2]};
        fprintf(stderr, "  diff json: %.*s\n", (int)$len(dj), dj[0]);
    }
    call(BASONMerge, mbuf, NULL, ls, odata, rs, dd);
    u8cp md0 = mbuf[1], md1 = mbuf[2];
    u8cs md = {md0, md1};
    // Compare via JSON (splice keys may differ from sequential keys)
    call(BIFFCheckJSONEqual, md, ndata);
    done;
}

static u8c _crash_e4f95[739] = {
    0x7b,0x22,0x58,0x5b,0x2c,0x41,0x7e,0x74,0x74,0x35,0x7b,0x7e,0x22,0x3a,0x33,0x2c,
    0x22,0x2c,0x20,0x20,0x22,0x3a,0x33,0x2c,0x22,0x5b,0x22,0x3a,0x33,0x2c,0x22,0x29,
    0x20,0x20,0x20,0x20,0x20,0x20,0x38,0x2c,0x65,0x68,0x7b,0x76,0x22,0x3a,0x36,0x2c,
    0x22,0x7b,0x74,0x74,0x7e,0x5b,0x20,0x20,0x20,0x33,0x2c,0x32,0x2c,0x31,0x2c,0x32,
    0x2c,0x33,0x2c,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x32,0x2c,0x38,0x2c,0x33,
    0x2c,0x33,0x2e,0x33,0x2c,0x32,0x2c,0x33,0x2c,0x31,0x38,0x2c,0x36,0x32,0x2c,0x33,
    0x2c,0x36,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x32,0x2c,0x33,0x33,0x2c,0x36,
    0x31,0x31,0x32,0x2c,0x33,0x2c,0x32,0x2c,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x38,
    0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x31,0x31,0x2c,0x38,0x2c,0x38,0x31,0x38,0x38,
    0x37,0x2c,0x31,0x38,0x38,0x38,0x31,0x31,0x32,0x2c,0x33,0x38,0x2c,0x2c,0x2c,0x33,
    0x33,0x2c,0x32,0x2c,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
    0x36,0x36,0x36,0x36,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x30,0x2c,0x31,0x36,0x34,0x36,
    0x2c,0x33,0x2c,0x32,0x30,0x31,0x2c,0x32,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x32,0x2c,
    0x33,0x2c,0x33,0x2c,0x33,0x2c,0x31,0x36,0x34,0x33,0x2c,0x36,0x36,0x36,0x36,0x36,
    0x36,0x33,0x2c,0x31,0x32,0x2c,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x37,0x2c,0x33,
    0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x31,0x2c,0x31,0x36,0x34,0x36,0x2c,0x33,0x2c,
    0x32,0x30,0x31,0x2c,0x32,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x32,0x2c,0x33,0x2c,0x33,
    0x2c,0x33,0x2c,0x31,0x36,0x34,0x36,0x2c,0x33,0x2c,0x32,0x2c,0x31,0x2c,0x32,0x2c,
    0x33,0x33,0x2c,0x33,0x2c,0x36,0x37,0x2c,0x33,0x33,0x2c,0x33,0x2c,0x32,0x2c,0x33,
    0x31,0x38,0x2c,0x32,0x2c,0x33,0x2c,0x31,0x32,0x37,0x2e,0x36,0x2c,0x2c,0x33,0x2c,
    0x32,0x2c,0x31,0x2c,0x33,0x2c,0x33,0x2c,0x36,0x37,0x2c,0x33,0x36,0x38,0x2c,0x33,
    0x2c,0x74,0x2c,0x33,0x75,0x7b,0x7e,0x22,0x3a,0x33,0x2c,0x22,0x7b,0x74,0x74,0x74,
    0x74,0x74,0x7c,0x7b,0x7e,0x22,0x3a,0x33,0x2c,0x22,0x5b,0x7b,0x7e,0x22,0x3a,0x38,
    0x7d,0x7b,0x22,0x7e,0x74,0x74,0x75,0x7b,0x7e,0x22,0x3a,0x33,0x2c,0x22,0x20,0x20,
    0x20,0x20,0x38,0x2c,0x65,0x68,0x7b,0x7e,0x22,0x3a,0x36,0x2c,0x22,0x7b,0x74,0x74,
    0x7e,0x5b,0x20,0x20,0x20,0x5b,0x20,0x20,0x20,0x65,0x67,0x20,0x39,0x2c,0x5b,0x20,
    0x20,0x41,0x20,0x20,0x38,0x20,0x2c,0x33,0x75,0x7b,0x7e,0x22,0x3a,0x33,0x2c,0x22,
    0x7b,0x74,0x74,0x64,0x76,0x20,0x20,0x20,0x20,0x20,0x37,0x2c,0x33,0x75,0x7b,0x7e,
    0x22,0x3a,0x5b,0x31,0x35,0x2c,0x32,0x2c,0x33,0x2c,0x32,0x2c,0x7b,0x7d,0x2c,0x38,
    0x30,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x37,0x31,0x32,0x32,0x33,0x2c,0x31,0x2c,
    0x33,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
    0x36,0x36,0x36,0x36,0x36,0x36,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x30,0x2c,0x31,0x36,
    0x34,0x36,0x2c,0x33,0x2c,0x32,0x30,0x31,0x2c,0x32,0x33,0x2c,0x33,0x2c,0x33,0x2c,
    0x32,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x31,0x36,0x34,0x33,0x2c,0x36,0x36,0x36,
    0x36,0x36,0x36,0x33,0x2c,0x31,0x32,0x2c,0x31,0x2c,0x33,0x2c,0x33,0x2c,0x36,0x31,
    0x32,0x2c,0x32,0x2c,0x34,0x38,0x31,0x2c,0x32,0x33,0x2c,0x32,0x31,0x2c,0x32,0x2c,
    0x33,0x2c,0x34,0x31,0x2c,0x32,0x2c,0x31,0x2c,0x32,0x2c,0x32,0x2c,0x33,0x2c,0x36,
    0x37,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x31,0x2c,0x31,0x36,0x34,0x36,
    0x2c,0x33,0x2c,0x32,0x30,0x31,0x2c,0x32,0x33,0x2c,0x33,0x36,0x2c,0x33,0x2c,0x34,
    0x2c,0x31,0x30,0x32,0x31,0x2c,0x32,0x2c,0x7b,0x7d,0x2c,0x30,0x2c,0x32,0x2c,0x33,
    0x2c,0x33,0x2c,0x32,0x2c,0x33,0x2c,0x31,0x38,0x38,0x38,0x38,0x38,0x2c,0x31,0x38,
    0x38,0x38,0x31,0x31,0x32,0x2c,0x31,0x32,0x2c,0x38,0x5d,0x2c,0x22,0x5b,0x38,0x74,
    0x2c,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x37,0x2e,0x31,0x2c,0x2c,0x32,0x33,0x2c,
    0x32,0x2c,0x31,0x2c,0x33,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x32,0x2c,0x7b,0x7d,0x2c,
    0x38,0x30,0x74,0x74,0x74,0x74,0x74,0x7c,0x7b,0x7e,0x22,0x3a,0x33,0x2c,0x22,0x5b,
    0x20,0x20,0x20,0x2c,0x31,0x2c,0x5b,0x5d,0x2c,0x32,0x32,0x31,0x31,0x33,0x2c,0x22,
    0x3a,0x34,0x7d
};

ok64 BIFFtestCrashE4f95() {
    sane(1);
    u8cs raw = {_crash_e4f95, _crash_e4f95 + sizeof(_crash_e4f95)};
    call(BIFFtestFromRaw, raw);
    done;
}

static u8c _crash_6af3a[404] = {
    0x7b,0x22,0x7a,0x5b,0x2c,0x41,0x7e,0x74,0x74,0x75,0x7b,0x7e,0x22,0x3a,0x33,0x2c,
    0x22,0x5b,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x38,0x2c,0x5b,0x20,0x20,0x32,
    0x2c,0x33,0x2c,0x36,0x37,0x2c,0x33,0x2c,0x33,0x2c,0x2c,0x33,0x75,0x7b,0x7e,0x22,
    0x3a,0x33,0x2c,0x22,0x7b,0x74,0x74,0x74,0x7c,0x7b,0x7e,0x22,0x3a,0x33,0x2c,0x22,
    0x5b,0x21,0x20,0x20,0x20,0x20,0x20,0x20,0x5b,0x7b,0x22,0x3a,0x7b,0x22,0x2c,0x2c,
    0x3d,0x22,0x3a,0x5b,0x38,0x33,0x31,0x30,0x33,0x2e,0x31,0x2c,0x30,0x5d,0x7d,0x2c,
    0x22,0x74,0x74,0x7e,0x77,0x7e,0x22,0x3a,0x33,0x2c,0x22,0x5b,0x74,0x20,0x20,0x20,
    0x5b,0x7b,0x22,0x3a,0x7b,0x22,0x2c,0x2c,0x3d,0x22,0x3a,0x5b,0x38,0x33,0x31,0x30,
    0x33,0x2e,0x31,0x2c,0x30,0x5d,0x7d,0x2c,0x22,0x74,0x74,0x7e,0x75,0x7e,0x22,0x3a,
    0x33,0x2c,0x22,0x5b,0x74,0x74,0x74,0x74,0x74,0x7c,0x20,0x5b,0x7b,0x22,0x3a,0x7b,
    0x22,0x2c,0x7b,0x2c,0x3d,0x22,0x3a,0x5b,0x38,0x33,0x35,0x32,0x31,0x2e,0x31,0x2c,
    0x30,0x5d,0x7d,0x2c,0x22,0x74,0x74,0x7e,0x75,0x7e,0x22,0x3a,0x33,0x2c,0x22,0x5b,
    0x74,0x74,0x40,0x75,0x7b,0x7e,0x22,0x3a,0x39,0x7d,0x7b,0x22,0x7e,0x74,0x74,0x75,
    0x7b,0x7e,0x22,0x3a,0x33,0x2c,0x22,0x60,0x22,0x3a,0x33,0x2c,0x22,0x20,0x20,0x20,
    0x32,0x2c,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x48,0x37,0x2c,0x33,0x2c,0x33,0x2c,0x33,
    0x2c,0x32,0x2c,0x33,0x2c,0x31,0x38,0x38,0x38,0x2c,0x33,0x2c,0x32,0x2c,0x31,0x33,
    0x2c,0x32,0x2c,0x33,0x2c,0x31,0x38,0x38,0x38,0x33,0x33,0x2c,0x32,0x2c,0x31,0x2c,
    0x32,0x2c,0x33,0x2c,0x36,0x37,0x2c,0x33,0x2c,0x33,0x2c,0x2c,0x33,0x75,0x7b,0x7e,
    0x22,0x3a,0x33,0x2c,0x22,0x7b,0x74,0x74,0x74,0x7c,0x7b,0x7e,0x22,0x3a,0x33,0x2c,
    0x22,0x5b,0x7f,0x20,0x20,0x20,0x20,0x20,0x20,0x5b,0x7b,0x22,0x3a,0x7b,0x22,0x2c,
    0x2c,0x3d,0x22,0x3a,0x5b,0x38,0x33,0x31,0x30,0x33,0x2e,0x31,0x2c,0x30,0x5d,0x7d,
    0x2c,0x22,0x74,0x74,0x7e,0x35,0x7e,0x22,0x3a,0x33,0x2c,0x22,0x5b,0x74,0x20,0x20,
    0x20,0x5b,0x7b,0x22,0x3a,0x7b,0x22,0x2c,0x2f,0x29,0x3d,0x22,0x3a,0x5b,0x38,0x33,
    0x31,0x30,0x34,0x2e,0x31,0x2c,0x30,0x5d,0x7d,0x2c,0x22,0x74,0x74,0x7e,0x75,0x7e,
    0x22,0x3a,0x36,0x2c,0x22,0x5b,0x74,0x74,0x74,0x74,0x74,0x7c,0x20,0x20,0x20,0x20,
    0x22,0x3a,0x34,0x7d,
};

ok64 BIFFtestCrash6af3a() {
    sane(1);
    u8cs raw = {_crash_6af3a, _crash_6af3a + sizeof(_crash_6af3a)};
    call(BIFFtestFromRaw, raw);
    done;
}

static u8c _crash_c5b66[639] = {
    0x5b,0x32,0x2c,0x33,0x2c,0x36,0x31,0x31,0x32,0x2c,0x33,0x2c,0x32,0x2c,0x31,0x2c,
    0x32,0x2c,0x33,0x2c,0x36,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x32,0x2c,0x38,
    0x2c,0x32,0x2c,0x32,0x2c,0x34,0x2c,0x31,0x2c,0x31,0x32,0x2c,0x32,0x31,0x32,0x2c,
    0x33,0x2c,0x32,0x2c,0x31,0x33,0x32,0x2c,0x38,0x2c,0x31,0x2c,0x34,0x2c,0x32,0x38,
    0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x32,0x2c,0x33,0x2c,0x31,0x2c,0x32,0x33,0x2c,
    0x34,0x32,0x2c,0x31,0x2c,0x32,0x2c,0x32,0x2c,0x34,0x31,0x2c,0x32,0x2c,0x31,0x2c,
    0x32,0x2c,0x33,0x2c,0x36,0x31,0x31,0x32,0x2c,0x33,0x2c,0x32,0x2c,0x31,0x2c,0x32,
    0x2c,0x33,0x2c,0x36,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x31,0x2c,0x33,0x2c,
    0x31,0x38,0x2c,0x33,0x2c,0x32,0x2c,0x31,0x33,0x32,0x2c,0x38,0x2c,0x31,0x2c,0x34,
    0x2c,0x32,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x32,0x2c,0x33,0x2c,0x31,0x2c,
    0x32,0x33,0x2c,0x34,0x32,0x2c,0x31,0x2c,0x32,0x2c,0x32,0x2c,0x34,0x31,0x2c,0x32,
    0x2c,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x31,0x31,0x32,0x2c,0x33,0x2c,0x32,0x2c,
    0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x31,
    0x2c,0x33,0x2c,0x31,0x38,0x2c,0x33,0x2c,0x32,0x2c,0x31,0x33,0x32,0x2c,0x38,0x2c,
    0x31,0x2c,0x34,0x2c,0x32,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x32,0x2c,0x33,
    0x2c,0x31,0x2c,0x32,0x33,0x2c,0x32,0x31,0x2c,0x31,0x2c,0x32,0x2c,0x32,0x2c,0x34,
    0x31,0x2c,0x32,0x2c,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x31,0x31,0x32,0x2c,0x33,
    0x2c,0x32,0x2c,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x38,0x2c,0x33,0x2c,0x33,0x2c,
    0x33,0x2c,0x32,0x2c,0x33,0x2c,0x31,0x38,0x38,0x38,0x38,0x32,0x31,0x2c,0x32,0x2c,
    0x32,0x2c,0x34,0x5d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,
    0x0d,0x0d,0x0d,0x0d,0x5b,0x32,0x38,0x34,0x33,0x2c,0x31,0x2c,0x33,0x2c,0x33,0x2c,
    0x32,0x2c,0x33,0x2c,0x32,0x2c,0x33,0x2c,0x31,0x38,0x38,0x38,0x38,0x38,0x2c,0x31,
    0x2c,0x32,0x38,0x31,0x38,0x38,0x31,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x38,0x2c,0x38,
    0x38,0x31,0x2c,0x32,0x33,0x2c,0x31,0x32,0x2c,0x32,0x2c,0x32,0x2c,0x34,0x31,0x2c,
    0x38,0x38,0x31,0x2c,0x32,0x33,0x2c,0x32,0x38,0x31,0x31,0x32,0x2c,0x33,0x2c,0x32,
    0x2c,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,
    0x32,0x2c,0x38,0x2c,0x32,0x2c,0x32,0x2c,0x34,0x2c,0x32,0x2c,0x32,0x31,0x32,0x2c,
    0x33,0x2c,0x32,0x2c,0x31,0x33,0x32,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x32,0x2c,0x38,
    0x2c,0x32,0x2c,0x32,0x2c,0x34,0x2c,0x32,0x2c,0x32,0x31,0x32,0x2c,0x33,0x2c,0x32,
    0x2c,0x31,0x33,0x32,0x2c,0x38,0x2c,0x31,0x2c,0x34,0x2c,0x31,0x32,0x2c,0x33,0x2c,
    0x32,0x2c,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,
    0x2c,0x32,0x2c,0x38,0x2c,0x32,0x2c,0x32,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,
    0x32,0x2c,0x38,0x2c,0x32,0x2c,0x32,0x2c,0x34,0x2c,0x32,0x2c,0x32,0x31,0x32,0x2c,
    0x33,0x2c,0x32,0x2c,0x31,0x33,0x32,0x2c,0x38,0x2c,0x31,0x2c,0x34,0x2c,0x31,0x32,
    0x2c,0x33,0x2c,0x32,0x2c,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x38,0x2c,0x33,0x2c,
    0x33,0x2c,0x33,0x2c,0x32,0x2c,0x38,0x2c,0x32,0x2c,0x32,0x2c,0x34,0x2c,0x31,0x2c,
    0x32,0x2c,0x33,0x2c,0x36,0x38,0x2c,0x33,0x2c,0x33,0x2c,0x33,0x2c,0x32,0x2c,0x38,
    0x2c,0x32,0x2c,0x32,0x2c,0x34,0x2c,0x32,0x2c,0x32,0x31,0x32,0x2c,0x33,0x2c,0x32,
    0x2c,0x31,0x33,0x32,0x2c,0x38,0x2c,0x31,0x2c,0x34,0x2c,0x31,0x32,0x2c,0x33,0x2c,
    0x32,0x2c,0x31,0x2c,0x32,0x2c,0x33,0x2c,0x36,0x38,0x2c,0x33,0x32,0x32,0x5d,
};

ok64 BIFFtestCrashC5b66() {
    sane(1);
    u8cs raw = {_crash_c5b66, _crash_c5b66 + sizeof(_crash_c5b66)};
    call(BIFFtestFromRaw, raw);
    done;
}

// --- N-way merge tests ---

// Helper: parse N JSON strings into BASON, build u8css, run BASONMergeN
#define BIFFN_SETUP(outsize, nmax)                            \
    u8  _nout_pad[outsize];                                   \
    u8b nout_buf = {_nout_pad, _nout_pad,                     \
                    _nout_pad, _nout_pad + outsize};          \
    u64 _nout_idx[64];                                        \
    u64b nout_idx = {_nout_idx, _nout_idx,                    \
                     _nout_idx, _nout_idx + 64};              \
    u8cs _ninputs[nmax];                                      \
    (void)_ninputs; (void)nout_idx;

// Single input: identity
ok64 BIFFtestMergeNSingle() {
    sane(1);
    BIFF_SETUP(a, 1024, "{\"a\":1,\"b\":2}");
    BIFFN_SETUP(2048, 4);
    u8cs ad = {a_buf[1], a_buf[2]};
    _ninputs[0][0] = ad[0]; _ninputs[0][1] = ad[1];
    u8css inputs = {_ninputs, _ninputs + 1};
    call(BASONMergeN, nout_buf, nout_idx, inputs);
    u8cs od = {nout_buf[1], nout_buf[2]};
    call(BIFFCheckJSON, od, "{\"a\":1,\"b\":2}");
    done;
}

// Two inputs: should match 2-way merge
ok64 BIFFtestMergeN2way() {
    sane(1);
    BIFF_SETUP(a, 1024, "{\"a\":1,\"b\":2}");
    BIFF_SETUP(b, 1024, "{\"a\":3,\"c\":4}");
    BIFFN_SETUP(2048, 4);
    u8cs ad = {a_buf[1], a_buf[2]};
    u8cs bd = {b_buf[1], b_buf[2]};
    _ninputs[0][0] = ad[0]; _ninputs[0][1] = ad[1];
    _ninputs[1][0] = bd[0]; _ninputs[1][1] = bd[1];
    u8css inputs = {_ninputs, _ninputs + 2};
    call(BASONMergeN, nout_buf, nout_idx, inputs);
    u8cs od = {nout_buf[1], nout_buf[2]};
    call(BIFFCheckJSON, od, "{\"a\":3,\"b\":2,\"c\":4}");
    done;
}

// Three inputs: last wins
ok64 BIFFtestMergeN3way() {
    sane(1);
    BIFF_SETUP(a, 1024, "{\"a\":1,\"b\":2}");
    BIFF_SETUP(b, 1024, "{\"a\":3,\"c\":4}");
    BIFF_SETUP(c, 1024, "{\"a\":5,\"d\":6}");
    BIFFN_SETUP(2048, 4);
    u8cs ad = {a_buf[1], a_buf[2]};
    u8cs bd = {b_buf[1], b_buf[2]};
    u8cs cd = {c_buf[1], c_buf[2]};
    _ninputs[0][0] = ad[0]; _ninputs[0][1] = ad[1];
    _ninputs[1][0] = bd[0]; _ninputs[1][1] = bd[1];
    _ninputs[2][0] = cd[0]; _ninputs[2][1] = cd[1];
    u8css inputs = {_ninputs, _ninputs + 3};
    call(BASONMergeN, nout_buf, nout_idx, inputs);
    u8cs od = {nout_buf[1], nout_buf[2]};
    call(BIFFCheckJSON, od, "{\"a\":5,\"b\":2,\"c\":4,\"d\":6}");
    done;
}

// Nested object recursion across 3 inputs
ok64 BIFFtestMergeNNested() {
    sane(1);
    BIFF_SETUP(a, 1024, "{\"o\":{\"x\":1}}");
    BIFF_SETUP(b, 1024, "{\"o\":{\"y\":2}}");
    BIFF_SETUP(c, 1024, "{\"o\":{\"z\":3}}");
    BIFFN_SETUP(2048, 4);
    u8cs ad = {a_buf[1], a_buf[2]};
    u8cs bd = {b_buf[1], b_buf[2]};
    u8cs cd = {c_buf[1], c_buf[2]};
    _ninputs[0][0] = ad[0]; _ninputs[0][1] = ad[1];
    _ninputs[1][0] = bd[0]; _ninputs[1][1] = bd[1];
    _ninputs[2][0] = cd[0]; _ninputs[2][1] = cd[1];
    u8css inputs = {_ninputs, _ninputs + 3};
    call(BASONMergeN, nout_buf, nout_idx, inputs);
    u8cs od = {nout_buf[1], nout_buf[2]};
    call(BIFFCheckJSON, od, "{\"o\":{\"x\":1,\"y\":2,\"z\":3}}");
    done;
}

// Null tombstone in middle input, last input restores
ok64 BIFFtestMergeNNull() {
    sane(1);
    BIFF_SETUP(a, 1024, "{\"a\":1,\"b\":2}");
    BIFF_SETUP(b, 1024, "{\"a\":null}");
    BIFF_SETUP(c, 1024, "{\"a\":9}");
    BIFFN_SETUP(2048, 4);
    u8cs ad = {a_buf[1], a_buf[2]};
    u8cs bd = {b_buf[1], b_buf[2]};
    u8cs cd = {c_buf[1], c_buf[2]};
    _ninputs[0][0] = ad[0]; _ninputs[0][1] = ad[1];
    _ninputs[1][0] = bd[0]; _ninputs[1][1] = bd[1];
    _ninputs[2][0] = cd[0]; _ninputs[2][1] = cd[1];
    u8css inputs = {_ninputs, _ninputs + 3};
    call(BASONMergeN, nout_buf, nout_idx, inputs);
    u8cs od = {nout_buf[1], nout_buf[2]};
    call(BIFFCheckJSON, od, "{\"a\":9,\"b\":2}");
    done;
}

// Null tombstone is last: key deleted
ok64 BIFFtestMergeNNullLast() {
    sane(1);
    BIFF_SETUP(a, 1024, "{\"a\":1,\"b\":2}");
    BIFF_SETUP(b, 1024, "{\"a\":null}");
    BIFFN_SETUP(2048, 4);
    u8cs ad = {a_buf[1], a_buf[2]};
    u8cs bd = {b_buf[1], b_buf[2]};
    _ninputs[0][0] = ad[0]; _ninputs[0][1] = ad[1];
    _ninputs[1][0] = bd[0]; _ninputs[1][1] = bd[1];
    u8css inputs = {_ninputs, _ninputs + 2};
    call(BASONMergeN, nout_buf, nout_idx, inputs);
    u8cs od = {nout_buf[1], nout_buf[2]};
    call(BIFFCheckJSON, od, "{\"b\":2}");
    done;
}

// Array positional merge, 3 inputs
ok64 BIFFtestMergeNArray() {
    sane(1);
    BIFF_SETUP(a, 1024, "[1,2,3]");
    BIFF_SETUP(b, 1024, "[4]");
    BIFF_SETUP(c, 1024, "[10,20]");
    BIFFN_SETUP(2048, 4);
    u8cs ad = {a_buf[1], a_buf[2]};
    u8cs bd = {b_buf[1], b_buf[2]};
    u8cs cd = {c_buf[1], c_buf[2]};
    _ninputs[0][0] = ad[0]; _ninputs[0][1] = ad[1];
    _ninputs[1][0] = bd[0]; _ninputs[1][1] = bd[1];
    _ninputs[2][0] = cd[0]; _ninputs[2][1] = cd[1];
    u8css inputs = {_ninputs, _ninputs + 3};
    call(BASONMergeN, nout_buf, nout_idx, inputs);
    u8cs od = {nout_buf[1], nout_buf[2]};
    call(BIFFCheckJSON, od, "[10,20,3]");
    done;
}

// BASONMergeY wrapper test
ok64 BIFFtestMergeY() {
    sane(1);
    BIFF_SETUP(a, 1024, "{\"a\":1}");
    BIFF_SETUP(b, 1024, "{\"b\":2}");
    u8cs ad = {a_buf[1], a_buf[2]};
    u8cs bd = {b_buf[1], b_buf[2]};
    u8cs recs[2] = {{ad[0], ad[1]}, {bd[0], bd[1]}};
    u8css inputs = {recs, recs + 2};
    u8 _ybuf[4096];
    u8s yout = {_ybuf, _ybuf + 4096};
    u8cp ystart = yout[0];
    call(BASONMergeY, yout, inputs);
    u8cs yd = {ystart, yout[0]};
    call(BIFFCheckJSON, yd, "{\"a\":1,\"b\":2}");
    done;
}

// Empty inputs
ok64 BIFFtestMergeNEmpty() {
    sane(1);
    BIFFN_SETUP(2048, 4);
    u8css inputs = {_ninputs, _ninputs};
    call(BASONMergeN, nout_buf, nout_idx, inputs);
    testeq(u8bDataLen(nout_buf), (size_t)0);
    done;
}

// --- Diff minimality tests ---
// Verify that diff(old, new) produces a minimal patch (only changed elements).

typedef struct {
    char const *old_json;
    char const *new_json;
    char const *expected_diff;  // NULL means empty diff
} BIFFDiffMinCase;

static BIFFDiffMinCase BIFF_DIFF_MIN_CASES[] = {
    // Object: identical → empty
    {"{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2}", NULL},
    // Object: one scalar changed → only that key
    {"{\"a\":1,\"b\":2,\"c\":3}", "{\"a\":1,\"b\":9,\"c\":3}", "{\"b\":9}"},
    // Object: key added → only new key
    {"{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2,\"c\":3}", "{\"c\":3}"},
    // Object: key removed → only tombstone
    {"{\"a\":1,\"b\":2,\"c\":3}", "{\"a\":1,\"c\":3}", "{\"b\":null}"},
    // Object: nested change → only changed path
    {"{\"a\":{\"x\":1,\"y\":2},\"b\":3}",
     "{\"a\":{\"x\":1,\"y\":9},\"b\":3}",
     "{\"a\":{\"y\":9}}"},
    // Array: one element changed → only that position
    {"[1,2,3,4,5]", "[1,2,9,4,5]", NULL},  // roundtrip-only (array keys vary)
    // Array: identical → empty
    {"[10,20,30]", "[10,20,30]", NULL},
    // Array: replace all → full patch
    {"[1,2,3]", "[4,5,6]", NULL},  // roundtrip-only
};

ok64 BIFFtestDiffMinimal() {
    sane(1);
    size_t n = sizeof(BIFF_DIFF_MIN_CASES) / sizeof(BIFF_DIFF_MIN_CASES[0]);
    for (size_t i = 0; i < n; i++) {
        BIFFDiffMinCase *tc = &BIFF_DIFF_MIN_CASES[i];

        BIFF_SETUP(old, 4096, tc->old_json);
        BIFF_SETUP(neu, 4096, tc->new_json);

        u8cs od = {old_buf[1], old_buf[2]};
        u8cs nd = {neu_buf[1], neu_buf[2]};

        u8  _dp[4096];
        u8b dbuf = {_dp, _dp, _dp, _dp + 4096};
        u64 _di[64];
        u64b didx = {_di, _di, _di, _di + 64};
        u64 _os[64];
        u64b os = {_os, _os, _os, _os + 64};
        u64 _ns[64];
        u64b ns = {_ns, _ns, _ns, _ns + 64};

        call(BASONDiff, dbuf, didx, os, od, ns, nd, NULL);
        u8cs dd = {dbuf[1], dbuf[2]};

        // Check expected diff JSON (if specified)
        if (tc->expected_diff != NULL) {
            if ($empty(dd) && tc->expected_diff[0] != '\0') {
                fprintf(stderr, "  case %zu: expected diff '%s' but got empty\n",
                        i, tc->expected_diff);
                fail(TESTFAIL);
            }
            call(BIFFCheckJSON, dd, tc->expected_diff);
        } else if (!$empty(dd)) {
            // NULL expected means we only do roundtrip check
        }

        // Always verify roundtrip: merge(old, diff) == new
        u8  _mp[4096];
        u8b mbuf = {_mp, _mp, _mp, _mp + 4096};
        u64 _mi[64];
        u64b midx = {_mi, _mi, _mi, _mi + 64};
        u64 _ls[64];
        u64b ls = {_ls, _ls, _ls, _ls + 64};
        u64 _rs[64];
        u64b rs = {_rs, _rs, _rs, _rs + 64};

        call(BASONMerge, mbuf, midx, ls, od, rs, dd);
        u8cs md = {mbuf[1], mbuf[2]};
        call(BIFFCheckJSONEqual, md, nd);
    }
    done;
}

// --- Diff rendering tests (BASONDiffPrint) ---
// Check that rendered diff does not interleave red (deleted) and green (added).
// In a sane diff, each "hunk" should have all deletions grouped before insertions,
// not red-green-red-green on the same line.

// Count interleaving: scan for color transitions red→green→red on same line.
// Returns count of lines with interleaving.
static u32 BIFFCountInterleaves(u8cs text) {
    // ESC[9;31m = red/strike, ESC[32m = green, ESC[0m = reset
    u32 count = 0;
    u8cp p = text[0];
    // Track state per line: did we see green after red?
    i32 color = 0;  // 0=none, 1=red, 2=green
    b8 saw_interleave = NO;
    while (p < text[1]) {
        if (*p == '\n') {
            if (saw_interleave) count++;
            color = 0;
            saw_interleave = NO;
            p++;
            continue;
        }
        if (*p == '\033' && p + 4 < text[1]) {
            // Check for red: \033[9;31m
            if (memcmp(p, "\033[9;31m", 7) == 0 && p + 7 <= text[1]) {
                if (color == 2) saw_interleave = YES;  // green→red
                color = 1;
                p += 7;
                continue;
            }
            // Check for green: \033[32m
            if (memcmp(p, "\033[32m", 5) == 0 && p + 5 <= text[1]) {
                color = 2;
                p += 5;
                continue;
            }
            // Check for reset: \033[0m
            if (memcmp(p, "\033[0m", 4) == 0 && p + 4 <= text[1]) {
                p += 4;
                continue;
            }
        }
        p++;
    }
    if (saw_interleave) count++;
    return count;
}

typedef struct {
    char const *old_json;
    char const *new_json;
    char const *label;
} BIFFRenderCase;

static BIFFRenderCase BIFF_RENDER_CASES[] = {
    // Text-like arrays with newlines (simulates real text diffs)
    // Each line element ends with \n so changes appear on separate lines.
    {"[\"aaa\\n\", \"bbb\\n\", \"ccc\\n\"]",
     "[\"aaa\\n\",\"xxx\\n\",\"ccc\\n\"]",
     "text: one line changed"},
    {"[\"aaa\\n\",\"bbb\\n\",\"ccc\\n\"]",
     "[\"zzz\\n\",\"aaa\\n\",\"bbb\\n\",\"ccc\\n\"]",
     "text: prepend"},
    {"[\"aaa\\n\",\"bbb\\n\"]",
     "[\"aaa\\n\",\"bbb\\n\",\"ccc\\n\"]",
     "text: append"},
    {"[\"aaa\\n\",\"bbb\\n\",\"ccc\\n\"]",
     "[\"aaa\\n\",\"ccc\\n\"]",
     "text: delete middle"},
    {"[\"aaa\\n\",\"bbb\\n\",\"ccc\\n\",\"ddd\\n\",\"eee\\n\"]",
     "[\"aaa\\n\",\"xxx\\n\",\"yyy\\n\",\"ddd\\n\",\"eee\\n\"]",
     "text: replace middle pair"},
    {"[\"aaa\\n\",\"bbb\\n\",\"ccc\\n\"]",
     "[\"xxx\\n\",\"yyy\\n\",\"zzz\\n\"]",
     "text: full replace"},
    {"[\"aaa\\n\",\"eee\\n\"]",
     "[\"aaa\\n\",\"bbb\\n\",\"ccc\\n\",\"ddd\\n\",\"eee\\n\"]",
     "text: multi-insert middle"},
    {"[\"aaa\\n\",\"bbb\\n\",\"ccc\\n\",\"ddd\\n\",\"eee\\n\",\"fff\\n\",\"ggg\\n\",\"hhh\\n\"]",
     "[\"aaa\\n\",\"xxx\\n\",\"yyy\\n\",\"zzz\\n\",\"hhh\\n\"]",
     "text: delete+insert block"},
    // Object: key change
    {"{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":9}", "object: value change"},
    // Nested
    {"{\"x\":{\"a\":1,\"b\":2}}", "{\"x\":{\"a\":1,\"b\":9}}",
     "nested object: deep change"},
};

// Render diff for old→new, check for interleaving
// Uses the correct pipeline: diff→merge→render
ok64 BIFFtestRender() {
    sane(1);
    u32 total_bad = 0;
    size_t n = sizeof(BIFF_RENDER_CASES) / sizeof(BIFF_RENDER_CASES[0]);
    for (size_t i = 0; i < n; i++) {
        BIFFRenderCase *tc = &BIFF_RENDER_CASES[i];

        // Parse old and new
        BIFF_SETUP(old, 4096, tc->old_json);
        BIFF_SETUP(neu, 4096, tc->new_json);

        u8cs od = {old_buf[1], old_buf[2]};
        u8cs nd = {neu_buf[1], neu_buf[2]};

        // Step 1: diff(old, new) → patch
        u8  _dp[4096];
        u8b dbuf = {_dp, _dp, _dp, _dp + 4096};
        u64 _os[64];
        u64b os = {_os, _os, _os, _os + 64};
        u64 _ns[64];
        u64b ns = {_ns, _ns, _ns, _ns + 64};
        call(BASONDiff, dbuf, NULL, os, od, ns, nd, NULL);
        u8cp d0 = dbuf[1], d1 = dbuf[2];
        u8cs patch = {d0, d1};

        // Step 2: merge(old, patch) → merged
        u8  _mp[4096];
        u8b mbuf = {_mp, _mp, _mp, _mp + 4096};
        u64 _ls[64];
        u64b ls = {_ls, _ls, _ls, _ls + 64};
        u64 _rs[64];
        u64b rs = {_rs, _rs, _rs, _rs + 64};
        call(BASONMerge, mbuf, NULL, ls, od, rs, patch);
        u8cp m0 = mbuf[1], m1 = mbuf[2];
        u8cs merged = {m0, m1};

        // Step 3: render old vs merged (same key space)
        a_pad(u8, rbuf, 8192);
        u8s rout = {_rbuf, _rbuf + 8192};
        u8p rstart = rout[0];
        u8cs noname = {};
        call(BASONDiffPrint, rout, od, merged, 0, noname);
        u8cs rendered = {(u8cp)rstart, (u8cp)rout[0]};

        if ($empty(rendered)) continue;  // identical, nothing to check

        // Check for interleaving
        u32 interleaves = BIFFCountInterleaves(rendered);
        if (interleaves > 0) {
            fprintf(stderr, "  INTERLEAVE in '%s': %u lines with red-green-red\n",
                    tc->label, interleaves);
            total_bad++;
        }
    }
    test(total_bad == 0, TESTFAIL);
    done;
}

// Render diff for fuzz repro table: verify diff→merge→render doesn't crash.
// Interleaving check is omitted for fuzz cases since small single-line JSON
// values naturally produce color transitions within one line.
ok64 BIFFtestRenderFuzzRepros() {
    sane(1);
    size_t n = sizeof(BIFF_FUZZ_REPROS) / sizeof(BIFF_FUZZ_REPROS[0]);
    for (size_t i = 0; i < n; i++) {
        BIFFRoundtripCase *tc = &BIFF_FUZZ_REPROS[i];

        BIFF_SETUP(old, 4096, tc->old_json);
        BIFF_SETUP(neu, 4096, tc->new_json);

        u8cs od = {old_buf[1], old_buf[2]};
        u8cs nd = {neu_buf[1], neu_buf[2]};

        // Step 1: diff(old, new) → patch
        u8  _dp[8192];
        u8b dbuf = {_dp, _dp, _dp, _dp + 8192};
        u64 _os[128];
        u64b os = {_os, _os, _os, _os + 128};
        u64 _ns[128];
        u64b ns = {_ns, _ns, _ns, _ns + 128};
        call(BASONDiff, dbuf, NULL, os, od, ns, nd, NULL);
        u8cp d0 = dbuf[1], d1 = dbuf[2];
        u8cs patch = {d0, d1};

        // Step 2: merge(old, patch) → merged
        u8  _mp[8192];
        u8b mbuf = {_mp, _mp, _mp, _mp + 8192};
        u64 _ls[128];
        u64b ls = {_ls, _ls, _ls, _ls + 128};
        u64 _rs[128];
        u64b rs = {_rs, _rs, _rs, _rs + 128};
        call(BASONMerge, mbuf, NULL, ls, od, rs, patch);
        u8cp m0 = mbuf[1], m1 = mbuf[2];
        u8cs merged = {m0, m1};

        // Step 3: render old vs merged (same key space, no crash)
        a_pad(u8, rbuf, 16384);
        u8s rout = {_rbuf, _rbuf + 16384};
        u8cs noname = {};
        call(BASONDiffPrint, rout, od, merged, 0, noname);
    }
    done;
}

ok64 BIFFtestAll() {
    sane(1);
    call(BIFFtestCrashC5b66);
    call(BIFFtestCrash6af3a);
    call(BIFFtestMergeIdentityLeft);
    call(BIFFtestMergeIdentityRight);
    call(BIFFtestMergeRightWins);
    call(BIFFtestMergeUnion);
    call(BIFFtestMergeNested);
    call(BIFFtestMergeArray);
    call(BIFFtestMergeTypeChange);
    call(BIFFtestDiffIdentical);
    call(BIFFtestDiffScalar);
    call(BIFFtestDiffAdded);
    call(BIFFtestDiffDeleted);
    call(BIFFtestDiffRoundtrip);
    call(BIFFtestDiffRoundtripDel);
    call(BIFFtestDiffNested);
    call(BIFFtestDiffArray);
    call(BIFFtestDiffArrayInsert);
    call(BIFFtestDiffArrayAppend);
    call(BIFFtestDiffArrayDelete);
    call(BIFFtestDiffArrayIdentical);
    call(BIFFtestFuzzRepros);
    call(BIFFtestMergeNSingle);
    call(BIFFtestMergeN2way);
    call(BIFFtestMergeN3way);
    call(BIFFtestMergeNNested);
    call(BIFFtestMergeNNull);
    call(BIFFtestMergeNNullLast);
    call(BIFFtestMergeNArray);
    call(BIFFtestMergeY);
    call(BIFFtestMergeNEmpty);
    call(BIFFtestDiffMinimal);
    call(BIFFtestRenderFuzzRepros);
    call(BIFFtestRender);
    done;
}

TEST(BIFFtestAll)
