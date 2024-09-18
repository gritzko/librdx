
#include "SKIP.h"

#include <unistd.h>

#include "01.h"
#include "B.h"
#include "FILE.h"
#include "HEX.h"
#include "TEST.h"

#define X(M, name) M##bl04##name
#include "SKIPx.h"
#undef X

pro(SKIP0) {
    sane(1);
    SKIPs skips = {};
    testeq(0, SKIPbl09pos(&skips, 0));
    testeq(0, SKIPbl09pos(&skips, 1));
    testeq(0, SKIPbl09pos(&skips, 2));
    SKIPs skips2 = {.pos = 1025};
    testeq(512, SKIPbl09pos(&skips2, 0));
    testeq(0, SKIPbl09pos(&skips2, 1));
    SKIPs skips4 = {.pos = 2049, .off = {23}};
    testeq(1024 + 512 + 23, SKIPbl09pos(&skips4, 0));
    testeq(1024, SKIPbl09pos(&skips4, 1));
    testeq(0, SKIPbl09pos(&skips4, 2));
    SKIPs skips5 = {.pos = 2048 + 512 + 1};
    skips5.off[1] = 9;
    testeq(2048, SKIPbl09pos(&skips5, 0));
    testeq(1024 + 9, SKIPbl09pos(&skips5, 1));
    testeq(0, SKIPbl09pos(&skips5, 2));
    done;
}

pro(SKIPcheck, Bu8 buf, Bu8 checked, SKIPs const* k) {
    sane(1);
    for (int h = k->len - 1; h >= 0; --h) {
        if (k->off[h] == SKIP_MASK) continue;
        SKIPs hop = {};
        ok64 o = SKIPbl04hop(&hop, buf, k, h);
        if (o != OK) {
            if (o == SKIPbof) continue;
            fail(o);
        } else if (Bat(checked, hop.pos) == 0) {
            Bat(checked, hop.pos) = 1;
            call(SKIPcheck, buf, checked, &hop);
        }
    }
    done;
}

pro(SKIP1) {
    sane(1);
    aBcpad(u8, pad, MB);
    aBcpad(u8, check, MB);
    SKIPs k = {};
    for (u64 u = 0; u < MB / 16; ++u) {
        $u8feed64(padidle, &u);
        call(SKIPbl04mayfeed, padbuf, &k);
    }
    // aBcpad(u8, hex, PAGESIZE * 2);
    // HEXfeedall(hexidle, paddata);
    SKIPs k2 = {};
    call(SKIPbl04drain, &k2, padbuf, k.pos);
    call(SKIPcheck, padbuf, checkbuf, &k2);
    done;
}

pro(SKIP2) {
    sane(1);
    $u8c path = $u8str("/tmp/SKIP2.txt");
    aB(u8, s);
    // call(FILEmapre, sbuf, path);
    int fd = 0;
    call(FILEcreate, &fd, path);
    Bu8 padbuf = {};
    u8$ padidle = Bu8idle(padbuf);
    call(FILEmap, (void**)padbuf, fd, PROT_READ | PROT_WRITE, 0);
    call(FILEclose, fd);
    aBcpad(u8, check, MB);
    SKIPs k = {};
    for (u64 i = 0; i < 8; ++i) {
        for (u64 u = 0; u < MB / 16 / 8; ++u) {
            $u8feed64(padidle, &u);
            call(SKIPbl04mayfeed, padbuf, &k);
        }
        call(FILEresize, fd, Busysize(padbuf) + SKIP_TERM_LEN);
        call(FILEclose, fd);
        zero(k);
        // TODO term comb trim close open
        call(SKIPbl04load, &k, padbuf);
    }
    SKIPs k2 = {};
    call(SKIPbl04drain, &k2, padbuf, k.pos);
    call(SKIPcheck, padbuf, checkbuf, &k2);
    call(FILEclose, fd);
    call(FILEunlink, path);
    done;
}

pro(SKIPtest) {
    sane(1);
    call(SKIP0);
    call(SKIP1);
    // call(SKIP2);
    done;
}

TEST(SKIPtest);
