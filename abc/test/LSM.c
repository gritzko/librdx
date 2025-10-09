#include "LSM.h"

#include <unistd.h>

#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/OK.h"
#include "abc/TEST.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"

fun int alpha($cu8c* a, $cu8c* b) {
    a$dup(u8c, aa, *a);
    a$dup(u8c, bb, *b);
    u8 ta, tb;
    u8cs keya, keyb, vala, valb;
    TLVDrainkv(&ta, keya, vala, aa);
    TLVDrainkv(&tb, keyb, valb, bb);
    int c = $cmp(keya, keyb);
    return c;
}

fun ok64 latest($u8 into, u8css from) {
    u8 ta = 0;
    u8cs max = {};
    for (int i = 0; i < $len(from); ++i) {
        u8cs rec;
        TLVDrain$(rec, $at(from, i));
        if (*$last(rec) > ta) $mv(max, rec);
    }
    $u8feed(into, max);
    return OK;
}

pro(LSM0) {
    sane(1);
    u8cs kv1[5][2] = {
        {$u8str("Four"), $u8str("1")},  {$u8str("One"), $u8str("2")},
        {$u8str("Three"), $u8str("5")}, {$u8str("Two"), $u8str("0")},
        {$u8str("Zero"), $u8str("7")},
    };
    u8cs kv2[5][2] = {
        {$u8str("Five"), $u8str("0")},  {$u8str("Four"), $u8str("0")},
        {$u8str("Seven"), $u8str("3")}, {$u8str("Six"), $u8str("4")},
        {$u8str("Two"), $u8str("6")},
    };
    aBcpad(u8, pad1, 1024);
    for (int i = 0; i < 5; ++i)
        call(TLVFeedkv, pad1idle, 'K', kv1[i][0], kv1[i][1]);
    aBcpad(u8, pad2, 1024);
    for (int i = 0; i < 5; ++i)
        call(TLVFeedkv, pad2idle, 'K', kv2[i][0], kv2[i][1]);

    aBpad2(u8cs, lsm, 4);
    call(HEAPu8csPushZ, lsmbuf, (u8cs*)pad1data, alpha);
    call(HEAPu8csPushZ, lsmbuf, (u8cs*)pad2data, alpha);

    aBcpad(u8, txt, 1024);
    call(LSMmerge, txtidle, lsmdata, alpha, latest);

    a$dup(u8c, res, txtdata);
    u8 n = '0';
    while (!$empty(res)) {
        u8 ta;
        u8cs keya;
        u8cs vala;
        TLVDrainkv(&ta, keya, vala, res);
        u8 c = **vala;
        want(c == n);
        ++n;
    }
    done;
}

fun ok64 nomerge($u8 into, u8css from) { return $u8feedall(into, **from); }

pro(LSM1) {
    sane(1);
    u8cs kv1[6][2] = {
        {$u8str("A"), $u8str("1")},  //
        {$u8str("C"), $u8str("3")},  //
        {$u8str("B"), $u8str("2")},  //
        {$u8str("D"), $u8str("4")},  //
        {$u8str("E"), $u8str("5")},  //
        {$u8str("B"), $u8str("2")},  //
    };
    aBpad2(u8, pad, 1024);
    Bzero(padbuf);
    for (int i = 0; i < 6; ++i)
        call(TLVFeedkv, padidle, 'K', kv1[i][0], kv1[i][1]);
    call(LSMsort, paddata, alpha, nomerge, padidle);
    int c = 'A';
    while (!$empty(paddata)) {
        u8 ta;
        u8cs keya;
        u8cs vala;
        call(TLVDrainkv, &ta, keya, vala,Bu8cdata(padbuf));
        //$println(keya);
        want(**keya == c);
        ++c;
    }
    done;
}

fun int ZINTz($cu8c* a, $cu8c* b) {
    a$dup(u8c, aa, *a);
    a$dup(u8c, bb, *b);
    u8cs vala, valb;
    u8 ta, tb;
    TLVDrain(&ta, vala, aa);
    TLVDrain(&tb, valb, bb);
    u64 au, bu;
    ZINTu64drain(&au, vala);
    ZINTu64drain(&bu, valb);
    return u64cmp(&au, &bu);
}

ok64 LSM1000000() {
    sane(1);
#define LEN (1 << 10)
    aB(u8, mil);
    call(Bu8alloc, milbuf, LEN * 16 * 2);
    aBpad(u8p, stack, 8);
    for (u64 i = 0; i < LEN; ++i) {
        call(TLVinitshort, milidle, 'I', stack);
        call(ZINTu64feed, milidle, i ^ 13);
        call(TLVendany, milidle, 'I', stack);
    }
    call(LSMsort, mildata, ZINTz, nomerge, milidle);
    for (u64 i = 0; i < LEN; ++i) {
        u8 t = 0;
        u8cs zint = {};
        call(TLVDrain, &t, zint,Bu8cdata(milbuf));
        want(t == 'I');
        u64 u = 0;
        call(ZINTu64drain, &u, zint);
        want(u == i);
    }
    call(Bu8free, milbuf);
    done;
}

pro(LSMtest) {
    sane(1);
    call(LSM0);
    call(LSM1);
    call(LSM1000000);
    done;
}

TEST(LSMtest);
