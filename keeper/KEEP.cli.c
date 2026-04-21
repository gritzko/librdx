//  keeper CLI — thin wrapper: parse, open, exec, close.
//
//  Also hosts the indexer fan-out: when keeper is invoked as the
//  standalone `keeper` binary, every object UNPKIndex resolves is
//  forwarded to graf and spot through their DOGUpdate contracts.
//  The fan-out lives here (not in keeplib) so keeplib stays free of
//  graflib/spotlib dependencies.
//
#include "KEEP.h"
#include "UNPK.h"

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/CLI.h"
#include "dog/SHA1.h"
#include "graf/GRAF.h"
#include "spot/CAPO.h"

//  UNPKIndex emit → GRAFUpdate + SPOTUpdate.  Called once per resolved
//  object inside KEEPIngestFile.  `content` is valid only for the
//  callback's lifetime; both indexers copy what they need.  A ro graf
//  or spot silently drops updates (they guard internally).
static void keeper_indexer_fanout(void *ctx, u8 type,
                                   sha1 const *sha, u8csc path,
                                   u8cs content) {
    (void)ctx;
    (void)sha;
    GRAFUpdate(type, content, path);
    SPOTUpdate(type, content, path);
}


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

    home h = {};
    call(HOMEOpen, &h, c.repo, rw);

    call(KEEPOpen, &h, rw);

    //  Indexer fan-out: only for rw verbs (fetch/push).  ro verbs
    //  don't ingest packs so there's nothing to forward.
    ok64 go = NONE;
    ok64 so = NONE;
    if (rw) {
        go = GRAFOpen(&h, YES);
        so = SPOTOpen(&h, YES);
        keep_indexer_emit = keeper_indexer_fanout;
        keep_indexer_ctx  = NULL;
    }

    ok64 ret = KEEPExec(&KEEP, &c);

    if (rw) {
        keep_indexer_emit = NULL;
        keep_indexer_ctx  = NULL;
        if (so == OK) SPOTClose();
        if (go == OK) GRAFClose();
    }

    KEEPClose();
    HOMEClose(&h);
    return ret;
}

MAIN(keepercli);
