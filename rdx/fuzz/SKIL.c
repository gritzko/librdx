// Fuzz test: SKIL skiplist roundtrip
// Input: u64 values, sorted and stored as {n: "n in words"}, then seek each
//
#include "abc/NUM.h"
#include "abc/PRO.h"
#include "abc/TEST.h"
#include "rdx/RDX.h"

#include <stdlib.h>

static int SKILi64cmp(const void* a, const void* b) {
    i64 va = *(const i64*)a;
    i64 vb = *(const i64*)b;
    return (va > vb) - (va < vb);
}

// Static buffers to avoid malloc/free syscalls per iteration
#define DATA_SIZE (256 * 1024)
#define TABS_SIZE (DATA_SIZE / 8)
static u8 databuf[DATA_SIZE];
static u64 tabsbuf[TABS_SIZE];

FUZZ(u8, SKILfuzz) {
    sane(1);

    // Need at least one u64
    if ($len(input) < 8) done;

    // Extract u64 values from input (up to 64 values)
    size_t count = $len(input) / 8;
    if (count > 64) count = 64;
    if (count == 0) done;

    a_pad(u64, keys, 64);
    for (size_t i = 0; i < count; i++) {
        u64 v = 0;
        for (int j = 0; j < 8; j++) {
            v |= ((u64)input[0][i * 8 + j]) << (j * 8);
        }
        u64bFeed1(keys, v);
    }

    // Sort and deduplicate (as i64 to match EULER comparison)
    qsort(_keys, count, sizeof(i64), SKILi64cmp);
    size_t dedup = 1;
    for (size_t i = 1; i < count; i++) {
        if (_keys[i] != _keys[dedup - 1]) {
            _keys[dedup++] = _keys[i];
        }
    }
    // dedup now has the count of unique keys in _keys[0..dedup-1]

    // Build SKIL structure: EULER { key: "key in words" }
    // Use static buffers to avoid syscalls
    u8b data = {databuf, databuf, databuf, databuf + DATA_SIZE};
    u64b tabs = {tabsbuf, tabsbuf, tabsbuf, tabsbuf + TABS_SIZE};

    rdx e = {};
    rdxWriteInit(&e, RDX_FMT_SKIL, data);
    e.opt = (u8p)tabs;
 
    e.type = RDX_TYPE_EULER;
    ok64 o = rdxNext(&e);
    if (o != OK) done;

    rdx child = {};
    o = rdxInto(&child, &e);
    if (o != OK) done;

    a_pad(u8, wordbuf, 256);
    for (size_t i = 0; i < dedup; i++) {
        u64 key = _keys[i];

        // Write tuple (key, "words")
        child.type = RDX_TYPE_TUPLE;
        child.id.seq = key;
        child.id.src = 0;
        o = rdxNext(&child);
        if (o != OK) done;

        rdx inner = {};
        o = rdxInto(&inner, &child);
        if (o != OK) done;

        // Key: integer
        inner.type = RDX_TYPE_INT;
        inner.i = key;
        inner.id.seq = key;
        inner.id.src = 0;
        o = rdxNext(&inner);
        if (o != OK) done;

        // Value: string
        u8bReset(wordbuf);
        o = NUMu8sFeed(wordbuf_idle, key);
        if (o != OK) done;
        inner.type = RDX_TYPE_STRING;
        inner.s[0] = (u8c*)_wordbuf;
        inner.s[1] = (u8c*)wordbuf[2];
        inner.flags = RDX_UTF_ENC_UTF8;
        inner.id.seq = key;
        inner.id.src = 0;
        o = rdxNext(&inner);
        if (o != OK) done;

        o = rdxOuto(&inner, &child);
        if (o != OK) done;
    }

    o = rdxOuto(&child, &e);
    if (o != OK) done;

    // Read back and verify: seek each key
    rdx e2;
    rdxInit(&e2, RDX_FMT_SKIL, data);
    o = rdxNext(&e2);
    must(o == OK, "Failed to read EULER");
    must(e2.type == RDX_TYPE_EULER, "Not an EULER");

    for (size_t i = 0; i < dedup; i++) {
        u64 key = _keys[i];

        rdx seek = {};
        seek.type = RDX_TYPE_INT;
        seek.i = key;

        o = rdxInto(&seek, &e2);
        must(o == OK, "Seek failed");
        must(seek.type == RDX_TYPE_TUPLE, "Expected TUPLE");

        // Verify tuple contents
        rdx inner = {};
        o = rdxInto(&inner, &seek);
        must(o == OK, "rdxInto inner failed");
        o = rdxNext(&inner);
        must(o == OK && inner.type == RDX_TYPE_INT, "int not found");
        must(inner.i == (i64)key, "Key mismatch");

        o = rdxNext(&inner);
        must(o == OK && inner.type == RDX_TYPE_STRING, "string not found");

        // Verify string matches expected
        u8bReset(wordbuf);
        NUMu8sFeed(wordbuf_idle, key);
        must(u8bDataLen(wordbuf) == $len(inner.s), "String length mismatch");

        o = rdxOuto(&inner, &seek);
        must(o == OK, "rdxOuto inner failed");
        o = rdxOuto(&seek, &e2);
        must(o == OK, "rdxOuto seek failed");
    }

    // Also scan all entries sequentially
    rdx e3;
    rdxInit(&e3, RDX_FMT_SKIL, data);
    o = rdxNext(&e3);
    must(o == OK, "rdxNext e3 failed");

    rdx scan = {};
    o = rdxInto(&scan, &e3);
    must(o == OK, "rdxInto scan failed");

    size_t scanned = 0;
    while ((o = rdxNext(&scan)) == OK) {
        must(scan.type == RDX_TYPE_TUPLE, "expected TUPLE in scan");
        scanned++;
    }
    must(scanned == dedup, "Scan count mismatch");

    done;
}
