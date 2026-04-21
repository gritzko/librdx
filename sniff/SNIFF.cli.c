//  sniff CLI — thin wrapper: parse, open, exec, close.
//
//  Hosts an indexer fan-out (mirroring keeper/KEEP.cli.c) so every
//  object sniff feeds into keeper via KEEPPackFeed during a local
//  commit also reaches graf/spot through their DOGUpdate contract —
//  no separate indexing pass.  sniff doesn't currently know per-blob
//  paths, so spot's tokenizer-driven indexing degrades to no-op for
//  blobs from local commits; graf's hash-only DAG entries populate
//  fully.
//
#include "SNIFF.h"

#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/CLI.h"
#include "dog/SHA1.h"
#include "graf/GRAF.h"
#include "keeper/KEEP.h"
#include "keeper/UNPK.h"
#include "spot/CAPO.h"

static void sniff_indexer_fanout(void *ctx, u8 type,
                                  sha1 const *sha, u8csc path,
                                  u8cs content) {
    (void)ctx;
    (void)sha;
    GRAFUpdate(type, content, path);
    SPOTUpdate(type, content, path);
}

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

    if (!need_state) return SNIFFExec(&c);

    // rw for anything that mutates .dogs/sniff state.
    a_cstr(v_status, "status");
    a_cstr(v_list,   "list");
    b8 ro = $eq(c.verb, v_status) || $eq(c.verb, v_list);
    b8 rw = !ro;

    home h = {};
    call(HOMEOpen, &h, reporoot, rw);
    call(SNIFFOpen, &h, rw);   // opens keeper singleton too

    //  Indexer fan-out for rw verbs (commit / stage paths that mutate
    //  keeper).  ro verbs don't write packs, so skip the open cost.
    ok64 go = NONE;
    ok64 so = NONE;
    if (rw) {
        go = GRAFOpen(&h, YES);
        so = SPOTOpen(&h, YES);
        keep_indexer_emit = sniff_indexer_fanout;
        keep_indexer_ctx  = NULL;
    }

    ok64 ret = SNIFFExec(&c);

    if (rw) {
        keep_indexer_emit = NULL;
        keep_indexer_ctx  = NULL;
        if (so == OK) SPOTClose();
        if (go == OK) GRAFClose();
    }

    SNIFFClose();
    HOMEClose(&h);
    return ret;
}

MAIN(sniffcli);
