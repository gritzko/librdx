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
    call(FILEmap, (void$)mapbuf, fd, PROT_READ | PROT_WRITE, 0);
    testeq(Bsize(mapbuf), 4096);
    Bat(mapbuf, 42) = 1;
    call(Bunfmap, mapbuf);
    call(Bfmap, mapbuf, fd, PROT_READ | PROT_WRITE, 0);
    call(FILEclose, fd);
    testeq(Blen(mapbuf), 4096);
    testeq(Bat(mapbuf, 41), 0);
    testeq(Bat(mapbuf, 42), 1);
    call(Bunfmap, mapbuf);
    done;
}

pro(FILE3) {
    sane(1);
    a$str(path, "/tmp/FILE3.txt");
    Bu8 buf = {};
    call(FILEmapre, (voidB)buf, path, PAGESIZE);
    Breset(buf);
    call($u8feed, Bu8idle(buf), path);
    call(FILEunmap, (voidB)buf);
    Bu8 buf2 = {};
    call(FILEmapro, (voidB)buf2, path);
    aB$(u8c, path2, buf2, 0, $len(path));
    $testeq(path, path2);
    call(FILEunmap, (voidB)buf2);
    // nedo(FILEunlink(path));
    done;
}

pro(FILEtest) {
    sane(1);
    call(FILEtest1);
    call(FILEtest2);
    call(FILE3);
    done;
}

TEST(FILEtest);
