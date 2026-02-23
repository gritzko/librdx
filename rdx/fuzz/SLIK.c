// Fuzz test: SLIK skiplist roundtrip
// Input: i64 values, sorted and stored as {n: "n in words"}, then seek each
//
#include <stdlib.h>
#include <strings.h>

#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/NUM.h"
#include "abc/PRO.h"
#include "abc/TEST.h"
#include "rdx/RDX.h"

static int SLIKi64cmp(const void* a, const void* b) {
    i64 va = *(const i64*)a;
    i64 vb = *(const i64*)b;
    return (va > vb) - (va < vb);
}

FUZZ(i64, SLIKfuzz) {
    sane(1);

    size_t count = $len(input);
    if (count == 0) done;
    if (count > 64) count = 64;

    // Copy to local buffer for sort/dedup
    a_pad(i64, keys, 64);
    for (size_t i = 0; i < count; i++) {
        i64bFeed1(keys, input[0][i]);
    }

    // Sort and deduplicate
    qsort(_keys, count, sizeof(i64), SLIKi64cmp);
    size_t dedup = 1;
    for (size_t i = 1; i < count; i++) {
        if (_keys[i] != _keys[dedup - 1]) {
            _keys[dedup++] = _keys[i];
        }
    }

    // Build SLIK structure: EULER { key: "key in words" }
    a_pad0(u64, tabs, PAGESIZE);
    a_pad(u8, data, PAGESIZE * 16);

    rdx e;
    rdxWriteInitSLIK(&e, data, tabs);
    e.type = RDX_TYPE_EULER;
    ok64 o = rdxWriteNextSLIK(&e);
    must(o == OK, "w1");

    rdx child = {};
    o = rdxWriteIntoSLIK(&child, &e);
    must(o == OK, "w2");

    a_pad(u8, wordbuf, 512);
    for (size_t i = 0; i < dedup; i++) {
        i64 key = _keys[i];

        // Write tuple (key, "words")
        child.type = RDX_TYPE_TUPLE;
        child.id.seq = key;
        child.id.src = 0;
        o = rdxWriteNextSLIK(&child);
        must(o == OK, "w3");

        rdx inner = {};
        o = rdxWriteIntoSLIK(&inner, &child);
        must(o == OK, "w4");

        // Key: integer
        inner.type = RDX_TYPE_INT;
        inner.i = key;
        inner.id.seq = 0;
        inner.id.src = 0;
        o = rdxWriteNextSLIK(&inner);
        must(o == OK, "w5");

        // Value: string
        u8bReset(wordbuf);
        o = NUMu8sFeed(wordbuf_idle, key);
        must(o == OK, "w6");
        inner.type = RDX_TYPE_STRING;
        u8csMv(inner.s, u8bDataC(wordbuf));
        inner.flags = RDX_UTF_ENC_UTF8;
        inner.id.seq = key;
        inner.id.src = 0;
        o = rdxWriteNextSLIK(&inner);
        must(o == OK, "w7");

        o = rdxWriteOutoSLIK(&inner, &child);
        must(o == OK, "w8");
    }

    o = rdxWriteOutoSLIK(&child, &e);
    must(o == OK, "w9");

    o = rdxWriteFinishSLIK(&e);
    must(o == OK, "w10");

    // Read back and verify: seek each key
    rdx e2;
    rdxInitSLIK(&e2, data, tabs);

    o = rdxNextSLIK(&e2);
    must(o == OK, "r1");
    must(e2.type == RDX_TYPE_EULER, "r2");

    for (size_t i = 0; i < dedup; i++) {
        i64 key = _keys[i];

        rdx seek = {};
        seek.type = RDX_TYPE_INT;
        seek.i = key;

        o = rdxIntoSLIK(&seek, &e2);
        must(o == OK, "seek");
        must(seek.type == RDX_TYPE_TUPLE, "r3");

        // Verify tuple contents
        rdx inner = {};
        o = rdxIntoSLIK(&inner, &seek);
        must(o == OK, "r4");
        o = rdxNextSLIK(&inner);
        must(o == OK && inner.type == RDX_TYPE_INT, "r5");
        must(inner.i == key, "r6");

        o = rdxNextSLIK(&inner);
        must(o == OK && inner.type == RDX_TYPE_STRING, "r7");

        // Verify string matches expected
        u8bReset(wordbuf);
        NUMu8sFeed(wordbuf_idle, key);
        must($eq(u8bData(wordbuf), inner.s), "r8");

        o = rdxOutoSLIK(&inner, &seek);
        must(o == OK, "r9");
        o = rdxOutoSLIK(&seek, &e2);
        must(o == OK, "r10");
    }

    // Also scan all entries sequentially
    rdx e3;
    rdxInitSLIK(&e3, data, tabs);

    o = rdxNextSLIK(&e3);
    must(o == OK, "s1");

    rdx scan = {};
    o = rdxIntoSLIK(&scan, &e3);
    must(o == OK, "s2");

    size_t scanned = 0;
    while ((o = rdxNextSLIK(&scan)) == OK) {
        must(scan.type == RDX_TYPE_TUPLE, "s3");
        scanned++;
    }
    must(scanned == dedup, "s4");

    done;
}
