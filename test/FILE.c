#include "FILE.h"

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

pro(FILEtest1) {
    sane(1);
    $u8c path = $u8str("/tmp/test1.txt");
    $u8c text = $u8str("Hello world!\n");
    int fd = 0;
    call(FILEcreate, &fd, path);
    call(FILEfeed, fd, text);
    call(FILEclose, fd);
    call(FILEunlink, path);
    done;
}

pro(FILEtest2) {
    sane(1);
    $u8c path = $u8str("/tmp/testA.txt");
    int fd = 0;
    call(FILEcreate, &fd, path);
    call(FILEresize, fd, 4096);
    aB(u8, map);
    call(FILEmap, (void$)map, fd, PROT_READ | PROT_WRITE, 0);
    testeq(Bsize(map), 4096);
    Bat(map, 42) = 1;
    call(Bunfmap, map);
    call(Bfmap, map, fd, PROT_READ | PROT_WRITE, 0);
    call(FILEclose, fd);
    testeq(Blen(map), 4096);
    testeq(Bat(map, 41), 0);
    testeq(Bat(map, 42), 1);
    call(Bunfmap, map);
    done;
}

pro(FILEtest) {
    sane(1);
    call(FILEtest1);
    call(FILEtest2);
    done;
}

TEST(FILEtest);
