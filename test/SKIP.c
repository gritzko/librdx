
#include "SKIP.h"

#include <unistd.h>

#include "01.h"
#include "FILE.h"
#include "HEX.h"
#include "TEST.h"

pro(SKIP0) {
    sane(1);
    SKIPs skips = {.gap = 9};
    testeq(0, SKIPpos(&skips, 0));
    testeq(0, SKIPpos(&skips, 1));
    testeq(0, SKIPpos(&skips, 2));
    SKIPs skips2 = {.gap = 9, .pos = 1025};
    testeq(512, SKIPpos(&skips2, 0));
    testeq(0, SKIPpos(&skips2, 1));
    SKIPs skips4 = {.gap = 9, .pos = 2049, .off = {23}};
    testeq(1024 + 512 + 23, SKIPpos(&skips4, 0));
    testeq(1024, SKIPpos(&skips4, 1));
    testeq(0, SKIPpos(&skips4, 2));
    SKIPs skips5 = {.gap = 9, .pos = 2048 + 512 + 1};
    skips5.off[1] = 9;
    testeq(2048, SKIPpos(&skips5, 0));
    testeq(1024 + 9, SKIPpos(&skips5, 1));
    testeq(0, SKIPpos(&skips5, 2));
    done;
}

pro(SKIPcheck, Bu8 buf, Bu8 checked, SKIPs const* k) {
    sane(1);
    for (int h = k->len - 1; h >= 0; --h) {
        if (k->off[h] == SKIP_MASK) continue;
        SKIPs hop = {};
        ok64 o = SKIPhop(&hop, buf, k, h);
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
    SKIPs k = {.gap = 4};
    for (u64 u = 0; u < MB / 16; ++u) {
        $u8feed64(padidle, &u);
        call(SKIPmayfeed, padbuf, &k);
    }
    // aBcpad(u8, hex, PAGESIZE * 2);
    // HEXfeedall(hexidle, paddata);
    a$str(sep, "---------------\n");
    FILEout(sep);
    FILEout(paddata);
    FILEout(sep);
    SKIPs k2 = {.gap = 4};
    call(SKIPdrain, &k2, padbuf, k.pos);
    call(SKIPcheck, padbuf, checkbuf, &k2);
    done;
}

pro(SKIPtest) {
    sane(1);
    call(SKIP0);
    call(SKIP1);
    done;
}

TEST(SKIPtest);
