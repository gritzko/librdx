#include "MMAP.h"

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

pro(MMAPtest1) {
    sane(1);
    u8b buf8 = {};
    call(u8bMap, buf8, 4096);
    aB(u32, word);
    call(u32bMap, wordbuf, 1024);
    testeq(Bsize(buf8), Bsize(wordbuf));

    Bat(buf8, 0) = 0xaa;
    Bat(buf8, 1) = 0xbb;
    Bat(buf8, 2) = 0xcc;
    Bat(buf8, 3) = 0xdd;
    $copy(wordidle, Bidle(buf8));
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

    call(u8bUnMap, buf8);
    call(u32bUnMap, wordbuf);
    done;
}

pro(MMAPtest) {
    sane(1);
    call(MMAPtest1);
    done;
}

TEST(MMAPtest);
