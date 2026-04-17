//  keeper CLI — thin wrapper: parse, open, exec, close.
//
#include "KEEP.h"

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/CLI.h"

ok64 keepercli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, KEEP_CLI_VERBS, KEEP_CLI_VAL_FLAGS);

    a_cstr(v_status, "status");
    a_cstr(v_refs,   "refs");
    a_cstr(v_verify, "verify");
    b8 ro = $eq(c.verb, v_status) || $eq(c.verb, v_refs)
         || $eq(c.verb, v_verify);
    b8 rw = !ro;

    keeper k = {};
    call(KEEPOpen, &k, c.repo, rw);
    ok64 ret = KEEPExec(&k, &c);
    KEEPClose(&k);
    return ret;
}

MAIN(keepercli);
