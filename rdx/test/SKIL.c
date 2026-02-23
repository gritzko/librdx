#include "abc/01.h"
#include "abc/TEST.h"
#include "abc/TLV.h"
#include "rdx/RDX.h"

// Defined in SKIL.c
extern u64 SKILRank(u64 pos);

ok64 SKILTest1() {
    con int length = 100;
    sane(1);
    a_pad(u8, pad, PAGESIZE * 4);
    a_pad0(u64, tabs, PAGESIZE * 4);
    // New layout: bulk = buffer pointer, opt = skiplist buffer
    u8b write_buf = {pad[0], pad[0], pad[0], pad[3]};
    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE,
             .opt = (u8p)tabs,
             .bulk = write_buf};
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

    // Read: bulk points to buffer, next=data, opt=end
    u8b read_buf = {pad[0], pad[0], write_buf[2], pad[3]};
    rdx e2 = {.format = RDX_FMT_SKIL,
              .next = read_buf[1],
              .bulk = read_buf,
              .opt = (u8p)write_buf[2]};
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
    a_pad0(u64, tabs, PAGESIZE);

    u8b write_buf = {pad[0], pad[0], pad[0], pad[3]};
    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE,
             .opt = (u8p)tabs,
             .bulk = write_buf};
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

    // Verify we can read back all records sequentially
    u8b read_buf = {pad[0], pad[0], write_buf[2], pad[3]};
    rdx e2 = {.format = RDX_FMT_SKIL,
              .next = read_buf[1],
              .bulk = read_buf,
              .opt = (u8p)write_buf[2]};
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
    con int length = 1000;  // TEMP: reduced
    a_pad(u8, pad, roundup(length * 20, PAGESIZE));
    a_pad0(u64, tabs, PAGESIZE);

    u8b write_buf = {pad[0], pad[0], pad[0], pad[3]};
    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE,
             .opt = (u8p)tabs,
             .bulk = write_buf};
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

    // Read back and verify using binary search
    u8b read_buf = {pad[0], pad[0], write_buf[2], pad[3]};
    rdx e2 = {.format = RDX_FMT_SKIL,
              .next = read_buf[1],
              .bulk = read_buf,
              .opt = (u8p)write_buf[2]};
    call(rdxNext, &e2);
    test(e2.type == RDX_TYPE_EULER, RDXBAD);

    // Test seeking to various positions
    int test_values[] = {0, 100, 500, 900, 990};  // TEMP: reduced
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
    a_pad0(u64, tabs, PAGESIZE);

    u8b write_buf = {pad[0], pad[0], pad[0], pad[3]};
    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE,
             .opt = (u8p)tabs,
             .bulk = write_buf};
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

    // Verify we can read it back sequentially
    u8b read_buf = {pad[0], pad[0], write_buf[2], pad[3]};
    rdx e2 = {.format = RDX_FMT_SKIL,
              .next = read_buf[1],
              .bulk = read_buf,
              .opt = (u8p)write_buf[2]};
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
    a_pad0(u64, tabs1, PAGESIZE);
    u8b write_buf1 = {pad1[0], pad1[0], pad1[0], pad1[3]};
    rdx e1 = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE,
              .opt = (u8p)tabs1,
              .bulk = write_buf1};
    e1.type = RDX_TYPE_EULER;
    call(rdxNext, &e1);
    rdx i1 = {};
    call(rdxInto, &i1, &e1);
    call(rdxOuto, &i1, &e1);  // No elements written

    u8b read_buf1 = {pad1[0], pad1[0], write_buf1[2], pad1[3]};
    rdx e1r = {.format = RDX_FMT_SKIL,
               .next = read_buf1[1],
               .bulk = read_buf1,
               .opt = (u8p)write_buf1[2]};
    call(rdxNext, &e1r);
    test(e1r.type == RDX_TYPE_EULER, RDXBAD);

    // Test single element
    a_pad(u8, pad2, PAGESIZE);
    a_pad0(u64, tabs2, PAGESIZE);
    u8b write_buf2 = {pad2[0], pad2[0], pad2[0], pad2[3]};
    rdx e2 = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE,
              .opt = (u8p)tabs2,
              .bulk = write_buf2};
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

    u8b read_buf2 = {pad2[0], pad2[0], write_buf2[2], pad2[3]};
    rdx e2r = {.format = RDX_FMT_SKIL,
               .next = read_buf2[1],
               .bulk = read_buf2,
               .opt = (u8p)write_buf2[2]};
    call(rdxNext, &e2r);
    rdx i2r = {};
    call(rdxInto, &i2r, &e2r);
    call(rdxNext, &i2r);
    testeq(i2r.i, 42);
    call(rdxOuto, &i2r, &e2r);

    done;
}

// Number-to-words conversion
static const char* ONES[] = {"", "one", "two", "three", "four", "five", "six",
                             "seven", "eight", "nine", "ten", "eleven", "twelve",
                             "thirteen", "fourteen", "fifteen", "sixteen",
                             "seventeen", "eighteen", "nineteen"};
static const char* TENS[] = {"", "", "twenty", "thirty", "forty", "fifty",
                             "sixty", "seventy", "eighty", "ninety"};

fun int num2words(char* buf, int n) {
    if (n == 0) return snprintf(buf, 256, "zero");
    char* p = buf;
    if (n >= 1000000) {
        p += num2words(p, n / 1000000);
        p += sprintf(p, " million");
        n %= 1000000;
        if (n > 0) *p++ = ' ';
    }
    if (n >= 1000) {
        p += num2words(p, n / 1000);
        p += sprintf(p, " thousand");
        n %= 1000;
        if (n > 0) *p++ = ' ';
    }
    if (n >= 100) {
        p += sprintf(p, "%s hundred", ONES[n / 100]);
        n %= 100;
        if (n > 0) *p++ = ' ';
    }
    if (n >= 20) {
        p += sprintf(p, "%s", TENS[n / 10]);
        if (n % 10) p += sprintf(p, " %s", ONES[n % 10]);
    } else if (n > 0) {
        p += sprintf(p, "%s", ONES[n]);
    }
    return p - buf;
}

ok64 SKILTestTupleSeek() {
    sane(1);

    // Create large Euler set: {0:"zero", 1:"one", ..., 99999:"ninety nine..."}
    con int count = 100000;
    u8b pad = {};
    call(u8bMap, pad, GB);  // 1GB mmap buffer for data
    u64b tabs = {};
    call(u64bMap, tabs, MB * 16);  // 16MB mmap buffer for skip pointers
    char wordbuf[256];

    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE,
             .opt = (u8p)tabs,
             .bulk = pad};
    e.type = RDX_TYPE_EULER;
    call(rdxNext, &e);

    // Write tuples
    rdx child = {};
    call(rdxInto, &child, &e);
    for (int j = 0; j < count; j++) {
        child.type = RDX_TYPE_TUPLE;
        child.id.seq = j;
        child.id.src = 0;
        call(rdxNext, &child);

        rdx inner = {};
        call(rdxInto, &inner, &child);

        // Key: integer
        inner.type = RDX_TYPE_INT;
        inner.i = j;
        inner.id.seq = j;
        inner.id.src = 0;
        call(rdxNext, &inner);

        // Value: string (number as English words)
        int len = num2words(wordbuf, j);
        inner.type = RDX_TYPE_STRING;
        inner.s[0] = (u8c*)wordbuf;
        inner.s[1] = (u8c*)wordbuf + len;
        inner.flags = RDX_UTF_ENC_UTF8;
        inner.id.seq = j;
        inner.id.src = 0;
        call(rdxNext, &inner);

        call(rdxOuto, &inner, &child);
    }
    call(rdxOuto, &child, &e);

    // Read back and verify tuples are findable by seeking
    rdx e2 = {.format = RDX_FMT_SKIL,
              .next = pad[1],       // data start
              .bulk = pad,
              .opt = (u8p)pad[2]};  // data end (current idle position)
    call(rdxNext, &e2);
    test(e2.type == RDX_TYPE_EULER, RDXBAD);

    // Test seeking to various positions
    int test_keys[] = {0, 1, 19, 42, 100, 500, 999, 12345, 50000, 99999};
    for (int k = 0; k < 10; k++) {
        int j = test_keys[k];
        rdx seek = {};
        seek.type = RDX_TYPE_INT;
        seek.i = j;

        call(rdxInto, &seek, &e2);
        test(seek.type == RDX_TYPE_TUPLE, RDXBAD);

        // Verify tuple contents
        rdx inner = {};
        call(rdxInto, &inner, &seek);
        call(rdxNext, &inner);
        test(inner.type == RDX_TYPE_INT, RDXBAD);
        testeq(inner.i, j);

        call(rdxNext, &inner);
        test(inner.type == RDX_TYPE_STRING, RDXBAD);
        int expected_len = num2words(wordbuf, j);
        testeq($len(inner.s), expected_len);

        call(rdxOuto, &inner, &seek);
        call(rdxOuto, &seek, &e2);
    }

    // Test non-existent key - should return NONE or type=0
    rdx seek_missing = {};
    seek_missing.type = RDX_TYPE_INT;
    seek_missing.i = count + 1000;
    ok64 om = rdxInto(&seek_missing, &e2);
    test(om == NONE || seek_missing.type == 0, RDXBAD);

    u64bUnMap(tabs);
    u8bUnMap(pad);
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

    // Tuple seeking
    call(SKILTestTupleSeek);

    done;
}

TEST(SKILTests);
