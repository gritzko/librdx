#include "be/VER.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

// ---- Test 1: Constructor/accessor roundtrip ----

ok64 VERtest1() {
    sane(1);
    struct {
        ron60 time;
        ron60 origin;
        u8 op;
    } cases[] = {
        {0, 0x123, VER_ANY},
        {0x456, 0x789, VER_LE},
        {0xABC, 0xDEF, VER_GT},
        {0x100, 0x200, VER_EQ},
        {0x0FFFFFFFFFFFFFFFUL, 0x0FFFFFFFFFFFFFFFUL, VER_GT},
        {0, 0, VER_ANY},
    };
    int n = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < n; i++) {
        ron120 v = VERMake(cases[i].time, cases[i].origin, cases[i].op);
        same(VERTime(&v), cases[i].time);
        same(VEROrigin(&v), cases[i].origin);
        same(VEROp(&v), cases[i].op);
    }
    done;
}

// ---- Test 2: u128cmp ordering — pure points sort chronologically ----

ok64 VERtest2() {
    sane(1);
    // Same origin, different times: should sort by time
    ron120 a = VERPoint(100, 0x123);
    ron120 b = VERPoint(200, 0x123);
    ron120 c = VERPoint(300, 0x123);
    want(u128cmp(&a, &b) < 0);
    want(u128cmp(&b, &c) < 0);
    want(u128cmp(&a, &c) < 0);

    // Same time, different origins: should sort by origin
    ron120 d = VERPoint(100, 0x10);
    ron120 e = VERPoint(100, 0x20);
    want(u128cmp(&d, &e) < 0);

    // Equal points
    ron120 f = VERPoint(100, 0x123);
    want(u128cmp(&a, &f) == 0);
    done;
}

// ---- Test 3: VERFormParse ----

ok64 VERtest3() {
    sane(1);
    struct {
        const char *query;
        int count;
        u8 ops[4];
    } cases[] = {
        {"main", 2, {VER_ANY, VER_ANY}},          // +base
        {"abc-main", 2, {VER_LE, VER_ANY}},        // +base
        {"abc+main", 1, {VER_GT}},                 // no base
        {"abc=main", 1, {VER_EQ}},                 // no base
        {"main&feat", 3, {VER_ANY, VER_ANY, VER_ANY}}, // +base
        {"abc-main&feat", 3, {VER_LE, VER_ANY, VER_ANY}}, // +base
        {"", 0, {}},
    };
    int n = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < n; i++) {
        ron120 fbuf[VER_MAX];
        ron120s form = {fbuf, fbuf + VER_MAX};
        u8cs q = {(u8cp)cases[i].query,
                  (u8cp)cases[i].query + strlen(cases[i].query)};
        call(VERFormParse, form, q);
        int count = form[0] - fbuf;
        same(count, cases[i].count);
        for (int j = 0; j < count; j++) {
            same(VEROp(&fbuf[j]), cases[i].ops[j]);
        }
    }
    done;
}

// ---- Test 4: VERFormMatch ----

ok64 VERtest4() {
    sane(1);
    u8cs q_main = {(u8cp)"main", (u8cp)"main" + 4};
    ron60 main_ron = 0;
    call(RONutf8sDrain, &main_ron, q_main);

    // VER_ANY: matches any timestamp
    {
        ron120 fbuf[1];
        fbuf[0] = VERMake(0, main_ron, VER_ANY);
        ron120cs form = {fbuf, fbuf + 1};
        want(VERFormMatch(form, 100, main_ron) == YES);
        want(VERFormMatch(form, 999, main_ron) == YES);
        want(VERFormMatch(form, 0, main_ron) == YES);
        want(VERFormMatch(form, 100, main_ron + 1) == NO);
    }

    // VER_LE: time <= entry_time
    {
        ron120 fbuf[1];
        fbuf[0] = VERMake(500, main_ron, VER_LE);
        ron120cs form = {fbuf, fbuf + 1};
        want(VERFormMatch(form, 100, main_ron) == YES);
        want(VERFormMatch(form, 500, main_ron) == YES);
        want(VERFormMatch(form, 501, main_ron) == NO);
    }

    // VER_GT: time > entry_time
    {
        ron120 fbuf[1];
        fbuf[0] = VERMake(500, main_ron, VER_GT);
        ron120cs form = {fbuf, fbuf + 1};
        want(VERFormMatch(form, 501, main_ron) == YES);
        want(VERFormMatch(form, 999, main_ron) == YES);
        want(VERFormMatch(form, 500, main_ron) == NO);
        want(VERFormMatch(form, 100, main_ron) == NO);
    }

    // VER_EQ: time == entry_time
    {
        ron120 fbuf[1];
        fbuf[0] = VERMake(500, main_ron, VER_EQ);
        ron120cs form = {fbuf, fbuf + 1};
        want(VERFormMatch(form, 500, main_ron) == YES);
        want(VERFormMatch(form, 499, main_ron) == NO);
        want(VERFormMatch(form, 501, main_ron) == NO);
    }

    // Multi-entry: match any
    {
        ron60 feat_ron = 0;
        u8cs q_feat = {(u8cp)"feat", (u8cp)"feat" + 4};
        call(RONutf8sDrain, &feat_ron, q_feat);

        ron120 fbuf[2];
        fbuf[0] = VERMake(0, main_ron, VER_ANY);
        fbuf[1] = VERMake(300, feat_ron, VER_LE);
        ron120cs form = {fbuf, fbuf + 2};
        want(VERFormMatch(form, 999, main_ron) == YES);
        want(VERFormMatch(form, 200, feat_ron) == YES);
        want(VERFormMatch(form, 300, feat_ron) == YES);
        want(VERFormMatch(form, 301, feat_ron) == NO);
    }
    done;
}

// ---- Test 5: VERutf8Feed roundtrip ----

ok64 VERtest5() {
    sane(1);
    ron60 main_ron = 0;
    u8cs q_main = {(u8cp)"main", (u8cp)"main" + 4};
    call(RONutf8sDrain, &main_ron, q_main);

    // VER_LE roundtrip (+base entry)
    ron120 v = VERMake(0x123, main_ron, VER_LE);
    u8 buf[64];
    u8s into = {buf, buf + sizeof(buf)};
    call(VERutf8Feed, into, &v);
    u8cs text = {buf, into[0]};
    want(!$empty(text));

    ron120 fbuf[VER_MAX];
    ron120s form = {fbuf, fbuf + VER_MAX};
    call(VERFormParse, form, text);
    same((int)(form[0] - fbuf), 2);
    same(VERTime(&fbuf[0]), 0x123);
    same(VEROrigin(&fbuf[0]), main_ron);
    same(VEROp(&fbuf[0]), VER_LE);
    same(VEROrigin(&fbuf[1]), 0);
    same(VEROp(&fbuf[1]), VER_ANY);

    // VER_GT roundtrip (no base entry)
    ron120 v2 = VERMake(0x456, main_ron, VER_GT);
    into[0] = buf;
    call(VERutf8Feed, into, &v2);
    u8cs text2 = {buf, into[0]};
    form[0] = fbuf;
    call(VERFormParse, form, text2);
    same((int)(form[0] - fbuf), 1);
    same(VERTime(&fbuf[0]), 0x456);
    same(VEROp(&fbuf[0]), VER_GT);

    // VER_EQ roundtrip (no base entry)
    ron120 v3 = VERMake(0x789, main_ron, VER_EQ);
    into[0] = buf;
    call(VERutf8Feed, into, &v3);
    u8cs text3 = {buf, into[0]};
    form[0] = fbuf;
    call(VERFormParse, form, text3);
    same((int)(form[0] - fbuf), 1);
    same(VERTime(&fbuf[0]), 0x789);
    same(VEROp(&fbuf[0]), VER_EQ);

    // VER_ANY (time=0, just origin) (+base entry)
    ron120 v4 = VERMake(0, main_ron, VER_ANY);
    into[0] = buf;
    call(VERutf8Feed, into, &v4);
    u8cs text4 = {buf, into[0]};
    form[0] = fbuf;
    call(VERFormParse, form, text4);
    same((int)(form[0] - fbuf), 2);
    same(VEROrigin(&fbuf[0]), main_ron);
    same(VEROp(&fbuf[0]), VER_ANY);

    done;
}

// ---- Test 6: Base key (0,0) inclusion via formula ----

ok64 VERtest6() {
    sane(1);
    struct {
        const char *query;
        b8 base_included;
    } cases[] = {
        {"main", YES},              // ANY → base included
        {"main&feat", YES},         // all ANY → base included
        {"abc-main", YES},          // LE → base included
        {"abc-main&feat", YES},     // LE + ANY → base included
        {"abc+main", NO},           // GT only → no base
        {"abc=main", NO},           // EQ only → no base
        {"abc+main&def=feat", NO},  // GT + EQ → no base
        {"abc+main&feat", YES},     // GT + ANY → base included
        {"abc-main&def+feat", YES}, // LE + GT → base included
    };
    int n = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < n; i++) {
        ron120 fbuf[VER_MAX];
        ron120s form = {fbuf, fbuf + VER_MAX};
        u8cs q = {(u8cp)cases[i].query,
                  (u8cp)cases[i].query + strlen(cases[i].query)};
        call(VERFormParse, form, q);
        b8 got = VERFormMatch((ron120cs){fbuf, form[0]}, 0, 0);
        want(got == cases[i].base_included);
    }

    // VERFormFromBranches: all-ANY, base included
    {
        ron60 main_ron = 0;
        u8cs q_main = $u8str("main");
        call(RONutf8sDrain, &main_ron, q_main);
        ron120 branches[1];
        branches[0] = VERMake(0, main_ron, VER_ANY);
        ron120 fbuf[VER_MAX];
        ron120s form = {fbuf, fbuf + VER_MAX};
        call(VERFormFromBranches, form, 1, branches);
        want(VERFormMatch((ron120cs){fbuf, form[0]}, 0, 0) == YES);
    }
    done;
}

ok64 maintest() {
    sane(1);
    call(VERtest1);
    call(VERtest2);
    call(VERtest3);
    call(VERtest4);
    call(VERtest5);
    call(VERtest6);
    done;
}

TEST(maintest)
