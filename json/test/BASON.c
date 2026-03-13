#include "BASON.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/JSON.h"
#include "abc/NUM.h"
#include "abc/PRO.h"
#include "abc/RON.h"
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
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(k1, key));
    testeq(0, $cmp(v1, val));

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq(0, $cmp(k2, key));

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'B');
    testeq(0, $cmp(k3, key));

    ok64 o = BASONDrain(stk, dat, &type, key, val);
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

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    testeq(0, $cmp(objkey, key));

    call(BASONInto, stk, dat, val);

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(ckey, key));
    testeq(0, $cmp(cval, val));

    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);

    call(BASONOuto, stk);

    o = BASONDrain(stk, dat, &type, key, val);
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

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(ckey, key));
    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(sk, key));
    testeq(0, $cmp(sv, val));

    o = BASONDrain(stk, dat, &type, key, val);
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

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    testeq(0, $cmp(okey, key));
    call(BASONInto, stk, dat, val);

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'A');
    testeq(0, $cmp(akey, key));
    call(BASONInto, stk, dat, val);

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq(0, $cmp(lkey, key));
    testeq(0, $cmp(lval, val));

    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    o = BASONDrain(stk, dat, &type, key, val);
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

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(k1, key));
    call(BASONOuto, stk);

    call(BASONDrain, stk, dat, &type, key, val);
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

    call(basonDrain, &x);
    testeq(x.type, 'O');
    testeq(0, $cmp(objkey, x.key));

    call(basonInto, &x);
    testeq(x.ptype, 'O');

    call(basonDrain, &x);
    testeq(x.type, 'S');
    testeq(0, $cmp(ckey, x.key));
    testeq(0, $cmp(cval, x.val));

    ok64 o = basonDrain(&x);
    testeq(o, BASONEND);

    call(basonOuto, &x);

    o = basonDrain(&x);
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

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'A');
    testeq(0, $cmp(akey, key));
    call(BASONInto, stk, dat, val);

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq(0, $cmp(n0k, key));
    testeq(0, $cmp(n0v, val));

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq(0, $cmp(n1k, key));
    testeq(0, $cmp(n1v, val));

    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(zk, key));
    testeq(0, $cmp(zv, val));

    o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    o = BASONDrain(stk, dat, &type, key, val);
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

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    testeq((size_t)$len(val), (size_t)0);

    call(BASONInto, stk, dat, val);
    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    o = BASONDrain(stk, dat, &type, key, val);
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
    call(BASONFeedInto, idx, buf, 'O', okey);

    char kbuf[4], vbuf[8];
    for (int i = 0; i < 50; i++) {
        int kn = snprintf(kbuf, sizeof(kbuf), "%03d", i);
        int vn = snprintf(vbuf, sizeof(vbuf), "val%03d", i);
        u8cs k = {(u8c *)kbuf, (u8c *)kbuf + kn};
        u8cs v = {(u8c *)vbuf, (u8c *)vbuf + vn};
        call(BASONFeed, idx, buf, 'S', k, v);
    }

    call(BASONFeedOuto, idx, buf);

    // Read back: open, read container, Into, then seek
    u8cs dat = {buf[1], buf[2]};
    call(BASONOpen, rstk, dat);

    u8 type; u8cs key, val;
    call(BASONDrain, rstk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, rstk, dat, val);

    // Seek for key "025"
    u8cs target = $u8str("025");
    call(BASONSeek, rstk, dat, target);

    // Read record at seek position — should be a data record
    call(BASONDrain, rstk, dat, &type, key, val);
    testeq(type, 'S');

    // Continue iterating — should work normally after seek
    ok64 o = BASONDrain(rstk, dat, &type, key, val);
    test(o == OK || o == BASONEND, BASONBAD);

    call(BASONOuto, rstk);
    o = BASONDrain(rstk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 10. BASONDrain skips index X records
ok64 BASONtestSkipIndex() {
    sane(1);
    a_pad(u8, buf, 4096);
    u64 _idx[128];
    u64b idx = {_idx, _idx, _idx, _idx + 128};
    u64 _rstk[64];
    u64b rstk = {_rstk, _rstk, _rstk, _rstk + 64};

    // Write Object with enough children to generate index
    u8cs okey = $u8str("obj");
    call(BASONFeedInto, idx, buf, 'O', okey);
    char kbuf[4], vbuf[8];
    int count = 0;
    for (int i = 0; i < 50; i++) {
        int kn = snprintf(kbuf, sizeof(kbuf), "%03d", i);
        int vn = snprintf(vbuf, sizeof(vbuf), "val%03d", i);
        u8cs k = {(u8c *)kbuf, (u8c *)kbuf + kn};
        u8cs v = {(u8c *)vbuf, (u8c *)vbuf + vn};
        call(BASONFeed, idx, buf, 'S', k, v);
        count++;
    }
    call(BASONFeedOuto, idx, buf);

    // Read back and count data records (should skip X index)
    u8cs dat = {buf[1], buf[2]};
    call(BASONOpen, rstk, dat);
    u8 type; u8cs key, val;

    call(BASONDrain, rstk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, rstk, dat, val);

    int read_count = 0;
    while (BASONDrain(rstk, dat, &type, key, val) == OK) {
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
    call(BASONFeedInto, idx, buf, 'O', okey);

    char kbuf[8];
    u8 vpad[128];
    for (int i = 0; i < N; i++) {
        int kn = snprintf(kbuf, sizeof(kbuf), "%05d", i);
        u8cs k = {(u8c *)kbuf, (u8c *)kbuf + kn};
        u8s vs = {vpad, vpad + sizeof(vpad)};
        call(NUMu8sFeed, vs, (u64)i);
        u8cs v = {(u8c *)vpad, (u8c *)vs[0]};
        call(BASONFeed, idx, buf, 'S', k, v);
    }
    call(BASONFeedOuto, idx, buf);

    // Verify >= 1MB
    test(u8bDataLen(buf) >= 1024 * 1024, BASONBAD);

    u8cs dat = {buf[1], buf[2]};

    // Seek and verify every element
    for (int i = 0; i < N; i++) {
        u64 _stk[16];
        u64b stk = {_stk, _stk, _stk, _stk + 16};

        call(BASONOpen, stk, dat);
        u8 type; u8cs key, val;
        call(BASONDrain, stk, dat, &type, key, val);
        testeq(type, 'O');
        call(BASONInto, stk, dat, val);

        int kn = snprintf(kbuf, sizeof(kbuf), "%05d", i);
        u8cs target = {(u8c *)kbuf, (u8c *)kbuf + kn};
        call(BASONSeek, stk, dat, target);

        // Scan forward to exact match
        b8 found = NO;
        while (BASONDrain(stk, dat, &type, key, val) == OK) {
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

// --- JSON → BASON parse tests ---

// 12. Parse scalar string
ok64 BASONtestParseString() {
    sane(1);
    BASON_SETUP(1024, 32);
    u64 _idx[64];
    u64b idx = {_idx, _idx, _idx, _idx + 64};

    u8cs json = $u8str("\"hello\"");
    call(BASONParseJSON, pad, idx, json);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq((size_t)$len(key), (size_t)0);
    u8cs exp = $u8str("hello");
    testeq(0, $cmp(exp, val));

    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 13. Parse scalar number
ok64 BASONtestParseNumber() {
    sane(1);
    BASON_SETUP(1024, 32);
    u64 _idx[64];
    u64b idx = {_idx, _idx, _idx, _idx + 64};

    u8cs json = $u8str("42");
    call(BASONParseJSON, pad, idx, json);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq((size_t)$len(key), (size_t)0);
    u8cs exp = $u8str("42");
    testeq(0, $cmp(exp, val));
    done;
}

// 14. Parse literals: true, false, null
ok64 BASONtestParseLiterals() {
    sane(1);
    u8 type; u8cs key, val;

    // true
    {
        BASON_SETUP(1024, 32);
        u64 _idx[64];
        u64b idx = {_idx, _idx, _idx, _idx + 64};
        u8cs json = $u8str("true");
        call(BASONParseJSON, pad, idx, json);
        u8cs dat = {pad[1], pad[2]};
        call(BASONOpen, stk, dat);
        call(BASONDrain, stk, dat, &type, key, val);
        testeq(type, 'B');
        u8cs exp = $u8str("true");
        testeq(0, $cmp(exp, val));
    }
    // false
    {
        BASON_SETUP(1024, 32);
        u64 _idx[64];
        u64b idx = {_idx, _idx, _idx, _idx + 64};
        u8cs json = $u8str("false");
        call(BASONParseJSON, pad, idx, json);
        u8cs dat = {pad[1], pad[2]};
        call(BASONOpen, stk, dat);
        call(BASONDrain, stk, dat, &type, key, val);
        testeq(type, 'B');
        u8cs exp = $u8str("false");
        testeq(0, $cmp(exp, val));
    }
    // null → type 'B', empty value
    {
        BASON_SETUP(1024, 32);
        u64 _idx[64];
        u64b idx = {_idx, _idx, _idx, _idx + 64};
        u8cs json = $u8str("null");
        call(BASONParseJSON, pad, idx, json);
        u8cs dat = {pad[1], pad[2]};
        call(BASONOpen, stk, dat);
        call(BASONDrain, stk, dat, &type, key, val);
        testeq(type, 'B');
        testeq((size_t)$len(val), (size_t)0);
    }
    done;
}

// 15. Flat object
ok64 BASONtestParseObject() {
    sane(1);
    BASON_SETUP(1024, 32);
    u64 _idx[64];
    u64b idx = {_idx, _idx, _idx, _idx + 64};

    u8cs json = $u8str("{\"a\":\"b\",\"c\":\"d\"}");
    call(BASONParseJSON, pad, idx, json);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    testeq((size_t)$len(key), (size_t)0);

    call(BASONInto, stk, dat, val);

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    u8cs ka = $u8str("a"), vb = $u8str("b");
    testeq(0, $cmp(ka, key));
    testeq(0, $cmp(vb, val));

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    u8cs kc = $u8str("c"), vd = $u8str("d");
    testeq(0, $cmp(kc, key));
    testeq(0, $cmp(vd, val));

    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 16. Array with RON64 keys
ok64 BASONtestParseArray() {
    sane(1);
    BASON_SETUP(1024, 32);
    u64 _idx[64];
    u64b idx = {_idx, _idx, _idx, _idx + 64};

    u8cs json = $u8str("[1,2,3]");
    call(BASONParseJSON, pad, idx, json);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'A');

    call(BASONInto, stk, dat, val);

    // Element 0 → key "1"
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    u8cs k1 = $u8str("1");
    testeq(0, $cmp(k1, key));
    u8cs v1 = $u8str("1");
    testeq(0, $cmp(v1, val));

    // Element 1 → key "2"
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    u8cs k2 = $u8str("2");
    testeq(0, $cmp(k2, key));
    u8cs v2 = $u8str("2");
    testeq(0, $cmp(v2, val));

    // Element 2 → key "3"
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    u8cs k3 = $u8str("3");
    testeq(0, $cmp(k3, key));
    u8cs v3 = $u8str("3");
    testeq(0, $cmp(v3, val));

    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 17. Nested: {"arr":[1,{"x":true}]}
ok64 BASONtestParseNested() {
    sane(1);
    BASON_SETUP(2048, 32);
    u64 _idx[128];
    u64b idx = {_idx, _idx, _idx, _idx + 128};

    u8cs json = $u8str("{\"arr\":[1,{\"x\":true}]}");
    call(BASONParseJSON, pad, idx, json);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    // Top-level object
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);

    // "arr" → array
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'A');
    u8cs karr = $u8str("arr");
    testeq(0, $cmp(karr, key));
    call(BASONInto, stk, dat, val);

    // [0] → 1
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    u8cs k1 = $u8str("1");
    testeq(0, $cmp(k1, key));

    // [1] → {"x":true}
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    u8cs k2 = $u8str("2");
    testeq(0, $cmp(k2, key));
    call(BASONInto, stk, dat, val);

    // "x" → true
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'B');
    u8cs kx = $u8str("x");
    testeq(0, $cmp(kx, key));
    u8cs vtrue = $u8str("true");
    testeq(0, $cmp(vtrue, val));

    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);  // out of {"x":true}

    o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);  // out of array

    o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);  // out of top object

    o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 18. Escape handling
ok64 BASONtestParseEscapes() {
    sane(1);
    BASON_SETUP(1024, 32);
    u64 _idx[64];
    u64b idx = {_idx, _idx, _idx, _idx + 64};

    u8cs json = $u8str("{\"k\":\"a\\tb\"}");
    call(BASONParseJSON, pad, idx, json);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    u8cs kk = $u8str("k");
    testeq(0, $cmp(kk, key));
    // Value should be "a\tb" (with literal tab)
    testeq((size_t)$len(val), (size_t)3);
    testeq(*val[0], 'a');
    testeq(*(val[0] + 1), '\t');
    testeq(*(val[0] + 2), 'b');
    done;
}

// 19. Roundtrip: parse JSON → BASON → iterate all and verify
ok64 BASONtestParseRoundtrip() {
    sane(1);
    BASON_SETUP(4096, 64);
    u64 _idx[128];
    u64b idx = {_idx, _idx, _idx, _idx + 128};

    u8cs json = $u8str(
        "{\"name\":\"Alice\",\"age\":30,\"active\":true,"
        "\"scores\":[100,200,300],\"addr\":{\"city\":\"NYC\"},\"x\":null}");
    call(BASONParseJSON, pad, idx, json);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    // Root object
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);

    // Keys are lex-sorted: active, addr, age, name, scores, x

    // "active" → true
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'B');
    u8cs kact = $u8str("active"), vtrue = $u8str("true");
    testeq(0, $cmp(kact, key));
    testeq(0, $cmp(vtrue, val));

    // "addr" → object with "city":"NYC"
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'O');
    u8cs kaddr = $u8str("addr");
    testeq(0, $cmp(kaddr, key));
    call(BASONInto, stk, dat, val);

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    u8cs kcity = $u8str("city"), vnyc = $u8str("NYC");
    testeq(0, $cmp(kcity, key));
    testeq(0, $cmp(vnyc, val));

    ok64 o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    // "age" → 30
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    u8cs kage = $u8str("age"), v30 = $u8str("30");
    testeq(0, $cmp(kage, key));
    testeq(0, $cmp(v30, val));

    // "name" → "Alice"
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'S');
    u8cs kname = $u8str("name"), valice = $u8str("Alice");
    testeq(0, $cmp(kname, key));
    testeq(0, $cmp(valice, val));

    // "scores" → array
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'A');
    u8cs kscores = $u8str("scores");
    testeq(0, $cmp(kscores, key));
    call(BASONInto, stk, dat, val);

    // [0]=100, [1]=200, [2]=300
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    u8cs v100 = $u8str("100");
    testeq(0, $cmp(v100, val));

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    u8cs v200 = $u8str("200");
    testeq(0, $cmp(v200, val));

    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'N');
    u8cs v300 = $u8str("300");
    testeq(0, $cmp(v300, val));

    o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    // "x" → null (type B, empty value)
    call(BASONDrain, stk, dat, &type, key, val);
    testeq(type, 'B');
    u8cs kx = $u8str("x");
    testeq(0, $cmp(kx, key));
    testeq((size_t)$len(val), (size_t)0);

    o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    o = BASONDrain(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// --- BASON → JSON export tests ---

// 20. Export flat object
ok64 BASONtestExportObject() {
    sane(1);
    BASON_SETUP(1024, 32);
    u64 _idx[64];
    u64b idx = {_idx, _idx, _idx, _idx + 64};

    u8cs json = $u8str("{\"a\":\"b\",\"c\":\"d\"}");
    call(BASONParseJSON, pad, idx, json);

    u8cs dat = {pad[1], pad[2]};
    u8 _out[1024];
    u8s out = {_out, _out + sizeof(_out)};
    call(BASONExportJSON, out, stk, dat);

    u8cs result = {(u8cp)_out, (u8cp)out[0]};
    testeq(0, $cmp(json, result));
    done;
}

// 21. Export array
ok64 BASONtestExportArray() {
    sane(1);
    BASON_SETUP(1024, 32);
    u64 _idx[64];
    u64b idx = {_idx, _idx, _idx, _idx + 64};

    u8cs json = $u8str("[1,2,3]");
    call(BASONParseJSON, pad, idx, json);

    u8cs dat = {pad[1], pad[2]};
    u8 _out[1024];
    u8s out = {_out, _out + sizeof(_out)};
    call(BASONExportJSON, out, stk, dat);

    u8cs result = {(u8cp)_out, (u8cp)out[0]};
    testeq(0, $cmp(json, result));
    done;
}

// 22. Export with escapes
ok64 BASONtestExportEscapes() {
    sane(1);
    BASON_SETUP(1024, 32);
    u64 _idx[64];
    u64b idx = {_idx, _idx, _idx, _idx + 64};

    u8cs json = $u8str("{\"k\":\"a\\tb\"}");
    call(BASONParseJSON, pad, idx, json);

    u8cs dat = {pad[1], pad[2]};
    u8 _out[1024];
    u8s out = {_out, _out + sizeof(_out)};
    call(BASONExportJSON, out, stk, dat);

    u8cs result = {(u8cp)_out, (u8cp)out[0]};
    testeq(0, $cmp(json, result));
    done;
}

// 23. Full roundtrip: JSON → BASON → JSON, compare compact forms
ok64 BASONtestJSONRoundtrip() {
    sane(1);

    u8cs cases[] = {
        $u8str("\"hello\""),
        $u8str("42"),
        $u8str("-3.14"),
        $u8str("true"),
        $u8str("false"),
        $u8str("null"),
        $u8str("[]"),
        $u8str("{}"),
        $u8str("[1,2,3]"),
        $u8str("{\"a\":\"b\"}"),
        $u8str("{\"active\":true,\"addr\":{\"city\":\"NYC\"},\"age\":30,"
               "\"name\":\"Alice\",\"scores\":[100,200,300],\"x\":null}"),
        $u8str("{\"a\":[1,[2,[3]]],\"b\":{\"c\":{\"d\":\"e\"}}}"),
        $u8str("{\"esc\":\"a\\tb\\nc\\\\d\\\"\"}"),
        $u8str("[{},{},{}]"),
        $u8str("{\"empty_arr\":[],\"empty_obj\":{}}"),
    };
    size_t ncases = sizeof(cases) / sizeof(cases[0]);

    for (size_t i = 0; i < ncases; i++) {
        // Compact-format the original (normalize whitespace)
        u8 _fmt[4096];
        u8s fmt = {_fmt, _fmt + sizeof(_fmt)};
        u8cs indent = {(u8cp)"", (u8cp)""};
        call(JSONFmt, fmt, cases[i], indent);
        u8cs expected = {(u8cp)_fmt, (u8cp)fmt[0]};

        // Parse JSON → BASON
        BASON_SETUP(4096, 64);
        u64 _idx[128];
        u64b idx = {_idx, _idx, _idx, _idx + 128};
        call(BASONParseJSON, pad, idx, cases[i]);

        // Export BASON → JSON
        u8cs dat = {pad[1], pad[2]};
        u8 _out[4096];
        u8s out = {_out, _out + sizeof(_out)};
        call(BASONExportJSON, out, stk, dat);

        u8cs result = {(u8cp)_out, (u8cp)out[0]};
        if ($cmp(expected, result) != 0) {
            fprintf(stderr, "roundtrip[%zu] mismatch:\n  expected: %.*s\n  got:      %.*s\n",
                    i, (int)$len(expected), expected[0],
                    (int)$len(result), result[0]);
            fail(TESTFAIL);
        }
    }
    done;
}

// 24. BASONFeedInc — fixed-width increment
ok64 BASONtestFeedInc() {
    sane(1);
    u8 _out[16];
    u8s out;
    u8cs result;

    // Single char: "0" → "1"
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs k0 = $u8str("0");
    call(BASONFeedInc, out, k0);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs exp1 = $u8str("1");
    testeq(0, $cmp(exp1, result));

    // "A" → "B"
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs kA = $u8str("A");
    call(BASONFeedInc, out, kA);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs expB = $u8str("B");
    testeq(0, $cmp(expB, result));

    // Two chars with carry: "0~" → "10"
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs k0t = $u8str("0~");
    call(BASONFeedInc, out, k0t);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs exp10 = $u8str("10");
    testeq(0, $cmp(exp10, result));

    // Overflow: "~" → SBADARG
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs kt = $u8str("~");
    ok64 o = BASONFeedInc(out, kt);
    testeq(o, SBADARG);

    // Overflow: "~~" → SBADARG
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs ktt = $u8str("~~");
    o = BASONFeedInc(out, ktt);
    testeq(o, SBADARG);

    done;
}

// 25. BASONFeedInfInc — variable-width sequential increment
ok64 BASONtestFeedInfInc() {
    sane(1);
    u8 _out[32];
    u8s out;
    u8cs result;

    // Empty → "1"
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs empty = {NULL, NULL};
    call(BASONFeedInfInc, out, empty);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs exp1 = $u8str("1");
    testeq(0, $cmp(exp1, result));

    // "1" → "2"
    out[0] = _out; out[1] = _out + sizeof(_out);
    call(BASONFeedInfInc, out, exp1);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs exp2 = $u8str("2");
    testeq(0, $cmp(exp2, result));

    // "V" → "W" (width-1 max → width-2 prefix)
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs kV = $u8str("V");
    call(BASONFeedInfInc, out, kV);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs expW = $u8str("W");
    testeq(0, $cmp(expW, result));

    // "W" → "W1" (short key padded to width 2, trailing zero banned)
    out[0] = _out; out[1] = _out + sizeof(_out);
    call(BASONFeedInfInc, out, expW);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs expW1 = $u8str("W1");
    testeq(0, $cmp(expW1, result));

    // "W1" → "W2"
    out[0] = _out; out[1] = _out + sizeof(_out);
    call(BASONFeedInfInc, out, expW1);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs expW2 = $u8str("W2");
    testeq(0, $cmp(expW2, result));

    // "W~" → "X" (digit[1] overflow → increment digit[0], trim)
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs kWt = $u8str("W~");
    call(BASONFeedInfInc, out, kWt);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs expX = $u8str("X");
    testeq(0, $cmp(expX, result));

    // "X" → "X1" (trailing zero banned)
    out[0] = _out; out[1] = _out + sizeof(_out);
    call(BASONFeedInfInc, out, expX);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs expX1 = $u8str("X1");
    testeq(0, $cmp(expX1, result));

    // Trim: "W5Z" (3 chars, but P=1, W=2) → "W6" (trim extra)
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs kW5Z = $u8str("W5Z");
    call(BASONFeedInfInc, out, kW5Z);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    u8cs expW6 = $u8str("W6");
    testeq(0, $cmp(expW6, result));

    // Monotonicity: generate 2000 sequential keys, verify strictly increasing
    u8 keys[2000][8];
    u8 lens[2000];
    u8cs prevk = {NULL, NULL};
    for (int i = 0; i < 2000; i++) {
        out[0] = keys[i]; out[1] = keys[i] + 8;
        call(BASONFeedInfInc, out, prevk);
        lens[i] = (u8)(out[0] - keys[i]);
        u8cs curk = {(u8cp)keys[i], (u8cp)keys[i] + lens[i]};
        if (i > 0) {
            test($cmp(prevk, curk) < 0, TESTFAIL);
        }
        prevk[0] = (u8cp)keys[i];
        prevk[1] = (u8cp)keys[i] + lens[i];
    }
    // After 2000 keys should be in width-2 territory
    test(lens[1999] >= 2, TESTFAIL);

    done;
}

// 26. BASONFindMid — midpoint key between bounds
ok64 BASONtestFindMid() {
    sane(1);
    u8 _out[32];
    u8s out;
    u8cs result;
    u8cs no = {NULL, NULL};

    // Between empty bounds
    out[0] = _out; out[1] = _out + sizeof(_out);
    call(BASONFindMid, out, no, no, 1, 2, 12345);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    test($len(result) > 0, TESTFAIL);

    // Between "A" and "Z": result strictly between
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs loA = $u8str("A"), hiZ = $u8str("Z");
    call(BASONFindMid, out, loA, hiZ, 1, 2, 42);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    test($cmp(loA, result) < 0, TESTFAIL);
    test($cmp(result, hiZ) < 0, TESTFAIL);

    // Left-open: between nothing and "V"
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs hiV = $u8str("V");
    call(BASONFindMid, out, no, hiV, 1, 2, 99);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    test($cmp(result, hiV) < 0, TESTFAIL);

    // Right-open: between "V" and nothing
    out[0] = _out; out[1] = _out + sizeof(_out);
    call(BASONFindMid, out, hiV, no, 1, 2, 99);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    test($cmp(hiV, result) < 0, TESTFAIL);

    // Multi-char bounds: between "W0" and "W~"
    out[0] = _out; out[1] = _out + sizeof(_out);
    u8cs loW0 = $u8str("W0"), hiWt = $u8str("W~");
    call(BASONFindMid, out, loW0, hiWt, 1, 2, 777);
    result[0] = (u8cp)_out; result[1] = (u8cp)out[0];
    test($cmp(loW0, result) < 0, TESTFAIL);
    test($cmp(result, hiWt) < 0, TESTFAIL);

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
    call(BASONtestParseString);
    call(BASONtestParseNumber);
    call(BASONtestParseLiterals);
    call(BASONtestParseObject);
    call(BASONtestParseArray);
    call(BASONtestParseNested);
    call(BASONtestParseEscapes);
    call(BASONtestParseRoundtrip);
    call(BASONtestExportObject);
    call(BASONtestExportArray);
    call(BASONtestExportEscapes);
    call(BASONtestJSONRoundtrip);
    call(BASONtestFeedInc);
    call(BASONtestFeedInfInc);
    call(BASONtestFindMid);
    done;
}

TEST(BASONtest);
