#include "$.h"
#include "FILE.h"
#include "INT.h"
#include "OK.h"
#include "RDXJ.h"
#include "RDXY.h"

u8 *input[4] = {};
u8 *output[4] = {};
$u8c *ins[4] = {};

fun pro(TLVsplit, $$u8c idle, $u8c data) {
    sane($ok(idle) && $ok(data));
    while (!$empty(data)) {
        $u8c next = {};
        call(TLVdrain$, next, data);
        call($$u8cfeed1, idle, next);
    }
    done;
}

fun ok64 RDXeatstd() { return FILEdrainall(Bu8idle(input), STDIN_FILENO); }

pro(RDXeatargs) {
    sane(1);
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

    call(RDXeatargs);

    if ($arglen == 1 || $eq($arg(1), CMD_TLV)) {
        if ($arglen < 3) {
            call(RDXeatstd);
            call(RDXJdrain, Bu8idle(output), Bu8cdata(input));
        } else {
            a$dup($u8c, in, B$u8cdata(ins));
            $eat(in) call(RDXJdrain, Bu8idle(output), **in);
        }
    } else if ($eq($arg(1), CMD_J) || $eq($arg(1), CMD_JDR)) {
        if ($arglen < 3) {
            call(RDXeatstd);
            call(RDXJfeed, Bu8idle(output), Bu8cdata(input));
        } else {
            call(RDXeatargs);
            a$dup($u8c, in, B$u8cdata(ins));
            call(RDXJfeed, Bu8idle(output), **in);
            ++*in;
            $eat(in) {
                call($u8feed2, Bu8idle(output), ',', '\n');
                call(RDXJfeed, Bu8idle(output), **in);
            }
        }
    } else if ($eq($arg(1), CMD_Y) || $eq($arg(1), CMD_MERGE)) {
        if ($arglen < 3) {
            call(RDXeatstd);
            call(TLVsplit, B$u8cidle(ins), Bu8cdata(input));
            call(RDXY, Bu8idle(output), B$u8cdata(ins));
        } else {
            fail(notimplyet);
        }
    } else {
        FILEerr(ERR_CMD);
    }

    call(FILEfeedall, STDOUT_FILENO, Bu8cdata(output));

    nedo(Bu8unmap(input), Bu8unmap(output), B$u8cunmap(ins));
}

MAIN(RDXcli);
