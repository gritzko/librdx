#include "KEYW.h"

#include "abc/PRO.h"
#include "abc/TEST.h"

// --- Fixture: a small keyword set with length diversity and a known
//     collision candidate (first two bytes + length land on the same
//     hash slot, forcing a linear probe). ---

static u8cs KW[] = {
    u8slit("if"),        // 2
    u8slit("do"),        // 2
    u8slit("int"),       // 3
    u8slit("for"),       // 3
    u8slit("else"),      // 4
    u8slit("enum"),      // 4
    u8slit("auto"),      // 4
    u8slit("while"),     // 5
    u8slit("break"),     // 5
    u8slit("return"),    // 6
    u8slit("struct"),    // 6
    u8slit("typedef"),   // 7
    u8slit("continue"),  // 8
    u8slit("_Static_assert"), // 14, stress long
};
#define NKW (sizeof(KW) / sizeof(KW[0]))

ok64 KEYWtest_basic() {
    sane(1);
    keyw k = {};
    call(KEYWOpen, &k, KW, NKW);

    //  Hits — every registered keyword must be found.
    for (u32 i = 0; i < NKW; i++) {
        u8csc tok = {KW[i][0], KW[i][1]};
        want(KEYWHas(&k, tok));
    }

    //  Misses — non-keywords with varied first chars / lengths.
    {
        u8csc t = u8slit("intx");        // superset of "int"
        want(!KEYWHas(&k, t));
    }
    {
        u8csc t = u8slit("in");          // prefix of "int"
        want(!KEYWHas(&k, t));
    }
    {
        u8csc t = u8slit("IF");          // case mismatch
        want(!KEYWHas(&k, t));
    }
    {
        u8csc t = u8slit("foobar");
        want(!KEYWHas(&k, t));
    }
    {
        u8csc t = u8slit("returns");     // suffix-drift from "return"
        want(!KEYWHas(&k, t));
    }
    {
        //  1-char token short-circuits without touching the table.
        u8csc t = u8slit("x");
        want(!KEYWHas(&k, t));
    }
    done;
}

ok64 KEYWtest_overflow() {
    sane(1);
    //  Too many keywords (> KEYW_MAX) must be rejected at open time.
    u8cs big[KEYW_MAX + 1];
    static u8 storage[KEYW_MAX + 1][4];
    for (u32 i = 0; i < KEYW_MAX + 1; i++) {
        storage[i][0] = 'a' + (i % 26);
        storage[i][1] = 'a' + ((i / 26) % 26);
        storage[i][2] = 'a' + ((i / (26 * 26)) % 26);
        storage[i][3] = 0;
        big[i][0] = storage[i];
        big[i][1] = storage[i] + 3;
    }
    keyw k = {};
    want(KEYWOpen(&k, big, KEYW_MAX + 1) != OK);
    done;
}

ok64 KEYWtest_short() {
    sane(1);
    //  1-char keyword rejected at open.
    u8cs bad[] = {u8slit("x")};
    keyw k = {};
    want(KEYWOpen(&k, bad, 1) != OK);
    done;
}

ok64 maintest() {
    sane(1);
    call(KEYWtest_basic);
    call(KEYWtest_overflow);
    call(KEYWtest_short);
    done;
}

TEST(maintest);
