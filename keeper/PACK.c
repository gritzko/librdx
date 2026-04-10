//  PACK: git packfile parser
//
#include "PACK.h"

#include "GIT.h"
#include "ZINF.h"
#include "abc/PRO.h"

static a_cstr(PACK_MAGIC, "PACK");

ok64 PACKDrainHdr(u8cs from, pack_hdr *hdr) {
    sane(u8csOK(from) && hdr);
    if ($size(from) < 12) return NODATA;

    if (memcmp(from[0], PACK_MAGIC[0], 4) != 0)
        return PACKBADFMT;
    from[0] += 4;

    // version + count: big-endian u32
    u32 v = 0, c = 0;
    u8sDrain32(from, &v);
    u8sDrain32(from, &c);
    hdr->version = flip32(v);
    hdr->count = flip32(c);

    done;
}

// Decode git varint: type in bits 6..4 of first byte, size in
// bit 3..0 of first byte then 7-bit continuation.
static ok64 PACKDrainVarint(u8cs from, u8 *type, u64 *size) {
    u8 c = 0;
    ok64 o = u8sDrain8(from, &c);
    if (o != OK) return NODATA;

    *type = (c >> 4) & 0x7;
    *size = c & 0x0f;
    u32 shift = 4;

    while (c & 0x80) {
        o = u8sDrain8(from, &c);
        if (o != OK) return NODATA;
        *size |= (u64)(c & 0x7f) << shift;
        shift += 7;
    }

    return OK;
}

// Decode OFS_DELTA negative offset varint
static ok64 PACKDrainOfs(u8cs from, u64 *ofs) {
    u8 c = 0;
    ok64 o = u8sDrain8(from, &c);
    if (o != OK) return NODATA;
    *ofs = c & 0x7f;

    while (c & 0x80) {
        o = u8sDrain8(from, &c);
        if (o != OK) return NODATA;
        *ofs = ((*ofs + 1) << 7) | (c & 0x7f);
    }

    return OK;
}

ok64 PACKDrainObjHdr(u8cs from, pack_obj *obj) {
    sane(u8csOK(from) && obj);
    memset(obj, 0, sizeof(*obj));

    ok64 rv = PACKDrainVarint(from, &obj->type, &obj->size);
    if (rv != OK) return rv;

    if (obj->type == PACK_OBJ_OFS_DELTA) {
        ok64 ro = PACKDrainOfs(from, &obj->ofs_delta);
        if (ro != OK) return ro;
    } else if (obj->type == PACK_OBJ_REF_DELTA) {
        if ($size(from) < GIT_SHA1_LEN) return PACKBADFMT;
        obj->ref_delta[0] = from[0];
        obj->ref_delta[1] = from[0] + GIT_SHA1_LEN;
        from[0] += GIT_SHA1_LEN;
    }

    done;
}

ok64 PACKInflate(u8cs from, u8s into, u64 size) {
    sane(u8csOK(from) && u8sOK(into));
    if ((u64)$size(into) < size) return NOROOM;

    u64 consumed = 0, produced = 0;
    int r = ZINFInflate(from[0], $size(from),
                        into[0], size,
                        &consumed, &produced);
    if (r == -1) return PACKFAIL;
    if (r == -2) return PACKBADOBJ;

    from[0] += consumed;
    into[0] += produced;

    done;
}
