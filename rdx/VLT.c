#include "VLT.h"

#include <string.h>

#include "abc/$.h"
#include "abc/B.h"
#include "abc/TLV.h"

ok64 _VLTreverse($u8 data, u64 deep) {
    sane($ok(data));
    while (!$empty(data)) {
        u8 lit;
        u32 blen, hlen;
        u8* e = data[1];
        {
            u8 head[32];
            lit = *(e - 1);
            head[0] = lit;
            if (TLVlong(lit)) {
                test($len(data) >= 6, TLVbadkv);
                u8 klen = *(e - 6);
                u32 l = *(u32*)(e - 1 - 4);
                hlen = 1 + 4 + 1 + klen;
                blen = l - klen - 1;
                memcpy(head + 1, e - 5, 4);
                head[5] = klen;
                memcpy(head + 1 + 4 + 1, e - hlen, klen);
            } else if (TLVshort(lit)) {
                test($len(data) >= 3, TLVbadkv);
                u8 l = *(e - 2);
                u8 klen = *(e - 3);
                head[1] = l;
                head[2] = klen;
                hlen = 1 + 1 + 1 + klen;
                blen = l - klen - 1;
                memcpy(head + 1 + 1 + 1, e - hlen, klen);
            } else {
                fail(TLVbadkv);
            }
            memmove(e - blen, e - hlen - blen, blen);
            memmove(e - hlen - blen, head, hlen);
        }
        u8 ndx = (lit & ~TLVaa) - 'A';
        if (u64bit(deep, ndx)) {
            $u8 body = {e - blen, e};
            call(_VLTreverse, body, deep);
        }
        data[1] = e - hlen - blen;
    }
    return OK;
}

ok64 VLTreverse(Bu8 vlt, u64 deep) {
    sane(Bok(vlt));
    a$dup(u8, data, Bdata(vlt));
    return _VLTreverse(data, deep);
}

ok64 VLTfeedTLV($u8 tlv, $u8c vlt, u64 deep) {
    sane($ok(tlv) && $ok(vlt) && $len(tlv) >= $len(vlt));
    a$dup(u8c, v, vlt);
    while (!$empty(v)) {
        u8 lit;
        u32 blen, hlen, llen;
        u8c* e = v[1];
        $u8 into = {tlv[1], tlv[1]};
        lit = *(e - 1);
        u32 tlvlen = 0;
        if (TLVlong(lit)) {
            test($len(v) >= 6, TLVbadkv);
            llen = 4;
            tlvlen = *(u32*)(e - 1 - 4);
        } else if (TLVshort(lit)) {
            test($len(v) >= 3, TLVbadkv);
            llen = 1;
            tlvlen = *(e - 2);
        } else {
            fail(TLVbadkv);
        }
        u8 klen = *(e - 1 - llen - 1);
        hlen = 1 + llen + 1 + klen;
        blen = tlvlen - klen - 1;
        into[0] = into[1] - hlen - blen;
        $u8feed1(into, lit);
        $u8feedn(into, (u8c*)&tlvlen, llen);
        $u8feed1(into, klen);
        $u8c key = {e - hlen, e - 1 - llen - 1};
        $u8feed(into, key);
        u8 ndx = (lit & ~TLVaa) - 'A';
        $u8c body = {e - hlen - blen, e - hlen};
        if (u64bit(deep, ndx)) {
            call(VLTfeedTLV, into, body, deep);
        } else {
            $u8feed(into, body);
        }
        v[1] = e - hlen - blen;
    }
    tlv[0] += $len(vlt);
    done;
}
