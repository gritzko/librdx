#include "LSM.h"

#include <unistd.h>

#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/OK.h"
#include "abc/TEST.h"
#include "abc/TLV.h"

fun int alpha($cu8c* a, $cu8c* b) {
    a$dup(u8c, aa, *a);
    a$dup(u8c, bb, *b);
    u8 ta, tb;
    $u8c keya, keyb, vala, valb;
    TLVdrainkv(&ta, keya, vala, aa);
    TLVdrainkv(&tb, keyb, valb, bb);
    int c = $cmp(keya, keyb);
    return c;
}

fun ok64 latest($u8 into, $$u8c from) {
    u8 ta = 0;
    $u8c max = {};
    for (int i = 0; i < $len(from); ++i) {
        $u8c rec;
        TLVdrain$(rec, $at(from, i));
        if (*$last(rec) > ta) $mv(max, rec);
    }
    $u8feed(into, max);
    return OK;
}

pro(LSM0) {
    sane(1);
    $u8c kv1[5][2] = {
        {$u8str("Four"), $u8str("1")},  {$u8str("One"), $u8str("2")},
        {$u8str("Three"), $u8str("5")}, {$u8str("Two"), $u8str("0")},
        {$u8str("Zero"), $u8str("7")},
    };
    $u8c kv2[5][2] = {
        {$u8str("Five"), $u8str("0")},  {$u8str("Four"), $u8str("0")},
        {$u8str("Seven"), $u8str("3")}, {$u8str("Six"), $u8str("4")},
        {$u8str("Two"), $u8str("6")},
    };
    aBcpad(u8, pad1, 1024);
    for (int i = 0; i < 5; ++i)
        call(TLVfeedkv, pad1idle, 'K', kv1[i][0], kv1[i][1]);
    aBcpad(u8, pad2, 1024);
    for (int i = 0; i < 5; ++i)
        call(TLVfeedkv, pad2idle, 'K', kv2[i][0], kv2[i][1]);

    aBpad2($u8c, lsm, 4);
    call(HEAP$u8cpushf, lsmbuf, ($u8c*)pad1data, alpha);
    call(HEAP$u8cpushf, lsmbuf, ($u8c*)pad2data, alpha);

    call(LSMsort, lsmdata, alpha);

    aBcpad(u8, txt, 1024);
    call(LSMmerge, txtidle, lsmdata, alpha, latest);

    a$dup(u8c, res, txtdata);
    u8 n = '0';
    while (!$empty(res)) {
        u8 ta;
        $u8c keya;
        $u8c vala;
        TLVdrainkv(&ta, keya, vala, res);
        u8 c = **vala;
        want(c == n);
        ++n;
    }
    done;
}

fun ok64 nomerge($u8 into, $$u8c from) { return $u8feedall(into, **from); }

pro(LSM1) {
    sane(1);
    $u8c kv1[6][2] = {
        {$u8str("A"), $u8str("1")},  //
        {$u8str("C"), $u8str("3")},  //
        {$u8str("B"), $u8str("2")},  //
        {$u8str("D"), $u8str("4")},  //
        {$u8str("E"), $u8str("5")},  //
        {$u8str("B"), $u8str("2")},  //
    };
    aBcpad(u8, pad1, 1024);
    aBpad2($u8c, padpad, 8);
    aBcpad(u8, pad2, 1024);
    Bzero(pad1buf);
    Bzero(padpadbuf);
    Bzero(pad2buf);
    for (int i = 0; i < 6; ++i)
        call(TLVfeedkv, pad1idle, 'K', kv1[i][0], kv1[i][1]);
    call($$u8cfeed1, padpadidle, pad1data);
    call(LSMmergehard, pad2idle, padpaddata, alpha, nomerge);
    int c = 'A';
    while (!$empty(pad2data)) {
        u8 ta;
        $u8c keya;
        $u8c vala;
        call(TLVdrainkv, &ta, keya, vala, pad2data);
        //$println(keya);
        want(**keya == c);
        ++c;
    }
    done;
}

pro(LSMtest) {
    sane(1);
    call(LSM0);
    call(LSM1);
    done;
}

TEST(LSMtest);
