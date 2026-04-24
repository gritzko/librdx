//  dog/AT: thin compat reader for sniff's `.sniff` log.
//
//  The log is a dog/ULOG — rows of `<ron60-ms>\t<verb>\t<uri>\n`.
//  See sniff/AT.h for URI schema.  `DOGAtTail` extracts the current
//  (branch, sha) for code that doesn't want to link against snifflib
//  (keeper, beagle) — it walks newest-first and accepts the first
//  `get`/`post`/`patch` row with a 40-hex fragment, ignoring the
//  `repo` anchor (row 0) and `put`/`delete` intent rows.
//
#include "AT.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/OK.h"
#include "dog/ULOG.h"

con ok64 DOGATNONE = 0xd61029d5d85ce;

ok64 DOGAtTail(u8bp branch_out, u8bp sha_out, u8cs reporoot) {
    sane(branch_out && sha_out && $ok(reporoot));

    //  Build path: <reporoot>/.sniff
    static u8c *const AT_REL_S[2] = {
        (u8c *)".sniff",
        (u8c *)".sniff" + sizeof(".sniff") - 1,
    };
    a_path(apath, reporoot, AT_REL_S);

    //  The log is already a well-formed ULOG; read-only open +
    //  backward scan.
    ulog l = {};
    ok64 o = ULOGOpen(&l, $path(apath));
    if (o != OK) fail(DOGATNONE);

    u32 n = ULOGCount(&l);
    if (n == 0) { ULOGClose(&l); fail(DOGATNONE); }

    //  Walk rows newest-first, accept the first with a 40-hex
    //  fragment.  Row 0 is a `repo` anchor (no fragment sha) and is
    //  naturally skipped by the fragment-length check.
    ron60 ts = 0, verb = 0;
    uri u = {};
    b8 found = NO;
    for (u32 i = n; i > 0; ) {
        i--;
        ok64 ro = ULOGRow(&l, i, &ts, &verb, &u);
        if (ro != OK) continue;

        u8cs query = {u.query[0], u.query[1]};
        u8cs frag  = {u.fragment[0], u.fragment[1]};

        //  Need a 40-hex fragment for sha (get/post/patch rows).
        if ((size_t)$len(frag) != 40) continue;

        u8bReset(branch_out);
        if (!$empty(query)) u8bFeed(branch_out, query);

        u8bReset(sha_out);
        u8bFeed(sha_out, frag);

        found = YES;
        break;
    }
    ULOGClose(&l);
    if (!found) fail(DOGATNONE);
    done;
}
