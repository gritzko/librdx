#include "LSM.h"

#include <unistd.h>

#include "abc/B.h"
#include "abc/OK.h"
#include "abc/TEST.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"

fun b8 alpha(u8cscp a, u8cscp b) {
    a_dup(u8c, aa, *a);
    a_dup(u8c, bb, *b);
    u8 ta, tb;
    u8cs keya, keyb, vala, valb;
    TLVDrainKeyVal(&ta, keya, vala, aa);
    TLVDrainKeyVal(&tb, keyb, valb, bb);
    int c = $cmp(keya, keyb);
    return c < 0;
}

fun ok64 latest(u8s into, u8css from) {
    u8 ta = 0;
    u8cs max = {};
    for (int i = 0; i < $len(from); ++i) {
        u8cs rec = {};
        TLVDrain$(rec, (u8c **)$at(from, i));
        if (*$last(rec) > ta) {
            ta = *$last(rec);
            $mv(max, rec);
        }
    }
    return u8sFeed(into, max);
}

ok64 LSM0() {
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
        call(TLVFeedKeyVal, pad1idle, 'K', kv1[i][0], kv1[i][1]);
    aBcpad(u8, pad2, 1024);
    for (int i = 0; i < 5; ++i)
        call(TLVFeedKeyVal, pad2idle, 'K', kv2[i][0], kv2[i][1]);

    aBpad2(u8cs, lsm, 4);
    call(HEAPu8csPushZ, lsmbuf, (u8cs *)pad1data, alpha);
    call(HEAPu8csPushZ, lsmbuf, (u8cs *)pad2data, alpha);

    aBcpad(u8, txt, 1024);
    call(LSMMerge, txtidle, lsmdata, TLVDrain$, alpha, latest);

    a_dup(u8c, res, txtdata);
    u8 n = '0';
    while (!$empty(res)) {
        u8 ta;
        u8cs keya;
        u8cs vala;
        TLVDrainKeyVal(&ta, keya, vala, res);
        u8 c = **vala;
        want(c == n);
        ++n;
    }
    done;
}

fun ok64 nomerge(u8s into, u8css from) { return u8sFeed(into, **from); }

ok64 LSM1() {
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
    zerob(padbuf);
    for (int i = 0; i < 6; ++i)
        call(TLVFeedKeyVal, padidle, 'K', kv1[i][0], kv1[i][1]);
    call(LSMSort, paddata, TLVDrain$, alpha, nomerge, padidle);
    int c = 'A';
    while (!$empty(paddata)) {
        u8 ta;
        u8cs keya;
        u8cs vala;
        call(TLVDrainKeyVal, &ta, keya, vala, u8bDataC(padbuf));
        //$println(keya);
        want(**keya == c);
        ++c;
    }
    done;
}

fun b8 ZINTz(u8cscp a, u8cscp b) {
    a_dup(u8c, aa, *a);
    a_dup(u8c, bb, *b);
    u8cs vala, valb;
    u8 ta, tb;
    TLVu8sDrain(aa, &ta, vala);
    TLVu8sDrain(bb, &tb, valb);
    u64 au, bu;
    ZINTu64drain(&au, vala);
    ZINTu64drain(&bu, valb);
    return u64Z(&au, &bu);
}

ok64 LSM1000000() {
    sane(1);
#define LEN (1 << 10)
    aB(u8, mil);
    call(u8bAllocate, milbuf, LEN * 16 * 2);
    aBpad(u8p, stack, 8);
    for (u64 i = 0; i < LEN; ++i) {
        call(TLVInitShort, milidle, 'I', stack);
        call(ZINTu64feed, milidle, i ^ 13);
        call(TLVEndAny, milidle, 'I', stack);
    }
    call(LSMSort, mildata, TLVDrain$, ZINTz, nomerge, milidle);
    for (u64 i = 0; i < LEN; ++i) {
        u8 t = 0;
        u8cs zint = {};
        call(TLVu8sDrain, u8bDataC(milbuf), &t, zint);
        want(t == 'I');
        u64 u = 0;
        call(ZINTu64drain, &u, zint);
        want(u == i);
    }
    call(u8bFree, milbuf);
    done;
}

// u64 slicer: take 8 bytes from stream
fun ok64 u64drain(u8csp rec, u8cs from) {
    if ($len(from) < sizeof(u64)) return NODATA;
    rec[0] = from[0];
    rec[1] = from[0] + sizeof(u64);
    from[0] += sizeof(u64);
    return OK;
}

// u64 comparator: compare via u8cs pointers
fun b8 u64less(u8cscp a, u8cscp b) {
    return u64Z((u64c *)*a[0], (u64c *)*b[0]);
}

// u64 merger: copy all (no actual merging)
fun ok64 u64copy(u8s into, u8css eqs) {
    $for(u8csc, e, eqs) {
        ok64 o = u8sFeed(into, *e);
        if (o != OK) return o;
    }
    return OK;
}

ok64 LSMu64() {
    sane(1);
#define N 256
    u64 nums[N];
    u64 copy[N];
    u64 seed = 12345;
    for (int i = 0; i < N; i++) {
        seed = seed * 1103515245 + 12345;
        nums[i] = seed;  // unique values
        copy[i] = nums[i];
    }
    $u8 data = {(u8 *)nums, (u8 *)(nums + N)};
    u8 tmpbuf[N * sizeof(u64)];
    $u8 tmp = {tmpbuf, tmpbuf + sizeof(tmpbuf)};
    call(LSMSort, data, u64drain, u64less, u64copy, tmp);
    u64 *copyslice[2] = {copy, copy + N};
    $sort(copyslice, u64cmp);
    for (int i = 0; i < N; i++) {
        want(nums[i] == copy[i]);
    }
#undef N
    done;
}

ok64 LSMtest() {
    sane(1);
    call(LSM0);
    call(LSM1);
    call(LSM1000000);
    call(LSMu64);
    done;
}

TEST(LSMtest);
