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

// Test 1: u8aOpen — feed bytes, grab slice, close
ok64 u8aOpenTest() {
    sane(1);

    Bu8 arena = {};
    call(u8bAllocate, arena, 4096);

    // Open a u8 sub-allocation
    u8gp g = u8aOpen(arena);

    // Gauge rest is the entire arena idle
    testeq(u8gRestLen(g), 4096);
    testeq(u8gLeftLen(g), 0);

    // Feed some bytes
    for (u8 i = 0; i < 100; ++i) call(u8gFeed1, g, i);
    testeq(u8gLeftLen(g), 100);

    // Close: commits data to past, returns slice
    u8cs s1 = {};
    u8aClose(arena, s1);
    testeq(u8csLen(s1), 100);

    // After close, past region consumed 100 bytes
    testeq(u8bPastLen(arena), 100);

    // Open again, feed more
    g = u8aOpen(arena);
    testeq(u8gRestLen(g), 4096 - 100);
    for (u8 i = 0; i < 50; ++i) call(u8gFeed1, g, i);
    u8cs s1b = {};
    u8aClose(arena, s1b);
    testeq(u8bPastLen(arena), 150);

    // Original slice still valid (same backing memory)
    testeq(u8csLen(s1), 100);
    testeq(*s1[0], 0);
    testeq(*(s1[1] - 1), 99);

    call(u8bFree, arena);
    done;
}

// Test 2: u32aOpen — feed u32s, verify alignment
ok64 u32aOpenTest() {
    sane(1);

    Bu8 arena = {};
    call(u8bAllocate, arena, 4096);

    // Feed one byte to misalign
    u8gp g0 = u8aOpen(arena);
    call(u8gFeed1, g0, 0xFF);
    u8cs pad = {};
    u8aClose(arena, pad);
    testeq(u8bPastLen(arena), 1);

    // Open u32 arena — should align up to 4
    u32gp g = u32aOpen(arena);
    test(((uintptr_t)g[1] & 3) == 0, FAIL);
    // Alignment padding ate 3 bytes
    test(u8bPastLen(arena) >= 4, FAIL);

    // Feed u32 values
    for (u32 i = 0; i < 50; ++i) call(u32gFeed1, g, i * 7);
    testeq(u32gLeftLen(g), 50);

    u32cs left = {};
    u32aClose(arena, left);
    testeq(u32csLen(left), 50);
    testeq(*left[0], 0);
    testeq(*(left[0] + 10), 70);

    // Past consumed: 4 (aligned) + 50*4 = 204 bytes
    testeq(u8bPastLen(arena), 4 + 50 * 4);

    // Slice data survives after close
    testeq(*left[0], 0);
    testeq(*(left[0] + 49), 49 * 7);

    call(u8bFree, arena);
    done;
}

// Test 3: Mixed u8 + u32 interleaved — model the LESS pattern
ok64 MixedArenaTest() {
    sane(1);

    Bu8 arena = {};
    call(u8bAllocate, arena, 8192);

    // Phase 1: write some u8 bytes
    u8gp g8 = u8aOpen(arena);
    u8cs hello = $u8str("hello");
    call(u8gFeed, g8, hello);
    u8cs s_hello = {};
    u8aClose(arena, s_hello);
    testeq(u8bPastLen(arena), 5);

    // Phase 2: write some u32 tokens
    u32gp g32 = u32aOpen(arena);
    test(((uintptr_t)g32[1] & 3) == 0, FAIL);
    for (u32 i = 0; i < 10; ++i) call(u32gFeed1, g32, i + 100);
    u32cs toks = {};
    u32aClose(arena, toks);
    testeq(u32csLen(toks), 10);

    // Phase 3: more u8 bytes
    g8 = u8aOpen(arena);
    u8cs world = $u8str("world");
    call(u8gFeed, g8, world);
    u8cs s_world = {};
    u8aClose(arena, s_world);

    // All data inside the arena
    test(u8bPastLen(arena) > 0, FAIL);
    test(u8bPastLen(arena) <= 8192, FAIL);

    // Original slices still point to valid data
    $testeq(s_hello, hello);
    testeq(u32csLen(toks), 10);
    testeq(*toks[0], 100);
    testeq(*(toks[0] + 9), 109);
    $testeq(s_world, world);

    call(u8bFree, arena);
    done;
}

// Test 4: Multiple cycles then arena reset
ok64 ArenaCycleTest() {
    sane(1);

    Bu8 arena = {};
    call(u8bAllocate, arena, 4096);

    // Do 10 open/feed/close cycles
    for (int cycle = 0; cycle < 10; ++cycle) {
        u32gp g = u32aOpen(arena);
        for (u32 i = 0; i < 20; ++i) call(u32gFeed1, g, i);
        testeq(u32gLeftLen(g), 20);
        u32cs cyc = {};
        u32aClose(arena, cyc);
    }

    // Arena should have consumed 10 * 20 * 4 = 800 bytes (plus alignment)
    test(u8bPastLen(arena) >= 800, FAIL);
    test(u8bPastLen(arena) <= 800 + 10 * 4, FAIL);

    // Reset the arena — reuse all space
    u8bReset(arena);
    testeq(u8bPastLen(arena), 0);
    testeq(u8bIdleLen(arena), 4096);

    // Can fill again after reset
    u8gp g8 = u8aOpen(arena);
    for (u8 i = 0; i < 200; ++i) call(u8gFeed1, g8, i);
    testeq(u8gLeftLen(g8), 200);
    u8cs refill = {};
    u8aClose(arena, refill);
    testeq(u8bPastLen(arena), 200);

    call(u8bFree, arena);
    done;
}

// Test 5: Verify slices remain valid after close
ok64 ArenaSliceValidTest() {
    sane(1);

    Bu8 arena = {};
    call(u8bAllocate, arena, 4096);

    // Allocate three slices
    u8gp g1 = u8aOpen(arena);
    u8cs pat1 = $u8str("AAAA");
    call(u8gFeed, g1, pat1);
    u8cs s1 = {};
    u8aClose(arena, s1);

    u32gp g2 = u32aOpen(arena);
    call(u32gFeed1, g2, 0xDEADBEEF);
    u32cs s2 = {};
    u32aClose(arena, s2);

    u8gp g3 = u8aOpen(arena);
    u8cs pat3 = $u8str("ZZZZ");
    call(u8gFeed, g3, pat3);
    u8cs s3 = {};
    u8aClose(arena, s3);

    // All three slices still valid, no overlap corruption
    $testeq(s1, pat1);
    testeq(u32csLen(s2), 1);
    testeq(*s2[0], (u32)0xDEADBEEF);
    $testeq(s3, pat3);

    // Slices don't overlap
    test((u8cp)s1[1] <= (u8cp)s2[0], FAIL);
    test((u8cp)s2[1] <= (u8cp)s3[0], FAIL);

    call(u8bFree, arena);
    done;
}

ok64 AREAtest() {
    sane(1);
    call(AREACarveTest);
    call(u8aOpenTest);
    call(u32aOpenTest);
    call(MixedArenaTest);
    call(ArenaCycleTest);
    call(ArenaSliceValidTest);
    done;
}

TEST(AREAtest)
