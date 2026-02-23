//
// Fuzz test: A + B = B + A (merge commutativity)
// Merge order should not affect result
//
#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/S.h"
#include "abc/TEST.h"
#include "rdx/RDX.h"

// Parse and verify one TLV from input, return length (0 on failure)
fun u64 ABparseTLV(u8cs input) {
    if ($len(input) < 2) return 0;
    rdx probe = {.format = RDX_FMT_TLV};
    probe.next = input[0]; probe.opt = (u8p)input[1];
    if (rdxNext(&probe) != OK) return 0;
    u64 len = probe.next - input[0];
    if (len == 0 || len > $len(input)) return 0;
    a_head(u8c, tlv, input, len);
    rdx vfy = {.format = RDX_FMT_TLV};
    vfy.next = tlv[0]; vfy.opt = (u8p)tlv[1];
    if (rdxVerifyAll(&vfy) != OK) return 0;
    return len;
}

// Normalize TLV into buffer
fun ok64 ABnorm(u8cs in, u8s buf) {
    sane($ok(in) && $ok(buf));
    rdx r = {.format = RDX_FMT_JDR};
    r.next = in[0]; r.opt = (u8p)in[1];
    rdx w = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
    w.bulk = (u8bp)buf[0]; w.opt = buf[1];
    call(rdxConvert, &w, &r);
    buf[0] = (u8p)w.bulk;
    done;
}

// Merge two TLVs into output buffer
fun ok64 ABmerge2(u8cs a, u8cs b, u8s out) {
    sane($ok(a) && $ok(b) && $ok(out));
    a_pad(rdx, inputs, 4096);
    rdxbZero(inputs);
    rdxp pa = 0, pb = 0;
    call(rdxbFedP, inputs, &pa);
    call(rdxbFedP, inputs, &pb);
    pa->format = pb->format = RDX_FMT_TLV;
    pa->next = a[0]; pa->opt = (u8p)a[1];
    pb->next = b[0]; pb->opt = (u8p)b[1];
    rdx w = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
    w.bulk = (u8bp)out[0]; w.opt = out[1];
    call(rdxMerge, &w, rdxbDataIdle(inputs));
    out[0] = (u8p)w.bulk;
    done;
}

// Dump TLV as hex and JDR for debugging
fun ok64 ABdump(u8cs label, u8cs tlv) {
    sane($ok(tlv));
    $println(label);
    a_pad(u8, hex, 4096);
    HEXPut(hex_idle, tlv);
    u8sJoin(hex_idle, hex);
    $println(hex_datac);
    a_pad(u8, jdr, PAGESIZE);
    rdx r = {.format = RDX_FMT_TLV};
    r.next = tlv[0]; r.opt = (u8p)tlv[1];
    rdx jw = {};
    rdxWriteInit(&jw, RDX_FMT_JDR, jdr);
    ok64 o = rdxCopy(&jw, &r);
    if (o == OK) {
        $println(jdr_datac);
    }
    done;
}

const char* AB_CASES[][2] = {
    {"()", "(0-0,0-0)"},
    // MULTIX cases
    {"<<>>", "<>"},
    {"<\"\">", "<>"},
    {0, 0},
};

ok64 ABtest(u8cs a, u8cs b) {
    sane(u8csOK(a) && u8csOK(b));

    // Normalize both
    a_pad(u8, normA, PAGESIZE);
    a_pad(u8, normB, PAGESIZE);
    if (ABnorm(a, normA_idle) != OK) done;
    if (ABnorm(b, normB_idle) != OK) done;

    // Merge A+B and B+A
    a_pad(u8, outAB, PAGESIZE);
    a_pad(u8, outBA, PAGESIZE);
    if (ABmerge2(normA_datac, normB_datac, outAB_idle) != OK) done;
    if (ABmerge2(normB_datac, normA_datac, outBA_idle) != OK) done;

    // A + B must equal B + A
    if (!$eq(outAB_datac, outBA_datac)) {
        a_cstr(a_label, "A (normalized):");
        a_cstr(b_label, "B (normalized):");
        a_cstr(ab_label, "A + B:");
        a_cstr(ba_label, "B + A:");
        ABdump(a_label, normA_datac);
        ABdump(b_label, normB_datac);
        ABdump(ab_label, outAB_datac);
        ABdump(ba_label, outBA_datac);
        assert(0 && "A+B != B+A");
    }

    done;
}

ok64 ABTestLoop() {
    sane(1);
    int i = 0;
    while (AB_CASES[i][0]) {
        a_cstr(a, AB_CASES[i][0]);
        a_cstr(b, AB_CASES[i][1]);
        call(ABtest, a, b);
        ++i;
    }
    done;
}

TEST(ABTestLoop);
