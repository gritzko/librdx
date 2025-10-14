#include "MMAP.h"

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

pro(MMAPtest1) {
    sane(1);
    u8b buf8 = {};
    call(MMAPopen, (voidbp)buf8, 4096);
    aB(u32, word);
    testeq(YES, Bnil(wordbuf));
    call(Bmmap, wordbuf, 1024);
    testeq(Bsize(buf8), Bsize(wordbuf));

    Bat(buf8, 0) = 0xaa;
    Bat(buf8, 1) = 0xbb;
    Bat(buf8, 2) = 0xcc;
    Bat(buf8, 3) = 0xdd;
    $copy(Bidle(wordbuf), Bidle(buf8));
    testeq(Bat(wordbuf, 0), 0xddccbbaa);

    call(Bremap2, buf8);
    call(Bmayremap, wordbuf, 2048);
    testeq(Bsize(buf8), Bsize(wordbuf));

    Bat(buf8, 8188) = 0xaa;
    Bat(buf8, 8189) = 0xbb;
    Bat(buf8, 8190) = 0xcc;
    Bat(buf8, 8191) = 0xee;
    $copy(Bidle(wordbuf), Bidle(buf8));
    testeq(Bat(wordbuf, 2047), 0xeeccbbaa);
    testeq(Bat(wordbuf, 0), 0xddccbbaa);

    call(MMAPclose, (voidbp)buf8);
    call(Bunmap, wordbuf);
    done;
}

pro(MMAPtest) {
    sane(1);
    call(MMAPtest1);
    done;
}

TEST(MMAPtest);
