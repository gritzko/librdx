#include "FILE.h"

#include <stdlib.h>

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

pro(FILEtest1) {
    sane(1);
    a_path(path, "/tmp/test1.txt");
    u8cs text = $u8str("Hello world!\n");
    int fd = 0;
    call(FILECreate, &fd, path);
    call(FILEFeed, fd, text);
    call(FILEClose, &fd);
    call(FILEunlink, path);
    done;
}

pro(FILEtest2) {
    sane(1);
    a_path(path, "/tmp/testA.txt");
    int fd = 0;
    call(FILECreate, &fd, path);
    call(FILEResize, &fd, 4096);
    aB(u8, map);
    call(FILEMapFD, mapbuf, &fd, PROT_READ | PROT_WRITE);
    testeq(Bsize(mapbuf), 4096);
    Bat(mapbuf, 42) = 1;
    call(FILEUnMap, mapbuf);
    call(FILEMapRO, mapbuf, path);
    testeq(Blen(mapbuf), 4096);
    testeq(Bat(mapbuf, 41), 0);
    testeq(Bat(mapbuf, 42), 1);
    call(FILEUnMap, mapbuf);
    done;
}

pro(FILE3) {
    sane(1);
    a_path(path, "/tmp/FILE3.txt");
    a_cstr(text, "Hello world!");
    Bu8 buf = {};
    call(FILEMapCreate, buf, path, PAGESIZE);
    u8bReset(buf);
    call(u8bFeed, buf, text);
    call(FILEUnMap, buf);
    Bu8 buf2 = {};
    call(FILEMapRO, buf2, path);
    aB$(u8c, path2, buf2, 0, $len(path));
    $testeq(path, path2);
    call(FILEUnMap, buf2);
    // nedo(FILEunlink(path));
    done;
}

pro(FILEtest4) {
    sane(1);
    a_path(path, "/tmp/FILEtest4.txt");
    a$str(one, "Hello");
    a$str(two, " beautiful");
    a$str(three, " world!");
    aBpad2(u8cs, queue, 4);
    call(u8cssFeed3, queueidle, one, two, three);
    int fd;
    call(FILECreate, &fd, path);
    call(FILEFeedv, fd, queuedata);
    want($empty(queuedata));
    aBpad2(u8, back, 64);
    testeq(0, lseek(fd, 0, SEEK_SET));
    call(FILEdrainall, backidle, fd);
    a$str(correct, "Hello beautiful world!");
    $testeq(correct, backdata);
    done;
}

pro(FILEtest5) {
    sane(1);
    // Test streaming I/O primitives
    a_path(path, "/tmp/FILEtest5.txt");
    a_cstr(testdata, "The quick brown fox jumps over the lazy dog");

    // Create test file
    int wfd;
    call(FILECreate, &wfd, path);
    a_dup(u8 const, data, testdata);
    call(FILEFeedall, wfd, data);
    call(FILEClose, &wfd);

    // Test FILEEnsureSoft
    int rfd;
    call(FILEOpen, &rfd, path, O_RDONLY);
    aBpad2(u8, buf, 64);
    call(FILEEnsureSoft, rfd, bufbuf, 10);
    test(u8bDataLen(bufbuf) >= 10,
         BADPOS);  // Should have read at least 10 bytes

    // Test FILEEnsureHard
    call(FILEEnsureHard, rfd, bufbuf, 20);
    want(u8bDataLen(bufbuf) >= 20);  // Must have exactly 20 bytes

    // Test FILEEnsureHard with buffer too small (should fail)
    aBpad2(u8, smallbuf, 8);
    ok64 err = FILEEnsureHard(rfd, smallbufbuf, 100);
    want(err != OK);  // Should fail - buffer too small

    call(FILEClose, &rfd);

    // Test FILEFlushThreshold
    int wfd2;
    call(FILECreate, &wfd2, path);
    aBpad2(u8, outbuf, 64);
    call(u8bFeed, outbufbuf, testdata);

    // Flush should not trigger (below threshold)
    call(FILEFlushThreshold, wfd2, outbufbuf, 100);
    want(u8bDataLen(outbufbuf) > 0);  // Still has data

    // Flush should trigger (above threshold)
    call(FILEFlushThreshold, wfd2, outbufbuf, 10);
    testeq(u8bPastLen(outbufbuf), u8csLen(testdata));  // Data moved to past

    call(FILEClose, &wfd2);
    call(FILEunlink, path);
    done;
}

pro(FILEtest) {
    sane(1);
    call(FILEtest1);
    call(FILEtest2);
    call(FILE3);
    call(FILEtest4);
    call(FILEtest5);
    done;
}

TEST(FILEtest);
