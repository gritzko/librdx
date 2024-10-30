#ifndef ABC_RDXJ_H
#define ABC_RDXJ_H
#include "INT.h"
#include "OK.h"
#include "RDX.h"
#include "RDXC.h"
#define RYU_OPTIMIZE_SIZE
#include "ryu/ryu.h"

con int RDXJenum = 0;

con ok64 RDXJbad = 0x289664e135b;
con ok64 RDXJfail = 0xc2d96a4e135b;

typedef struct {
    u32* l;
    u32 toks;
    u8 lit;
} RDXJnest;

typedef struct {
    $u8c text;
    $u8 tlv;

    RDXJnest stack[RDX_MAX_NEST];
    u32 nest;

    u8 lit;
    u128 id;
    $u8c val;
} RDXJstate;

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

fun ok64 RDXJfeedSesc($u8 tlv, $u8c txt) {
    while (!$empty(txt) && !$empty(tlv)) {
        if (**txt != '\\') {
            **tlv = **txt;
            ++*tlv;
            ++*txt;
            continue;
        }
        ++*txt;
        if ($empty(txt)) return RDXJbad;
        switch (**txt) {
            case 't':
                **tlv = '\t';
                break;
            case 'r':
                **tlv = '\r';
                break;
            case 'n':
                **tlv = '\n';
                break;
            case 'b':
                **tlv = '\b';
                break;
            case 'f':
                **tlv = '\f';
                break;
            case '0':
                **tlv = 0;
                break;
            case '\\':
                **tlv = '\\';
                break;
            case '/':
                **tlv = '/';
                break;
            case '"':
                **tlv = '"';
                break;
            case 'u':
                return notimplyet;
            default:
                return RDXJbad;
        }
        ++*tlv;
        ++*txt;
    }
    if (!$empty(txt)) return RDXnospace;
    return OK;
}

fun pro(RDXJdrainSesc, $u8 txt, $u8c tlv) {
    sane($ok(txt) && $ok(tlv));
    if ($len(txt) < $len(tlv)) return RDXnospace;
    u8 t = 0;
    $u8c key = {};
    $u8c val = {};
    call(TLVdrainkv, &t, key, val, tlv);
    while (!$empty(val) && !$empty(txt)) {
        switch (**val) {
            case '\t':
                call($u8feed2, txt, '\\', 't');
                break;
            case '\r':
                call($u8feed2, txt, '\\', 'r');
                break;
            case '\n':
                call($u8feed2, txt, '\\', 'n');
                break;
            case '\b':
                call($u8feed2, txt, '\\', 'b');
                break;
            case '\f':
                call($u8feed2, txt, '\\', 'f');
                break;
            case '\\':
                call($u8feed2, txt, '\\', '\\');
                break;
            case '/':
                call($u8feed2, txt, '\\', '/');
                break;
            case '"':
                call($u8feed2, txt, '\\', '"');
                break;
            case 0:
                call($u8feed2, txt, '\\', '0');
                break;
                // TODO \u etc
            default:
                $u8feed1(txt, **val);
        }
        ++*val;
    }
    done;
}

fun pro(RDXJdrainS, $u8 txt, $u8c tlv) {
    sane($ok(txt) && $ok(tlv));
    call($u8feed1, txt, '"');
    call(RDXJdrainSesc, txt, tlv);
    call($u8feed1, txt, '"');
    done;
}

ok64 RDXJlexer(RDXJstate* state);

fun ok64 RDXJdrain($u8 tlv, $u8c rdxj) {
    aBcpad(u64, stack, RDX_MAX_NEST);
    RDXJstate state = {
        .tlv = $dup(tlv),
        .text = $dup(rdxj),
        .nest = 1,
    };
    ok64 o = RDXJlexer(&state);
    $mv(tlv, state.tlv);
    $mv(rdxj, state.text);
    return o;
}

ok64 _RDXJfeed($u8 rdxj, $u8c tlv, u8 prnt);

fun pro(RDXJfeed, $u8 rdxj, $u8c tlv) {
    sane($ok(rdxj) && $ok(tlv));
    do {
        call(_RDXJfeed, rdxj, tlv, 0);
        if (!$empty(tlv)) call($u8feed2, rdxj, ',', '\n');
    } while (!$empty(tlv));
    done;
}

#endif
