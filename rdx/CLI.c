#include "CLI.h"

#include <limits.h>
#include <unistd.h>

#include "abc/01.h"
#include "abc/PRO.h"

a$strc(UNITokmsg, "\tOK\n");
a$strc(UNITfailmsg, "\tFAIL\n");

a$strc(RDXlits, "FIRSTPLEX");

ok64 JDRdrainargs($u8 into) {
    a$dup($u8c, jdr, Bdata(STD_ARGS));
    ++jdr[0];  // program name
    sane($ok(into) && $ok(jdr));
    aBcpad(u8, pad, PAGESIZE);
    a$dup($u8c, j, jdr);
    while (!$empty(j)) {
        a$dup(u8, _, into);
        a$dup(u8c, next, **j);
        ok64 o = JDRdrain(_, next);
        if (ok64is(o, noroom)) return o;
        if (o != OK) {
            a$dup(u8c, str, **j);
            while (!$empty(str) && *$last(str) == ',') $u8retract(str, 1);
            call($u8feed1, padidle, '"');
            call(JDResc, padidle, str);
            call($u8feed1, padidle, '"');
        } else {
            call($u8feedall, padidle, **j);
        }
        call($u8feed1, padidle, ' ');
        ++*j;
    }
    return JDRdrain(into, paddata);
}

ok64 CLIhelp(CLIcmd const commands[]) {
    aBcpad(u8, help, PAGESIZE);
    for (int i = 0; commands[i].fn != nil; ++i) {
        $u8feed(helpidle, commands[i].name);
        $u8feed1(helpidle, '\t');
        $u8feed(helpidle, commands[i].legend);
        $u8feed1(helpidle, '\n');
    }
    return FILEfeedall(STDERR_FILENO, helpdata);
}

ok64 CLI(CLIcmd const commands[], void* context) {
    sane(1);
    aBcpad(u8, cmds, PAGESIZE);
    call(JDRdrainargs, cmdsidle);
    u8c$ cmds = cmdsdata;
    a$strc(help, "help");
    a$strc(mmhelp, "--help");

    while (!$empty(cmds)) {
        u8 t = 0;
        id128 id = {};
        $u8c val = {};
        $u8c verb = {};
        a$strc(args, "");
        u64 sub = 0;
        call(RDXdrain, &t, &id, val, cmds);
        if (t == RDX_TUPLE) {
            id128 _;
            test(!$empty(val), CLIbadarg);
            call(RDXdrain, &t, &_, verb, val);
            test(t == RDX_TERM, CLInoverb);
            $mv(args, val);
        } else if (t == RDX_TERM) {
            $mv(verb, val);
        } else if (t == RDX_STRING && $eq(mmhelp, val)) {
            return CLIhelp(commands);
        } else {
            fail(CLInoverb);
        }

        if ($eq(help, verb)) {
            return CLIhelp(commands);
        }

        int v = 0;
        while (!$eq(commands[v].name, verb) && commands[v].fn != nil) ++v;
        test(commands[v].fn != nil, CLIunknown);

        call(commands[v].fn, context, args);
    }
    test($empty(cmds), CLIbadarg);

    done;
}

fun b8 is_jdr_file($u8c fn) {
    if ($len(fn) < 5) return NO;
    a$last(u8c, nm, fn, 4);
    a$strc(jdr, ".jdr");
    return $eq(nm, jdr);
}

ok64 RDXingest(Bu8 buf, $u8c fn) {
    sane(Bvoid(buf) && $ok(fn));
    int fd = FILE_CLOSED;
    call(FILEopen, &fd, fn, O_RDONLY);
    call(FILEmapro2, buf, &fd);
    call(FILEclose, &fd);
    if (is_jdr_file(fn)) {  // TODO TLVvalid
        Bu8 b2 = {};
        call(Bu8map, b2, roundup(Bdatalen(buf) * 2, PAGESIZE));
        aBcpad(u8, err, 128);
        call(JDRparse, Bu8idle(b2), erridle, Bu8cdata(buf));
        Bu8unmap(buf);
        Bmv(buf, b2);
    }
    done;
}

ok64 RDXstdingest(Bu8 buf) {
    sane(Bvoid(buf));
    call(Bu8map, buf, MB);
    call(FILEdrainall, Bu8idle(buf), STDIN_FILENO);
    if (OK != TLVvalid(Bu8cdata(buf), RDXlits)) {
        fprintf(stderr, "it is JDR!!!\n");
        Bu8 b2 = {};
        call(Bu8map, b2, roundup(Bdatalen(buf) * 2, PAGESIZE));
        aBcpad(u8, err, 128);
        call(JDRparse, Bu8idle(b2), erridle, Bu8cdata(buf));
        Bu8unmap(buf);
        Bmv(buf, b2);
    }
    // TODO remap
    done;
}

ok64 RDXingestall(BBu8 buf, $u8c args) {
    sane(Bok(buf) && $ok(args));
    Bu8$ bidle = BBu8idle(buf);
    if ($empty(args)) {
        call(RDXstdingest, **bidle);
        $fed(bidle);
    }
    while (!$empty(args)) {
        u8 t = 0;
        $u8c val = {};
        id128 _;
        call(RDXdrain, &t, &_, val, args);
        test(t == RDX_STRING || t == RDX_TERM, badarg);
        call(RDXingest, **bidle, val);
        $fed(bidle);
    }
    done;
}
