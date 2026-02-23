//
// Fuzz test: A + B = B + A (merge commutativity)
// Merge order should not affect result
//
#include "rdx/RDX.h"

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/S.h"
#include "abc/TEST.h"

// Normalize TLV into buffer
fun ok64 ABnorm(u8b in, u8b out) {
    sane($ok(in) && $ok(out));
    rdx r;
    rdxInit(&r, RDX_FMT_TLV, in);
    rdx w;
    rdxInit(&w, RDX_FMT_TLV | RDX_FMT_WRITE, out);
    call(rdxConvert, &w, &r);
    done;
}

// Merge two TLVs into output buffer
fun ok64 ABmerge2(u8b a, u8b b, u8b out) {
    sane($ok(a) && $ok(b) && $ok(out));
    a_pad(rdx, inputs, 4096);
    rdxbZero(inputs);
    rdxp pa = 0, pb = 0;
    call(rdxbFedP, inputs, &pa);
    call(rdxbFedP, inputs, &pb);
    rdxInit(pa, RDX_FMT_TLV, a);
    rdxInit(pb, RDX_FMT_TLV, b);
    rdx w;
    rdxInit(&w, RDX_FMT_TLV | RDX_FMT_WRITE, out);
    call(rdxMerge, &w, rdxbDataIdle(inputs));
    done;
}

// Merge two TLVs via Y iterator + rdxCopy (must equal rdxMerge)
fun ok64 ABmergeY(u8b a, u8b b, u8b out) {
    sane($ok(a) && $ok(b) && $ok(out));
    a_pad(rdx, inputs, 4096);
    rdxbZero(inputs);
    rdxp pa = 0, pb = 0;
    call(rdxbFedP, inputs, &pa);
    call(rdxbFedP, inputs, &pb);
    rdxInit(pa, RDX_FMT_TLV, a);
    rdxInit(pb, RDX_FMT_TLV, b);
    rdx y = {.format = RDX_FMT_Y};
    rdxgMv(y.ins, rdxbDataIdle(inputs));
    rdx w;
    rdxInit(&w, RDX_FMT_TLV | RDX_FMT_WRITE, out);
    call(rdxCopy, &w, &y);
    done;
}

// Dump TLV as hex and JDR for debugging
fun ok64 ABdump(u8cs label, u8b tlv) {
    sane($ok(tlv));
    $println(label);
    a_pad(u8, hex, 4096);
    HEXPut(hex_idle, u8bDataC(tlv));
    u8sJoin(hex_idle, hex);
    $println(hex_datac);
    a_pad(u8, jdr, PAGESIZE);
    rdx r;
    rdxInit(&r, RDX_FMT_TLV, tlv);
    rdx jw;
    rdxInit(&jw, RDX_FMT_JDR | RDX_FMT_WRITE, jdr);
    ok64 o = rdxCopy(&jw, &r);
    if (o == OK) {
        $println(jdr_datac);
    }
    done;
}

FUZZ(u8, ABfuzz) {
    sane(1);
    if ($len(input) < 4 || $len(input) > PAGESIZE) done;

    u8b inp = {(u8p)*input, (u8p)*input, (u8p)input[1], (u8p)input[1]};

    rdx vfy;
    rdxInit(&vfy, RDX_FMT_TLV, inp);
    if (rdxVerifyAll(&vfy) != OK) return 0;
//fprintf(stderr, "verified\n");

    rdx probe;
    rdxInit(&probe, RDX_FMT_TLV, inp);
    if (rdxNext(&probe) != OK) return 0;
    u8p m = (u8p)probe.next;
    if (rdxNext(&probe) != OK) return 0;
    u8p e = (u8p)probe.next;
//fprintf(stderr, "split\n");

    // Imitate buffers from input slices
    u8b bufA = {(u8p)input[0], (u8p)input[0], m, m};
    u8b bufB = {m, m, e, e};

    // Normalize both
    a_pad(u8, normA, PAGESIZE);
    a_pad(u8, normB, PAGESIZE);
    if (ABnorm(bufA, normA) != OK) done;
    if (ABnorm(bufB, normB) != OK) done;
//fprintf(stderr, "norm\n");

    // Merge A+B and B+A
    a_pad(u8, outAB, PAGESIZE);
    a_pad(u8, outBA, PAGESIZE);
    if (ABmerge2(normA, normB, outAB) != OK) done;
    if (ABmerge2(normB, normA, outBA) != OK) done;
//fprintf(stderr, "merge\n");

    // A + B must equal B + A
    if (!$eq(u8bDataC(outAB), u8bDataC(outBA))) {
        a_cstr(a_label, "A (normalized):");
        a_cstr(b_label, "B (normalized):");
        a_cstr(ab_label, "A + B:");
        a_cstr(ba_label, "B + A:");
        ABdump(a_label, normA);
        ABdump(b_label, normB);
        ABdump(ab_label, outAB);
        ABdump(ba_label, outBA);
        must(0, "A+B != B+A");
    }

    // Y iterator merge must equal rdxMerge
    a_pad(u8, outYAB, PAGESIZE);
    a_pad(u8, outYBA, PAGESIZE);
    if (ABmergeY(normA, normB, outYAB) != OK) done;
    if (ABmergeY(normB, normA, outYBA) != OK) done;
//fprintf(stderr, "merge2\n");

    if (!$eq(u8bDataC(outAB), u8bDataC(outYAB))) {
        a_cstr(a_label, "A (normalized):");
        a_cstr(b_label, "B (normalized):");
        a_cstr(ab_label, "rdxMerge(A+B):");
        a_cstr(yab_label, "Y copy(A+B):");
        ABdump(a_label, normA);
        ABdump(b_label, normB);
        ABdump(ab_label, outAB);
        ABdump(yab_label, outYAB);
        must(0, "rdxMerge != Y copy");
    }

    if (!$eq(u8bDataC(outBA), u8bDataC(outYBA))) {
        a_cstr(a_label, "A (normalized):");
        a_cstr(b_label, "B (normalized):");
        a_cstr(ba_label, "rdxMerge(B+A):");
        a_cstr(yba_label, "Y copy(B+A):");
        ABdump(a_label, normA);
        ABdump(b_label, normB);
        ABdump(ba_label, outBA);
        ABdump(yba_label, outYBA);
        must(0, "rdxMerge != Y copy (reversed)");
    }
//fprintf(stderr, "happy\n");

    done;
}
