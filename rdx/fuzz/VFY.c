//
// Fuzz test: TLV -> JDR -> TLV roundtrip
// Any valid TLV that passes rdxVerifyAll() should roundtrip identically
//
#include "rdx/RDX.h"

#include "abc/TEST.h"

FUZZ(u8, VFYfuzz) {
    sane(1);

    if ($len(input) == 0) done;

    // Step 0: Verify the input TLV is valid
    // TLV reader: next=position, opt=range_end, bulk=buffer (NULL for no PAGE)
    rdx vfy = {.format = RDX_FMT_TLV};
    vfy.next = input[0];
    vfy.opt = (u8p)input[1];
    vfy.bulk = NULL;  // no PAGE in fuzz test

    ok64 o = rdxVerifyAll(&vfy);
    if (o != OK) done;  // Invalid input, skip

    // Buffers for roundtrip
    a_pad(u8, jdr_buf, PAGESIZE);
    a_pad(u8, tlv_buf, PAGESIZE);

    // Step 1: TLV -> JDR
    rdx tlv_read = {.format = RDX_FMT_TLV};
    tlv_read.next = input[0];
    tlv_read.opt = (u8p)input[1];
    tlv_read.bulk = NULL;

    rdx jdr_write = {};
    rdxWriteInit(&jdr_write, RDX_FMT_JDR, jdr_buf);

    o = rdxCopy(&jdr_write, &tlv_read);
    if (o != OK) done;  // Conversion failed

    // Step 2: JDR -> TLV
    rdx jdr_read = {.format = RDX_FMT_JDR};
    jdr_read.next = jdr_buf_datac[0];
    jdr_read.opt = (u8p)jdr_buf_datac[1];

    // TLV writer: bulk points to buffer
    rdx tlv_write = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
    tlv_write.bulk = tlv_buf;

    o = rdxCopy(&tlv_write, &jdr_read);
    if (o != OK) done;  // Conversion failed

    // Step 3: Compare - this is the key invariant
    // Valid TLV that passed rdxVerifyAll must roundtrip identically
    must($len(tlv_buf_datac) == $len(input), "length mismatch");
    must(memcmp(*input, *tlv_buf_datac, $len(input)) == 0, "content mismatch");

    done;
}
