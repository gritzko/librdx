//  DELT test: encode a delta, apply it, verify the result equals the
//  target.  Table of (base, target) pairs covering a few shapes:
//  identical inputs, head-rewrite, tail-append, full-replace, single
//  edit in the middle, and a larger random-ish blob for the extend
//  paths (forward + backward).

#include "keeper/DELT.h"

#include <string.h>

#include "abc/B.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "abc/TEST.h"

typedef struct {
    const char *name;
    const char *base;
    const char *target;
} delt_case;

static const delt_case CASES[] = {
    {"identical",
     "the quick brown fox jumps over the lazy dog",
     "the quick brown fox jumps over the lazy dog"},

    {"head-rewrite",
     "the quick brown fox jumps over the lazy dog",
     "that quick brown fox jumps over the lazy dog"},

    {"tail-append",
     "the quick brown fox jumps over the lazy dog",
     "the quick brown fox jumps over the lazy dog — twice"},

    {"middle-edit",
     "the quick brown fox jumps over the lazy dog",
     "the quick RED fox jumps over the lazy dog"},

    {"full-replace",
     "the quick brown fox jumps over the lazy dog",
     "some entirely unrelated content here"},

    {"tiny-base",
     "abc",
     "xyzabc"},

    {"empty-target",
     "the quick brown fox",
     ""},

    {"empty-base",
     "",
     "unrelated"},
};

static ok64 round_trip(delt_case const *c) {
    sane(c);
    u8cs base   = {(u8cp)c->base,   (u8cp)c->base   + strlen(c->base)};
    u8cs target = {(u8cp)c->target, (u8cp)c->target + strlen(c->target)};

    Bu8 delta_buf = {};
    call(u8bMap, delta_buf, 1u << 16);

    a_dup(u8c, bcs, base);
    a_dup(u8c, tcs, target);
    ok64 eo = DELTEncode(bcs, tcs, delta_buf);
    //  DELTFAIL is legitimate (delta >= target size); skip apply in
    //  that case — raw would be used instead.
    if (eo != OK && eo != DELTFAIL) {
        fprintf(stderr, "DELT %s: encode failed %s\n", c->name, ok64str(eo));
        u8bUnMap(delta_buf);
        fail(TESTFAIL);
    }

    if (eo == OK) {
        Bu8 out = {};
        call(u8bMap, out, 1u << 16);

        a_dup(u8c, dcs, u8bDataC(delta_buf));
        a_dup(u8c, bcs2, base);
        u8g og = {u8bIdleHead(out), u8bIdleHead(out), u8bTerm(out)};
        ok64 ao = DELTApply(dcs, bcs2, og);
        if (ao != OK) {
            fprintf(stderr, "DELT %s: apply failed %s\n", c->name, ok64str(ao));
            u8bUnMap(delta_buf);
            u8bUnMap(out);
            fail(TESTFAIL);
        }

        u64 produced = (u64)(og[1] - og[0]);
        if (produced != u8csLen(target) ||
            memcmp(og[0], target[0], produced) != 0) {
            fprintf(stderr, "DELT %s: result mismatch (%llu bytes)\n",
                    c->name, (unsigned long long)produced);
            u8bUnMap(delta_buf);
            u8bUnMap(out);
            fail(TESTFAIL);
        }

        u8bUnMap(out);
    }

    u8bUnMap(delta_buf);
    done;
}

ok64 DELTroundtrip() {
    sane(1);
    for (size_t i = 0; i < sizeof(CASES)/sizeof(CASES[0]); i++) {
        call(round_trip, &CASES[i]);
    }
    done;
}

//  Back-extension stress: a repeated prefix in the target that
//  overlaps the back-extension region.  Regression test for the old
//  "flush then back-extend" double-emit bug.
ok64 DELTbackext() {
    sane(1);

    a_cstr(base,   "aaaa_BBBB_cccc");
    a_cstr(target, "zzz_BBBB_cccc_aaaa_BBBB_cccc");

    Bu8 delta = {};
    call(u8bMap, delta, 4096);

    a_dup(u8c, bcs, base);
    a_dup(u8c, tcs, target);
    ok64 eo = DELTEncode(bcs, tcs, delta);
    if (eo != OK && eo != DELTFAIL) { u8bUnMap(delta); fail(TESTFAIL); }

    if (eo == OK) {
        Bu8 out = {};
        call(u8bMap, out, 4096);

        a_dup(u8c, dcs, u8bDataC(delta));
        a_dup(u8c, bcs2, base);
        u8g og = {u8bIdleHead(out), u8bIdleHead(out), u8bTerm(out)};
        call(DELTApply, dcs, bcs2, og);

        u64 n = (u64)(og[1] - og[0]);
        want(n == u8csLen(tcs));
        want(memcmp(og[0], tcs[0], n) == 0);
        u8bUnMap(out);
    }
    u8bUnMap(delta);
    done;
}

//  Long-run blob: base and target share a big middle section.  Delta
//  should be much smaller than target.
ok64 DELTlongrun() {
    sane(1);

    //  1 KiB of deterministic-but-random bytes with a deliberate edit.
    enum { N = 1024 };
    a_pad(u8, base,   N);
    a_pad(u8, target, N);
    for (u64 i = 0; i < N; i++) {
        u8 c = (u8)(((i * 1103515245u) + 12345u) >> 16);
        u8bFeed1(base, c);
        u8bFeed1(target, c);
    }
    //  Change 8 bytes in the middle of target.
    u8p t0 = u8bDataHead(target);
    for (int i = 0; i < 8; i++) t0[N/2 + i] = (u8)('A' + i);

    Bu8 delta = {};
    call(u8bMap, delta, 1u << 16);

    a_dup(u8c, bcs, u8bDataC(base));
    a_dup(u8c, tcs, u8bDataC(target));
    ok64 eo = DELTEncode(bcs, tcs, delta);
    want(eo == OK);
    //  Should be a clear win.
    want(u8bDataLen(delta) < N / 2);

    Bu8 out = {};
    call(u8bMap, out, 1u << 16);
    a_dup(u8c, dcs, u8bDataC(delta));
    a_dup(u8c, bcs2, u8bDataC(base));
    u8g og = {u8bIdleHead(out), u8bIdleHead(out), u8bTerm(out)};
    call(DELTApply, dcs, bcs2, og);

    u64 n = (u64)(og[1] - og[0]);
    want(n == N);
    want(memcmp(og[0], t0, N) == 0);

    u8bUnMap(delta);
    u8bUnMap(out);
    done;
}

ok64 maintest() {
    sane(1);
    call(DELTroundtrip);
    call(DELTbackext);
    call(DELTlongrun);
    done;
}

TEST(maintest)
