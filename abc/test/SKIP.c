
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

#define X(M, name) M##bl04##name
#define offX u8
#include "SKIPx.h"
#undef X

// Avg skip record: 2 entries, 4+1 bytes,
// OVerhead 1% => gap 1<<9=512
#define X(M, name) M##bl09##name
#include "SKIPx.h"
#undef X

#define SCALE (64 * KB)
//(KB * 128)

pro(SKIP0) {
    sane(1);
    SKIPbl09tab skips = {};
    testeq(0, SKIPbl09pos(&skips, 0));
    testeq(0, SKIPbl09pos(&skips, 1));
    testeq(0, SKIPbl09pos(&skips, 2));
    SKIPbl09tab skips2 = {.pos = 1025};
    testeq(512, SKIPbl09pos(&skips2, 0));
    testeq(0, SKIPbl09pos(&skips2, 1));
    SKIPbl09tab skips4 = {.pos = 2049, .off = {23}};
    testeq(1024 + 512 + 23, SKIPbl09pos(&skips4, 0));
    testeq(1024, SKIPbl09pos(&skips4, 1));
    testeq(0, SKIPbl09pos(&skips4, 2));
    SKIPbl09tab skips5 = {.pos = 2048 + 512 + 1, .off = {9, 9, 9}};
    testeq(2048 + 9, SKIPbl09pos(&skips5, 0));
    testeq(2048 + 9, SKIPbl09pos(&skips5, 1));
    testeq(2048 + 9, SKIPbl09pos(&skips5, 2));
    testeqv(2, SKIPbl09top(1025), "%i");
    done;
}

pro(SKIPcheck, Bu8 buf, Bu8 checked, SKIPbl04tab const* k) {
    sane(1);
    for (int h = SKIPbl04len(k->pos) - 1; h >= 0; --h) {
        if (k->off[h] == 0xff) continue;
        size_t pp = SKIPbl04pos(k, h);
        if (pp == 0) continue;
        SKIPbl04tab hop = {};
        ok64 o = SKIPbl04hop(&hop, buf, k, h);
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

pro(SKIP1) {
    sane(1);
    aBcpad(u8, check, SCALE / 8);
    aBcpad(u8, pad, SCALE);
    Bzero(checkbuf);
    SKIPbl04tab k = {};
    for (u64 u = 0; u < SCALE / 16; ++u) {
        call($u8feed64, padidle, &u);
        call(SKIPbl04mayfeed, padbuf, &k);
    }
    // aBcpad(u8, hex, PAGESIZE * 2);
    // HEXfeedall(hexidle, paddata);
    call(SKIPbl04term, padbuf, &k);
    SKIPbl04tab k2 = {};
    // call(SKIPbl04drain, &k2, padbuf, k.pos);
    call(SKIPbl04load, &k2, padbuf);
    call(SKIPcheck, padbuf, checkbuf, &k2);
    done;
}

pro(SKIP2) {
    sane(1);
    $u8c path = $u8str("/tmp/SKIP2.txt");
    FILEunlink(path);
    aB(u8, pad);
    aBcpad(u8, check, SCALE);
    call(FILEmapre, (voidB)padbuf, path, SCALE);
    COMBinit(padbuf);
    SKIPbl04tab k = {};
    for (u64 i = 0; i < 8; ++i) {
        for (u64 u = 0; u < SCALE / 16; ++u) {
            call($u8feed64, padidle, &u);
            call(SKIPbl04mayfeed, padbuf, &k);
        }
        call(SKIPbl04term, padbuf, &k);
        size_t ds = Bdatalen(padbuf);
        size_t bs = Busysize(padbuf);
        COMBsave(padbuf);
        call(FILEunmap, (voidB)padbuf);
        call(FILEmapre, (voidB)padbuf, path, SCALE * (i + 2));
        COMBload(padbuf);
        testeq(ds, Bdatalen(padbuf));
        testeq(bs, Busysize(padbuf));
        testeq(SCALE * (i + 2), Bsize(padbuf));
        zero(k);
        call(SKIPbl04trim, &k, padbuf);
        Bzero(checkbuf);
        call(SKIPcheck, padbuf, checkbuf, &k);
    }
    call(FILEunlink, path);
    done;
}

fun int cmp($cc a, $cc b) {
    u64* aa = (u64*)*a;
    u64* bb = (u64*)*b;
    return u64cmp(aa, bb);
}

pro(SKIP3) {
    sane(1);
    aBcpad(u8, pad, SCALE);
    aBcpad(u8, check, SCALE);
    SKIPbl04tab k = {};
    for (u64 u = 0; u < SCALE / 16; ++u) {
        $u8feed64(padidle, &u);
        call(SKIPbl04mayfeed, padbuf, &k);
    }
    call(SKIPbl04term, padbuf, &k);
    for (u64 u = 0; u < SCALE / 16; ++u) {
        $u8c gap = {};
        a$rawc(raw, u);
        call(SKIPbl04find, gap, padbuf, raw, cmp);
        // size_t l = $len(gap);
        // testeq(0, l & 7);
        u64c* head = (u64c*)*gap;
        want(*head <= u && u - *head < 4);
        /*u64 head = 0;
        $u8drain64(&head, gap);
        fprintf(stderr, "%lu IN %lu+%lu?\n", u, head, l);
        want(head <= u && head + l > u);*/
    }
    done;
}

pro(SKIPtest) {
    sane(1);
    call(SKIP0);
    call(SKIP1);
    call(SKIP2);
    call(SKIP3);
    done;
}

TEST(SKIPtest);
