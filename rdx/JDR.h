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

con ok64 JDRbad = 0x289664e135b;
con ok64 JDRfail = 0xc2d96a4e135b;
con ok64 JDRbadint = 0x38cada2599b353;
con ok64 JDRbadnest = 0xe37a72a2599b353;
con ok64 JDRnoroom = 0x31cf3db3c9b353;

typedef struct {
    u8c$ text;
    u8$ tlv;

    u8pB stack;

    u32 line;
    u32 col;

    $u8c val;
    u8 pre;
} JDRstate;

fun ok64 RDXid128feed($u8 txt, id128 id) {
    if (unlikely($len(txt) < 3)) return RDXnospace;
    a$dup(u8, t, txt);
    ok64 o = OK;
    if (RDXsrc(id)) {
        o = u64hexfeed(t, RDXsrc(id));
        if (o == OK) o = $u8feed1(t, **ID128DELIM);
    }
    if (o == OK) o = u64hexfeed(t, RDXtime(id));
    if (o == OK) $mv(txt, t);
    return o;
}

fun ok64 RDXid128drain(id128* id, $cu8c txt) {
    a$dup(u8c, t, txt);
    u8c* p = $u8find(t, *ID128DELIM);
    ok64 o = OK;
    id128 res = {};
    if (p == nil) {  // FIXME not INT
        o = u64hexdrain(&RDXtime(res), t);
    } else {
        $u8c src = {t[0], p};
        $u8c time = {p + 1, t[1]};
        ok64 o = u64hexdrain(&RDXtime(res), time);
        if (o == OK) o = u64hexdrain(&RDXsrc(res), src);
    }
    if (o == OK) *id = res;
    return o;
}

fun pro(RDXFtxt2tlv, $u8 tlv, $cu8c txt, id128 time) {
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

fun pro(RDXFtlv2txt, $u8 txt, $cu8c tlv) {
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

fun pro(RDXItxt2tlv, $u8 tlv, $cu8c txt, id128 time) {
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

fun pro(RDXItlv2txt, $u8 txt, $cu8c tlv) {
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

fun pro(RDXRtxt2tlv, $u8 tlv, $cu8c txt, id128 time) {
    sane($ok(tlv) && $ok(txt));
    id128 id = {};
    call(RDXid128drain, &id, txt);
    call(RDXCfeedR, tlv, id, time);
    done;
}

fun pro(RDXRtlv2txt, $u8 txt, $cu8c tlv) {
    sane($ok(txt) && $ok(tlv));
    id128 time;
    RDXref v = {};
    call(RDXCdrainR, &v, &time, tlv);
    if (RDXsrc(v) == 0) call($u8feed1, txt, '0');
    call(RDXid128feed, txt, v);
    done;
}

ok64 JDRfeedSesc($u8 tlv, $u8c txt);

ok64 JDRdrainSesc($u8 txt, $u8c tlv);

fun pro(JDRdrainS, $u8 txt, $u8c tlv) {
    sane($ok(txt) && $ok(tlv));
    call($u8feed1, txt, '"');
    call(JDRdrainSesc, txt, tlv);
    call($u8feed1, txt, '"');
    done;
}

ok64 JDRlexer(JDRstate* state);

fun ok64 JDRdrain($u8 tlv, $u8c rdxj) {
    sane($ok(tlv) && $ok(rdxj));
    aBcpad(u8p, stack, RDX_MAX_NEST);
    a$dup(u8c, tlv0, tlv);
    JDRstate state = {
        .text = rdxj,
        .stack = (u8pB)stackbuf,
        .tlv = tlv,
    };
    call(JDRlexer, &state);
    int fd = FILE_CLOSED;
    a$strc(_fn2, "_rdx.tlv");
    FILEcreate(&fd, _fn2);
    tlv0[1] = tlv[0];
    FILEfeedall(fd, tlv0);
    FILEclose(&fd);
    done;
}

ok64 JDRfeed1($u8 rdxj, $u8c tlv, u64 style);

fun pro(JDRfeed, $u8 rdxj, $u8c tlv) {
    sane($ok(rdxj) && $ok(tlv));
    do {
        call(JDRfeed1, rdxj, tlv, 0);
        if (!$empty(tlv)) call($u8feed2, rdxj, ',', '\n');
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
