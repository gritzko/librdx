#ifndef LIBRDX_TLV_H
#define LIBRDX_TLV_H
#include "01.h"
#include "B.h"
#include "INT.h"
#include "OK.h"
#include "PRO.h"
#include "S.h"

static const ok64 TLVbadrec = 0x1d55f9a5a36a67;
static const ok64 TLVnodata = 0x1d55fcb3a25e25;
static const ok64 TLVbadtype = 0x7557e6968e3dd29;
static const ok64 TLVnoroom = 0x1d55fcb3db3cf1;
static const ok64 TLVoverflo = 0x7557f3ea9daac33;
static const ok64 TLVbadcall = 0x7557e69689e5c30;
static const ok64 TLVbadarg = 0x1d55f9a5a25dab;
static const ok64 TLVbadkv = 0x7557e6968bfa;
static const ok64 TLVtoolong = 0x7557f8cf3c33cab;

#define TLVaA 0x20
#define TLV_MAX_LEN ((1 << 30) - 1)
#define TLV_TINY_TYPE '0'

fun int TLVtiny(u8 t) { return t >= '0' && t <= '9'; }
fun int TLVlong(u8 t) { return t >= 'A' && t <= 'Z'; }
fun int TLVshort(u8 t) { return t >= 'a' && t <= 'z'; }
fun u8 TLVup(u8 t) { return t & ~TLVaA; }

fun u32 TLVlen(size_t len) { return len <= 0xff ? 2 + len : 5 + len; }
fun u32 TLVtinylen(size_t len) {
    if (len <= 9) return len + 1;
    return TLVlen(len);
}

fun ok64 TLVprobe(u8* t, u32* hlen, u32* blen,
                  $cu8c data) {  // FIXME tiny on request, inline short
    if ($empty(data)) return TLVnodata;
    if (TLVshort(**data)) {
        if ($len(data) < 2) return TLVnodata;
        *t = **data - TLVaA;
        *hlen = 2;
        *blen = (*data)[1];
    } else if (TLVtiny(**data)) {
        *t = TLV_TINY_TYPE;
        *hlen = 1;
        *blen = **data - '0';
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

fun ok64 TLVdrain(u8* t, u8c$ value, $u8c from) {
    sane(t != nil && value != nil && $ok(from));
    u32 hlen = 0, blen = 0;
    call(TLVprobe, t, &hlen, &blen, from);
    value[0] = from[0] + hlen;
    value[1] = value[0] + blen;
    from[0] += hlen + blen;
    done;
}

fun ok64 TLVdrain$(u8c$ rec, $u8c from) {
    sane(rec != nil && $ok(from));
    u32 hlen = 0, blen = 0;
    u8 t = 0;
    call(TLVprobe, &t, &hlen, &blen, from);
    rec[0] = from[0];
    rec[1] = from[0] + hlen + blen;
    from[0] += hlen + blen;
    done;
}

fun ok64 TLVpick(u8* type, $u8c value, $cu8c tlv, size_t offset) {
    a$tail(u8c, keytlv, tlv, offset);
    return TLVdrain(type, value, keytlv);
}

fun ok64 TLVtake(u8 t, $u8c value, $u8c from) {
    sane(value != NULL && from != NULL);
    u32 hlen = 0;
    u32 blen = 0;
    u8 fact = 0;
    call(TLVprobe, &fact, &hlen, &blen, from);
    test(fact == '0' || fact == t, TLVbadtype);
    value[0] = from[0] + hlen;
    value[1] = from[0] + hlen + blen;
    from[0] += hlen + blen;
    done;
}

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

fun ok64 TLVfeed($u8 into, u8 type, $u8c value) {
    sane(TLVlong(type) && into != NULL && value != NULL);
    u32 len = $len(value);
    test($len(into) >= len + 5, TLVnoroom);
    TLVhead(into, type, len);
    $u8copy(into, value);
    *into += len;
    done;
}

fun ok64 TLVtinyfeed($u8 into, u8 type, $u8c value) {
    sane($ok(into) && $ok(value));
    size_t len = $len(value);
    test($len(into) >= len + 5, TLVnoroom);  // todo
    if (len > 9) {
        TLVhead(into, type, len);
    } else {
        **into = '0' + len;
        ++*into;
    }
    $u8copy(into, value);
    *into += len;
    done;
}

/** Open a TLV header for a record of unknown length.
 *  The buffer must be stable during the whole write;
 *  no shifts, no reallocs, no remaps.
 * @deprecated */
fun ok64 TLVopen($u8 tlv, u8 type, u32** len) {
    sane($ok(tlv) && len != nil && TLVlong(type));
    test($len(tlv) >= 5, TLVnoroom);
    **tlv = type;
    ++*tlv;
    *len = (u32*)*tlv;
    u32 zero = 0;
    $u8feed32(tlv, &zero);
    done;
}

/** Open a TLV header for a yet-unwritten (short) record. */
ok64 TLVinitshort($u8 tlv, u8 type, Bu8p stack);

/** Open a TLV header for a yet-unwritten (long) record. */
ok64 TLVinitlong($u8 tlv, u8 type, Bu8p stack);

/** Finalize the header once the record is complete. */
ok64 TLVendany($u8 tlv, u8 type, Bu8p stack);

// @deprecated
fun ok64 TLVclose($u8 tlv, u8 type, u32* const* len) {
    sane($ok(tlv) && TLVlong(type) && len != nil && *len != nil &&
         (u8*)*len < *tlv && *(*((u8**)len) - 1) == type);
    size_t d = *tlv - (u8*)*len;
    test(d <= TLV_MAX_LEN && d >= 4, TLVbadrec);
    d -= 4;
    if (d > 0xff) {
        **len = d;
    } else {
        u8* p = *(u8**)len - 1;
        *p += TLVaA;
        ++p;
        *p = d;
        ++p;
        const size_t shift = sizeof(u32) - sizeof(u8);
        memmove(p, p + shift, d);
        *tlv -= shift;
    }
    done;
}

fun ok64 TLVfeedkv($u8 tlv, u8c type, $u8c key, $cu8c val) {
    sane($ok(tlv) && $ok(key) && ($empty(val) || $ok(val)));
    size_t keylen = $len(key);
    test(keylen < 0x100, TLVbadarg);
    u64 blen = keylen + $len(val);
    test($len(tlv) >= blen + 1 + 4 + 1, TLVnoroom);
    TLVhead(tlv, type, blen + 1);
    **tlv = keylen;
    ++*tlv;
    $feed(tlv, key);
    $feed(tlv, val);
    done;
}

fun ok64 TLVdrainkv(u8* type, $u8c key, $u8c val, $u8c tlv) {
    sane(type != nil && key != nil && val != nil && $ok(tlv));
    u32 hlen = 0, blen = 0;
    call(TLVprobe, type, &hlen, &blen, tlv);
    $u8c body = {tlv[0] + hlen, tlv[0] + hlen + blen};
    test($len(body) > 0 && $len(body) >= **body, TLVbadkv);
    key[0] = body[0] + 1;
    key[1] = key[0] + **body;
    val[0] = key[1];
    val[1] = body[1];
    tlv[0] = body[1];
    done;
}

fun ok64 TLVvalid($cu8c tlv, $cu8c lits) {
    sane($ok(tlv) && $ok(lits));
    a$dup(u8c, dup, tlv);
    while (!$empty(dup)) {
        u8 t = 0;
        $u8c _ = {};
        call(TLVdrain, &t, _, dup);
        if (!$empty(lits) && $u8find(lits, &t) == nil) fail(TLVbadrec);
    }
    done;
}

#endif
