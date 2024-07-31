#ifndef ABC_LINE_H
#define ABC_LINE_H
#include "INT.h"
#include "PRO.h"
#include "TLV.h"
#include "ZINT.h"

con ok64 LINEeof = 0x2ace9397495;
con ok64 LINEbadrec = 0x9e9da8966397495;
con ok64 LINEnodata = 0x978968cf2397495;
con ok64 LINEnoroom = 0xc73cf6cf2397495;
con ok64 LINElimit = 0x38b71b70397495;

typedef struct line {
    u128 id;
    $u8c body;
    u8c$ rest;
    u8 _[7];
    u8 type;
} line;

// typedef int linecmpfn(line const* a, line const* b);

fun int linecmp(line const* a, line const* b) {
    int ret = u8cmp(&a->type, &b->type);
    if (ret == 0) {
        ret = u128cmp(&a->id, &b->id);
        if (ret == 0) {
            ret = $u8cmp(a->body, b->body);
        }
    }
    return ret;
}

fun int lineidhicmp(line const* a, line const* b) {
    return u64cmp(&a->id._64[1], &b->id._64[1]);
}

fun int linebodycmp(line const* a, line const* b) {
    return $cmp(a->body, b->body);
}

fun int linetypebodycmp(line const* a, line const* b) {
    int ret = u8cmp(&a->type, &b->type);
    if (ret == 0) $cmp(a->body, b->body);
    return ret;
}

#define X(M, n) M##line##n
#include "Bx.h"
#include "HEAPx.h"
#undef X

#define aLINE(n, s) line n = {.id = {0, 0}, .body = NULL, .rest = s, .type = 0}
#define aLINEc(n, s) \
    linec n = {.id = {0, 0}, .body = NULL, .rest = s, .type = 0}

fun pro(LINEdrain, line* l) {
    u8 t = 0, tt = 0;
    u32 hlen = 0, blen = 0;
    u32 thlen = 0, tblen = 0;
    call(TLVprobe, &t, &hlen, &blen, l->rest);
    a$part(u8c, idvaltlv, l->rest, hlen, blen);

    call(TLVprobe, &tt, &thlen, &tblen, idvaltlv);
    a$part(u8c, idtlv, idvaltlv, thlen, tblen);
    test(tt == '0' || tt == 'T', LINEbadrec);
    u128 tmp = {};
    call(ZINTu128drain, &tmp, idtlv);
    a$rpart(u8c, body, idvaltlv, tblen + thlen);
    l->type = t;
    l->id = u128xor(l->id, tmp);
    $set(l->body, body);
    l->rest[0] = body[1];

    done;
}

fun pro(LINEfeed, line* in, line const* a) {
    aBpad(u8, pad, 16);
    u8$ idin = Bu8idle(pad);
    u8c$ idis = Bu8cdata(pad);
    u128 x = u128xor(in->id, a->id);
    ZINTu128feed(idin, &x);
    size_t len = $len(a->body) + $len(idis) + ($len(idis) > 9 ? 2 : 1);
    u8$ into = (u8**)in->rest;
    test($len(into) >= len + 5, LINEnoroom);
    TLVhead(into, a->type, len);
    TLVtinyhead(into, 'T', $len(idis));  // todo micro
    $u8feed(into, idis);
    in->body[0] = into[0];
    $u8feed(into, a->body);
    in->body[1] = into[0];
    in->type = a->type;
    in->id = a->id;
    done;
}

typedef ok64 linemergefn(line* into, $cline from);
#define LINEmaxmergelen 64

fun pro(LINEpush, Bline into, $u8c rest, linecmpfn cmp) {
    test($len(rest) > 0, LINEnodata);
    aLINE(l, rest);
    call(LINEdrain, &l);
    call(HEAPlinepush, into, &l, cmp);
    done;
}

fun pro(LINEmerge, line* into, $line from, linecmpfn cmp, linemergefn merge) {
    test($len(from) <= LINEmaxmergelen, LINElimit);
    aBpad(line, pad, LINEmaxmergelen);
    line$ idle = Blineidle(pad);
    line$ merges = Blinedata(pad);
    do {
        $linefeedp(idle, *(linec$)from);
        lineswap($head(from), $last(from));
        --$term(from);
        HEAPlinedown(from, cmp);
    } while (!$empty(from) && 0 == cmp(*pad, *from));
    if ($len(merges) == 1) {
        call(LINEfeed, into, *merges);
    } else {
        call(merge, into, merges);
    }
    while (!$empty(merges)) {
        ok64 o = LINEdrain(*merges);
        if (o == OK) {
            linemv($term(from), *merges);
            ++$term(from);
            HEAPlineup(from, cmp);
        }
        ++*merges;
    }
    done;
}

#endif
