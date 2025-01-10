#include "TLV.h"

#include "OK.h"
#include "PRO.h"

ok64 TLVopenshort($u8 tlv, u8 type, Bu8p stack) {
    sane($ok(tlv) && Bok(stack) && TLVlong(type));
    call(Bu8ppush, stack, &$head(tlv));
    call($u8feed2, tlv, type | TLVaa, 0);
    done;
}

ok64 TLVopenlong($u8 tlv, u8 type, Bu8p stack) {
    sane($ok(tlv) && Bok(stack) && TLVlong(type));
    call(Bu8ppush, stack, &$head(tlv));
    u8 head[] = {type, 0, 0, 0, 0};
    a$(u8c, h, head);
    call($u8feedall, tlv, h);
    done;
}

ok64 TLVcloseany($u8 tlv, u8 type, Bu8p stack) {
    sane($ok(tlv) && Bok(stack) && !Bempty(stack) &&
         *Btop(stack) + 2 <= $head(tlv));
    u8* start = *Btop(stack);
    size_t len = $head(tlv) - start;
    if (TLVlong(*start)) {
        test(len >= 4 + 1, FAILsanity);
        len -= 4 + 1;
        if (len < 0x100) {
            $u8c from = {start + 1 + 4, tlv[0]};
            $u8 into = {start + 1 + 1, tlv[0] - 4 + 1};
            $u8move(into, from);
            tlv[0] -= 4 - 1;
            *start |= TLVaa;
            *(start + 1) = (u8)len;
        } else {
            *(u32*)start = len;
        }
    } else if (TLVshort(*start)) {
        len -= 1 + 1;
        if (len >= 0x100) {
            test(len <= TLV_MAX_LEN, TLVtoolong);
            test($len(tlv) >= 4 - 1, TLVnoroom);
            $u8c from = {start + 1 + 1, tlv[0]};
            $u8 into = {start + 1 + 4, tlv[0] + 4 - 1};
            $u8move(into, from);
            tlv[0] += 4 - 1;
            *start &= ~TLVaa;
            *(u32*)start = len;
        } else {
            *(start + 1) = (u8)len;
        }
    } else {
        fail(TLVbadrec);
    }
    Bu8ppop(stack);
    done;
}
