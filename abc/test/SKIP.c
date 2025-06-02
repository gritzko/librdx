
#include "SKIP.h"

#include <stdint.h>
#include <unistd.h>

#include "01.h"
#include "B.h"
#include "COMB.h"
#include "FILE.h"
#include "HEX.h"
#include "PRO.h"
#include "TEST.h"

#define X(M, name) M##u8##name
#include "SKIPx.h"
#undef X

#define SCALE (64 * KB)
//(KB * 128)

ok64 SKIP0() {
    sane(1);
    SKIPu8tab skips = {};
    testeq(0, SKIPu8pos(&skips, 0));
    testeq(0, SKIPu8pos(&skips, 1));
    testeq(0, SKIPu8pos(&skips, 2));
    SKIPu8tab skips2 = {.pos = 1025};
    testeq(1024 - 256, SKIPu8pos(&skips2, 0));
    testeq(512, SKIPu8pos(&skips2, 1));
    SKIPu8tab skips4 = {.pos = 2049, .off = {23}};
    testeq(2048 - 256 + 23, SKIPu8pos(&skips4, 0));
    testeq(1024, SKIPu8pos(&skips4, 2));
    testeq(0, SKIPu8pos(&skips4, 3));
    SKIPu8tab skips5 = {.pos = 2048 + 1, .off = {9, 10, 11}};
    testeq(2048 - 256 + 9, SKIPu8pos(&skips5, 0));
    testeq(2048 - 512 + 10, SKIPu8pos(&skips5, 1));
    testeq(1024 + 11, SKIPu8pos(&skips5, 2));
    testeqv(3, SKIPu8top(1025), "%i");
    done;
}

ok64 SKIPcheck(Bu8 buf, Bu8 checked, SKIPu8tab const* k) {
    sane(1);
    for (int h = SKIPu8len(k->pos) - 1; h >= 0; --h) {
        if (k->off[h] == 0xff) continue;
        size_t pp = SKIPu8pos(k, h);
        if (pp == 0) continue;
        SKIPu8tab hop = {};
        ok64 o = SKIPu8hop(&hop, buf, k, h);
        if (o != OK) {
            if (o == SKIPnone) continue;
            fail(o);
        } else if (!Bitat(checked, hop.pos)) {
            Bitset(checked, hop.pos);
            call(SKIPcheck, buf, checked, &hop);
        }
    }
    done;
}

ok64 SKIP1() {
    sane(1);
    aBcpad(u8, check, SCALE / 8);
    aBcpad(u8, pad, SCALE);
    Bzero(checkbuf);
    SKIPu8tab k = {};
    for (u64 u = 0; u < SCALE / 16; ++u) {
        call($u8feed64, padidle, &u);
        call(SKIPu8mayfeed, padbuf, &k);
    }
    // aBcpad(u8, hex, PAGESIZE * 2);
    // HEXfeedall(hexidle, paddata);
    call(SKIPu8finish, padbuf, &k);
    SKIPu8tab k2 = {};
    // call(SKIPu8drain, &k2, padbuf, k.pos);
    call(SKIPu8load, &k2, padbuf);
    call(SKIPcheck, padbuf, checkbuf, &k2);
    done;
}

ok64 SKIP2() {
    sane(1);
    $u8c path = $u8str("/tmp/SKIP2.txt");
    FILEunlink(path);
    aB(u8, pad);
    aBcpad(u8, check, SCALE);
    int fd = FILE_CLOSED;
    call(FILEmapnew, padbuf, &fd, path, SCALE);
    COMBinit(padbuf);
    SKIPu8tab k = {};
    for (u64 i = 0; i < 8; ++i) {
        for (u64 u = 0; u < SCALE / 16; ++u) {
            call($u8feed64, padidle, &u);
            call(SKIPu8mayfeed, padbuf, &k);
        }
        call(SKIPu8finish, padbuf, &k);
        size_t ds = Bdatalen(padbuf);
        size_t bs = Busysize(padbuf);
        COMBsave(padbuf);
        call(FILEremap, padbuf, &fd, SCALE * (i + 2));
        COMBload(padbuf);
        testeq(ds, Bdatalen(padbuf));
        testeq(bs, Busysize(padbuf));
        testeq(SCALE * (i + 2), Bsize(padbuf));
        zero(k);
        Bzero(checkbuf);
        call(SKIPcheck, padbuf, checkbuf, &k);
    }
    call(FILEunlink, path);
    done;
}

fun int cmp($cc a, $cc b) {
    u64* aa = (u64*)*a;
    u64* bb = (u64*)*b;
    fprintf(stderr, "\t%lu <> %lu\n", *aa, *bb);
    return u64z(aa, bb);
}

ok64 SKIP3() {
    sane(1);
    aBcpad(u8, pad, SCALE);
    aBcpad(u8, check, SCALE);
    SKIPu8tab k = {};
    for (u64 u = 0; u < SCALE / 16; ++u) {
        $u8feed64(padidle, &u);
        call(SKIPu8mayfeed, padbuf, &k);
    }
    call(SKIPu8finish, padbuf, &k);
    for (u64 u = 0; u < SCALE / 16; ++u) {
        $u8c gap = {};
        a$rawc(raw, u);
        call(SKIPu8find, gap, padbuf, raw, cmp);
        u64c* head = (u64c*)*gap;
        want(*head <= u);  // && u - *head < 256 / 8);
        fprintf(stderr, "%lu IN %lu [%lu,%lu) of %lu?\n", u, *head,
                gap[0] - *padbuf, gap[1] - *padbuf, Bdatalen(padbuf));
    }
    done;
}

fun int tlvcmp($cc a, $cc b) {
    if ($len(a) < 8 + 2 || $len(b) < 8 + 2) {
        abort();
    }
    u64* aa = (u64*)(*a + 2);
    u64* bb = (u64*)(*b + 2);
    return u64z(aa, bb);
}

ok64 SKIP4() {
    sane(1);
    aBcpad(u8, pad, SCALE);
    aBcpad(u8, check, SCALE);
    SKIPu8tab k = {};
    for (u64 u = 0; u < SCALE / 16; ++u) {
        a$rawc(raw, u);
        call(TLVfeed, padidle, 'I', raw);
        call(SKIPu8mayfeed, padbuf, &k);
    }
    call(SKIPu8finish, padbuf, &k);
    for (u64 u = 0; u < SCALE / 16; ++u) {
        u8 t = 0;
        $u8c gap = {}, val = {};
        u8 u10[10] = {
            'i',
            8,
        };
        *(u64*)(u10 + 2) = u;
        a$rawc(raw, u10);
        call(SKIPu8findTLV, gap, padbuf, raw, tlvcmp);
        call(TLVdrain, &t, val, gap);
        want(t == 'I');
        want($len(val) == 8);
        want(u == *(u64*)*val);
    }
    done;
}

ok64 SKIPtest() {
    sane(1);
    call(SKIP0);
    call(SKIP1);
    call(SKIP2);
    call(SKIP3);
    call(SKIP4);
    done;
}

TEST(SKIPtest);
