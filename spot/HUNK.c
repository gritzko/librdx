#include "HUNK.h"

#include "abc/PRO.h"

ok64 HUNKu8sFeed(u8s into, HUNKhunk const *hk) {
    sane(u8sOK(into) && hk != NULL);
    u8s inner = {};
    call(TLVu8sStart, into, inner, HUNK_TLV);
    if (!$empty(hk->title))
        call(TLVu8sFeed, inner, HUNK_TLV_TTL, hk->title);
    if (!$empty(hk->text))
        call(TLVu8sFeed, inner, HUNK_TLV_TXT, hk->text);
    if (!$empty(hk->toks)) {
        u8cs tkb = {(u8cp)hk->toks[0], (u8cp)hk->toks[1]};
        call(TLVu8sFeed, inner, HUNK_TLV_TOK, tkb);
    }
    if (!$empty(hk->hili)) {
        u8cs hib = {(u8cp)hk->hili[0], (u8cp)hk->hili[1]};
        call(TLVu8sFeed, inner, HUNK_TLV_HILI, hib);
    }
    call(TLVu8sEnd, into, inner, HUNK_TLV);
    done;
}

ok64 HUNKu8sDrain(u8cs from, HUNKhunk *hk) {
    sane($ok(from) && hk != NULL);
    u8 t = 0;
    u8cs body = {};
    call(TLVu8sDrain, from, &t, body);
    test(t == HUNK_TLV, TLVBADTYPE);
    *hk = (HUNKhunk){};
    while (!$empty(body)) {
        u8 st = 0;
        u8cs val = {};
        call(TLVu8sDrain, body, &st, val);
        switch (st) {
        case HUNK_TLV_TTL:
            $mv(hk->title, val);
            break;
        case HUNK_TLV_TXT:
            $mv(hk->text, val);
            break;
        case HUNK_TLV_TOK:
            hk->toks[0] = (tok32c *)val[0];
            hk->toks[1] = (tok32c *)val[1];
            break;
        case HUNK_TLV_HILI:
            hk->hili[0] = (tok32c *)val[0];
            hk->hili[1] = (tok32c *)val[1];
            break;
        default:
            break;
        }
    }
    done;
}
