#include "FILE.h"

#include <stdlib.h>

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

pro(FILEtest1) {
    sane(1);
    u8cs path = $u8str("/tmp/test1.txt");
    u8cs text = $u8str("Hello world!\n");
    int fd = 0;
    call(FILEcreate, &fd, path);
    call(FILEFeed, fd, text);
    call(FILEclose, &fd);
    call(FILEunlink, path);
    done;
}

pro(FILEtest2) {
    sane(1);
    u8cs path = $u8str("/tmp/testA.txt");
    int fd = 0;
    call(FILEcreate, &fd, path);
    call(FILEresize, &fd, 4096);
    aB(u8, map);
    call(FILEmap, mapbuf, &fd, PROT_READ | PROT_WRITE);
    testeq(Bsize(mapbuf), 4096);
    Bat(mapbuf, 42) = 1;
    call(FILEunmap, mapbuf);
    call(FILEmap, mapbuf, &fd, PROT_READ | PROT_WRITE);
    call(FILEclose, &fd);
    testeq(Blen(mapbuf), 4096);
    testeq(Bat(mapbuf, 41), 0);
    testeq(Bat(mapbuf, 42), 1);
    call(FILEunmap, mapbuf);
    done;
}

pro(FILE3) {
    sane(1);
    a$str(path, "/tmp/FILE3.txt");
    Bu8 buf = {};
    int fd = FILE_CLOSED;
    call(FILEmapnew, buf, &fd, path, PAGESIZE);
    Breset(buf);
    call($u8feed, Bu8idle(buf), path);
    call(FILEunmap, buf);
    Bu8 buf2 = {};
    call(FILEmapro, buf2, path);
    aB$(u8c, path2, buf2, 0, $len(path));
    $testeq(path, path2);
    call(FILEunmap, buf2);
    // nedo(FILEunlink(path));
    done;
}

pro(FILEtest4) {
    sane(1);
    a$str(path, "/tmp/FILEtest4.txt");
    a$str(one, "Hello");
    a$str(two, " beautiful");
    a$str(three, " world!");
    aBpad2(u8cs, queue, 4);
    call(u8css_feed3, queueidle, one, two, three);
    int fd;
    call(FILEcreate, &fd, path);
    call(FILEFeedv, fd, queuedata);
    want($empty(queuedata));
    aBpad2(u8, back, 64);
    testeq(0, lseek(fd, 0, SEEK_SET));
    call(FILEdrainall, backidle, fd);
    a$str(correct, "Hello beautiful world!");
    $testeq(correct, backdata);
    done;
}

pro(FILEtest) {
    sane(1);
    call(FILEtest1);
    call(FILEtest2);
    call(FILE3);
    call(FILEtest4);
    done;
}

TEST(FILEtest);
