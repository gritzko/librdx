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
    u32 tlvpos;
    u32 toks : 27, node : 5;
} rdx64;

typedef struct {
    $u8c text;
    u8B tlv;
    u64B stack;

    u8 lit;
    u128 id;
    u8B pad;
} RDXJstate;

fun ok64 RDXid128feed($u8 txt, id128 id) {
    u8* p = *txt;
    ok64 o = u64hexfeed(txt, RDXtime(id));
    if ($len(txt) <= 1) o = RDXnospace;
    if (o == OK) {
        $feed1(txt, **ID128DELIM);
        o = u64hexfeed(txt, RDXsrc(id));
    }
    if (o != OK) *txt = p;
    return o;
}

fun ok64 RDXid128drain(id128* id, $cu8c txt) {
    u8c* p = $u8find(txt, *ID128DELIM);
    if (p == nil) return RDXbad;
    $u8c time = {txt[0], p};
    $u8c src = {p + 1, txt[1]};
    id128 res = {};
    ok64 o = u64hexdrain(&RDXtime(res), time);
    if (o == OK) o = u64hexdrain(&RDXsrc(res), src);
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

fun pro(RDXJfeedS, $u8 tlv, $cu8c txt, id128 time) {
    sane($ok(tlv) && $ok(txt) && $len(txt) >= 2 && *$head(txt) == '"' &&
         *$last(txt) == '"');
    $u8c txt2 = {txt[0] + 1, txt[1] - 1};
    a$dup(u8, into, tlv);
    aBcpad(u8, id, 16);
    call(ZINTu128feed, ididle, time);
    aBcpad(u8, pad, 255);
    $u8feed1(padidle, $len(iddata));
    $u8feed(padidle, iddata);
    a$dup(u8c, keydata, paddata);
    ok64 o = RDXJfeedSesc(padidle, txt2);
    if (o == OK) {
        call(TLVfeed, tlv, RDX_STRING, paddata);
    } else if (o == RDXnospace) {
        call($u8feed1, tlv, RDX_STRING);
        u32* len = (u32*)*tlv;
        *tlv += sizeof(u32);
        u32 oldlen = $len(tlv);
        call($u8feed, tlv, keydata);
        $mv(txt2, txt);
        call(RDXJfeedSesc, tlv, txt2);
        *len = oldlen - $len(tlv);  // TODO nicer
    } else {
        fail(o);
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

fun ok64 RDXJfeed($u8 tlv, $u8c rdxj) { return notimplyet; }

ok64 RDXJdrain($u8 rdxj, $u8c tlv);

#endif
