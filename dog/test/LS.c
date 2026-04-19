//  LS tests: feed paths, round-trip via offset, memory semantics.
//
#include "dog/LS.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

// ---- Test 1: basic feed + get round-trip ----

ok64 LStest1() {
    sane(1);
    ls s = {};
    call(LSOpen, &s, 4096);

    u64 o1 = 0, o2 = 0, o3 = 0;
    a_cstr(p1, "Makefile");
    a_cstr(p2, "src/main.c");
    a_cstr(p3, "");  // empty string

    call(LSFeed, &s, p1, &o1);
    call(LSFeed, &s, p2, &o2);
    call(LSFeed, &s, p3, &o3);

    want(o1 == 0);
    want(o2 == 9);       // 8 + 1 for NUL
    want(o3 == 9 + 11);  // 10 + 1 for NUL
    want(LSLen(&s) == o3 + 1);  // empty + NUL

    u8cs g1 = {}, g2 = {}, g3 = {};
    LSGet(&s, o1, g1);
    LSGet(&s, o2, g2);
    LSGet(&s, o3, g3);

    want(u8csLen(g1) == 8 && memcmp(g1[0], "Makefile", 8) == 0);
    want(u8csLen(g2) == 10 && memcmp(g2[0], "src/main.c", 10) == 0);
    want(u8csLen(g3) == 0);  // empty path

    LSClose(&s);
    done;
}

// ---- Test 2: offsets stable after many feeds ----

ok64 LStest2() {
    sane(1);
    ls s = {};
    call(LSOpen, &s, 1 << 16);

    u64 offs[100];
    char buf[32];
    for (int i = 0; i < 100; i++) {
        int n = snprintf(buf, sizeof(buf), "path/%d/file.txt", i);
        u8csc p = {(u8cp)buf, (u8cp)buf + n};
        call(LSFeed, &s, p, &offs[i]);
    }

    // Read back every entry; contents must match what was fed.
    for (int i = 0; i < 100; i++) {
        int n = snprintf(buf, sizeof(buf), "path/%d/file.txt", i);
        u8cs got = {};
        LSGet(&s, offs[i], got);
        want((int)u8csLen(got) == n);
        want(memcmp(got[0], buf, (size_t)n) == 0);
    }

    LSClose(&s);
    done;
}

// ---- Test 3: NOROOM on over-large feed ----

ok64 LStest3() {
    sane(1);
    ls s = {};
    call(LSOpen, &s, 16);  // tiny buffer

    a_cstr(small, "hi");
    u64 off = 0;
    call(LSFeed, &s, small, &off);
    want(off == 0);
    want(LSLen(&s) == 3);

    //  Fill until full
    ok64 rc = OK;
    while (rc == OK) {
        rc = LSFeed(&s, small, &off);
    }
    want(rc == LSNOROOM);

    LSClose(&s);
    done;
}

ok64 maintest() {
    sane(1);
    call(LStest1);
    call(LStest2);
    call(LStest3);
    done;
}

TEST(maintest)
