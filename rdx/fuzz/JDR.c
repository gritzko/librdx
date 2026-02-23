//
// Fuzz test: JDR round-trip stability
// If JDR parses, then JDR -> TLV -> JDR -> TLV must produce identical TLV
//
#include "rdx/RDX.h"

#include "abc/FILE.h"
#include "abc/OK.h"
#include "abc/HEX.h"
#include "abc/S.h"
#include "abc/TEST.h"

#include "abc/PRO.h"

#define FUZZFAIL(o)                                                 \
    do {                                                            \
        fflush(stdout);                                             \
        fprintf(stderr, "<%s at %s:%i\n", ok64str(o), __func__, __LINE__); \
        abort();                                                    \
    } while (0)

// Dump TLV as hex and JDR for debugging
fun ok64 JDRdump(u8cs label, u8cs data, b8 is_tlv) {
    sane($ok(data));
    $println(label);

    if (is_tlv) {
        // Print hex for TLV
        a_pad(u8, hex, 4096);
        HEXPut(hex_idle, data);
        u8sJoin(hex_idle, hex);
        $println(hex_datac);
        // Print as JDR
        a_pad(u8, jdr, PAGESIZE);
        rdx r = {.format = RDX_FMT_TLV};
        r.next = (u8p)data[0]; r.opt = (u8p)data[1];
        rdx jw = {};
        rdxWriteInit(&jw, RDX_FMT_JDR, jdr);
        ok64 o = rdxCopy(&jw, &r);
        if (o == OK) {
            $println(jdr_datac);
        }
    } else {
        // Print JDR as-is
        $println(data);
    }
    done;
}

FUZZ(u8, JDRfuzz) {
    sane(1);

    if ($len(input) == 0 || $len(input) > PAGESIZE) done;

    // Step 1: Try to parse input as JDR and convert to TLV
    a_pad(u8, tlv1, PAGESIZE);
    {
        rdx jdr1 = {.format = RDX_FMT_JDR};
        jdr1.next = *input;
        jdr1.opt = (u8p)input[1];
        rdx tw1 = {};
        rdxWriteInit(&tw1, RDX_FMT_TLV, tlv1);

        ok64 o = rdxCopy(&tw1, &jdr1);
        if (o != OK) done;  // Invalid JDR, skip
    }

    // Validate TLV
    {
        rdx v = {.format = RDX_FMT_TLV};
        v.next = (u8p)tlv1_datac[0]; v.opt = (u8p)tlv1_datac[1];
        ok64 o = rdxVerifyAll(&v);
        if (o != OK) done;  // Invalid values, skip
    }

    // Step 2: Convert TLV back to JDR
    a_pad(u8, jdr2, PAGESIZE);
    {
        rdx tr = {.format = RDX_FMT_TLV};
        tr.next = (u8p)tlv1_datac[0]; tr.opt = (u8p)tlv1_datac[1];
        rdx jw = {};
        rdxWriteInit(&jw, RDX_FMT_JDR, jdr2);

        ok64 o = rdxCopy(&jw, &tr);
        if (ok64is(o, NOROOM)) done;
        if (o != OK) {
            a_cstr(in_label, "INPUT JDR:");
            a_cstr(tlv1_label, "TLV1:");
            JDRdump(in_label, input, NO);
            JDRdump(tlv1_label, tlv1_datac, YES);
            FUZZFAIL(o);
        }
    }

    // Step 3: Parse JDR back to TLV again
    a_pad(u8, tlv2, PAGESIZE);
    {
        rdx jdr2r = {.format = RDX_FMT_JDR};
        jdr2r.next = *jdr2_datac;
        jdr2r.opt = (u8p)jdr2_datac[1];
        rdx tw2 = {};
        rdxWriteInit(&tw2, RDX_FMT_TLV, tlv2);

        ok64 o = rdxCopy(&tw2, &jdr2r);
        if (ok64is(o, NOROOM)) done;
        if (o != OK) {
            a_cstr(in_label, "INPUT JDR:");
            a_cstr(jdr2_label, "JDR2:");
            JDRdump(in_label, input, NO);
            JDRdump(jdr2_label, jdr2_datac, NO);
            FUZZFAIL(o);
        }
    }

    // Step 4: TLV1 must equal TLV2
    if (!$eq(tlv1_datac, tlv2_datac)) {
        a_cstr(in_label, "INPUT JDR:");
        a_cstr(tlv1_label, "TLV1:");
        a_cstr(jdr2_label, "JDR2:");
        a_cstr(tlv2_label, "TLV2:");
        JDRdump(in_label, input, NO);
        JDRdump(tlv1_label, tlv1_datac, YES);
        JDRdump(jdr2_label, jdr2_datac, NO);
        JDRdump(tlv2_label, tlv2_datac, YES);
        FUZZFAIL(NOEQ);
    }

    done;
}
