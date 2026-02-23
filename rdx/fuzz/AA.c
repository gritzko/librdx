//
// Fuzz test: A + A = A (merge idempotency)
// Any valid TLV merged with itself should produce identical output
//
#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/S.h"
#include "abc/TEST.h"
#include "rdx/RDX.h"

fun ok64 AAdump(u8cs label, u8cs tlv) {
    sane($ok(tlv));
    // Print label and hex
    $println(label);
    a_pad(u8, hex, 4096);
    HEXPut(hex_idle, tlv);
    u8sJoin(hex_idle, hex);
    $println(hex_datac);

    // Print JDR
    a_pad(u8, jdr, PAGESIZE);
    rdx r = {.format = RDX_FMT_TLV};
    r.next = (u8p)tlv[0]; r.opt = (u8p)tlv[1];
    rdx jw = {};
    rdxWriteInit(&jw, RDX_FMT_JDR, jdr);
    ok64 o = rdxCopy(&jw, &r);
    if (o == OK) {
        $println(jdr_datac);
    }
    done;
}

FUZZ(u8, AAfuzz) {
    sane(1);

    if ($len(input) == 0) done;

    u8p bufb = (u8p)input[0];
    u8p bufe = (u8p)input[1];
    u8p inbuf[4] = {bufb, bufb, bufe, bufe};
    // Step 0: Verify the input TLV is valid
    rdx vfy;
    rdxInit(&vfy, RDX_FMT_TLV, inbuf);

    ok64 o = rdxVerifyAll(&vfy);
    if (o != OK) done;  // Invalid input, skip

    // Step 1: Normalize input first (dedup, sort)
    a_pad(u8, norm_buf, PAGESIZE);
    rdx r, nw;
    rdxInit(&r, RDX_FMT_TLV, inbuf);
    rdxWriteInit(&nw, RDX_FMT_TLV, norm_buf);

    o = rdxConvert(&nw, &r);
    if (o != OK) done;  // Normalize failed

    // Step 2: Set up two readers pointing to normalized input
    a_pad(rdx, inputs, 16);
    rdxbZero(inputs);

    rdxp a = 0;
    call(rdxbFedP, inputs, &a);
    rdxInit(a, RDX_FMT_TLV, norm_buf);

    rdxp b = 0;
    call(rdxbFedP, inputs, &b);
    rdxInit(b, RDX_FMT_TLV, norm_buf);

    rdxsFed1(rdxbData(inputs));

    // Step 3: Merge A + A
    a_pad(u8, out_buf, PAGESIZE);
    rdx w;
    rdxWriteInit(&w, RDX_FMT_TLV, out_buf);

    o = rdxMerge(&w, rdxbDataIdle(inputs));
    if (o != OK) done;  // Merge failed

    // Step 4: A + A must equal A (on normalized input)
    if ($len(out_buf_datac) != $len(norm_buf_datac) ||
        memcmp(*norm_buf_datac, *out_buf_datac, $len(norm_buf_datac)) != 0) {
        a_cstr(in_label, "INPUT TLV (normalized):");
        a_cstr(out_label, "OUTPUT TLV:");
        AAdump(in_label, norm_buf_datac);
        AAdump(out_label, out_buf_datac);
        must(0, "A+A != A");
    }

    done;
}
