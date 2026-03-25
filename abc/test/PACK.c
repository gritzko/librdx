#include "PACK.h"

#include <string.h>
#include <unistd.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

#define TEST_FILE "/tmp/pack_test.lz4"

ok64 PACKtestBasic() {
    sane(1);

    // Init PAGE registry
    call(PAGEInit, 8);

    pack pw = {};

    // Create pack for writing
    call(PACKCreate, &pw, TEST_FILE, PAGESIZE * 10);

    // Write some data directly to buffer
    u8p buf = pw.pg->buf[0];
    for (int i = 0; i < PAGESIZE * 3; i++) {
        buf[i] = (u8)(i & 0xFF);
    }
    // Update buffer pointer to reflect written data
    ((u8 **)pw.pg->buf)[2] = buf + PAGESIZE * 3;

    // Flush
    call(PACKFlush, &pw);

    // Verify data length after flush
    test(pw.datalen == 3 * PAGESIZE, PACKFAIL);

    // Write more data
    buf = pw.pg->buf[2];
    for (int i = 0; i < PAGESIZE + 100; i++) {
        buf[i] = (u8)((i + 0x33) & 0xFF);
    }
    ((u8 **)pw.pg->buf)[2] = buf + PAGESIZE + 100;

    // Close (flushes remaining)
    call(PACKClose, &pw);

    // Reopen for reading
    pack pr = {};
    call(PACKOpen, &pr, TEST_FILE);

    test(pr.datalen == 4 * PAGESIZE + 100, PACKFAIL);  // 3 + 1 full + 1 partial

    // Read first page
    call(PACKEnsure, pr.pg, NO, 0, PAGESIZE);
    test(PAGEIdxRead(pr.pg, 0) == PAGE_LOADED, PACKFAIL);

    // Verify data
    u8cp data = pr.pg->buf[0];
    for (int i = 0; i < PAGESIZE; i++) {
        test(data[i] == (u8)(i & 0xFF), PACKCORRUPT);
    }

    // Read page 3 (first of second batch)
    call(PACKEnsure, pr.pg, NO, PAGESIZE * 3, PAGESIZE);
    test(PAGEIdxRead(pr.pg, 3) == PAGE_LOADED, PACKFAIL);

    data = pr.pg->buf[0] + PAGESIZE * 3;
    for (int i = 0; i < PAGESIZE; i++) {
        test(data[i] == (u8)((i + 0x33) & 0xFF), PACKCORRUPT);
    }

    call(PACKClose, &pr);

    // Cleanup
    unlink(TEST_FILE);

    done;
}

ok64 PACKtestLargeFile() {
    sane(1);

    pack pw = {};

    // Create pack for 100 pages
    call(PACKCreate, &pw, TEST_FILE, PAGESIZE * 100);

    // Write pattern data
    u8p buf = pw.pg->buf[0];
    u64 buflen = pw.pg->buf[3] - pw.pg->buf[0];

    for (int round = 0; round < 10; round++) {
        // Fill buffer
        for (u64 i = 0; i < buflen; i++) {
            buf[i] = (u8)((round * 17 + i) & 0xFF);
        }
        ((u8 **)pw.pg->buf)[2] = buf + buflen;

        call(PACKFlush, &pw);
    }

    call(PACKClose, &pw);

    // Verify
    pack pr = {};
    call(PACKOpen, &pr, TEST_FILE);

    // Read random pages
    call(PACKEnsure, pr.pg, NO, PAGESIZE * 50, PAGESIZE);
    test(PAGEIdxRead(pr.pg, 50) == PAGE_LOADED, PACKFAIL);

    call(PACKEnsure, pr.pg, NO, PAGESIZE * 99, PAGESIZE);
    test(PAGEIdxRead(pr.pg, 99) == PAGE_LOADED, PACKFAIL);

    call(PACKClose, &pr);

    unlink(TEST_FILE);

    done;
}

ok64 PACKtestIndex() {
    sane(1);

    // Test index helper functions
    u64 block[4] = {0};

    // Set some lengths
    PACKIdxSetLen(block, 0, 100);
    PACKIdxSetLen(block, 5, 200);
    PACKIdxSetLen(block, 11, 300);

    test(PACKIdxLen(block, 0) == 100, PACKFAIL);
    test(PACKIdxLen(block, 5) == 200, PACKFAIL);
    test(PACKIdxLen(block, 11) == 300, PACKFAIL);

    // Test offset calculation
    block[0] = 1000;  // base offset
    for (int i = 0; i < 12; i++) {
        PACKIdxSetLen(block, i, 50 + i);
    }

    // Page 0 offset should be base
    u64 off0 = block[0];
    test(off0 == 1000, PACKFAIL);

    // Page 3 offset = base + len[0] + len[1] + len[2]
    // = 1000 + 50 + 51 + 52 = 1153
    u64 expected = 1000 + 50 + 51 + 52;
    u64 off3 = block[0];
    for (int i = 0; i < 3; i++) {
        off3 += PACKIdxLen(block, i);
    }
    test(off3 == expected, PACKFAIL);

    done;
}

ok64 PACKtest() {
    sane(1);
    call(PACKtestIndex);
    call(PACKtestBasic);
    call(PACKtestLargeFile);
    done;
}

TEST(PACKtest);
