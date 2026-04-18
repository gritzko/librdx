//  sniff CLI — thin wrapper: parse, open, exec, close.
//
#include "SNIFF.h"

#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/CLI.h"

ok64 sniffcli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, SNIFF_VERBS, SNIFF_VAL_FLAGS);

    char cwd[1024];
    u8cs reporoot = {};
    if ($ok(c.repo)) {
        $mv(reporoot, c.repo);
    } else {
        if (!getcwd(cwd, sizeof(cwd))) fail(SNIFFFAIL);
        a_cstr(cwds, cwd);
        reporoot[0] = cwds[0];
        reporoot[1] = cwds[1];
        c.repo[0] = cwds[0];
        c.repo[1] = cwds[1];
    }

    // Help and stop don't need an open state.
    a_cstr(v_help, "help");
    a_cstr(v_stop, "stop");
    b8 need_state = !$eq(c.verb, v_help) && !$eq(c.verb, v_stop)
                 && !CLIHas(&c, "-h") && !CLIHas(&c, "--help");

    if (!need_state) {
        sniff s_empty = {};
        return SNIFFExec(&s_empty, &c);
    }

    // rw for anything that mutates .dogs/sniff state.
    a_cstr(v_status, "status");
    a_cstr(v_list,   "list");
    b8 ro = $eq(c.verb, v_status) || $eq(c.verb, v_list);
    b8 rw = !ro;

    home h = {};
    call(HOMEOpen, &h, reporoot, rw);

    sniff s = {};
    call(SNIFFOpen, &s, &h, rw);
    ok64 ret = SNIFFExec(&s, &c);
    SNIFFClose(&s);
    HOMEClose(&h);
    return ret;
}

MAIN(sniffcli);
