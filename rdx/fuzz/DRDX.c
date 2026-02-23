//
// Fuzz test: rdxDiff runs OK
// Input: two TLVs - doc (with IDs) and neu (stripped target)
//
#include "rdx/RDX.h"

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/S.h"
#include "abc/TEST.h"

// Parse and verify one TLV from input, return length (0 on failure)
fun u64 DIFFParseTLV(u8cs input) {
    if ($len(input) < 2) return 0;
    rdx probe = {.format = RDX_FMT_TLV};
    probe.next = input[0];
    probe.opt = (u8p)input[1];
    probe.bulk = NULL;
    if (rdxNext(&probe) != OK) return 0;
    u64 len = probe.next - input[0];  // next advanced to record end
    if (len == 0 || len > $len(input)) return 0;
    a_head(u8c, tlv, input, len);
    // Full verification including ordering
    rdx vfy = {.format = RDX_FMT_TLV};
    vfy.next = tlv[0];
    vfy.opt = (u8p)tlv[1];
    vfy.bulk = NULL;
    if (rdxVerifyAll(&vfy) != OK) return 0;
    return len;
}

FUZZ(u8, DIFFfuzz) {
    sane(1);
    if ($len(input) < 4 || $len(input) > PAGESIZE) done;

    // Parse two TLVs from input: doc and neu
    u64 lenDoc = DIFFParseTLV(input);
    if (lenDoc == 0 || lenDoc >= $len(input)) done;
    a_head(u8c, doc_tlv, input, lenDoc);
    a_rest(u8c, rest, input, lenDoc);
    u64 lenNeu = DIFFParseTLV(rest);
    if (lenNeu == 0) done;
    a_head(u8c, neu_tlv, rest, lenNeu);

    // Skip if doc and neu have different root types or non-container roots
    {
        rdx doc = {.format = RDX_FMT_TLV};
        doc.next = doc_tlv[0];
        doc.opt = (u8p)doc_tlv[1];
        doc.bulk = NULL;
        rdx neu = {.format = RDX_FMT_TLV};
        neu.next = neu_tlv[0];
        neu.opt = (u8p)neu_tlv[1];
        neu.bulk = NULL;
        if (rdxNext(&doc) != OK || rdxNext(&neu) != OK) done;
        if (doc.type != neu.type) done;
        // Skip non-container roots (diff requires container)
        if (!rdxTypePlex(&doc)) done;
        // Skip tombstoned root elements (seq bit 0 = 1)
        if (doc.id.seq & 1) done;
        if (neu.id.seq & 1) done;
    }

    // Strip neu (diff expects stripped input)
    a_pad(u8, neu_buf, PAGESIZE);
    {
        rdx neu = {.format = RDX_FMT_TLV};
        neu.next = neu_tlv[0];
        neu.opt = (u8p)neu_tlv[1];
        neu.bulk = NULL;
        rdx neu_w = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
        neu_w.bulk = neu_buf;
        if (rdxStrip(&neu_w, &neu) != OK) done;
    }

    // Skip if neu_stripped is invalid (e.g. duplicate keys in EULER)
    {
        rdx neu_vfy = {.format = RDX_FMT_TLV};
        neu_vfy.next = neu_buf[1];  // data start
        neu_vfy.opt = neu_buf[2];   // idle = data end
        neu_vfy.bulk = NULL;
        if (rdxVerifyAll(&neu_vfy) != OK) done;
    }

    // Compute patch = diff(doc, neu_stripped) - just check it runs OK
    a_pad(u8, patch_buf, PAGESIZE);
    {
        rdx doc = {.format = RDX_FMT_TLV};
        doc.next = doc_tlv[0];
        doc.opt = (u8p)doc_tlv[1];
        doc.bulk = NULL;
        rdx neu = {.format = RDX_FMT_TLV};
        neu.next = neu_buf[1];  // data start
        neu.opt = neu_buf[2];   // idle = data end
        neu.bulk = NULL;
        rdx patch = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
        patch.bulk = patch_buf;
        ok64 o = rdxDiff(&patch, &doc, &neu);
        must(o == OK, "rdxDiff failed");
    }

    done;
}
