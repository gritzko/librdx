#include "RDX.h"

#include <unistd.h>

#include "$.h"
#include "B.h"
#include "FILE.h"
#include "INT.h"
#include "RDXJ.h"
#include "RDXY.h"

u8 *input[4] = {};
u8 *output[4] = {};

pro(RDXeatargs) {
    sane(1);
    aBpad2($u8c, in, RDXY_MAX_INPUTS);
    if ($arglen < 3) {
        call(FILEdrainall, Bu8idle(input), STDIN_FILENO);
        skip;
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

    a$str(CMD_J, "j");
    a$str(ERR_CMD, "Command not recognized\n");

    if ($arglen == 1) {
        call(RDXeatargs);
        call(RDXJdrain, Bu8idle(output), Bu8cdata(input));
    } else if ($eq($arg(1), CMD_J)) {
        call(RDXeatargs);
        call(RDXJfeed, Bu8idle(output), Bu8cdata(input));
    } else {
        FILEerr(ERR_CMD);
    }

    call(FILEfeedall, STDOUT_FILENO, Bu8cdata(output));

    nedo(Bu8unmap(input); Bu8unmap(output));
}

MAIN(RDXcli);
