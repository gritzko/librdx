//
// DIFF tests
//

#include "DIFF.h"
#include "INT.h"
#include "S.h"

#define X(M, name) M##u8##name
#include "DIFFx.h"
#undef X

#include "PRO.h"
#include "TEST.h"

ok64 DIFFVerify(u8cs a, u8cs b, e32 *edl, i32 edllen) {
    sane(1);
    i32 ai = 0, bi = 0;
    i32 alen = $len(a), blen = $len(b);

    for (i32 i = 0; i < edllen; i++) {
        u8 op = DIFF_OP(edl[i]);
        u32 len = DIFF_LEN(edl[i]);
        switch (op) {
            case DIFF_EQ:
                for (u32 j = 0; j < len; j++) {
                    want(ai < alen && bi < blen);
                    want(a[0][ai] == b[0][bi]);
                    ai++;
                    bi++;
                }
                break;
            case DIFF_DEL:
                ai += len;
                want(ai <= alen);
                break;
            case DIFF_INS:
                bi += len;
                want(bi <= blen);
                break;
        }
    }
    want(ai == alen);
    want(bi == blen);
    done;
}

ok64 DIFFCount(e32 *edl, i32 len, i32 *dels, i32 *ins, i32 *eqs) {
    sane(1);
    *dels = *ins = *eqs = 0;
    for (i32 i = 0; i < len; i++) {
        switch (DIFF_OP(edl[i])) {
            case DIFF_EQ:  *eqs += DIFF_LEN(edl[i]); break;
            case DIFF_DEL: *dels += DIFF_LEN(edl[i]); break;
            case DIFF_INS: *ins += DIFF_LEN(edl[i]); break;
        }
    }
    done;
}

ok64 test_empty() {
    sane(1);
    u8cs a = {(u8 *)"", (u8 *)""};
    u8cs b = {(u8 *)"", (u8 *)""};

    e32 edlbuf[16];
    e32g edl = {edlbuf, edlbuf + 16, edlbuf};
    i32 workbuf[64];
    i32s work = {workbuf, workbuf + 64};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    want(edl[0] - edl[2] == 0);
    done;
}

ok64 test_equal() {
    sane(1);
    u8 buf[] = "hello";
    u8cs a = {buf, buf + 5};
    u8cs b = {buf, buf + 5};

    e32 edlbuf[16];
    e32g edl = {edlbuf, edlbuf + 16, edlbuf};
    i32 workbuf[256];
    i32s work = {workbuf, workbuf + 256};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    i32 len = edl[0] - edl[2];

    want(len == 1);
    want(DIFF_OP(edl[2][0]) == DIFF_EQ);
    want(DIFF_LEN(edl[2][0]) == 5);
    call(DIFFVerify, a, b, edl[2], len);
    done;
}

ok64 test_different() {
    sane(1);
    u8 abuf[] = "abc";
    u8 bbuf[] = "xyz";
    u8cs a = {abuf, abuf + 3};
    u8cs b = {bbuf, bbuf + 3};

    e32 edlbuf[16];
    e32g edl = {edlbuf, edlbuf + 16, edlbuf};
    i32 workbuf[256];
    i32s work = {workbuf, workbuf + 256};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    i32 len = edl[0] - edl[2];

    i32 dels, ins, eqs;
    call(DIFFCount, edl[2], len, &dels, &ins, &eqs);
    want(dels == 3 && ins == 3 && eqs == 0);
    call(DIFFVerify, a, b, edl[2], len);
    done;
}

ok64 test_insert() {
    sane(1);
    u8 abuf[] = "ac";
    u8 bbuf[] = "abc";
    u8cs a = {abuf, abuf + 2};
    u8cs b = {bbuf, bbuf + 3};

    e32 edlbuf[16];
    e32g edl = {edlbuf, edlbuf + 16, edlbuf};
    i32 workbuf[256];
    i32s work = {workbuf, workbuf + 256};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    i32 len = edl[0] - edl[2];

    i32 dels, ins, eqs;
    call(DIFFCount, edl[2], len, &dels, &ins, &eqs);
    want(dels == 0 && ins == 1 && eqs == 2);
    call(DIFFVerify, a, b, edl[2], len);
    done;
}

ok64 test_delete() {
    sane(1);
    u8 abuf[] = "abc";
    u8 bbuf[] = "ac";
    u8cs a = {abuf, abuf + 3};
    u8cs b = {bbuf, bbuf + 2};

    e32 edlbuf[16];
    e32g edl = {edlbuf, edlbuf + 16, edlbuf};
    i32 workbuf[256];
    i32s work = {workbuf, workbuf + 256};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    i32 len = edl[0] - edl[2];

    i32 dels, ins, eqs;
    call(DIFFCount, edl[2], len, &dels, &ins, &eqs);
    want(dels == 1 && ins == 0 && eqs == 2);
    call(DIFFVerify, a, b, edl[2], len);
    done;
}

ok64 test_mixed() {
    sane(1);
    u8 abuf[] = "ABCDEFGH";
    u8 bbuf[] = "ABXYZFGH";
    u8cs a = {abuf, abuf + 8};
    u8cs b = {bbuf, bbuf + 8};

    e32 edlbuf[32];
    e32g edl = {edlbuf, edlbuf + 32, edlbuf};
    i32 workbuf[1024];
    i32s work = {workbuf, workbuf + 1024};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    i32 len = edl[0] - edl[2];

    i32 dels, ins, eqs;
    call(DIFFCount, edl[2], len, &dels, &ins, &eqs);
    want(eqs == 5 && dels == 3 && ins == 3);
    call(DIFFVerify, a, b, edl[2], len);
    done;
}

ok64 test_a_empty() {
    sane(1);
    u8 bbuf[] = "xyz";
    u8cs a = {bbuf + 3, bbuf + 3};
    u8cs b = {bbuf, bbuf + 3};

    e32 edlbuf[16];
    e32g edl = {edlbuf, edlbuf + 16, edlbuf};
    i32 workbuf[256];
    i32s work = {workbuf, workbuf + 256};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    i32 len = edl[0] - edl[2];

    want(len == 1);
    want(DIFF_OP(edl[2][0]) == DIFF_INS);
    want(DIFF_LEN(edl[2][0]) == 3);
    call(DIFFVerify, a, b, edl[2], len);
    done;
}

ok64 test_b_empty() {
    sane(1);
    u8 abuf[] = "xyz";
    u8cs a = {abuf, abuf + 3};
    u8cs b = {abuf + 3, abuf + 3};

    e32 edlbuf[16];
    e32g edl = {edlbuf, edlbuf + 16, edlbuf};
    i32 workbuf[256];
    i32s work = {workbuf, workbuf + 256};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    i32 len = edl[0] - edl[2];

    want(len == 1);
    want(DIFF_OP(edl[2][0]) == DIFF_DEL);
    want(DIFF_LEN(edl[2][0]) == 3);
    call(DIFFVerify, a, b, edl[2], len);
    done;
}

ok64 test_apply(u8cs a, u8cs b) {
    sane(1);
    e32 edlbuf[32];
    e32g edl = {edlbuf, edlbuf + 32, edlbuf};
    i32 workbuf[1024];
    i32s work = {workbuf, workbuf + 1024};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    e32cs edlc = {edl[2], edl[0]};

    u8 outbuf[64];
    u8s out = {outbuf, outbuf + 64};
    u8 *outstart = out[0];
    o = DIFFu8sApply(out, a, b, edlc);
    want(o == OK);

    want((u64)(out[0] - outstart) == (u64)$len(b));
    if ($len(b) > 0) want(memcmp(outstart, b[0], $len(b)) == 0);
    done;
}

ok64 test_apply_cases() {
    sane(1);
    u8 hello[] = "hello";
    u8 abc[] = "abc";
    u8 xyz[] = "xyz";
    u8 ac[] = "ac";
    u8 abcdef[] = "ABCDEFGH";
    u8 abxyz[] = "ABXYZFGH";

    u8cs h = {hello, hello + 5};
    u8cs a3 = {abc, abc + 3};
    u8cs x3 = {xyz, xyz + 3};
    u8cs a2 = {ac, ac + 2};
    u8cs d8 = {abcdef, abcdef + 8};
    u8cs x8 = {abxyz, abxyz + 8};
    u8cs empty = {abc, abc};

    call(test_apply, h, h);
    call(test_apply, a3, x3);
    call(test_apply, a2, a3);
    call(test_apply, a3, a2);
    call(test_apply, d8, x8);
    call(test_apply, empty, x3);
    call(test_apply, x3, empty);
    done;
}

// Test edge case: repeated 0xff in input
ok64 test_repeated_ff() {
    sane(1);
    u8 abuf[] = {0xff, 0xff};
    u8 bbuf[] = {0xff, 0xff, 0x31, 0xff, 0xff, 0xff, 0x01, 0x00, 0x9d, 0x00, 0x84};
    u8cs a = {abuf, abuf + 2};
    u8cs b = {bbuf, bbuf + 11};

    e32 edlbuf[64];
    e32g edl = {edlbuf, edlbuf + 64, edlbuf};
    i32 workbuf[1024];
    i32s work = {workbuf, workbuf + 1024};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    i32 len = edl[0] - edl[2];
    call(DIFFVerify, a, b, edl[2], len);
    done;
}

// Test edge case: repeated char in target
ok64 test_repeated_char() {
    sane(1);
    u8 abuf[] = "b";
    u8 bbuf[] = "b&b";
    u8cs a = {abuf, abuf + 1};
    u8cs b = {bbuf, bbuf + 3};

    e32 edlbuf[64];
    e32g edl = {edlbuf, edlbuf + 64, edlbuf};
    i32 workbuf[1024];
    i32s work = {workbuf, workbuf + 1024};

    ok64 o = DIFFu8s(edl, work, a, b);
    want(o == OK);
    i32 len = edl[0] - edl[2];

    i32 dels, ins, eqs;
    call(DIFFCount, edl[2], len, &dels, &ins, &eqs);
    // a="b" to b="b&b" should be: EQ 1, INS 2 (total: eqs=1, ins=2)
    want(dels == 0 && ins == 2 && eqs == 1);
    call(DIFFVerify, a, b, edl[2], len);
    done;
}

ok64 all_tests() {
    sane(1);
    call(test_empty);
    call(test_equal);
    call(test_different);
    call(test_insert);
    call(test_delete);
    call(test_mixed);
    call(test_a_empty);
    call(test_b_empty);
    call(test_apply_cases);
    call(test_repeated_ff);
    call(test_repeated_char);
    done;
}

TEST(all_tests)
