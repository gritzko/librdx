#ifndef RDX_UNIT_H
#define RDX_UNIT_H

#include <stdio.h>
#include <unistd.h>

#include "JDR.h"
#include "RDX.h"
#include "abc/01.h"
#include "abc/ANSI.h"
#include "abc/B.h"
#include "abc/BUF.h"
#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/TEST.h"
#include "abc/TLV.h"

typedef ok64 (*UNITfn)($cu8c rdx);

a$strc(UNITokmsg, "\tOK\n");
a$strc(UNITfailmsg, "\tFAIL\n");

fun ok64 HEXfeedsep($u8 hex, $u8c bin, u8 sep) {
    while (!$empty(bin) && $len(hex) >= 3) {
        **hex = $at(BASE16, **bin >> 4);
        ++*hex;
        **hex = $at(BASE16, **bin & 0xf);
        ++*hex;
        ++*bin;
        **hex = sep;
        ++*hex;
    }
    return OK;
}

fun ok64 UNITsafefeed($u8 into, $cu8c bin) {
    for (u8c *p = bin[0]; p < bin[1]; ++p) {
        if (*p >= ' ' && *p < 127) {
            u8s_feed1(into, *p);
        } else {
            u8s_feed1(into, '.');
        }
    }
    return OK;
}

fun ok64 HEXdump($u8 into, u8cs b) {
    sane($ok(into) && $ok(b));
    a$dup(u8c, bin, b);
    while (!$empty(bin) && $len(into) >= 16 + 32 + 16 + 2) {
        a$dup(u8c, chunk, bin);
        if ($len(chunk) > 16) {
            chunk[1] = chunk[0] + 16;
        }
        bin[0] = chunk[1];
        UNITsafefeed(into, chunk);
        for (int i = 0; i < 16 - $len(chunk); ++i) u8s_feed1(into, ' ');
        u8s_feed1(into, '\t');
        HEXfeedsep(into, chunk, ' ');
        u8s_feed1(into, '\n');
    }
    return OK;
}

fun ok64 UNITdump($u8 into, $u8c rdx) {
    a$dup(u8c, c2, rdx);
    a$dup(u8c, c3, rdx);
    JDRfeed(into, c2);
    u8s_feed1(into, '\n');
    HEXdump(into, c3);
    return OK;
}

ok64 UNITdrain(Bu8 tests, UNITfn fn) {
    Bu8 rdx = {};
    size_t dl = Bdatalen(tests);
    Bu8alloc(rdx, roundup(Bdatalen(tests) * 2, PAGESIZE));
    a$dup(u8c, jdr, Bu8data(tests));
    // ok64 o = JDRdrain(Bu8$2(rdx), jdr);
    aBcpad(u8, err, 128);
    ok64 o = JDRparse(Bu8$2(rdx), erridle, jdr);
    size_t rl = Bdatalen(rdx);
    if (o != OK) {
        a$str(msg, "JDR parse failed: ");
        FILEfeedall(STDERR_FILENO, msg);
        FILEfeedall(STDERR_FILENO, errdata);
    }
    a$dup(u8c, cases, Bu8$1(rdx));
    aBcpad(u8, msg, PAGESIZE);
    int cs = 0;
    $u8c caserdx = {};
    while (o == OK && !$empty(cases)) {
        $u8c rec = {};
        o = TLVdrain$(rec, cases);
        if (o != OK) break;
        switch (cs) {
            case 0:
                if ((~TLVaA & **rec) == RDX_STRING && $len(rec) == 3) {
                    cs = 1;
                }
                break;
            case 1:
                if ((~TLVaA & **rec) == RDX_STRING && $len(rec) > 3) {
                    cs = 2;
                    $u8c key, body = {};
                    u8 t;
                    TLVdrainkv(&t, key, body, rec);
                    while (!$empty(body) && *body[0] == '\n') ++body[0];
                    while (!$empty(body) && *(body[1] - 1) == '\n') --body[1];
                    $u8feedall(msgidle, body);
                } else {
                    cs = 0;
                }
                break;
            case 2:
                if ((~TLVaA & **rec) == RDX_STRING && $len(rec) == 3) {
                    cs = 3;
                    caserdx[0] = rec[1];
                } else {
                    cs = 0;
                }
                break;
            case 3:
                if ((~TLVaA & **rec) == RDX_STRING && $len(rec) == 3) {
                    caserdx[1] = rec[0];
                    cs = 1;
                    o = fn(caserdx);
                } else if ($empty(cases)) {
                    caserdx[1] = rec[1];
                    cs = 1;
                    o = fn(caserdx);
                } else {
                    break;
                }
                if (o == OK) {
                    escfeed(msgidle, LIGHT_GREEN);
                    $u8feedall(msgidle, UNITokmsg);
                    escfeed(msgidle, 0);
                } else {
                    escfeed(msgidle, LIGHT_RED);
                    $u8feedall(msgidle, UNITfailmsg);
                    escfeed(msgidle, 0);
                    UNITdump(msgidle, caserdx);
                }
                FILEfeedall(STDOUT_FILENO, msgdata);
                Breset(msgbuf);
                break;
        }
    }
    Bu8free(rdx);
    return o;
}

fun ok64 UNITfail(u8cs correct, u8cs fact) {
    aBpad2(u8, pad, PAGESIZE);
    a$strc(expstr, "\nEXPECTED:\n");
    a$strc(factstr, "\nFACT:\n");
    Bump(padbuf, PAGESIZE / 2);
    $u8feed(paddata, expstr);
    a$dup(u8c, c2, correct);
    JDRfeed(paddata, c2);
    u8s_feed1(paddata, '\n');
    HEXdump(paddata, correct);
    Back(padbuf);
    $u8feed(padidle, factstr);
    a$dup(u8c, f2, fact);
    JDRfeed(padidle, f2);
    u8s_feed1(padidle, '\n');
    HEXdump(padidle, fact);
    Backpast(padbuf);
    return FILEfeed(STDOUT_FILENO, Bu8cdata(padbuf));
}

fun ok64 JDRdrainargs($u8 into, u8css jdr) {
    a$dup(u8cs, j, jdr);
    id128 id128zero = {};
    while (!$empty(j)) {
        a$dup(u8, dup, into);
        a$dup(u8c, next, **j);
        ok64 o = JDRdrain(into, next);
        if (ok64is(o, noroom)) return o;
        if (o != OK) {
            $mv(into, dup);
            a$dup(u8c, str, **j);
            while (!$empty(str) && *$last(str) == ',') $u8retract(str, 1);
            o = RDXfeed(into, RDX_STRING, id128zero, str);
            if (o != OK) return o;
        }
        ++*j;
    }
    return OK;
}

#endif
