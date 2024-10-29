#include <fcntl.h>
#include <unistd.h>

#include "$.h"
#include "FILE.h"
#include "INT.h"
#include "OK.h"
#include "PRO.h"
#include "RDXJ.h"
#include "RDXY.h"

u8 *input[4] = {};
u8 *output[4] = {};
$u8c *ins[4] = {};

fun pro(TLVsplit, $$u8c idle, $cu8c data) {
    sane($ok(idle) && $ok(data));
    a$dup(u8c, d, data);
    while (!$empty(d)) {
        $u8c next = {};
        call(TLVdrain$, next, d);
        call($$u8cfeed1, idle, next);
    }
    done;
}

pro(RDXeatfile, int fd, b8 jdr) {
    sane(fd > FILE_CLOSED);
    if (jdr) {
        call(FILEdrainall, Bu8idle(output), fd);
        call(RDXJdrain, Bu8idle(input), Bu8cdata(output));
        Breset(output);
    } else {
        call(FILEdrainall, Bu8idle(input), fd);
    }
    call(TLVsplit, B$u8cidle(ins), Bu8cdata(input));
    done;
}

pro(RDXeatargs, b8 jdr) {
    sane(1);
    if ($arglen < 3) {
        call(RDXeatfile, STDIN_FILENO, jdr);
        skip;
    }
    a$str(jext, ".rdxj");
    for (int i = 2; i < $arglen; ++i) {
        int fd = FILE_CLOSED;
        call(FILEopen, &fd, $arg(i), O_RDONLY);
        a$last(u8c, ext, $arg(i), 5);
        b8 rdxj = $eq(ext, jext);
        call(RDXeatfile, fd, jdr || rdxj);
        call(FILEclose, fd);
    }
    done;
}

pro(RDXj) {
    sane(1);
    // call(FILEmapro, (voidB)rdxjbuf, path);
    // call(FILEunmap, rdxjbuf);
    done;
}

pro(RDXcli) {
    sane(1);

    call(Bu8map, output, 1UL << 32);
    call(Bu8map, input, 1UL << 32);
    call(B$u8cmap, ins, RDXY_MAX_INPUTS);

    a$str(CMD_J, "j");
    a$str(CMD_JDR, "jdr");
    a$str(CMD_Y, "y");
    a$str(CMD_TLV, "tlv");
    a$str(CMD_MERGE, "merge");
    a$str(ERR_CMD, "Command not recognized\n");

    if ($arglen == 1 || $eq($arg(1), CMD_TLV)) {
        call(RDXeatargs, YES);
        a$dup($u8c, in, B$u8cdata(ins));
        $eat(in) call($u8feed, Bu8idle(output), **in);
    } else if ($eq($arg(1), CMD_J) || $eq($arg(1), CMD_JDR)) {
        call(RDXeatargs, NO);
        a$dup($u8c, in, B$u8cdata(ins));
        call(RDXJfeed, Bu8idle(output), **in);
        ++*in;
        $eat(in) {
            call($u8feed2, Bu8idle(output), ',', '\n');
            call(RDXJfeed, Bu8idle(output), **in);
        }
        call($u8feed1, Bu8idle(output), '\n');
    } else if ($eq($arg(1), CMD_Y) || $eq($arg(1), CMD_MERGE)) {
        call(RDXeatargs, NO);
        call(RDXY, Bu8idle(output), B$u8cdata(ins));
    } else {
        FILEerr(ERR_CMD);
    }

    call(FILEfeedall, STDOUT_FILENO, Bu8cdata(output));

    nedo(Bu8unmap(input), Bu8unmap(output), B$u8cunmap(ins));
}

MAIN(RDXcli);
