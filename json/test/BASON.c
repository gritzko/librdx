#include "BASON.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "abc/FILE.h"
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
    BASON_SETUP(256, 16);

    // write 3 records
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

    // should return END
    ok64 o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 2. Into/Outo single container
ok64 BASONtestIntoOuto() {
    sane(1);
    BASON_SETUP(1024, 16);

    // write: O{key="obj"} containing S{key="name", val="Alice"}
    u8cs objkey = $u8str("obj");
    call(TLKVInto, pad, 'O', objkey);
    u8cs ckey = $u8str("name"), cval = $u8str("Alice");
    call(TLKVFeed, u8bIdle(pad), 'S', ckey, cval);
    call(TLKVOuto, pad);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    // read container
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    testeq(0, $cmp(objkey, key));

    // descend
    call(BASONInto, stk, dat, val);

    // read child
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(ckey, key));
    testeq(0, $cmp(cval, val));

    // no more children
    ok64 o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);

    // ascend
    call(BASONOuto, stk);

    // no more top-level records
    o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    done;
}

// 3. Container followed by sibling — verify Outo restores correctly
ok64 BASONtestSibling() {
    sane(1);
    BASON_SETUP(1024, 16);

    // write: O{...} then S{key="after", val="ok"}
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

    // read container, descend, consume child, ascend
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(ckey, key));
    ok64 o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    // read sibling after the container
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
    BASON_SETUP(1024, 16);

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

    // outer object
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    testeq(0, $cmp(okey, key));
    call(BASONInto, stk, dat, val);

    // inner array
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'A');
    testeq(0, $cmp(akey, key));
    call(BASONInto, stk, dat, val);

    // leaf
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
    BASON_SETUP(1024, 16);

    u8cs objkey = $u8str("obj");
    call(TLKVInto, pad, 'O', objkey);
    u8cs k1 = $u8str("a"), v1 = $u8str("1");
    u8cs k2 = $u8str("b"), v2 = $u8str("2");
    u8cs k3 = $u8str("c"), v3 = $u8str("3");
    call(TLKVFeed, u8bIdle(pad), 'S', k1, v1);
    call(TLKVFeed, u8bIdle(pad), 'S', k2, v2);
    call(TLKVFeed, u8bIdle(pad), 'S', k3, v3);
    call(TLKVOuto, pad);
    // sibling after container
    u8cs sk = $u8str("end"), sv = $u8str("!");
    call(TLKVFeed, u8bIdle(pad), 'S', sk, sv);

    u8cs dat = {pad[1], pad[2]};
    call(BASONOpen, stk, dat);
    u8 type; u8cs key, val;

    // read container, descend, read only first child, then ascend early
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(k1, key));
    // skip remaining children
    call(BASONOuto, stk);

    // should read sibling after container
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'S');
    testeq(0, $cmp(sk, key));
    testeq(0, $cmp(sv, val));
    done;
}

// 6. API B: bason struct convenience
ok64 BASONtestStructAPI() {
    sane(1);
    BASON_SETUP(1024, 16);

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
    BASON_SETUP(2048, 16);

    // O{key="a"} [ A{key="arr"} [ N{0:1}, N{1:2} ], S{key="z", val="end"} ]
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

    // O
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'O');
    call(BASONInto, stk, dat, val);

    // A (first child of O)
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'A');
    testeq(0, $cmp(akey, key));
    call(BASONInto, stk, dat, val);

    // N{0:1}
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq(0, $cmp(n0k, key));
    testeq(0, $cmp(n0v, val));

    // N{1:2}
    call(BASONNext, stk, dat, &type, key, val);
    testeq(type, 'N');
    testeq(0, $cmp(n1k, key));
    testeq(0, $cmp(n1v, val));

    ok64 o = BASONNext(stk, dat, &type, key, val);
    testeq(o, BASONEND);
    call(BASONOuto, stk);

    // S{z:end} (second child of O, sibling of A)
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
    BASON_SETUP(256, 16);

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
    done;
}

TEST(BASONtest);
