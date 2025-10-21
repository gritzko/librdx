#include "TLV.h"

#include "OK.h"
#include "PRO.h"

ok64 TLVDrain(u8* t, u8c$ value, u8cs from) {
    sane(t != NULL && value != NULL && $ok(from));
    u32 hlen = 0, blen = 0;
    call(TLVprobe, t, &hlen, &blen, from);
    value[0] = from[0] + hlen;
    value[1] = value[0] + blen;
    from[0] += hlen + blen;
    done;
}

ok64 TLVtake(u8 t, u8cs value, $u8c from) {
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

ok64 TLVFeed($u8 into, u8 type, u8cs value) {
    sane(TLVlong(type) && into != NULL && value != NULL);
    u32 len = $len(value);
    test($len(into) >= len + 5, TLVnoroom);
    TLVhead(into, type, len);
    u8sCopy(into, value);
    *into += len;
    done;
}

ok64 TLVopen($u8 tlv, u8 type, u32** len) {
    sane($ok(tlv) && len != NULL && TLVlong(type));
    test($len(tlv) >= 5, TLVnoroom);
    **tlv = type;
    ++*tlv;
    *len = (u32*)*tlv;
    u32 zero = 0;
    u8sFeed32(tlv, &zero);
    done;
}

ok64 TLVclose($u8 tlv, u8 type, u32* const* len) {
    sane($ok(tlv) && TLVlong(type) && len != NULL && *len != NULL &&
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
        con size_t shift = sizeof(u32) - sizeof(u8);
        memmove(p, p + shift, d);
        *tlv -= shift;
    }
    done;
}
ok64 TLVFeedKeyVal($u8 tlv, u8c type, u8cs key, $cu8c val) {
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
ok64 TLVDrain$(u8c$ rec, u8cs from) {
    sane(rec != NULL && $ok(from));
    u32 hlen = 0, blen = 0;
    u8 t = 0;
    call(TLVprobe, &t, &hlen, &blen, from);
    rec[0] = from[0];
    rec[1] = from[0] + hlen + blen;
    from[0] += hlen + blen;
    done;
}

ok64 TLVDrainKeyVal(u8* type, u8cs key, $u8c val, $u8c tlv) {
    sane(type != NULL && key != NULL && val != NULL && $ok(tlv));
    u32 hlen = 0, blen = 0;
    call(TLVprobe, type, &hlen, &blen, tlv);
    u8cs body = {tlv[0] + hlen, tlv[0] + hlen + blen};
    test($len(body) > 0 && $len(body) >= **body, TLVbadkv);
    key[0] = body[0] + 1;
    key[1] = key[0] + **body;
    val[0] = key[1];
    val[1] = body[1];
    tlv[0] = body[1];
    done;
}
ok64 TLVinitshort($u8 tlv, u8 type, Bu8p stack) {
    sane($ok(tlv) && Bok(stack) && TLVlong(type));
    call(u8pbPush, stack, &$head(tlv));
    call(u8sFeed2, tlv, type | TLVaA, 0);
    done;
}

ok64 TLVinitlong($u8 tlv, u8 type, Bu8p stack) {
    sane($ok(tlv) && Bok(stack) && TLVlong(type));
    call(u8pbPush, stack, &$head(tlv));
    u8 head[] = {type & ~TLVaA, 0, 0, 0, 0};
    a$(u8c, h, head);
    call(u8sFeed, tlv, h);
    done;
}

ok64 TLVendany($u8 tlv, u8 type, Bu8p stack) {
    sane($ok(tlv) && Bok(stack) && !Bempty(stack) &&
         *Btop(stack) + 2 <= $head(tlv));
    u8* start = *Btop(stack);
    size_t len = $head(tlv) - start;
    if (TLVlong(*start)) {
        test(len >= 4 + 1, FAILsanity);
        len -= 4 + 1;
        if (len < 0x100) {
            u8cs from = {start + 1 + 4, tlv[0]};
            $u8 into = {start + 1 + 1, tlv[0] - 4 + 1};
            $u8move(into, from);
            tlv[0] -= 4 - 1;
            *start |= TLVaA;
            *(start + 1) = (u8)len;
        } else {
            *(u32*)(start + 1) = len;
        }
    } else if (TLVshort(*start)) {
        len -= 1 + 1;
        if (len >= 0x100) {
            test(len <= TLV_MAX_LEN, TLVtoolong);
            test($len(tlv) >= 4 - 1, TLVnoroom);
            u8cs from = {start + 1 + 1, tlv[0]};
            $u8 into = {start + 1 + 4, tlv[0] + 4 - 1};
            $u8move(into, from);
            tlv[0] += 4 - 1;
            *start &= ~TLVaA;
            *(u32*)(start + 1) = len;
        } else {
            *(start + 1) = (u8)len;
        }
    } else {
        fail(TLVbadrec);
    }
    u8pbPop(stack);
    done;
}
