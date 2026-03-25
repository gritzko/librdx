#include "FILE.h"

#include <dirent.h>
#include <stdlib.h>

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

ok64 FILEtest1() {
    sane(1);
    a_path(path, $cstr("/tmp"));
    a_cstr(tmpl, "FILEtest1_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    u8cs text = $u8str("Hello world!\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(path));
    call(FILEFeed, fd, text);
    call(FILEClose, &fd);
    call(FILEUnLink, path8cgIn(path));
    done;
}

ok64 FILEtest2() {
    sane(1);
    a_path(path, $cstr("/tmp"));
    a_cstr(tmpl, "FILEtest2_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(path));
    call(FILEResize, &fd, 4096);
    u8bp mapbuf = NULL;
    call(FILEMapFD, &mapbuf, &fd, PROT_READ | PROT_WRITE);
    testeq(Bsize(mapbuf), 4096);
    Bat(mapbuf, 42) = 1;
    call(FILEUnMap, mapbuf);
    call(FILEMapRO, &mapbuf, path8cgIn(path));
    testeq(Blen(mapbuf), 4096);
    testeq(Bat(mapbuf, 41), 0);
    testeq(Bat(mapbuf, 42), 1);
    call(FILEUnMap, mapbuf);
    done;
}

ok64 FILE3() {
    sane(1);
    a_path(path, $cstr("/tmp"));
    a_cstr(tmpl, "FILE3_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    a_cstr(text, "Hello world!");
    u8bp buf = NULL;
    call(FILEMapCreate, &buf, path8cgIn(path), PAGESIZE);
    u8bReset(buf);
    call(u8bFeed, buf, text);
    call(FILEUnMap, buf);
    u8bp buf2 = NULL;
    call(FILEMapRO, &buf2, path8cgIn(path));
    call(FILEUnMap, buf2);
    // nedo(FILEUnLink(path));
    done;
}

ok64 FILEtest4() {
    sane(1);
    a_path(path, $cstr("/tmp"));
    a_cstr(tmpl, "FILEtest4_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    a$str(one, "Hello");
    a$str(two, " beautiful");
    a$str(three, " world!");
    aBpad2(u8cs, queue, 4);
    call(u8cssFeed3, queueidle, one, two, three);
    int fd;
    call(FILECreate, &fd, path8cgIn(path));
    call(FILEFeedv, fd, queuedata);
    want($empty(queuedata));
    aBpad2(u8, back, 64);
    testeq(0, lseek(fd, 0, SEEK_SET));
    call(FILEdrainall, backidle, fd);
    a$str(correct, "Hello beautiful world!");
    $testeq(correct, backdata);
    done;
}

ok64 FILEtest5() {
    sane(1);
    // Test streaming I/O primitives
    a_path(path, $cstr("/tmp"));
    a_cstr(tmpl, "FILEtest5_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    a_cstr(testdata, "The quick brown fox jumps over the lazy dog");

    // Create test file
    int wfd;
    call(FILECreate, &wfd, path8cgIn(path));
    a_dup(u8 const, data, testdata);
    call(FILEFeedall, wfd, data);
    call(FILEClose, &wfd);

    // Test FILEEnsureSoft
    int rfd;
    call(FILEOpen, &rfd, path8cgIn(path), O_RDONLY);
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
    call(FILECreate, &wfd2, path8cgIn(path));
    aBpad2(u8, outbuf, 64);
    call(u8bFeed, outbufbuf, testdata);

    // Flush should not trigger (below threshold)
    call(FILEFlushThreshold, wfd2, outbufbuf, 100);
    want(u8bDataLen(outbufbuf) > 0);  // Still has data

    // Flush should trigger (above threshold)
    call(FILEFlushThreshold, wfd2, outbufbuf, 10);
    testeq(u8bPastLen(outbufbuf), u8csLen(testdata));  // Data moved to past

    call(FILEClose, &wfd2);
    call(FILEUnLink, path8cgIn(path));
    done;
}

ok64 FILEtest6() {
    sane(1);
    // Test FILEMakeDir and FILERmDir (non-recursive)
    a_path(dirpath, $cstr("/tmp"));
    a_cstr(tmpl, "FILEtest6_XXXXXX");
    call(path8gAddTmp, path8gIn(dirpath), tmpl);

    // Create directory
    call(FILEMakeDir, path8cgIn(dirpath));

    // Verify it exists
    struct stat s = {};
    test(OK == FILEStat(&s, path8cgIn(dirpath)), FILEFAIL);
    test((s.st_mode & S_IFMT) == S_IFDIR, FILEFAIL);

    // Remove directory (non-recursive)
    call(FILERmDir, path8cgIn(dirpath), false);

    // Verify it's gone
    test(OK != FILEStat(&s, path8cgIn(dirpath)), FILEFAIL);

    done;
}

ok64 FILEtest7() {
    sane(1);
    // Test FILERmDir fails on non-empty directory when non-recursive
    a_path(dirpath, $cstr("/tmp"));
    a_cstr(tmpl, "FILEtest7_XXXXXX");
    call(path8gAddTmp, path8gIn(dirpath), tmpl);

    // Create directory
    call(FILEMakeDir, path8cgIn(dirpath));

    // Create file inside
    a_cstr(fname, "file.txt");
    call(path8gPush, path8gIn(dirpath), fname);
    int fd;
    call(FILECreate, &fd, path8cgIn(dirpath));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(dirpath));

    // FILERmDir should fail on non-empty dir when non-recursive
    ok64 err = FILERmDir(path8cgIn(dirpath), false);
    test(err != OK, FILEFAIL);

    // Clean up with recursive delete
    call(FILERmDir, path8cgIn(dirpath), true);

    // Verify it's gone
    struct stat s = {};
    test(OK != FILEStat(&s, path8cgIn(dirpath)), FILEFAIL);

    done;
}

ok64 FILEtest8() {
    sane(1);
    // Test FILERmDir recursive on nested structure
    a_path(path, $cstr("/tmp"));
    a_cstr(tmpl, "FILEtest8_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    a_cstr(sub1, "sub1");
    a_cstr(sub2, "sub2");
    a_cstr(f1, "file1.txt");
    a_cstr(f2, "file2.txt");
    a_cstr(f3, "file3.txt");

    // Create nested structure: base/sub1/sub2
    call(FILEMakeDir, path8cgIn(path));
    call(path8gPush, path8gIn(path), sub1);
    call(FILEMakeDir, path8cgIn(path));
    call(path8gPush, path8gIn(path), sub2);
    call(FILEMakeDir, path8cgIn(path));
    call(path8gPop, path8gIn(path));
    call(path8gPop, path8gIn(path));

    // Create files
    int fd;
    call(path8gPush, path8gIn(path), f1);
    call(FILECreate, &fd, path8cgIn(path));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(path));

    call(path8gPush, path8gIn(path), sub1);
    call(path8gPush, path8gIn(path), f2);
    call(FILECreate, &fd, path8cgIn(path));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(path));

    call(path8gPush, path8gIn(path), sub2);
    call(path8gPush, path8gIn(path), f3);
    call(FILECreate, &fd, path8cgIn(path));
    call(FILEClose, &fd);

    // Verify deepest file exists
    struct stat s = {};
    test(OK == FILEStat(&s, path8cgIn(path)), FILEFAIL);

    // Back to base and delete recursively
    call(path8gPop, path8gIn(path));
    call(path8gPop, path8gIn(path));
    call(path8gPop, path8gIn(path));
    call(FILERmDir, path8cgIn(path), true);

    // Verify it's gone
    test(OK != FILEStat(&s, path8cgIn(path)), FILEFAIL);

    done;
}

// Test FILERmDir with a_path full path (no path8gPush)
// Reproduces PUT.c cleanup pattern
ok64 FILEtest8b() {
    sane(1);

    // Step 1: create a temp dir using the normal path8g pattern
    a_path(setup, $cstr("/tmp"));
    a_cstr(tmpl, "FILEtest8b_XXXXXX");
    call(path8gAddTmp, path8gIn(setup), tmpl);
    call(FILEMakeDir, path8cgIn(setup));

    // Create nested content
    a_cstr(sub, "sub");
    call(path8gPush, path8gIn(setup), sub);
    call(FILEMakeDir, path8cgIn(setup));
    a_cstr(fname, "file.txt");
    call(path8gPush, path8gIn(setup), fname);
    int fd;
    call(FILECreate, &fd, path8cgIn(setup));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(setup));
    call(path8gPop, path8gIn(setup));

    // Step 2: get the path as a C string, then use a_path to reconstruct
    // This is the pattern from PUT.c: a_path(path, g_tmpdir)
    char cpath[256];
    snprintf(cpath, sizeof(cpath), "%.*s",
             (int)u8bDataLen(setup), (char *)setup[1]);

    // Verify path8cgOK on a_path result (the suspected failure)
    a_path(repath, $cstr(cpath));
    b8 ok_before = path8cgOK(path8cgIn(repath));
    fprintf(stderr, "  path8cgOK before Term: %d\n", ok_before);
    fprintf(stderr, "  byte at idle: 0x%02x\n", *repath[2]);

    // Fix: null-terminate
    call(path8gTerm, path8gIn(repath));
    b8 ok_after = path8cgOK(path8cgIn(repath));
    fprintf(stderr, "  path8cgOK after Term: %d\n", ok_after);
    test(ok_after, FAIL);

    // Now remove with FILERmDir
    call(FILERmDir, path8cgIn(repath), true);

    // Verify it's gone
    struct stat s = {};
    test(OK != FILEStat(&s, path8cgIn(repath)), FILEFAIL);

    done;
}

// Test FILEerrno translation
ok64 FILEtest9() {
    sane(1);

    // FILEStat on non-existent file should return FILENOENT
    a_path(nofile, $cstr("/tmp"));
    a_cstr(tmpl, "FILEtest9_XXXXXX");
    call(path8gAddTmp, path8gIn(nofile), tmpl);
    struct stat s = {};
    ok64 res = FILEStat(&s, path8cgIn(nofile));
    test(res == FILENOENT, FILEFAIL);
    
    // Verify FILEerrno translates correctly
    test(FILEerrno(ENOENT) == FILENOENT, FILEFAIL);
    test(FILEerrno(EACCES) == FILEACCES, FILEFAIL);
    test(FILEerrno(EEXIST) == FILEEXIST, FILEFAIL);
    test(FILEerrno(0) == OK, FILEFAIL);
    test(FILEerrno(9999) == FILEFAIL, FILEFAIL);  // unknown errno
    
    done;
}

// Test file iterator (into/next/outo pattern)
ok64 FILEIterTest() {
    sane(1);
    // Create test directory structure
    a_path(base, $cstr("/tmp"));
    a_cstr(tmpl, "FILEIterTest_XXXXXX");
    call(path8gAddTmp, path8gIn(base), tmpl);
    call(FILEMakeDir, path8cgIn(base));

    // Create files and subdirs
    a_cstr(f1, "file1.txt");
    a_cstr(f2, "file2.txt");
    a_cstr(sub, "subdir");
    a_cstr(f3, "nested.txt");

    int fd;
    call(path8gPush, path8gIn(base), f1);
    call(FILECreate, &fd, path8cgIn(base));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(base));

    call(path8gPush, path8gIn(base), f2);
    call(FILECreate, &fd, path8cgIn(base));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(base));

    call(path8gPush, path8gIn(base), sub);
    call(FILEMakeDir, path8cgIn(base));
    call(path8gPush, path8gIn(base), f3);
    call(FILECreate, &fd, path8cgIn(base));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(base));
    call(path8gPop, path8gIn(base));

    // Now test iterator
    int file_count = 0;
    int dir_count = 0;
    fileit it = {};
    call(FILEIterOpen, &it, path8gIn(base));
    scan(FILENext, &it) {
        if (it.type == DT_REG) file_count++;
        if (it.type == DT_DIR) {
            dir_count++;
            // Recurse into subdir
            fileit child = {};
            call(FILEInto, &child, &it);
            scan(FILENext, &child) {
                if (child.type == DT_REG) file_count++;
            }
            seen(END);
            call(FILEOuto, &child, &it);
        }
    }
    seen(END);
    call(FILEIterClose, &it);

    testeq(file_count, 3);  // file1.txt, file2.txt, nested.txt
    testeq(dir_count, 1);   // subdir

    // Cleanup
    call(FILERmDir, path8cgIn(base), true);
    done;
}

// Test sorted file iterator
ok64 FILEIterSortedTest() {
    sane(1);
    // Create test directory structure
    a_path(base, $cstr("/tmp"));
    a_cstr(tmpl, "FILEIterSorted_XXXXXX");
    call(path8gAddTmp, path8gIn(base), tmpl);
    call(FILEMakeDir, path8cgIn(base));

    // Create files with names that sort differently than creation order
    a_cstr(fz, "zebra.txt");
    a_cstr(fa, "alpha.txt");
    a_cstr(fm, "middle.txt");
    a_cstr(sub, "beta_dir");
    a_cstr(fc, "charlie.txt");

    int fd;
    // Create in reverse alphabetical order
    call(path8gPush, path8gIn(base), fz);
    call(FILECreate, &fd, path8cgIn(base));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(base));

    call(path8gPush, path8gIn(base), fm);
    call(FILECreate, &fd, path8cgIn(base));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(base));

    call(path8gPush, path8gIn(base), sub);
    call(FILEMakeDir, path8cgIn(base));
    call(path8gPush, path8gIn(base), fc);
    call(FILECreate, &fd, path8cgIn(base));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(base));
    call(path8gPop, path8gIn(base));

    call(path8gPush, path8gIn(base), fa);
    call(FILECreate, &fd, path8cgIn(base));
    call(FILEClose, &fd);
    call(path8gPop, path8gIn(base));

    // Test sorted iterator
    aB(u8, sortbuf);
    call(u8bAllocate, sortbufbuf, 4096);

    fileit it = {};
    call(FILEIterOpenSorted, &it, path8gIn(base), sortbufbuf, FILEentryZ);

    // Collect entries in order
    u8 names[4][32] = {};
    int count = 0;
    scan(FILENext, &it) {
        // Extract just the filename (last component)
        u8cp p = it.path[1];
        while (p > it.path[0] && *(p - 1) != '/') p--;
        size_t len = it.path[1] - p;
        if (len < 32) {
            memcpy(names[count], p, len);
        }
        if (it.type == DT_DIR) {
            // Recurse - should inherit sorting
            fileit child = {};
            call(FILEInto, &child, &it);
            scan(FILENext, &child) {
                // Child iteration
            }
            seen(END);
            call(FILEOuto, &child, &it);
        }
        count++;
    }
    seen(END);
    call(FILEIterClose, &it);

    // Verify sorted order: alpha.txt, beta_dir, middle.txt, zebra.txt
    testeq(count, 4);
    testeq(strcmp((char *)names[0], "alpha.txt"), 0);
    testeq(strcmp((char *)names[1], "beta_dir"), 0);
    testeq(strcmp((char *)names[2], "middle.txt"), 0);
    testeq(strcmp((char *)names[3], "zebra.txt"), 0);

    // Cleanup
    call(u8bFree, sortbufbuf);
    call(FILERmDir, path8cgIn(base), true);
    done;
}

// Test FILEBook - booked VA range with growable file mapping
ok64 FILEBookTest() {
    sane(1);
    a_path(path, $cstr("/tmp"));
    a_cstr(tmpl, "FILEBookTest_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);

    // Create with 1MB booked range, 4KB initial size
    u8bp buf = NULL;
    call(FILEBookCreate, &buf, path8cgIn(path), 1 * MB, 4 * KB);

    // Verify initial size
    testeq(Bsize(buf), roundup(4 * KB, PAGESIZE));

    // Save base address
    u8p base = buf[0];

    // Write some data
    u8bReset(buf);
    a_cstr(text, "Hello booked world!");
    call(u8bFeed, buf, text);

    // Extend to 64KB
    call(FILEBookExtend, buf, 64 * KB);

    // Verify base address unchanged (the whole point!)
    testeq(buf[0], base);
    testeq(Bsize(buf), roundup(64 * KB, PAGESIZE));

    // Verify data survived
    testeq(memcmp(buf[1], "Hello booked world!", 19), 0);

    // Write more data at higher offset
    u8p far = buf[0] + 60 * KB;
    memcpy(far, "Far away data", 13);

    // Sync to disk
    call(FILEMSync, buf);

    // Extend again
    call(FILEBookExtend, buf, 128 * KB);
    testeq(buf[0], base);  // still same address

    // Verify far data survived
    testeq(memcmp(buf[0] + 60 * KB, "Far away data", 13), 0);

    // Verify FILEIsBooked
    testeq(FILEIsBooked(buf), YES);

    // Cleanup
    call(FILEUnBook, buf);
    call(FILEUnLink, path8cgIn(path));

    done;
}

// Test FILEBook with existing file
ok64 FILEBookExistingTest() {
    sane(1);
    a_path(path, $cstr("/tmp"));
    a_cstr(tmpl, "FILEBookExist_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);

    // Create file with some content first
    u8bp prebuf = NULL;
    call(FILEMapCreate, &prebuf, path8cgIn(path), PAGESIZE);
    u8bReset(prebuf);
    a_cstr(initial, "Initial content here");
    call(u8bFeed, prebuf, initial);
    call(FILEUnMap, prebuf);

    // Now book with larger range
    u8bp buf = NULL;
    call(FILEBook, &buf, path8cgIn(path), 1 * MB);

    // Verify existing content is there
    testeq(memcmp(buf[0], "Initial content here", 20), 0);

    // Extend and write more
    call(FILEBookExtend, buf, 64 * KB);
    u8p far = buf[0] + 32 * KB;
    memcpy(far, "Extended data", 13);

    call(FILEUnBook, buf);
    call(FILEUnLink, path8cgIn(path));

    done;
}

// Test FILEBookExtend beyond booked range fails
ok64 FILEBookLimitTest() {
    sane(1);
    a_path(path, $cstr("/tmp"));
    a_cstr(tmpl, "FILEBookLimit_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);

    u8bp buf = NULL;
    call(FILEBookCreate, &buf, path8cgIn(path), 64 * KB, 4 * KB);

    // Extending beyond booked range should fail
    ok64 err = FILEBookExtend(buf, 128 * KB);
    test(err != OK, FILEFAIL);

    // But extending within range should work
    call(FILEBookExtend, buf, 32 * KB);

    call(FILEUnBook, buf);
    call(FILEUnLink, path8cgIn(path));

    done;
}

// Test FILEBookEnsure - auto-grow
ok64 FILEBookEnsureTest() {
    sane(1);
    a_path(path, $cstr("/tmp"));
    a_cstr(tmpl, "FILEBookEnsure_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);

    // Create with small initial size (1 page), large booked range
    u8bp book = NULL;
    call(FILEBookCreate, &book, path8cgIn(path), 8 * MB, PAGESIZE);
    u8bReset(book);

    // Write in a loop, calling FILEBookEnsure before each write
    for (int i = 0; i < 1000; i++) {
        call(FILEBookEnsure, book, 4096);
        memset(*u8bIdle(book), 'A' + (i % 26), 4096);
        u8sFed(u8bIdle(book), 4096);
    }

    // Verify ~4MB written, file grew automatically
    testeq(u8bDataLen(book), (size_t)(1000 * 4096));

    // Trim and verify
    call(FILETrimBook, book);
    call(FILEUnBook, book);
    call(FILEUnLink, path8cgIn(path));

    done;
}

ok64 FILEtest() {
    sane(1);
    call(FILEtest1);
    call(FILEtest2);
    call(FILE3);
    call(FILEtest4);
    call(FILEtest5);
    call(FILEtest6);
    call(FILEtest7);
    call(FILEtest8);
    call(FILEtest8b);
    call(FILEtest9);
    call(FILEIterTest);
    call(FILEIterSortedTest);
    call(FILEBookTest);
    call(FILEBookExistingTest);
    call(FILEBookLimitTest);
    call(FILEBookEnsureTest);
    done;
}

TEST(FILEtest);
