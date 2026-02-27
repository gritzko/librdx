#include "BASON.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/NUM.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// Helper: build BASON data into a pad buffer, stack for reading
#define BASON_SETUP(padsize, stksize) \
    a_pad(u8, pad, padsize);          \
    u64 _stk[stksize];               \
    u64b stk = {_stk, _stk, _stk, _stk + stksize};

// 1. Iterate flat stream of leaf records via API A
ok64 BASONtestNextFlat() {
    sane(1);
    BASON_SETUP(256, 32);

    u8cs k1 = $u8str("a"), v1 = $u8str("1");
    u8cs k2 = $u8str("b"), v2 = $u8str("2");
    u8cs k3 = $u8str("c"), v3 = $u8str("3");
    call(TLKVFeed, u8bIdle(pad), 'S', k1, v1);
    call(TLKVFeed, u8bIdle(pad), 'N', k2, v2);
    call(TLKVFeed, u8bIdle(pad), 'B', k3, v3);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);

    u8 type; u8cs key, val;
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(k1, key));
    testeq(0, $cmp(v1, val));

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq(0, $cmp(k2, key));

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'B');
    testeq(0, $cmp(k3, key));

    ok64 o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 2. Into/Outo single container
ok64 BASONtestIntoOuto() {
    sane(1);
    BASON_SETUP(1024, 32);

    u8cs objkey = $u8str("obj");
    call(TLKVInto, pad, 'O', objkey);
    u8cs ckey = $u8str("name"), cval = $u8str("Alice");
    call(TLKVFeed, u8bIdle(pad), 'S', ckey, cval);
    call(TLKVOuto, pad);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    testeq(0, $cmp(objkey, key));

    call(BASONInto, stk, dat, val);

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(ckey, key));
    testeq(0, $cmp(cval, val));

    ok64 o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);

    call(BASONOuto, stk);

    o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 3. Container followed by sibling
ok64 BASONtestSibling() {
    sane(1);
    BASON_SETUP(1024, 32);

    u8cs objkey = $u8str("obj");
    call(TLKVInto, pad, 'O', objkey);
    u8cs ckey = $u8str("x"), cval = $u8str("y");
    call(TLKVFeed, u8bIdle(pad), 'S', ckey, cval);
    call(TLKVOuto, pad);
    u8cs sk = $u8str("after"), sv = $u8str("ok");
    call(TLKVFeed, u8bIdle(pad), 'S', sk, sv);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(ckey, key));
    ok64 o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(sk, key));
    testeq(0, $cmp(sv, val));

    o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 4. Nested containers: O > A > N leaf
ok64 BASONtestNested() {
    sane(1);
    BASON_SETUP(1024, 32);

    u8cs okey = $u8str("root");
    u8cs akey = $u8str("arr");
    u8cs lkey = $u8str("0"), lval = $u8str("42");

    call(TLKVInto, pad, 'O', okey);
    call(TLKVInto, pad, 'A', akey);
    call(TLKVFeed, u8bIdle(pad), 'N', lkey, lval);
    call(TLKVOuto, pad);
    call(TLKVOuto, pad);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    testeq(0, $cmp(okey, key));
    call(BASONInto, stk, dat, val);

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'A');
    testeq(0, $cmp(akey, key));
    call(BASONInto, stk, dat, val);

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq(0, $cmp(lkey, key));
    testeq(0, $cmp(lval, val));

    ok64 o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 5. Early Outo — skip unread children
ok64 BASONtestEarlyOuto() {
    sane(1);
    BASON_SETUP(1024, 32);

    u8cs objkey = $u8str("obj");
    call(TLKVInto, pad, 'O', objkey);
    u8cs k1 = $u8str("a"), v1 = $u8str("1");
    u8cs k2 = $u8str("b"), v2 = $u8str("2");
    u8cs k3 = $u8str("c"), v3 = $u8str("3");
    call(TLKVFeed, u8bIdle(pad), 'S', k1, v1);
    call(TLKVFeed, u8bIdle(pad), 'S', k2, v2);
    call(TLKVFeed, u8bIdle(pad), 'S', k3, v3);
    call(TLKVOuto, pad);
    u8cs sk = $u8str("end"), sv = $u8str("!");
    call(TLKVFeed, u8bIdle(pad), 'S', sk, sv);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(k1, key));
    call(BASONOuto, stk);

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(sk, key));
    testeq(0, $cmp(sv, val));
    done;
}

// 6. API B: bason struct convenience
ok64 BASONtestStructAPI() {
    sane(1);
    BASON_SETUP(1024, 32);

    u8cs objkey = $u8str("obj");
    call(TLKVInto, pad, 'O', objkey);
    u8cs ckey = $u8str("x"), cval = $u8str("y");
    call(TLKVFeed, u8bIdle(pad), 'S', ckey, cval);
    call(TLKVOuto, pad);

    bason x = {.type = 0, .ptype = 0, .data = pad, .stack = stk};

    call(basonOpen, &x);

    call(basonNext, &x);
    testeq(x.type, 'O');
    testeq(0, $cmp(objkey, x.key));

    call(basonInto, &x);
    testeq(x.ptype, 'O');

    call(basonNext, &x);
    testeq(x.type, 'S');
    testeq(0, $cmp(ckey, x.key));
    testeq(0, $cmp(cval, x.val));

    ok64 o = basonNext(&x);
    testeq(o, BASONEND);

    call(basonOuto, &x);

    o = basonNext(&x);
    testeq(o, BASONEND);
    done;
}

// 7. Nested Into/Outo with siblings at each level
ok64 BASONtestNestedSiblings() {
    sane(1);
    BASON_SETUP(2048, 32);

    u8cs okey = $u8str("a");
    call(TLKVInto, pad, 'O', okey);
    u8cs akey = $u8str("arr");
    call(TLKVInto, pad, 'A', akey);
    u8cs n0k = $u8str("0"), n0v = $u8str("1");
    u8cs n1k = $u8str("1"), n1v = $u8str("2");
    call(TLKVFeed, u8bIdle(pad), 'N', n0k, n0v);
    call(TLKVFeed, u8bIdle(pad), 'N', n1k, n1v);
    call(TLKVOuto, pad);
    u8cs zk = $u8str("z"), zv = $u8str("end");
    call(TLKVFeed, u8bIdle(pad), 'S', zk, zv);
    call(TLKVOuto, pad);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'A');
    testeq(0, $cmp(akey, key));
    call(BASONInto, stk, dat, val);

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq(0, $cmp(n0k, key));
    testeq(0, $cmp(n0v, val));

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq(0, $cmp(n1k, key));
    testeq(0, $cmp(n1v, val));

    ok64 o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(zk, key));
    testeq(0, $cmp(zv, val));

    o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 8. Empty container
ok64 BASONtestEmptyContainer() {
    sane(1);
    BASON_SETUP(256, 32);

    u8cs objkey = $u8str("empty");
    call(TLKVInto, pad, 'O', objkey);
    call(TLKVOuto, pad);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    testeq((size_t)$len(val), (size_t)0);

    call(BASONInto, stk, dat, val);
    ok64 o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 9. Write with page index, seek by key prefix
ok64 BASONtestWriteSeek() {
    sane(1);
    a_pad(u8, buf, 4096);
    u64 _idx[128];
    u64b idx = {_idx, _idx, _idx, _idx + 128};
    u64 _rstk[64];
    u64b rstk = {_rstk, _rstk, _rstk, _rstk + 64};

    // Write an Object with sorted keys using BASONw helpers
    u8cs okey = $u8str("root");
    call(BASONwInto, idx, buf, 'O', okey);

    char kbuf[4], vbuf[8];
    for (int i = 0; i < 50; i++) {
        int kn = snprintf(kbuf, sizeof(kbuf), "%03d", i);
        int vn = snprintf(vbuf, sizeof(vbuf), "val%03d", i);
        u8cs k = {(u8c *)kbuf, (u8c *)kbuf + kn};
        u8cs v = {(u8c *)vbuf, (u8c *)vbuf + vn};
        call(BASONwFeed, idx, buf, 'S', k, v);
    }

    call(BASONwOuto, idx, buf);

    // Read back: open, read container, Into, then seek
    u8cs dat = {buf[1], buf[2]};
    call(BASONOpen, rstk, dat);

    u8 type; u8cs key, val;
    call(BASONNext, rstk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, rstk, dat, val);

    // Seek for key "025"
    u8cs target = $u8str("025");
    call(BASONSeek, rstk, dat, target);

    // Read record at seek position — should be a data record
    call(BASONNext, rstk, dat, &type, key, val);
    testeq(type, 'S');

    // Continue iterating — should work normally after seek
    ok64 o = BASONNext(rstk, dat, &type, key, val);
    test(o == OK || o == BASONEND, BASONBAD);

    call(BASONOuto, rstk);
    o = BASONNext(rstk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 10. BASONNext skips index X records
ok64 BASONtestSkipIndex() {
    sane(1);
    a_pad(u8, buf, 4096);
    u64 _idx[128];
    u64b idx = {_idx, _idx, _idx, _idx + 128};
    u64 _rstk[64];
    u64b rstk = {_rstk, _rstk, _rstk, _rstk + 64};

    // Write Object with enough children to generate index
    u8cs okey = $u8str("obj");
    call(BASONwInto, idx, buf, 'O', okey);
    char kbuf[4], vbuf[8];
    int count = 0;
    for (int i = 0; i < 50; i++) {
        int kn = snprintf(kbuf, sizeof(kbuf), "%03d", i);
        int vn = snprintf(vbuf, sizeof(vbuf), "val%03d", i);
        u8cs k = {(u8c *)kbuf, (u8c *)kbuf + kn};
        u8cs v = {(u8c *)vbuf, (u8c *)vbuf + vn};
        call(BASONwFeed, idx, buf, 'S', k, v);
        count++;
    }
    call(BASONwOuto, idx, buf);

    // Read back and count data records (should skip X index)
    u8cs dat = {buf[1], buf[2]};
    call(BASONOpen, rstk, dat);
    u8 type; u8cs key, val;

    call(BASONNext, rstk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, rstk, dat, val);

    int read_count = 0;
    while (BASONNext(rstk, dat, &type, key, val) == OK) {
        testeq(type, 'S');
        read_count++;
    }
    testeq(read_count, count);

    call(BASONOuto, rstk);
    done;
}

// 11. Write 30000 entries {key: NUM(key)} (>1MB), seek each via page index
ok64 BASONtestSeekAll() {
    sane(1);
    u8b buf;
    call(u8bMap, buf, 2 * 1024 * 1024);
    u64 _idx[8192];
    u64b idx = {_idx, _idx, _idx, _idx + 8192};

    int N = 30000;
    u8cs okey = $u8str("nums");
    call(BASONwInto, idx, buf, 'O', okey);

    char kbuf[8];
    u8 vpad[128];
    for (int i = 0; i < N; i++) {
        int kn = snprintf(kbuf, sizeof(kbuf), "%05d", i);
        u8cs k = {(u8c *)kbuf, (u8c *)kbuf + kn};
        u8s vs = {vpad, vpad + sizeof(vpad)};
        call(NUMu8sFeed, vs, (u64)i);
        u8cs v = {(u8c *)vpad, (u8c *)vs[0]};
        call(BASONwFeed, idx, buf, 'S', k, v);
    }
    call(BASONwOuto, idx, buf);

    // Verify >= 1MB
    test(u8bDataLen(buf) >= 1024 * 1024, BASONBAD);

    u8cs dat = {buf[1], buf[2]};

    // Seek and verify every element
    for (int i = 0; i < N; i++) {
        u64 _stk[16];
        u64b stk = {_stk, _stk, _stk, _stk + 16};

        call(BASONOpen, stk, dat);
        u8 type; u8cs key, val;
        call(BASONNext, stk, dat, &type, key, val);
        testeq(type, 'O');
        call(BASONInto, stk, dat, val);

        int kn = snprintf(kbuf, sizeof(kbuf), "%05d", i);
        u8cs target = {(u8c *)kbuf, (u8c *)kbuf + kn};
        call(BASONSeek, stk, dat, target);

        // Scan forward to exact match
        b8 found = NO;
        while (BASONNext(stk, dat, &type, key, val) == OK) {
            if ($len(key) == (size_t)kn &&
                memcmp(key[0], kbuf, kn) == 0) {
                found = YES;
                break;
            }
        }
        test(found, BASONBAD);

        // Verify value = NUM(i)
        u8s vs = {vpad, vpad + sizeof(vpad)};
        call(NUMu8sFeed, vs, (u64)i);
        u8cs expected = {(u8c *)vpad, (u8c *)vs[0]};
        testeq((size_t)$len(val), (size_t)$len(expected));
        testeq(0, memcmp(val[0], expected[0], $len(expected)));
    }

    call(u8bUnMap, buf);
    done;
}

ok64 BASONtest() {
    sane(1);
    call(BASONtestNextFlat);
    call(BASONtestIntoOuto);
    call(BASONtestSibling);
    call(BASONtestNested);
    call(BASONtestEarlyOuto);
    call(BASONtestStructAPI);
    call(BASONtestNestedSiblings);
    call(BASONtestEmptyContainer);
    call(BASONtestWriteSeek);
    call(BASONtestSkipIndex);
    call(BASONtestSeekAll);
    done;
}

TEST(BASONtest);
