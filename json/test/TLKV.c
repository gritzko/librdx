#include "TLKV.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// 1. Short form roundtrip for all 5 BASON types
ok64 TLKVtestShortRoundtrip() {
    sane(1);
    a_pad(u8, pad, 256);
    u8cs key = $u8str("ab");
    u8cs val = $u8str("xyz");
    u8 types[] = {'B', 'A', 'S', 'O', 'N'};
    for (int i = 0; i < 5; i++) {
        u8bReset(pad);
        call(TLKVFeed, u8bIdle(pad), types[i], key, val);
        u8cs rkey, rval;
        u8 rtype;
        u8c **from = u8bDataC(pad);
        call(TLKVDrain, from, &rtype, rkey, rval);
        testeq(rtype, types[i]);
        testeq(0, $cmp(key, rkey));
        testeq(0, $cmp(val, rval));
    }
    done;
}

// 2. Long form roundtrip (key too long: 16 bytes)
ok64 TLKVtestLongKey() {
    sane(1);
    a_pad(u8, pad, 256);
    u8cs key = $u8str("0123456789abcdef");  // 16 bytes
    u8cs val = $u8str("v");
    call(TLKVFeed, u8bIdle(pad), 'S', key, val);
    // must be long form: first byte uppercase
    testeq(*pad[1] & TLVaA, 0);
    u8cs rkey, rval;
    u8 rtype;
    u8c **from = u8bDataC(pad);
    call(TLKVDrain, from, &rtype, rkey, rval);
    testeq(rtype, 'S');
    testeq(0, $cmp(key, rkey));
    testeq(0, $cmp(val, rval));
    done;
}

// 3. Long form roundtrip (val too long: 16 bytes)
ok64 TLKVtestLongVal() {
    sane(1);
    a_pad(u8, pad, 256);
    u8cs key = $u8str("k");
    u8cs val = $u8str("0123456789abcdef");  // 16 bytes
    call(TLKVFeed, u8bIdle(pad), 'N', key, val);
    testeq(*pad[1] & TLVaA, 0);
    u8cs rkey, rval;
    u8 rtype;
    u8c **from = u8bDataC(pad);
    call(TLKVDrain, from, &rtype, rkey, rval);
    testeq(rtype, 'N');
    testeq(0, $cmp(key, rkey));
    testeq(0, $cmp(val, rval));
    done;
}

// 4. Exact binary encoding (short form)
// type='S', key="ab", val="xyz"
// expected: [0x73, 0x23, 'a', 'b', 'x', 'y', 'z']
ok64 TLKVtestExactShort() {
    sane(1);
    a_pad(u8, pad, 64);
    u8cs key = $u8str("ab");
    u8cs val = $u8str("xyz");
    call(TLKVFeed, u8bIdle(pad), 'S', key, val);
    u8 expected[] = {0x73, 0x23, 'a', 'b', 'x', 'y', 'z'};
    a$(u8c, exp, expected);
    u8c **data = u8bDataC(pad);
    $testeq(exp, data);
    done;
}

// 5. Exact binary encoding (long form)
// type='S', key="a", val=20 bytes
// header: [0x53, 0x14,0x00,0x00,0x00, 0x01, 'a', val...]
ok64 TLKVtestExactLong() {
    sane(1);
    a_pad(u8, pad, 64);
    u8cs key = $u8str("a");
    u8c valbuf[20];
    memset((void *)valbuf, 'x', 20);
    u8cs val = {valbuf, valbuf + 20};
    call(TLKVFeed, u8bIdle(pad), 'S', key, val);
    u8c **data = u8bDataC(pad);
    // check header bytes
    testeq($at(data, 0), 0x53);  // 'S' uppercase = long
    testeq($at(data, 1), 0x14);  // vlen=20 LE byte 0
    testeq($at(data, 2), 0x00);
    testeq($at(data, 3), 0x00);
    testeq($at(data, 4), 0x00);
    testeq($at(data, 5), 0x01);  // klen=1
    testeq($at(data, 6), 'a');   // key
    testeq($len(data), 6 + 1 + 20);  // header + key + val
    done;
}

// 6. Boundary: max short (15+15)
ok64 TLKVtestMaxShort() {
    sane(1);
    a_pad(u8, pad, 64);
    u8c kbuf[15], vbuf[15];
    memset((void *)kbuf, 'k', 15);
    memset((void *)vbuf, 'v', 15);
    u8cs key = {kbuf, kbuf + 15};
    u8cs val = {vbuf, vbuf + 15};
    call(TLKVFeed, u8bIdle(pad), 'S', key, val);
    // should be short: tag has TLVaA bit
    testeq(*pad[1] & TLVaA, TLVaA);
    // nibbles should be 0xFF
    testeq(*(pad[1] + 1), 0xff);
    testeq((size_t)u8bDataLen(pad), (size_t)(2 + 15 + 15));
    // roundtrip
    u8cs rkey, rval;
    u8 rtype;
    u8c **from = u8bDataC(pad);
    call(TLKVDrain, from, &rtype, rkey, rval);
    testeq(0, $cmp(key, rkey));
    testeq(0, $cmp(val, rval));
    done;
}

// 7. Boundary: min long (16+0)
ok64 TLKVtestMinLong() {
    sane(1);
    a_pad(u8, pad, 64);
    u8c kbuf[16];
    memset((void *)kbuf, 'k', 16);
    u8cs key = {kbuf, kbuf + 16};
    u8cs val = {(u8c *)kbuf, (u8c *)kbuf};  // empty
    call(TLKVFeed, u8bIdle(pad), 'O', key, val);
    testeq(*pad[1] & TLVaA, 0);  // long form
    u8cs rkey, rval;
    u8 rtype;
    u8c **from = u8bDataC(pad);
    call(TLKVDrain, from, &rtype, rkey, rval);
    testeq(rtype, 'O');
    testeq(0, $cmp(key, rkey));
    testeq((size_t)$len(rval), (size_t)0);
    done;
}

// 8. Empty key and empty val
ok64 TLKVtestEmpty() {
    sane(1);
    a_pad(u8, pad, 64);
    u8c dummy = 0;
    u8cs key = {&dummy, &dummy};  // empty
    u8cs val = {&dummy, &dummy};  // empty
    call(TLKVFeed, u8bIdle(pad), 'B', key, val);
    testeq(*pad[1] & TLVaA, TLVaA);  // short form
    testeq(*(pad[1] + 1), 0x00);     // nibbles = 0
    testeq((size_t)u8bDataLen(pad), (size_t)2);
    u8cs rkey, rval;
    u8 rtype;
    u8c **from = u8bDataC(pad);
    call(TLKVDrain, from, &rtype, rkey, rval);
    testeq(rtype, 'B');
    testeq((size_t)$len(rkey), (size_t)0);
    testeq((size_t)$len(rval), (size_t)0);
    done;
}

// 9. Container Into/Outo roundtrip
ok64 TLKVtestContainer() {
    sane(1);
    a_pad(u8, pad, 1024);
    u8cs objkey = $u8str("obj");
    // open container
    call(TLKVInto, pad, 'O', objkey);
    // feed a child
    u8cs ckey = $u8str("name");
    u8cs cval = $u8str("Alice");
    call(TLKVFeed, u8bIdle(pad), 'S', ckey, cval);
    // close container
    call(TLKVOuto, pad);
    // drain and verify
    u8cs rkey, rval;
    u8 rtype;
    u8c **from = u8bDataC(pad);
    call(TLKVDrain, from, &rtype, rkey, rval);
    testeq(rtype, 'O');
    testeq(0, $cmp(objkey, rkey));
    // val should contain the child record
    size_t child_len = TLKVLen($len(ckey), $len(cval));
    testeq($len(rval), child_len);
    // drain child from val
    u8cs cckey, ccval;
    u8 cctype;
    call(TLKVDrain, rval, &cctype, cckey, ccval);
    testeq(cctype, 'S');
    testeq(0, $cmp(ckey, cckey));
    testeq(0, $cmp(cval, ccval));
    done;
}

// 10. Container shrink to short form
ok64 TLKVtestContainerShrink() {
    sane(1);
    a_pad(u8, pad, 1024);
    u8cs objkey = $u8str("k");
    call(TLKVInto, pad, 'O', objkey);
    // feed a small child: key="a", val="b" -> 4 bytes
    u8cs ckey = $u8str("a");
    u8cs cval = $u8str("b");
    call(TLKVFeed, u8bIdle(pad), 'S', ckey, cval);
    call(TLKVOuto, pad);
    // should be short form: tag has TLVaA bit
    u8c **data = u8bDataC(pad);
    testeq(**data & TLVaA, TLVaA);
    // roundtrip
    u8cs rkey, rval;
    u8 rtype;
    call(TLKVDrain, data, &rtype, rkey, rval);
    testeq(rtype, 'O');
    testeq(0, $cmp(objkey, rkey));
    done;
}

// 11. Container no-shrink (>15 bytes children)
ok64 TLKVtestContainerNoShrink() {
    sane(1);
    a_pad(u8, pad, 1024);
    u8cs objkey = $u8str("k");
    call(TLKVInto, pad, 'O', objkey);
    // feed enough children to exceed 15 bytes value
    u8cs ckey = $u8str("field");
    u8cs cval = $u8str("a long value!!");  // 14 bytes
    call(TLKVFeed, u8bIdle(pad), 'S', ckey, cval);
    call(TLKVOuto, pad);
    // should be long form: tag does NOT have TLVaA bit
    u8c **data = u8bDataC(pad);
    testeq(**data & TLVaA, 0);
    // roundtrip
    u8cs rkey, rval;
    u8 rtype;
    call(TLKVDrain, data, &rtype, rkey, rval);
    testeq(rtype, 'O');
    testeq(0, $cmp(objkey, rkey));
    done;
}

// 12. Nested containers: Into 'O' -> Into 'A' -> Feed leaf -> Outo 'A' -> Outo 'O'
ok64 TLKVtestNested() {
    sane(1);
    a_pad(u8, pad, 1024);
    u8cs okey = $u8str("root");
    u8cs akey = $u8str("arr");
    u8cs lkey = $u8str("0");
    u8cs lval = $u8str("42");
    // open outer object
    call(TLKVInto, pad, 'O', okey);
    // open inner array
    call(TLKVInto, pad, 'A', akey);
    // feed leaf
    call(TLKVFeed, u8bIdle(pad), 'N', lkey, lval);
    // close inner array
    call(TLKVOuto, pad);
    // close outer object
    call(TLKVOuto, pad);
    // drain outer
    u8cs rkey, rval;
    u8 rtype;
    u8c **from = u8bDataC(pad);
    call(TLKVDrain, from, &rtype, rkey, rval);
    testeq(rtype, 'O');
    testeq(0, $cmp(okey, rkey));
    // drain inner array from outer's val
    u8cs akey2, aval2;
    u8 atype2;
    call(TLKVDrain, rval, &atype2, akey2, aval2);
    testeq(atype2, 'A');
    testeq(0, $cmp(akey, akey2));
    // drain leaf from array's val
    u8cs lkey2, lval2;
    u8 ltype2;
    call(TLKVDrain, aval2, &ltype2, lkey2, lval2);
    testeq(ltype2, 'N');
    testeq(0, $cmp(lkey, lkey2));
    testeq(0, $cmp(lval, lval2));
    done;
}

ok64 TLKVtest() {
    sane(1);
    call(TLKVtestShortRoundtrip);
    call(TLKVtestLongKey);
    call(TLKVtestLongVal);
    call(TLKVtestExactShort);
    call(TLKVtestExactLong);
    call(TLKVtestMaxShort);
    call(TLKVtestMinLong);
    call(TLKVtestEmpty);
    call(TLKVtestContainer);
    call(TLKVtestContainerShrink);
    call(TLKVtestContainerNoShrink);
    call(TLKVtestNested);
    done;
}

TEST(TLKVtest);
