//  dog/AT: standalone tail reader for `.sniff/at.log`.
//
#include "AT.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/OK.h"

//  Match keeper/KEEP.h's KEEPNONE without pulling the keeper header.
//  Value mirrors the one in keeper/KEEP.h; keep in sync.
con ok64 DOGATNONE = 0x50e3995d85ce;

ok64 DOGAtTail(u8bp branch_out, u8bp sha_out, u8cs reporoot) {
    sane(branch_out && sha_out && $ok(reporoot));

    //  Build path: <reporoot>/.sniff/at.log
    static u8c *const AT_REL_S[2] = {
        (u8c *)".sniff/at.log",
        (u8c *)".sniff/at.log" + sizeof(".sniff/at.log") - 1,
    };
    a_path(apath, reporoot, AT_REL_S);

    u8bp map = NULL;
    ok64 o = FILEMapRO(&map, $path(apath));
    if (o != OK) fail(DOGATNONE);

    a_dup(u8c, full, u8bData(map));
    //  Find last non-empty line.
    u8cp base = full[0];
    u64 n = (u64)(full[1] - base);
    if (n == 0) { u8bUnMap(map); fail(DOGATNONE); }
    u64 end = n;
    while (end > 0 && base[end - 1] == '\n') end--;
    if (end == 0) { u8bUnMap(map); fail(DOGATNONE); }
    u64 start = end;
    while (start > 0 && base[start - 1] != '\n') start--;
    u8cs line = {base + start, base + end};

    //  Parse: <ron60>\t?<branch>\t?<sha>
    a_dup(u8c, rest, line);
    a_dup(u8c, scan, rest);
    ok64 r = OK;
    if (u8csFind(scan, '\t') != OK) r = DOGATNONE;
    if (r == OK) {
        rest[0] = scan[0] + 1;
        if (u8csEmpty(rest) || *rest[0] != '?') r = DOGATNONE;
    }
    if (r == OK) {
        u8csUsed(rest, 1);
        a_dup(u8c, scan2, rest);
        if (u8csFind(scan2, '\t') != OK) r = DOGATNONE;
        if (r == OK) {
            u8cs branch_slice = {rest[0], scan2[0]};
            rest[0] = scan2[0] + 1;
            u8bReset(branch_out);
            u8bFeed(branch_out, branch_slice);
            if (u8csEmpty(rest) || *rest[0] != '?') r = DOGATNONE;
            if (r == OK) {
                u8csUsed(rest, 1);
                if (u8csLen(rest) != 40) r = DOGATNONE;
                if (r == OK) {
                    u8bReset(sha_out);
                    u8bFeed(sha_out, rest);
                }
            }
        }
    }
    u8bUnMap(map);
    return r;
}
