//  graf CLI — thin wrapper: parse, open, exec, close.
//
#include "GRAF.h"

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/CLI.h"

ok64 grafcli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, GRAF_CLI_VERBS, GRAF_CLI_VAL_FLAGS);

    // Most graf verbs read .dogs/graf/; index writes. Use rw=YES to
    // keep parity with the previous behavior (always mkdir -p).
    graf g = {};
    call(GRAFOpen, &g, c.repo, YES);
    ok64 ret = GRAFExec(&g, &c);
    GRAFClose(&g);
    return ret;
}

MAIN(grafcli);
