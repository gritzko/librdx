#include "RDX.h"

#include <fcntl.h>
#include <unistd.h>

#include "JDR.h"
#include "RDXC.h"
#include "UNIT.h"
#include "Y.h"
#include "abc/BUF.h"
#include "abc/FILE.h"
#include "abc/OK.h"
#include "abc/S.h"

con ok64 RDXbadverb = 0x6cd866968ea9da6;

u8* tmp[4] = {};
$u8c* ins[4] = {};

fun ok64 TLVsplit(u8css idle, $cu8c data) {
    sane($ok(idle) && $ok(data));
    a$dup(u8c, d, data);
    while (!$empty(d)) {
        u8cs next = {};
        call(TLVDrain$, next, d);
        call(u8cssFeed1, idle, next);
    }
    done;
}

fun ok64 RDXtry($cu8c data) {
    a$dup(u8c, d, data);
    ok64 o = OK;
    while (!$empty(d) && o == OK) {
        u8cs rec = {};
        o = TLVDrain$(rec, d);
        if (o == OK && !RDXisPLEX(**rec) && !RDXisFIRST(**rec)) return RDXbad;
    }
    return o;
}

pro(RDXeatfile, int fd) {
    sane(fd > FILE_CLOSED);
    Bu8 buf = {};
    call(FILEmapro2, buf, &fd);
    ok64 jdr = RDXtry(Bu8cdata(buf));
    if (jdr == OK) {
        call(TLVsplit, Bu8csidle(ins), Bu8cdata(buf));
    } else {
        aBcpad(u8, err, 128);
        try(JDRparse, u8bIdle(tmp), erridle, Bu8cdata(buf));
        nedo {
            FILEFeed(STDERR_FILENO, errdata);
            done;
        }
        call(TLVsplit, Bu8csidle(ins), Bu8cdata(tmp));
        Bu8Ate(tmp);
    }
    done;
}

pro(RDXeatfiles, u8cs args) {
    sane(1);
    if (!Bempty(ins)) {
    } else if ($empty(args) || TLVup(**args) != RDX_STRING) {
        call(RDXeatfile, STDIN_FILENO);
    }
    while (!$empty(args) && TLVup(**args) == RDX_STRING) {
        u8cs str = {};
        id128 id = {};
        RDXCdrainS(str, &id, args);
        int fd = FILE_CLOSED;
        call(FILEopen, &fd, str, O_RDONLY);
        call(RDXeatfile, fd);
        call(FILEclose, &fd);
    }
    done;
}

pro(RDX_print, u8cs args) {
    sane(1);
    Bate(tmp);
    a$dup(u8cs, in, Bu8csdata(ins));
    u8$ idle = u8bIdle(tmp);
    call(JDRfeed, idle, **in);
    ++*in;
    $eat(in) {
        call(u8sFeed2, idle, ',', '\n');
        call(JDRfeed, idle, **in);
    }
    call(u8sFeed1, idle, '\n');

    int fd = STDOUT_FILENO;
    if (TLVup(**args) == RDX_STRING) {
        u8cs str = {};
        RDXCdrainS(str, NULL, args);
        call(FILEcreate, &fd, str);
    }
    call(FILEFeedall, fd, Bu8cdata(tmp));
    if (fd != STDOUT_FILENO) FILEclose(&fd);
    done;
}

ok64 RDX_parse(u8cs args) {
    sane(1);
    call(RDXeatfiles, args);
    done;
}

ok64 RDX_write(u8cs args) {
    sane(1);
    u8 t = 0;
    id128 id = {};
    u8cs val = {};
    u64 sub = 0;
    call(RDXdrain, &t, &id, val, args);
    test(t == RDX_STRING, badarg);
    int fd = FILE_CLOSED;
    call(FILEcreate, &fd, val);
    $eat(Bu8csdata(ins)) call(FILEFeedall, fd, *ins[1]);
    call(FILEclose, &fd);
    Breset(ins);
    done;
}

ok64 RDX_merge(u8cs args) {
    sane(1);
    call(RDXeatfiles, args);
    Bate(tmp);
    u8$ idle = u8bIdle(tmp);
    call(Y, idle, Bu8csdata(ins));
    Breset(ins);
    call(u8cssFeed1, Bu8csidle(ins), Bu8cdata(tmp));
    Bate(tmp);
    done;
}

fun b8 is_tilda(u8cs data) {
    u8 _tilda[] = {'t', 2, 0, '~'};
    u8cs tilda = $u8raw(_tilda);
    return $eq(data, tilda);
}

ok64 yfn($cu8c cases) {
    sane($ok(cases));
    a$dup(u8c, tlv, cases);
    while (!$empty(tlv)) {
        u8cs in = {};
        aBpad2(u8cs, elem, PAGESIZE);
        u8cs correct = {};
        aBcpad(u8, res, PAGESIZE);
        call(TLVDrain$, in, tlv);
        do {
            u8cssFeed1(elemidle, in);
            call(TLVDrain$, in, tlv);
        } while (!is_tilda(in));
        call(TLVDrain$, correct, tlv);

        call(Y, residle, elemdata);

        if (!$eq(correct, resdata)) {
            UNITfail(correct, resdata);
            fail(FAILeq);
        }
    }
    done;
}

ok64 RDX_test(u8cs args) {
    sane(1);
    while (!$empty(args) && TLVup(**args) == RDX_STRING) {
        u8cs path = {};
        call(RDXCdrainS, path, NULL, args);
        Bu8 rdxjbuf = {};
        call(FILEmapro, rdxjbuf, path);
        call(UNITdrain, rdxjbuf, yfn);
        call(FILEunmap, rdxjbuf);
    }
    done;
}

ok64 RDX_clean(u8cs args) {
    sane(1);
    // call(FILEmapro, (voidB)rdxjbuf, path);
    // call(FILEunmap, rdxjbuf);
    done;
}

ok64 RDX_diff(u8cs args) {
    sane(1);
    // call(FILEmapro, (voidB)rdxjbuf, path);
    // call(FILEunmap, rdxjbuf);
    done;
}

typedef ok64 (*cmdfn)(u8cs args);

typedef struct {
    u8cs name;
    cmdfn fn;
} cmd_t;

cmd_t COMMANDS[] = {
    {$u8str("print"), RDX_print},  //
    {$u8str("j"), RDX_print},      //
    {$u8str("parse"), RDX_parse},  //
    {$u8str("p"), RDX_parse},      //
    {$u8str("write"), RDX_write},  //
    {$u8str("w"), RDX_write},      //
    {$u8str("test"), RDX_test},    //
    {$u8str("t"), RDX_test},       //
    {$u8str("merge"), RDX_merge},  //
    {$u8str("y"), RDX_merge},      //
    {$u8str("diff"), RDX_diff},    //
    {$u8str("d"), RDX_diff},       //
    {$u8str("clean"), RDX_clean},  //
    {$u8str("c"), RDX_clean},      //
    {$u8str(""), NULL},
};

ok64 RDXcli() {
    sane(1);
    a$dup(u8cs, stdargs, Bu8csdata(STD_ARGS));
    ++*stdargs;  // program name
    aBcpad(u8, cmds, PAGESIZE);
    call(JDRdrainargs, cmdsidle, stdargs);
    u8c$ cmds = cmdsdata;

    call(Bu8map, tmp, 1UL << 32);
    call(Bu8csalloc, ins, Y_MAX_INPUTS * 8);

    while (!$empty(cmds)) {
        u8 t = 0;
        id128 id = {};
        u8cs val = {};
        u8cs verb = {};
        u64 sub = 0;
        call(RDXdrain, &t, &id, val, cmds);
        if (t == RDX_TUPLE) {
            id128 _;
            test(!$empty(val), badarg);
            call(RDXdrain, &t, &_, verb, val);
            test(t == RDX_TERM, RDXbadverb);
            if (!$empty(val)) {
                u8cs s = {};
                call(RDXdrain, &t, &_, s, val);
                test(t == RDX_TERM, badarg);
                call(RONdrain64, &sub, s);
            }
        } else if (t == RDX_TERM) {
            $mv(verb, val);
        } else {
            fail(RDXbadverb);
        }
        int v = 0;
        while (!$eq(COMMANDS[v].name, verb) && COMMANDS[v].fn != NULL) ++v;
        test(COMMANDS[v].fn != NULL, RDXbadverb);

        call(COMMANDS[v].fn, cmds);
    }
    test($empty(cmds), badarg);

    Bu8unmap(tmp);
    Bu8csfree(ins);

    done;
}

MAIN(RDXcli);
