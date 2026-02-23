//
// Fuzz test: SLIK format roundtrip via TLV
// Takes TLV input, writes to SLIK format, reads back, verifies identical
//
#include "rdx/RDX.h"

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/S.h"
#include "abc/TEST.h"

// Recursive copy from TLV to SLIK format
fun ok64 SLIKCopyToSLIK(rdxp slikw, rdxp tlv) {
    sane(slikw && tlv);
    ok64 o = OK;

    while ((o = rdxNext(tlv)) == OK) {
        // Set up SLIK writer element
        slikw->type = tlv->type;
        slikw->id = tlv->id;

        switch (tlv->type) {
            case RDX_TYPE_INT:
                slikw->i = tlv->i;
                break;
            case RDX_TYPE_FLOAT:
                slikw->f = tlv->f;
                break;
            case RDX_TYPE_STRING:
                u8csMv(slikw->s, tlv->s);
                slikw->flags = tlv->flags;
                break;
            case RDX_TYPE_TERM:
                u8csMv(slikw->t, tlv->t);
                break;
            case RDX_TYPE_REF:
                slikw->r = tlv->r;
                break;
            default:
                break;
        }

        // Write the element
        call(rdxWriteNextSLIK, slikw);

        // Recurse into containers
        if (rdxTypePlex(tlv)) {
            rdx tlv_child = {.format = RDX_FMT_TLV};
            call(rdxInto, &tlv_child, tlv);

            rdx slik_child = {};
            call(rdxWriteIntoSLIK, &slik_child, slikw);

            call(SLIKCopyToSLIK, &slik_child, &tlv_child);

            call(rdxWriteOutoSLIK, &slik_child, slikw);
            call(rdxOuto, &tlv_child, tlv);
        }
    }

    if (o != END) fail(o);
    done;
}

// Recursive comparison: SLIK vs TLV
fun ok64 SLIKCompare(rdxp slik, rdxp tlv) {
    sane(slik && tlv);
    ok64 oslik = OK, otlv = OK;
    int elem = 0;

    while (1) {
        oslik = rdxNextSLIK(slik);
        otlv = rdxNext(tlv);

        if (oslik == END && otlv == END) done;
        must(oslik != END && otlv != END, "length mismatch");
        must(oslik == OK, "slik read error");
        must(otlv == OK, "tlv read error");

        // Compare type
        must(slik->type == tlv->type, "type mismatch");

        // Compare id
        must(slik->id.seq == tlv->id.seq && slik->id.src == tlv->id.src,
             "id mismatch");

        // Compare values
        switch (slik->type) {
            case RDX_TYPE_INT:
                must(slik->i == tlv->i, "int mismatch");
                break;
            case RDX_TYPE_FLOAT:
                must(slik->f == tlv->f, "float mismatch");
                break;
            case RDX_TYPE_STRING:
                must($eq(slik->s, tlv->s), "string mismatch");
                break;
            case RDX_TYPE_TERM:
                must($eq(slik->t, tlv->t), "term mismatch");
                break;
            case RDX_TYPE_REF:
                must(id128Eq(&slik->r, &tlv->r), "ref mismatch");
                break;
            default:
                break;
        }

        // Recurse into containers
        if (rdxTypePlex(slik)) {
            rdx slik_child = {};
            rdx tlv_child = {.format = RDX_FMT_TLV};

            call(rdxIntoSLIK, &slik_child, slik);
            call(rdxInto, &tlv_child, tlv);

            call(SLIKCompare, &slik_child, &tlv_child);

            call(rdxOutoSLIK, &slik_child, slik);
            call(rdxOuto, &tlv_child, tlv);
        }
        elem++;
    }
}

FUZZ(u8, SLIK3fuzz) {
    sane(1);

    if ($len(input) == 0 || $len(input) > PAGESIZE) done;

    a_fake(u8c, inbuf, input);
    // Verify input is valid TLV
    rdx vfy;
    rdxInit(&vfy, RDX_FMT_TLV, (u8**)inbuf);
    if (rdxVerifyAll(&vfy) != OK) done;

    // Normalize input (dedup, sort)
    a_pad(u8, norm_buf, PAGESIZE);
    rdx r;
    rdxInit(&r, RDX_FMT_TLV, (u8**)inbuf);
    rdx nw;
    rdxWriteInit(&nw, RDX_FMT_TLV, norm_buf);

    if (rdxConvert(&nw, &r) != OK) done;

    // Allocate SLIK buffer and skip stack
    a_pad(u8, slik_buf, PAGESIZE * 4);
    a_pad0(u64, slik_stack, PAGESIZE);

    // Write to SLIK format
    rdx slikw = {};
    rdxWriteInitSLIK(&slikw, slik_buf, slik_stack);

    rdx tlv_read;
    rdxInit(&tlv_read, RDX_FMT_TLV, norm_buf);

    ok64 o = SLIKCopyToSLIK(&slikw, &tlv_read);
    if (o != OK) done;

    o = rdxWriteFinishSLIK(&slikw);
    if (o != OK) done;

    // Read back from SLIK and compare with normalized TLV
    a_pad0(u64, read_stack, PAGESIZE);
    rdx slik_read = {};
    rdxInitSLIK(&slik_read, slik_buf, read_stack);

    rdx tlv_cmp;
    rdxInit(&tlv_cmp, RDX_FMT_TLV, norm_buf);

    call(SLIKCompare, &slik_read, &tlv_cmp);

    done;
}
