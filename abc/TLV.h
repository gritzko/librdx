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

ok64 _TLVu8sFeed(u8s into, u8 type, u8csc value);

fun ok64 TLVu8sFeed(u8s into, u8 type, u8csc value) {
    if ($len(value) > 0xff || $len(into) < 2 + $len(value))
        return _TLVu8sFeed(into, type, value);
    u8sFeed2(into, type | TLVaA, (u8)$len(value));
    u8sFeed(into, value);
    return OK;
}

ok64 _TLVu8sDrain(u8cs from, u8p type, u8csp value);

fun ok64 TLVu8sDrain(u8cs from, u8p type, u8csp value) {
    if ($len(from) < 2 || !(**from & TLVaA))
        return _TLVu8sDrain(from, type, value);
    *type = **from & ~TLVaA;
    u8 len = *(1 + *from);
    if ($len(from) < 2 + len) return TLVnodata;
    *from += 2;
    value[0] = *from;
    *from += len;
    value[1] = *from;
    return OK;
}

fun ok64 TLVu8bInto(u8bp into, u8 type) {
    if (u8bIdleLen(into) < 5) return TLVnoroom;
    size_t dl = u8bDataLen(into);
    if (unlikely(dl > u32max)) return TLVtoolong;
    u8bFeed1(into, type & ~TLVaA);
    u8sFeed32(u8bIdle(into), (u32*)&dl);  // all le
    ((u8**)into)[1] = into[2];
    return OK;
}

fun ok64 TLVu8bOuto(u8bp into, u8 type) {
    if (u8bPastLen(into) < 5) return TLVnodata;
    size_t ndl = u8bDataLen(into);
    a_tail(u8c, hdr, u8bPast(into), 5);
    u8 t = 0;
    u64 l = 0;
    u8sDrain1(hdr, &t);
    u32p lp = (u32p)*hdr;
    u8sDrain32(hdr, (u32p)&l);
    ((u8**)into)[1] = into[1] - 5 - l;
    *lp = ndl;
    u8cs empty = {};
    if (ndl <= 0xff) {
        u8bSplice(into, l + 2, 3, empty);
        *u8s_atp(u8bData(into), l) |= TLVaA;
    }
    return OK;
}

// ----------------------

fun int TLVlong(u8 t) { return t >= 'A' && t <= 'Z'; }
fun int TLVshort(u8 t) { return t >= 'a' && t <= 'z'; }

fun u32 TLVlen(size_t len) { return len <= 0xff ? 2 + len : 5 + len; }


ok64 TLVDrain$(u8c$ rec, u8cs from);

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

ok64 TLVFeedKeyVal($u8 tlv, u8c type, u8cs key, $cu8c val);

ok64 TLVDrainKeyVal(u8* type, u8cs key, u8cs val, u8cs tlv);

#endif
