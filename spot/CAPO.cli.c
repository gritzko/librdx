//  spot CLI — thin wrapper: parse, open, exec, close.
//
#include "CAPO.h"
#include "SPOT_VERSION.h"

#include <stdio.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/CLI.h"

ok64 capocli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, SPOT_CLI_VERBS, SPOT_CLI_VAL_FLAGS);

    if (CLIHas(&c, "-v") || CLIHas(&c, "--version")) {
        fprintf(stderr, "spot %s %s\n", SPOT_GIT_TAG, SPOT_COMMIT_HASH);
        done;
    }

    spot s = {};
    call(SPOTOpen, &s, c.repo, NO);
    ok64 ret = SPOTExec(&s, &c);
    SPOTClose(&s);
    return ret;
}

MAIN(capocli);
