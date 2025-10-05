#ifndef ABC_JDR_H
#define ABC_JDR_H
#include "RDX.h"
#include "RDXC.h"
#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/OK.h"
#include "abc/UTF8.h"
#define RYU_OPTIMIZE_SIZE
#include "ryu/ryu.h"

#define JDRenum 0

con ok64 JDRbad = 0x4cd6e6968;
con ok64 JDRfail = 0x1335baa5b70;
con ok64 JDRbadF = 0x1335b9a5a0f;
con ok64 JDRbadI = 0x1335b9a5a12;
con ok64 JDRbadR = 0x1335b9a5a1b;
con ok64 JDRbadS = 0x1335b9a5a1c;
con ok64 JDRbadT = 0x1335b9a5a1d;
con ok64 JDRbadnest = 0x4cd6e6968ca9df8;
con ok64 JDRnoroom = 0x1335bcb3db3cf1;
con ok64 JDRsyntax = 0x1335bdfdcb897c;

typedef struct {
    u8c$ text;
    u8$ tlv;

    u8pBp stack;

    u32 line;
    u32 col;

    $u8c val;
    u8 pre;
} JDRstate;

fun ok64 RDXid128feed($u8 txt, id128 id) {
    if (unlikely($len(txt) < 3)) return RDXnoroom;
    a$dup(u8, t, txt);
    ok64 o = OK;
    if (id128src(id)) {
        o = RONfeed64(t, id128src(id));
        if (o == OK) o = $u8feed1(t, **ID128DELIM);
    }
    if (o == OK) o = RONfeed64(t, id128time(id));
    if (o == OK) $mv(txt, t);
    return o;
}

fun ok64 RDXid128drain(id128* id, $cu8c txt) {
    a$dup(u8c, t, txt);
    u8c* p = $u8find(t, *ID128DELIM);
    ok64 o = OK;
    id128 res = {};
    if (p == nil) {  // FIXME not INT
        o = RONdrain64(&id128time(res), t);
    } else {
        $u8c src = {t[0], p};
        $u8c time = {p + 1, t[1]};
        o = RONdrain64(&id128time(res), time);
        if (o == OK) o = RONdrain64(&id128src(res), src);
    }
    if (o == OK) *id = res;
    return o;
}

fun ok64 RDXFtxt2tlv($u8 tlv, $cu8c txt, id128 time) {
    sane($ok(tlv) && $ok(txt));
    size_t tl = $len(txt);
    test(tl < 32, RDXbad);
    u8 str[32];
    memcpy(str, *txt, tl);
    str[tl] = 0;
    double d = strtod((char*)str, nil);
    call(RDXCfeedF, tlv, d, time);
    done;
}

fun ok64 RDXFtlv2txt($u8 txt, $cu8c tlv) {
    sane($ok(txt) && $ok(tlv));
    u128 time;
    RDXfloat v;
    call(RDXCdrainF, &v, &time, tlv);
    u8 res[32];
    int len = d2s_buffered_n(v, (char*)res);
    $u8c $res = {res, res + len};
    call($u8feed, txt, $res);
    done;
}

fun ok64 RDXItxt2tlv($u8 tlv, $cu8c txt, id128 time) {
    sane($ok(tlv) && $ok(txt));
    size_t tl = $len(txt);
    test(tl < 32, RDXbad);
    u8 str[32];
    memcpy(str, *txt, tl);
    str[tl] = 0;
    i64 i = strtol((char*)str, nil, 10);
    call(RDXCfeedI, tlv, i, time);
    done;
}

fun ok64 RDXItlv2txt($u8 txt, $cu8c tlv) {
    sane($ok(txt) && $ok(tlv));
    RDXint v = 0;
    id128 time = {};
    call(RDXCdrainI, &v, &time, tlv);
    u8 res[32];
    int len = sprintf((char*)res, "%li", v);
    $u8c $res = {res, res + len};
    call($u8feed, txt, $res);
    done;
}

fun ok64 RDXRtxt2tlv($u8 tlv, $cu8c txt, id128 time) {
    sane($ok(tlv) && $ok(txt));
    id128 id = {};
    call(RDXid128drain, &id, txt);
    call(RDXCfeedR, tlv, id, time);
    done;
}

fun ok64 RDXRtlv2txt($u8 txt, $cu8c tlv) {
    sane($ok(txt) && $ok(tlv));
    id128 time;
    RDXref v = {};
    a$dup(u8c, dup, tlv);
    call(RDXCdrainR, &v, &time, dup);
    if (id128src(v) == 0) call($u8feed1, txt, '0');
    call(RDXid128feed, txt, v);
    done;
}

ok64 JDRfeedSesc($u8 tlv, $u8c txt);

ok64 JDRdrainSesc($u8 txt, $u8c tlv);

fun ok64 JDRdrainS($u8 txt, $u8c tlv) {
    sane($ok(txt) && $ok(tlv));
    call($u8feed1, txt, '"');
    call(JDRdrainSesc, txt, tlv);
    call($u8feed1, txt, '"');
    done;
}

ok64 JDRlexer(JDRstate* state);

fun ok64 JDRparse($u8 tlv, $u8 errmsg, $u8c jdr) {
    aBcpad(u8p, stack, RDX_MAX_NEST);
    a$dup(u8c, j, jdr);
    JDRstate state = {
        .text = j,
        .stack = (u8pBp)stackbuf,
        .tlv = tlv,
    };
    ok64 o = JDRlexer(&state);
    if (o != OK) {
        size_t off = j[0] - jdr[0];
        $printf(errmsg, "%s at %ld:%ld\n", okstr(o), (size_t)state.line + 1,
                off - state.col);
        u8cp p = *j;
        $u8c line = {p, p};
        while (line[0] > jdr[0] && $len(line) < 32 && *(line[0] - 1) != '\n')
            --line[0];
        size_t n = $len(line);
        while (line[1] < jdr[1] && $len(line) < 64 && *(line[1]) != '\n')
            ++line[1];
        $u8feed(errmsg, line);
        $u8feedcn(errmsg, ' ', n);
        $u8feed2(errmsg, '^', '\n');
    }
    return o;
}

fun ok64 JDRdrain($u8 tlv, $u8c jdr) {
    sane($ok(tlv) && $ok(jdr));
    aBcpad(u8p, stack, RDX_MAX_NEST);
    JDRstate state = {
        .text = jdr,
        .stack = (u8pBp)stackbuf,
        .tlv = tlv,
    };
    call(JDRlexer, &state);
    done;
}

ok64 JDRfeed1($u8 jdr, $u8c tlv, u64 style);

fun ok64 JDRfeed($u8 jdr, $u8c tlv) {
    sane($ok(jdr) && $ok(tlv));
    do {
        call(JDRfeed1, jdr, tlv, 0);
        if (!$empty(tlv)) call($u8feed2, jdr, ',', '\n');
    } while (!$empty(tlv));
    done;
}

enum {
    StyleStampSpace = 1 << 8,
    StyleStamps = 1 << 9,
    StyleCommaNL = 1 << 10,
    StyleTopCommaNL = 1 << 11,
    StyleIndentTab = 1 << 12,
    StyleIndentSpace4 = 1 << 13,
    StyleTrailingComma = 1 << 14,
    StyleBracketTuples = 1 << 15,
    StyleSkipComma = 1 << 16,
};

static const int StyleCommaSpacers =
    StyleCommaNL | StyleIndentSpace4 | StyleIndentTab;

#endif
