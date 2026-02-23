// NUMP: NUM Pair utilities for SLIK tests
//
// Pair structure (JDR):
//   (n, "n in words")
//
// Bucket structure (JDR):
//   (buckndx, {(n, "words"), (n, "words"), ...})
//
// Full test structure (JDR):
//   {
//     (0, {(5, "five"), (6, "six"), (7, "seven")}),
//     (3, {(1, "one"), (2, "two")}),
//     ...
//   }
//
#ifndef RDX_TEST_NUMP_H
#define RDX_TEST_NUMP_H

#include "abc/NUM.h"
#include "abc/PRO.h"
#include "rdx/RDX.h"

// Feed a pair: TUPLE { n, "n in words" }
fun ok64 NUMPFeedPair(rdxp parent, i64 n) {
    sane(parent);
    a_pad(u8, wordbuf, 256);

    parent->type = RDX_TYPE_TUPLE;
    parent->id.seq = n;
    call(rdxWriteNextSLIK, parent);

    rdx c = {};
    call(rdxWriteIntoSLIK, &c, parent);

    // Key: INT = n
    c.type = RDX_TYPE_INT;
    c.i = n;
    c.id.seq = 0;
    call(rdxWriteNextSLIK, &c);

    // Value: STRING = "n in words"
    call(NUMu8sFeed, u8bIdle(wordbuf), n);
    c.type = RDX_TYPE_STRING;
    c.s[0] = (u8c*)wordbuf[1];
    c.s[1] = (u8c*)wordbuf[2];
    c.flags = RDX_UTF_ENC_UTF8;
    c.id.seq = 1;
    call(rdxWriteNextSLIK, &c);

    call(rdxWriteOutoSLIK, &c, parent);
    done;
}

// Feed pairs: sequence of TUPLE { n, "words" } for each n in nums
fun ok64 NUMPFeedPairs(rdxp parent, i64cs nums) {
    sane(parent);

    $for(i64c, n, nums) {
        call(NUMPFeedPair, parent, *n);
    }

    done;
}

// Feed bucket: TUPLE { buckndx, EULER { pairs... } }
fun ok64 NUMPFeedBucket(rdxp parent, i64 buckndx, i64cs nums) {
    sane(parent);

    // Outer tuple
    parent->type = RDX_TYPE_TUPLE;
    parent->id.seq = buckndx;
    call(rdxWriteNextSLIK, parent);

    rdx c = {};
    call(rdxWriteIntoSLIK, &c, parent);

    // First element: bucket index (key for EULER sorting)
    c.type = RDX_TYPE_INT;
    c.i = buckndx;
    c.id.seq = 0;
    call(rdxWriteNextSLIK, &c);

    // Second element: EULER containing pairs
    c.type = RDX_TYPE_EULER;
    c.id.seq = 1;
    call(rdxWriteNextSLIK, &c);

    rdx p = {};
    call(rdxWriteIntoSLIK, &p, &c);
    call(NUMPFeedPairs, &p, nums);
    call(rdxWriteOutoSLIK, &p, &c);

    call(rdxWriteOutoSLIK, &c, parent);
    done;
}

// Drain a pair: expects TUPLE { n, "words" }
fun ok64 NUMPDrainPair(rdxp parent, i64 n) {
    sane(parent);
    a_pad(u8, wordbuf, 256);

    call(rdxNextSLIK, parent);
    test(parent->type == RDX_TYPE_TUPLE, RDXBAD);

    rdx c = {};
    call(rdxIntoSLIK, &c, parent);

    // Key: INT = n
    call(rdxNextSLIK, &c);
    test(c.type == RDX_TYPE_INT, RDXBAD);
    test(c.i == n, RDXBAD);

    // Value: STRING
    call(rdxNextSLIK, &c);
    test(c.type == RDX_TYPE_STRING, RDXBAD);
    call(NUMu8sFeed, u8bIdle(wordbuf), n);
    test($eq(u8bData(wordbuf), c.s), RDXBAD);

    test(rdxNextSLIK(&c) == END, RDXBAD);
    call(rdxOutoSLIK, &c, parent);
    done;
}

// Drain pairs: sequence of pairs
fun ok64 NUMPDrainPairs(rdxp parent, i64cs nums) {
    sane(parent);

    $for(i64c, n, nums) {
        call(NUMPDrainPair, parent, *n);
    }

    done;
}

// Seek pair: use rdxIntoSLIK to seek TUPLE by first child value
// After return, target is AT the found TUPLE
fun ok64 NUMPSeekPair(rdxp target, rdxp parent, i64 n) {
    sane(target && parent);

    // Seek by first child value (INT = n)
    target->type = RDX_TYPE_INT;
    target->i = n;
    ok64 o = rdxIntoSLIK(target, parent);
    if (o != OK) {
        fprintf(stderr, "NUMPSeekPair(%ld): rdxIntoSLIK failed: %s\n", n, ok64str(o));
        fail(o);
    }
    if (target->type != RDX_TYPE_TUPLE) {
        fprintf(stderr, "NUMPSeekPair(%ld): expected TUPLE, got type=%u\n", n, target->type);
        fail(RDXBAD);
    }
    done;
}

// Verify pair contents at current position (target AT a TUPLE)
fun ok64 NUMPVerifyPair(rdxp target, i64 n) {
    sane(target && target->type == RDX_TYPE_TUPLE);
    a_pad(u8, wordbuf, 256);

    rdx c = {};
    call(rdxIntoSLIK, &c, target);

    // Key: INT = n
    ok64 next_o = rdxNextSLIK(&c);
    if (next_o != OK) {
        fprintf(stderr, "NUMPVerifyPair(%ld): rdxNextSLIK failed: %s\n", n, ok64str(next_o));
        fail(RDXBAD);
    }
    if (c.type != RDX_TYPE_INT) {
        fprintf(stderr, "NUMPVerifyPair(%ld): expected INT, got type=%u\n", n, c.type);
        fail(RDXBAD);
    }
    if (c.i != n) {
        fprintf(stderr, "NUMPVerifyPair(%ld): expected i=%ld, got i=%ld\n", n, n, c.i);
        fail(RDXBAD);
    }

    // Value: STRING
    call(rdxNextSLIK, &c);
    test(c.type == RDX_TYPE_STRING, RDXBAD);
    call(NUMu8sFeed, u8bIdle(wordbuf), n);
    test($eq(u8bData(wordbuf), c.s), RDXBAD);

    test(rdxNextSLIK(&c) == END, RDXBAD);
    call(rdxOutoSLIK, &c, target);
    done;
}

// Drain bucket: expects TUPLE { buckndx, EULER { pairs... } }
fun ok64 NUMPDrainBucket(rdxp parent, i64 buckndx, i64cs nums) {
    sane(parent);

    call(rdxNextSLIK, parent);
    test(parent->type == RDX_TYPE_TUPLE, RDXBAD);
    test((i64)parent->id.seq == buckndx, RDXBAD);

    rdx c = {};
    call(rdxIntoSLIK, &c, parent);

    // First element: bucket index
    call(rdxNextSLIK, &c);
    test(c.type == RDX_TYPE_INT, RDXBAD);
    test(c.i == buckndx, RDXBAD);

    // Second element: EULER with pairs
    call(rdxNextSLIK, &c);
    test(c.type == RDX_TYPE_EULER, RDXBAD);

    rdx p = {};
    call(rdxIntoSLIK, &p, &c);
    call(NUMPDrainPairs, &p, nums);
    test(rdxNextSLIK(&p) == END, RDXBAD);
    call(rdxOutoSLIK, &p, &c);

    test(rdxNextSLIK(&c) == END, RDXBAD);
    call(rdxOutoSLIK, &c, parent);
    done;
}

#endif
