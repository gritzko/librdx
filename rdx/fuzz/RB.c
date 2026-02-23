//
// Fuzz test: RB self-contained format roundtrip
// Takes TLV input, writes to RB format, reads back, verifies structure
//
#include "rdx/RB.h"

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/S.h"
#include "abc/TEST.h"
#include "rdx/RDX.h"

// Recursive copy from TLV to RB format
fun ok64 RBCopyToRB(rdxp rbw, rdxp tlv) {
    sane(rbw && tlv);
    ok64 o = OK;

    while ((o = rdxNext(tlv)) == OK) {
        // Set up RB writer element
        rbw->type = tlv->type;
        rbw->id = tlv->id;

        switch (tlv->type) {
            case RDX_TYPE_INT:
                rbw->i = tlv->i;
                break;
            case RDX_TYPE_FLOAT:
                rbw->f = tlv->f;
                break;
            case RDX_TYPE_STRING:
                u8csMv(rbw->s, tlv->s);
                break;
            case RDX_TYPE_TERM:
                u8csMv(rbw->t, tlv->t);
                break;
            case RDX_TYPE_REF:
                rbw->r = tlv->r;
                break;
            default:
                break;
        }

        // Write the element
        call(rdxWriteNextRB, rbw);

        // Recurse into containers
        if (rdxTypePlex(tlv)) {
            rdx tlv_child = {.format = RDX_FMT_TLV};
            call(rdxInto, &tlv_child, tlv);

            rdx rb_child = {};
            call(rdxWriteIntoRB, &rb_child, rbw);

            call(RBCopyToRB, &rb_child, &tlv_child);

            call(rdxWriteOutoRB, &rb_child, rbw);
            call(rdxOuto, &tlv_child, tlv);
        }
    }

    if (o != END) fail(o);
    done;
}

// Recursive comparison: RB vs TLV
fun ok64 RBCompare(rdxp rb, rdxp tlv) {
    sane(rb && tlv);
    ok64 orb = OK, otlv = OK;

    while (1) {
        orb = rdxNextRB(rb);
        otlv = rdxNext(tlv);

        if (orb == END && otlv == END) done;
        if (orb == END || otlv == END) {
            fprintf(stderr, "Length mismatch: rb=%s tlv=%s\n", ok64str(orb),
                    ok64str(otlv));
            abort();
        }
        if (orb != OK) fail(orb);
        if (otlv != OK) fail(otlv);

        // Compare type
        if (rb->type != tlv->type) {
            fprintf(stderr, "Type mismatch: rb=%c tlv=%c\n",
                    RDX_TYPE_LIT[rb->type], RDX_TYPE_LIT[tlv->type]);
            abort();
        }

        // Compare values
        switch (rb->type) {
            case RDX_TYPE_INT:
                if (rb->i != tlv->i) {
                    fprintf(stderr, "INT mismatch: rb=%ld tlv=%ld\n", rb->i,
                            tlv->i);
                    abort();
                }
                break;
            case RDX_TYPE_FLOAT:
                if (rb->f != tlv->f) {
                    fprintf(stderr, "FLOAT mismatch: rb=%g tlv=%g\n", rb->f,
                            tlv->f);
                    abort();
                }
                break;
            case RDX_TYPE_STRING:
                if (!$eq(rb->s, tlv->s)) {
                    fprintf(stderr, "STRING mismatch\n");
                    abort();
                }
                break;
            case RDX_TYPE_TERM:
                if (!$eq(rb->t, tlv->t)) {
                    fprintf(stderr, "TERM mismatch\n");
                    abort();
                }
                break;
            case RDX_TYPE_REF:
                if (!id128Eq(&rb->r, &tlv->r)) {
                    fprintf(stderr, "REF mismatch\n");
                    abort();
                }
                break;
            default:
                break;
        }

        // Recurse into containers
        if (rdxTypePlex(rb)) {
            rdx rb_child = {.format = RDX_FMT_RB};
            rdx tlv_child = {.format = RDX_FMT_TLV};

            call(rdxIntoRB, &rb_child, rb);
            call(rdxInto, &tlv_child, tlv);

            call(RBCompare, &rb_child, &tlv_child);

            call(rdxOutoRB, &rb_child, rb);
            call(rdxOuto, &tlv_child, tlv);
        }
    }
}

FUZZ(u8, RBfuzz) {
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

    a_pad(u8, rb_buf, PAGESIZE);

    // Write to RB format
    rdx rbw = {};
    rdxInit(&rbw, RDX_FMT_RB | RDX_FMT_WRITE, rb_buf);

    rdx tlv_read;
    rdxInit(&tlv_read, RDX_FMT_TLV, norm_buf);

    if (RBCopyToRB(&rbw, &tlv_read) != OK) done;

    // Read back from RB and compare with normalized TLV
    rdx rb_read = {};
    rdxInit(&rb_read, RDX_FMT_RB, rb_buf);

    rdx tlv_cmp;
    rdxInit(&tlv_cmp, RDX_FMT_TLV, norm_buf);

    call(RBCompare, &rb_read, &tlv_cmp);

    done;
}
