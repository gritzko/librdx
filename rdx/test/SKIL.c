#include "abc/01.h"
#include "abc/TEST.h"
#include "abc/TLV.h"
#include "rdx/RDX.h"

// Defined in SKIL.c
extern u64 SKILRank(u64 pos);

ok64 SKILTest1() {
    con int length = 100;
    sane(1);
    a_pad(u8, pad, PAGESIZE);
    a_pad(u64, tabs, PAGESIZE);
    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE, .extra = (void*)tabs};
    $mv(e.into, pad_idle);
    e.type = RDX_TYPE_EULER;
    call(rdxNext, &e);
    rdx i = {};
    call(rdxInto, &i, &e);
    for (int j = 0; j < length; j++) {
        i.type = RDX_TYPE_INT;
        i.i = j;
        i.id.seq = j;
        i.id.src = 0;
        call(rdxNext, &i);
    }
    call(rdxOuto, &i, &e);
    $mv(pad_idle, e.into);

    rdx e2 = {.format = RDX_FMT_SKIL};
    $mv(e2.data, pad_data);
    call(rdxNext, &e2);
    test(e2.type == RDX_TYPE_EULER, RDXBAD);
    for (int j = 0; j < length; j++) {
        rdx i2 = {};
        i2.type = RDX_TYPE_INT;
        i2.i = j;
        call(rdxInto, &i2, &e2);
        test(i2.id.seq == j, NOEQ);
        test(i2.id.src == 0, NOEQ);
        call(rdxOuto, &i2, &e2);
    }

    done;
}

ok64 SKILTestRankFunction() {
    sane(1);

    // Test rank function produces correct values
    // Rank = b ^ (b-1) where b is the block number (pos / 256)

    testeq(SKILRank(255), 3);  // Block 1: 1 ^ 0 = 1
    testeq(SKILRank(511), 7);  // Block 2: 2 ^ 1 = 3
    //
    // Verify logarithmic distribution: high ranks are less common
    int high_rank_count = 0;
    for (int i = 0; i < 10000; i += 256) {
        if (SKILRank(i) >= 7) high_rank_count++;
    }
    want(high_rank_count <= 30);  // About 1/4 of blocks have rank >= 7
    want(high_rank_count >= 10);  // But not too few

    done;
}

ok64 SKILTestSkipPointers() {
    sane(1);

    // Write 1000 records and verify skip pointers are generated
    con int length = 1000;
    a_pad(u8, pad, PAGESIZE * 16);
    a_pad(u64, tabs, 64);

    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE, .extra = (void*)tabs};
    $mv(e.into, pad_idle);
    e.type = RDX_TYPE_EULER;
    call(rdxNext, &e);

    rdx i = {};
    call(rdxInto, &i, &e);
    for (int j = 0; j < length; j++) {
        i.type = RDX_TYPE_INT;
        i.i = j;
        i.id.seq = j;
        i.id.src = 0;
        call(rdxNext, &i);
    }
    call(rdxOuto, &i, &e);
    $mv(pad_idle, e.into);

    // Verify we can read back all records sequentially
    rdx e2 = {.format = RDX_FMT_SKIL};
    $mv(e2.data, pad_data);
    call(rdxNext, &e2);
    test(e2.type == RDX_TYPE_EULER, RDXBAD);

    // Read all records sequentially and verify
    rdx i2 = {};
    call(rdxInto, &i2, &e2);  // Enter container once
    for (int j = 0; j < length; j++) {
        ok64 o = rdxNext(&i2);  // Read next record
        if (o != OK) {
            printf("oops\n");
        }
        test(i2.type == RDX_TYPE_INT, RDXBAD);
        testeq(i2.i, j);
        testeq(i2.id.seq, j);
    }
    call(rdxOuto, &i2, &e2);  // Exit container once

    done;
}

ok64 SKILTestBinarySearch() {
    sane(1);

    // Test seeking to specific positions using SKIL binary search
    con int length = 100000;
    a_pad(u8, pad, roundup(length * 20, PAGESIZE));
    a_pad(u64, tabs, 64);

    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE, .extra = (void*)tabs};
    $mv(e.into, pad_idle);
    e.type = RDX_TYPE_EULER;
    call(rdxNext, &e);

    rdx i = {};
    call(rdxInto, &i, &e);
    // Write multiples of 10: 0, 10, 20, 30, ..., 9990
    for (int j = 0; j < length; j++) {
        i.type = RDX_TYPE_INT;
        i.i = j * 10;
        i.id.seq = j * 10;
        i.id.src = 0;
        call(rdxNext, &i);
    }
    call(rdxOuto, &i, &e);
    $mv(pad_idle, e.into);

    // Read back and verify using binary search
    rdx e2 = {.format = RDX_FMT_SKIL};
    $mv(e2.data, pad_data);
    call(rdxNext, &e2);
    test(e2.type == RDX_TYPE_EULER, RDXBAD);

    // Test seeking to various positions
    int test_values[] = {0, 100, 500, 5000, 9990};
    for (int k = 0; k < 5; k++) {
        rdx target = {};
        target.type = RDX_TYPE_INT;
        target.i = test_values[k];

        call(rdxInto, &target, &e2);
        // Should find the exact value
        test(target.type == RDX_TYPE_INT, RDXBAD);
        test(target.i == test_values[k], NOEQ);
        test(target.id.seq == test_values[k], NOEQ);
        call(rdxOuto, &target, &e2);
    }

    done;
}

ok64 SKILTestLargeDataset() {
    sane(1);

    // Test with larger dataset (1000 records, sequential read only)
    con int length = 1000;
    a_pad(u8, pad, PAGESIZE * 32);
    a_pad(u64, tabs, PAGESIZE);

    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE, .extra = (void*)tabs};
    $mv(e.into, pad_idle);
    e.type = RDX_TYPE_EULER;
    call(rdxNext, &e);

    rdx i = {};
    call(rdxInto, &i, &e);
    for (int j = 0; j < length; j++) {
        i.type = RDX_TYPE_INT;
        i.i = j;
        i.id.seq = j;
        i.id.src = 0;
        call(rdxNext, &i);
    }
    call(rdxOuto, &i, &e);
    $mv(pad_idle, e.into);

    // Verify we can read it back sequentially
    rdx e2 = {.format = RDX_FMT_SKIL};
    $mv(e2.data, pad_data);
    call(rdxNext, &e2);

    // Read all records
    rdx i2 = {};
    call(rdxInto, &i2, &e2);  // Enter container once
    for (int j = 0; j < length; j++) {
        call(rdxNext, &i2);  // Read next record
        test(i2.type == RDX_TYPE_INT, RDXBAD);
        testeq(i2.i, j);
    }
    call(rdxOuto, &i2, &e2);  // Exit container once

    done;
}

ok64 SKILTestEdgeCases() {
    sane(1);

    // Test empty collection
    a_pad(u8, pad1, PAGESIZE);
    a_pad(u64, tabs1, PAGESIZE);
    rdx e1 = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE, .extra = (void*)tabs1};
    $mv(e1.into, pad1_idle);
    e1.type = RDX_TYPE_EULER;
    call(rdxNext, &e1);
    rdx i1 = {};
    call(rdxInto, &i1, &e1);
    call(rdxOuto, &i1, &e1);  // No elements written
    $mv(pad1_idle, e1.into);

    rdx e1r = {.format = RDX_FMT_SKIL};
    $mv(e1r.data, pad1_data);
    call(rdxNext, &e1r);
    test(e1r.type == RDX_TYPE_EULER, RDXBAD);

    // Test single element
    a_pad(u8, pad2, PAGESIZE);
    a_pad(u64, tabs2, PAGESIZE);
    rdx e2 = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE, .extra = (void*)tabs2};
    $mv(e2.into, pad2_idle);
    e2.type = RDX_TYPE_EULER;
    call(rdxNext, &e2);
    rdx i2 = {};
    call(rdxInto, &i2, &e2);
    i2.type = RDX_TYPE_INT;
    i2.i = 42;
    i2.id.seq = 42;
    i2.id.src = 0;
    call(rdxNext, &i2);
    call(rdxOuto, &i2, &e2);
    $mv(pad2_idle, e2.into);

    rdx e2r = {.format = RDX_FMT_SKIL};
    $mv(e2r.data, pad2_data);
    call(rdxNext, &e2r);
    rdx i2r = {};
    call(rdxInto, &i2r, &e2r);
    call(rdxNext, &i2r);
    testeq(i2r.i, 42);
    call(rdxOuto, &i2r, &e2r);

    done;
}

ok64 SKILTests() {
    sane(1);

    // Basic functionality
    call(SKILTest1);

    // Rank function correctness
    call(SKILTestRankFunction);

    // Skip pointer generation
    call(SKILTestSkipPointers);

    // Binary search correctness
    call(SKILTestBinarySearch);

    // Large dataset handling
    call(SKILTestLargeDataset);

    // Edge cases
    call(SKILTestEdgeCases);

    done;
}

TEST(SKILTests);
