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
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd);

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
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd);

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
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd);

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
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd);

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
    call(BASONDiff, diff_buf, diff_idx, ostk, od, nstk, nd);

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
    call(BASONDiff, diff_buf, diff_idx, ostk, od, nstk, nd);

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
    call(BASONDiff, diff_buf, diff_idx, ostk, od, nstk, nd);

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
    call(BASONDiff, out_buf, out_idx, lstk, od, rstk, nd);

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
};

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

        call(BASONDiff, dbuf, didx, os, od, ns, nd);
        u8cs dd = {dbuf[1], dbuf[2]};

        if ($len(dd) == 0) {
            // empty diff: old == new in BASON bytes
            test($len(od) == $len(nd), TESTFAIL);
            test(memcmp(od[0], nd[0], $len(od)) == 0, TESTFAIL);
            continue;
        }

        // merge(old, diff)
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

        // verify: merge result == new (BASON bytes)
        if ($len(md) != $len(nd) ||
            memcmp(md[0], nd[0], $len(nd)) != 0) {
            fprintf(stderr, "  repro[%zu] FAIL: old=%s new=%s\n",
                    i, tc->old_json, tc->new_json);
            // show what we got
            u8 _jb[4096];
            u8b jb = {_jb, _jb, _jb, _jb + 4096};
            BIFFExportJSON(jb, md);
            u8cs got = {jb[1], jb[2]};
            fprintf(stderr, "  got: %.*s\n", (int)$len(got), got[0]);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 BIFFtestAll() {
    sane(1);
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
    call(BIFFtestFuzzRepros);
    done;
}

TEST(BIFFtestAll)
