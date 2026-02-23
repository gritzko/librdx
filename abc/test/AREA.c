#include "AREA.h"
#include "INT.h"
#include "PRO.h"
#include "TEST.h"

ok64 AREACarveTest() {
    sane(1);

    Bu8 area = {};
    call(AREAOpen, area, MB);

    // Carve three typed buffers
    Bu8 bytes = {};
    AREACarve(area, u8, bytes, 1000);

    Bu32 words = {};
    AREACarve(area, u32, words, 500);

    Bu64 quads = {};
    AREACarve(area, u64, quads, 200);

    // Each buffer is empty but has correct capacity
    testeq(u8bDataLen(bytes), 0);
    testeq(u8bLen(bytes), 1000);
    testeq(u32bDataLen(words), 0);
    testeq(u32bLen(words), 500);
    testeq(u64bDataLen(quads), 0);
    testeq(u64bLen(quads), 200);

    // Buffers don't overlap: each starts at or past the previous end
    test((u8p)words[0] >= (u8p)bytes[3], FAIL);
    test((u8p)quads[0] >= (u8p)words[3], FAIL);

    // Alignment: u32 buffer is 4-aligned, u64 buffer is 8-aligned
    test(((size_t)words[0] & 3) == 0, FAIL);
    test(((size_t)quads[0] & 7) == 0, FAIL);

    // Everything lives inside the area
    test(AREAContains(area, bytes[0]), FAIL);
    test(AREAContains(area, words[0]), FAIL);
    test(AREAContains(area, quads[0]), FAIL);
    test((u8cp)bytes[3] <= (u8cp)area[3], FAIL);
    test((u8cp)words[3] <= (u8cp)area[3], FAIL);
    test((u8cp)quads[3] <= (u8cp)area[3], FAIL);

    // Total consumed <= 1MB
    size_t used = u8bBusyLen(area);
    size_t expected = 1000 * sizeof(u8) + 500 * sizeof(u32) +
                      200 * sizeof(u64);
    test(used >= expected, FAIL);
    test(used <= expected + 16, FAIL);

    // Remaining area room
    size_t left = u8bIdleLen(area);
    test(left + used == (size_t)Bsize(area), FAIL);

    // Actually use the buffers: feed and read back
    for (u8 i = 0; i < 100; ++i) call(u8bFeed1, bytes, i);
    testeq(u8bDataLen(bytes), 100);
    testeq(Bat(bytes, 50), 50);

    for (u32 i = 0; i < 100; ++i) call(u32bFeed1, words, i * 7);
    testeq(u32bDataLen(words), 100);
    testeq(Bat(words, 10), 70);

    for (u64 i = 0; i < 100; ++i) call(u64bFeed1, quads, i * 13);
    testeq(u64bDataLen(quads), 100);
    testeq(Bat(quads, 5), 65);

    // Outside pointer is not contained
    u8 stack_var = 0;
    test(!AREAContains(area, &stack_var), FAIL);

    call(AREAClose, area);
    done;
}

ok64 AREAtest() {
    sane(1);
    call(AREACarveTest);
    done;
}

TEST(AREAtest)
