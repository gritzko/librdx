//
// Created by gritzko on 11/20/25.
//
#include "RDX.h"
#include "abc/PRO.h"

ok64 UTF8Escape(u8s txt, u8cs val) {
    sane(u8sOK(txt) && u8csOK(val));
    switch (**val) {
        case '\t':
            call(u8sFeed2, txt, '\\', 't');
            break;
        case '\r':
            call(u8sFeed2, txt, '\\', 'r');
            break;
        case '\n':
            call(u8sFeed2, txt, '\\', 'n');
            break;
        case '\b':
            call(u8sFeed2, txt, '\\', 'b');
            break;
        case '\f':
            call(u8sFeed2, txt, '\\', 'f');
            break;
        case '\\':
            call(u8sFeed2, txt, '\\', '\\');
            break;
        // Note: '/' escaping is optional in JSON/JDR, skip it for roundtrip consistency
        case '"':
            call(u8sFeed2, txt, '\\', '"');
            break;
        default:
            if (**val < 0x20) {
                // Control characters: use \u00XX escape
                static const char hex[] = "0123456789abcdef";
                u8 c = **val;
                call(u8sFeed2, txt, '\\', 'u');
                call(u8sFeed2, txt, '0', '0');
                call(u8sFeed2, txt, hex[c >> 4], hex[c & 0xf]);
            } else {
                call(u8sFeed1, txt, **val);
            }
    }
    ++*val;
    done;
}

ok64 UTF8UnEscape(u8s tlv, u8cs txt) {
    if ($empty(tlv)) return NOROOM;
    if ($empty(txt)) return ok64sub(RDXBAD, RON_r);
    if (**txt != '\\') {
        **tlv = **txt;
        ++*tlv;
        ++*txt;
        return OK;
    }
    ++*txt;
    if ($empty(txt)) return ok64sub(RDXBAD, RON_s);
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
        case 'u': {
            if ($len(txt) < 5) return HEXNODATA;
            u8cs hex = {*txt + 1, *txt + 5};
            *txt += 4;
            u64 cp = 0;
            ok64 o = u64hexdrain(&cp, hex);
            if (o != OK) return o;
            o = utf8sFeed32(tlv, cp);
            if (o != OK) return o;
            --*tlv;
            break;
        }
        default:
            return ok64sub(RDXBAD, RON_t);
    }
    ++*tlv;
    ++*txt;
    return OK;
}

ok64 UTF8EscapeAll(u8s into, u8cs from) {
    sane(u8sOK(into) && u8csOK(from));
    while (!u8csEmpty(from)) {
        call(UTF8Escape, into, from);
    }
    done;
}

ok64 UTF8UnEscapeAll(u8s tlv, u8cs txt) {
    sane(u8sOK(tlv) && u8csOK(txt));
    while (!$empty(txt) && !$empty(tlv)) {
        if (**txt != '\\') {
            **tlv = **txt;
            ++*tlv;
            ++*txt;
            continue;
        }
        call(UTF8UnEscape, tlv, txt);
    }
    if (!$empty(txt)) return NOROOM;
    return OK;
}

ok64 UTF8EscTick(u8s into, u8cs from) { return NOTIMPLYET; }
ok64 UTF8UnEscTick(u8s into, u8cs from) { return NOTIMPLYET; }
ok64 UTF8EscTickAll(u8s into, u8cs from) { return NOTIMPLYET; }
ok64 UTF8UnEscTickAll(u8s into, u8cs from) { return NOTIMPLYET; }

ok64 UTFRecodeCB(u8cs from, u8 enc, u8 coder, u8csCB cb, voidp ctx) {
    sane(u8csOK(from) && enc < RDX_UTF_ENC_LEN && coder < UTF8_CODER_LEN && cb);
    a_pad(u8, pad, 256);
    UTFRecode re = UTABLE[enc][coder];
    a_dup(u8c, src, from);
    ok64 o = OK;
    do {
        o = re(pad_idle, src);
        if ($len(pad_data)) call(cb, pad_datac, ctx);
        u8bReset(pad);
    } while (o == NOROOM);
    return o;
}
