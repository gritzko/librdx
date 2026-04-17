//  bro CLI — thin wrapper: parse, open, exec, close.
//
#include "BRO.h"

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/CLI.h"

ok64 brocli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, NULL, NULL);  // no verbs, no val-flags

    bro b = {};
    call(BROOpen, &b, c.repo, NO);
    ok64 ret = BROExec(&b, &c);
    BROClose(&b);
    return ret;
}

MAIN(brocli);
