#include "RDX.h"

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

#include "CLI.h"
#include "JDR.h"
#include "RDXC.h"
#include "UNIT.h"
#include "Y.h"
#include "abc/$.h"
#include "abc/01.h"
#include "abc/BUF.h"
#include "abc/FILE.h"
#include "abc/LSM.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/TLV.h"

con ok64 RDXbadverb = 0x6cd866968ea9da6;

b8 JDR_OUT = 0;

fun ok64 TLVsplit($$u8c idle, $cu8c data) {
    sane($ok(idle) && $ok(data));
    a$dup(u8c, d, data);
    while (!$empty(d)) {
        $u8c next = {};
        call(TLVdrain$, next, d);
        call($$u8cfeed1, idle, next);
    }
    done;
}

fun ok64 RDXtry($cu8c data) {
    a$dup(u8c, d, data);
    ok64 o = OK;
    while (!$empty(d) && o == OK) {
        $u8c rec = {};
        o = TLVdrain$(rec, d);
        if (o == OK && !RDXisPLEX(**rec) && !RDXisFIRST(**rec)) return RDXbad;
    }
    return o;
}

ok64 RDX_merge(void* ctx, $u8c args) {
    sane($ok(args));
    aBcpad(Bu8, b, LSM_MAX_INPUTS);
    aBcpad($u8c, in, LSM_MAX_INPUTS);
    Bzero(bbuf);
    Bzero(inbuf);

    call(RDXingestall, bbuf, args);
    size_t total = 0;
    $for(Bu8, b, BBu8cdata(bbuf)) {
        $$u8cfeed1(inidle, Bu8cdata(*b));
        total += Bdatalen(*b);
    }

    u8** resbuf = (u8**)ctx;
    call(Bu8map, resbuf, roundup(total * 2, PAGESIZE));
    // call(Y, Bu8idle(resbuf), B$u8cdata(inbuf));
    call(Y, Bu8idle(resbuf), B$u8cdata(inbuf));

    $for(Bu8, b, BBu8cdata(bbuf)) Bu8unmap(*b);
    done;
}

ok64 RDX_cat(void* ctx, $u8c args) {
    sane(ctx != nil && $ok(args));
    aBcpad(Bu8, b, LSM_MAX_INPUTS);
    Bzero(bbuf);
    u8** resbuf = (u8**)ctx;

    call(RDXingestall, bbuf, args);
    size_t total = 0;
    $for(Bu8, b, BBu8cdata(bbuf)) total += Bdatalen(*b);

    call(Bu8map, resbuf, roundup(total * 2, PAGESIZE));
    $eat(bdata) call($u8feedall, Bu8idle(resbuf), Bu8cdata(**bdata));

    $for(Bu8, b, BBu8cdata(bbuf)) Bu8unmap(*b);
    done;
}

fun b8 is_tilda($u8c data) {
    u8 _tilda[] = {'t', 2, 0, '~'};
    $u8c tilda = $u8raw(_tilda);
    return $eq(data, tilda);
}

ok64 yfn($cu8c cases) {
    sane($ok(cases));
    a$dup(u8c, tlv, cases);
    while (!$empty(tlv)) {
        $u8c in = {};
        aBpad2($u8c, elem, PAGESIZE);
        $u8c correct = {};
        aBcpad(u8, res, PAGESIZE);
        call(TLVdrain$, in, tlv);
        do {
            $$u8cfeed1(elemidle, in);
            call(TLVdrain$, in, tlv);
        } while (!is_tilda(in));
        call(TLVdrain$, correct, tlv);

        call(Y, residle, elemdata);

        if (!$eq(correct, resdata)) {
            UNITfail(correct, resdata);
            fail(FAILeq);
        }
    }
    done;
}

ok64 RDX_test(void* ctx, $u8c args) {
    sane(1);
    while (!$empty(args)) {
        Bu8 test = {};
        u8 t = 0;
        $u8c val = {};
        id128 _;
        call(RDXdrain, &t, &_, val, args);
        test(t == RDX_STRING || t == RDX_TERM, badarg);
        call(RDXingest, test, val);  // TODO .rdx .jdr
        call(UNITdrain, test, yfn);
        call(Bu8unmap, test);
    }
    done;
}

ok64 RDX_delta(void* ctx, $u8c args) {
    sane(1);
    // call(FILEmapro, (voidB)rdxjbuf, path);
    // call(FILEunmap, rdxjbuf);
    done;
}

// parse, test, print
// -[ ] 1GB stdin?!!  expando?!  Bu8remap()
// -[ ] 3way?
CLIcmd COMMANDS[] = {
    {$u8str("cat"), RDX_cat,
     $u8str("testA.rdx testB.jdr -- parse/print RDX data")},  //
    {$u8str("test"), RDX_test,
     $u8str("testA.rdx testB.jdr -- run RDX test script(s)")},  //
    {$u8str("merge"), RDX_merge,
     $u8str("fileA.jdr fileB.rdx -- merge RDX file(s)")},  //
    {$u8str("delta"), RDX_delta,
     $u8str("old.rdx new.rdx     -- produce an RDX delta")},  //
    {$u8str(""), nil, $u8str("")},
};

ok64 RDXcli() {
    sane(1);

    if (!$empty(Bdata(STD_ARGS))) {
        u8c$ fn = $at(B$u8cdata(STD_ARGS), 0);
        a$last(u8c, nm, fn, 3);
        a$strc(jdr, "jdr");
        if ($eq(nm, jdr)) JDR_OUT = 1;
    }

    Bu8 resbuf = {};

    try(CLI, COMMANDS, (void*)resbuf);
    nedo fprintf(stderr, "Error: %s\n", ok64str(__));

    then if (!Bu8empty(resbuf)) {
        if (JDR_OUT) {
            Bu8 jdrbuf = {};
            call(Bu8map, jdrbuf, roundup(Bdatalen(resbuf) * 2, PAGESIZE));
            call(JDRfeed, Bu8idle(jdrbuf), Bu8cdata(resbuf));
            call($u8feed1, Bu8idle(jdrbuf), '\n');
            Bu8unmap(resbuf);
            Bmv(resbuf, jdrbuf);
        }
        call(FILEfeedall, STDOUT_FILENO, Bu8cdata(resbuf));
    }

    Bu8unmap(resbuf);
    done;
}

MAIN(RDXcli);
