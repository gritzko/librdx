// Fuzz test: SLIK hierarchical seek with ascending runs
// Input: u64 values, broken into ascending runs, seek and verify
//
#include "abc/01.h"
#include "abc/NUM.h"
#include "abc/PRO.h"
#include "abc/TEST.h"
#include "rdx/RDX.h"
#include "rdx/test/NUMP.h"

// Find run start for index i (ascending run = each elem > prev)
fun u64 RunStart(i64cs data, u64 i) {
    u64 start = 0;
    for (u64 j = 1; j <= i; j++) {
        if (data[0][j] <= data[0][j - 1]) start = j;
    }
    return start;
}

// Find run end (exclusive) for index i
fun u64 RunEnd(i64cs data, u64 i) {
    u64 len = $len(data);
    for (u64 j = i + 1; j < len; j++) {
        if (data[0][j] <= data[0][j - 1]) return j;
    }
    return len;
}

FUZZ(i64, SLIK2fuzz) {
    sane(1);

    // Need at least one value
    if ($len(input) < 1) done;

    // Limit size
    if ($len(input) > 64) input[1] = input[0] + 64;
    i64cs data = {input[0], input[1]};

    a_pad(u8, pad, PAGESIZE * 16);
    a_pad0(u64, tabs, PAGESIZE * 4);

    // Write: break into ascending runs, each run is a bucket
    rdx root = {};
    rdxWriteInitSLIK(&root, pad, tabs);
    root.type = RDX_TYPE_EULER;
    ok64 o = rdxWriteNextSLIK(&root);
    must(o == OK, "write root");

    rdx bucket = {};
    o = rdxWriteIntoSLIK(&bucket, &root);
    must(o == OK, "into root");

    u64 i = 0;
    while (i < $len(data)) {
        u64 runstart = i;
        u64 runend = RunEnd(data, i);
        i64cs run = {data[0] + runstart, data[0] + runend};
        o = NUMPFeedBucket(&bucket, runstart, run);
        must(o == OK, "feed bucket");
        i = runend;
    }
    o = rdxWriteOutoSLIK(&bucket, &root);
    must(o == OK, "outo root");
    o = rdxWriteFinishSLIK(&root);
    must(o == OK, "finish root");

    // Read setup
    a_pad0(u64, readstack, 256);
    rdx r;
    rdxInitSLIK(&r, pad, readstack);
    o = rdxNextSLIK(&r);
    must(o == OK && r.type == RDX_TYPE_EULER, "read root");

    // Seek each index
    for (u64 idx = 0; idx < $len(data); idx++) {
        u64 runstart = RunStart(data, idx);
        u64 runend = RunEnd(data, idx);

        // Seek bucket by runstart index
        rdx rbucket = {};
        rbucket.type = RDX_TYPE_INT;
        rbucket.i = runstart;
        o = rdxIntoSLIK(&rbucket, &r);
        must(o == OK && rbucket.type == RDX_TYPE_TUPLE, "seek bucket");
        must(rbucket.id.seq == runstart, "bucket id");

        // Enter bucket: skip buckndx, enter EULER
        rdx inner = {};
        o = rdxIntoSLIK(&inner, &rbucket);
        must(o == OK, "into bucket");
        o = rdxNextSLIK(&inner);
        must(o == OK && inner.type == RDX_TYPE_INT, "buckndx");
        o = rdxNextSLIK(&inner);
        must(o == OK && inner.type == RDX_TYPE_EULER, "inner euler");

        // Seek to pair at idx
        rdx pair = {};
        o = NUMPSeekPair(&pair, &inner, data[0][idx]);
        must(o == OK, "seek pair");
        o = NUMPVerifyPair(&pair, data[0][idx]);
        must(o == OK, "verify pair");

        // Drain remaining pairs after idx
        for (u64 n = idx + 1; n < runend; n++) {
            o = NUMPDrainPair(&pair, data[0][n]);
            must(o == OK, "drain pair");
        }
        must(rdxNextSLIK(&pair) == END, "pair end");

        o = rdxOutoSLIK(&pair, &inner);
        must(o == OK, "outo pair");
        o = rdxOutoSLIK(&inner, &rbucket);
        must(o == OK, "outo inner");
        o = rdxOutoSLIK(&rbucket, &r);
        must(o == OK, "outo bucket");
    }

    done;
}
