#include "PACK.h"
#include "SHA.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

#ifndef TEST_CPP_PARSER
#error "TEST_CPP_PARSER must be defined"
#endif

#define PACK_TEST_FILE "/tmp/pack_file_test.lz4"

ok64 PACKfileTest() {
    sane(1);

    // Init PAGE registry
    call(PAGEInit, 16);

    const char *srcpath = TEST_CPP_PARSER;
    sha256 hash_orig = {};
    sha256 hash_read = {};

    // === Phase 1: Read source file, hash it, write to pack ===

    int srcfd = open(srcpath, O_RDONLY);
    test(srcfd >= 0, PACKfail);

    // Get file size
    off_t fsize = lseek(srcfd, 0, SEEK_END);
    test(fsize > 0, PACKfail);
    lseek(srcfd, 0, SEEK_SET);

    // Create pack with 8K buffer (2 pages)
    pack pw = {};
    call(PACKCreate, &pw, PACK_TEST_FILE, fsize);

    // Hash state for original
    SHAstate sha_orig;
    SHAOpen(&sha_orig);

    // Read directly into pack buffer using FILEDrain
    ok64 o = OK;
    while (o == OK) {
        u8sp idle = u8bIdle(pw.pg->buf);
        u8p before = *idle;

        o = FILEDrain(srcfd, idle);
        if (o != OK && o != FILEend) fail(o);

        // Hash data that was read
        if (*idle > before) {
            u8cs chunk = {before, *idle};
            SHAFeed(&sha_orig, chunk);
        }

        // Flush if buffer nearly full
        if (u8bIdleLen(pw.pg->buf) < PAGESIZE) {
            call(PACKFlush, &pw);
        }
    }

    close(srcfd);
    SHAClose(&sha_orig, &hash_orig);

    // Close pack (writes remaining + index)
    call(PACKClose, &pw);

    // === Phase 2: Open pack, read back, verify hash ===

    pack pr = {};
    call(PACKOpen, &pr, PACK_TEST_FILE);

    // Verify data length matches original file size
    test(pr.datalen == (u64)fsize, PACKfail);

    // Hash state for read-back
    SHAstate sha_read;
    SHAOpen(&sha_read);

    // Read sequentially using PAGE callback with slice
    u8cp end = pr.pg->buf[2];
    for (u8cp pos = pr.pg->buf[0]; pos < end; pos += PAGESIZE) {
        u8cp chunk_end = pos + PAGESIZE;
        if (chunk_end > end) chunk_end = end;

        u8cs slice = {pos, chunk_end};
        call(PAGEEnsureSlice, pr.pg, slice);

        // Hash the decompressed data
        SHAFeed(&sha_read, slice);
    }

    SHAClose(&sha_read, &hash_read);

    // Verify hashes match
    test(sha256eq(&hash_orig, &hash_read), PACKcorrupt);

    call(PACKClose, &pr);

    // Cleanup
    unlink(PACK_TEST_FILE);

    done;
}

TEST(PACKfileTest);
