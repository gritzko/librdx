#ifndef LIBRDX_TLV_H
#define LIBRDX_TLV_H
#include "01.h"
#include "B.h"
#include "INT.h"
#include "OK.h"
#include "S.h"

con ok64 TLVbadrec = 0x1d55f9a5a36a67;
con ok64 TLVnodata = 0x1d55fcb3a25e25;
con ok64 TLVbadtype = 0x7557e6968e3dd29;
con ok64 TLVnoroom = 0x1d55fcb3db3cf1;
con ok64 TLVoverflo = 0x7557f3ea9daac33;
con ok64 TLVbadcall = 0x7557e69689e5c30;
con ok64 TLVbadarg = 0x1d55f9a5a25dab;
con ok64 TLVbadkv = 0x7557e6968bfa;
con ok64 TLVtoolong = 0x7557f8cf3c33cab;

#define TLVaA 0x20
#define TLV_MAX_LEN ((1 << 30) - 1)

fun int TLVlong(u8 t) { return t >= 'A' && t <= 'Z'; }
fun int TLVshort(u8 t) { return t >= 'a' && t <= 'z'; }
fun u8 TLVup(u8 t) { return t & ~TLVaA; }

fun u32 TLVlen(size_t len) { return len <= 0xff ? 2 + len : 5 + len; }

fun ok64 TLVprobe(u8* t, u32* hlen, u32* blen, $cu8c data) {
    if ($empty(data)) return TLVnodata;
    if (TLVshort(**data)) {
        if ($len(data) < 2) return TLVnodata;
        *t = **data - TLVaA;
        *hlen = 2;
        *blen = (*data)[1];
    } else if (TLVlong(**data)) {
        if ($len(data) < 5) return TLVnodata;
        *t = **data;
        *hlen = 5;
        *blen = $at(data, 1) | ($at(data, 2) << 8) | ($at(data, 3) << 16) |
                ($at(data, 4) << 24);
    } else {
        return TLVbadrec;
    }
    return (*hlen + *blen) <= $len(data) ? OK : TLVnodata;
}

ok64 TLVDrain(u8* t, u8c$ value, u8cs from);

ok64 TLVDrain$(u8c$ rec, u8cs from);

fun ok64 TLVpick(u8* type, u8cs value, $cu8c tlv, size_t offset) {
    a$tail(u8c, keytlv, tlv, offset);
    return TLVDrain(type, value, keytlv);
}

ok64 TLVtake(u8 t, u8cs value, $u8c from);

fun void TLVhead($u8 into, u8 type, u32 len) {
    if (len <= 0xff) {
        **into = type + TLVaA;
        ++*into;
        **into = len;
        ++*into;
    } else {
        **into = type;
        ++*into;
        put32(into, len);
    }
}

ok64 TLVFeed($u8 into, u8 type, u8cs value);

/** Open a TLV header for a record of unknown length.
 *  The buffer must be stable during the whole write;
 *  no shifts, no reallocs, no remaps.
 * @deprecated */
ok64 TLVopen($u8 tlv, u8 type, u32** len);

/** Open a TLV header for a yet-unwritten (short) record. */
ok64 TLVinitshort($u8 tlv, u8 type, Bu8p stack);

/** Open a TLV header for a yet-unwritten (long) record. */
ok64 TLVinitlong($u8 tlv, u8 type, Bu8p stack);

/** Finalize the header once the record is complete. */
ok64 TLVendany($u8 tlv, u8 type, Bu8p stack);

// @deprecated
ok64 TLVclose($u8 tlv, u8 type, u32* const* len);

ok64 TLVFeedkv($u8 tlv, u8c type, u8cs key, $cu8c val);

ok64 TLVDrainKeyVal(u8* type, u8cs key, $u8c val, $u8c tlv);

#endif
