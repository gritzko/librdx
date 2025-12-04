
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

pro(SKIP0) {
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

pro(SKIPcheck, u8bp buf, u8bp checked, SKIPu8tab const* k) {
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
        } else if (!BitAt(checked, hop.pos)) {
            BitSet(checked, hop.pos);
            call(SKIPcheck, buf, checked, &hop);
        }
    }
    done;
}

pro(SKIP1) {
    sane(1);
    a_pad(u8, check, SCALE / 8);
    a_pad(u8, pad, SCALE);
    zerob(check);
    SKIPu8tab k = {};
    for (u64 u = 0; u < SCALE / 16; ++u) {
        call(u8sFeed64, pad_idle, &u);
        call(SKIPu8mayfeed, pad, &k);
    }
    // aBcpad(u8, hex, PAGESIZE * 2);
    // HEXfeedall(hexidle, paddata);
    call(SKIPu8finish, pad, &k);
    SKIPu8tab k2 = {};
    // call(SKIPu8drain, &k2, pad, k.pos);
    call(SKIPu8load, &k2, pad);
    call(SKIPcheck, pad, check, &k2);
    done;
}

pro(SKIP2) {
    sane(1);
    a_path(path, "/tmp/SKIP2.txt");
    FILEunlink(path);
    u8b pad = {};
    a_pad(u8, check, SCALE);
    call(FILEMapCreate, pad, path, SCALE);
    COMBinit(pad);
    SKIPu8tab k = {};
    for (u64 i = 0; i < 8; ++i) {
        for (u64 u = 0; u < SCALE / 16; ++u) {
            call(u8sFeed64, u8bIdle(pad), &u);
            call(SKIPu8mayfeed, pad, &k);
        }
        call(SKIPu8finish, pad, &k);
        size_t ds = Bdatalen(pad);
        size_t bs = Busysize(pad);
        COMBsave(pad);
        call(FILEReMap, pad, SCALE * (i + 2));
        COMBload(pad);
        testeq(ds, Bdatalen(pad));
        testeq(bs, Busysize(pad));
        testeq(SCALE * (i + 2), Bsize(pad));
        zero(k);
        zerob(check);
        call(SKIPcheck, pad, check, &k);
    }
    call(FILEunlink, path);
    done;
}

fun int cmp($cc a, $cc b) {
    u64* aa = (u64*)*a;
    u64* bb = (u64*)*b;
    fprintf(stderr, "\t%lu <> %lu\n", *aa, *bb);
    return u64cmp(aa, bb);
}

pro(SKIP3) {
    sane(1);
    a_pad(u8, pad, SCALE);
    a_pad(u8, check, SCALE);
    SKIPu8tab k = {};
    for (u64 u = 0; u < SCALE / 16; ++u) {
        u8sFeed64(pad_idle, &u);
        call(SKIPu8mayfeed, pad, &k);
    }
    call(SKIPu8finish, pad, &k);
    for (u64 u = 0; u < SCALE / 16; ++u) {
        u8cs gap = {};
        a_rawc(raw, u);
        call(SKIPu8find, gap, pad, raw, cmp);
        u64c* head = (u64c*)*gap;
        want(*head <= u);  // && u - *head < 256 / 8);
        fprintf(stderr, "%lu IN %lu [%lu,%lu) of %lu?\n", u, *head,
                gap[0] - *pad, gap[1] - *pad, u8bDataLen(pad));
    }
    done;
}

fun int tlvcmp($cc a, $cc b) {
    if ($len(a) < 8 + 2 || $len(b) < 8 + 2) {
        abort();
    }
    u64* aa = (u64*)(*a + 2);
    u64* bb = (u64*)(*b + 2);
    return u64cmp(aa, bb);
}

pro(SKIP4) {
    sane(1);
    a_pad(u8, pad, SCALE);
    a_pad(u8, check, SCALE);
    SKIPu8tab k = {};
    for (u64 u = 0; u < SCALE / 16; ++u) {
        a_rawc(raw, u);
        call(TLVu8sFeed, pad_idle, 'I', raw);
        call(SKIPu8mayfeed, pad, &k);
    }
    call(SKIPu8finish, pad, &k);
    for (u64 u = 0; u < SCALE / 16; ++u) {
        u8 t = 0;
        u8cs gap = {}, val = {};
        u8 u10[10] = {
            'i',
            8,
        };
        *(u64*)(u10 + 2) = u;
        a_rawc(raw, u10);
        call(SKIPu8findTLV, gap, pad, raw, tlvcmp);
        call(TLVu8sDrain, gap, &t, val);
        want(t == 'I');
        want($len(val) == 8);
        want(u == *(u64*)*val);
    }
    done;
}

pro(SKIPtest) {
    sane(1);
    call(SKIP0);
    call(SKIP1);
    call(SKIP2);
    call(SKIP3);
    call(SKIP4);
    done;
}

TEST(SKIPtest);
