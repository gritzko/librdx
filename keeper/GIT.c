//  GIT: parsers for git objects
//
#include "GIT.h"

#include <string.h>

#include "abc/HEX.h"
#include "abc/PRO.h"

//  Tree entry: <mode SP name>\0<20-byte-sha1>.  Find the NUL via
//  u8csFind; slice before = "<mode> <name>"; the 20 bytes that follow
//  are the raw SHA-1.
ok64 GITu8sDrainTree(u8cs obj, u8csp file, u8csp sha1) {
    sane(u8csOK(obj) && file && sha1);
    if ($empty(obj)) return NODATA;

    u8cp start = obj[0];
    u8cs scan = {obj[0], obj[1]};
    if (u8csFind(scan, 0) != OK) return GITBADFMT;

    file[0] = start;
    file[1] = scan[0];
    u8csUsed(scan, 1);  // consume NUL

    if ((u64)u8csLen(scan) < GIT_SHA1_LEN) return GITBADFMT;
    sha1[0] = scan[0];
    sha1[1] = scan[0] + GIT_SHA1_LEN;

    obj[0] = sha1[1];  // advance past this entry
    done;
}

//  Commit header iterator:
//    - blank line (leading '\n') → field empty, value = body, obj consumed.
//    - otherwise one "<field> <value>\n" line per call.
ok64 GITu8sDrainCommit(u8cs obj, u8csp field, u8csp value) {
    sane(u8csOK(obj) && field && value);
    if ($empty(obj)) return NODATA;

    if (*obj[0] == '\n') {
        field[0] = field[1] = obj[0];        // empty field
        u8csUsed(obj, 1);                    // skip blank line
        value[0] = obj[0];
        value[1] = obj[1];
        obj[0] = obj[1];                     // body consumed whole
        done;
    }

    //  End-of-line via u8csFind; the line (excluding '\n') is
    //  [obj[0], nl).  Inside that line, find the mandatory space.
    u8cs nl_scan = {obj[0], obj[1]};
    b8   has_nl  = (u8csFind(nl_scan, '\n') == OK);
    u8cp nl      = has_nl ? nl_scan[0] : obj[1];

    u8cs sp_scan = {obj[0], nl};
    if (u8csFind(sp_scan, ' ') != OK) return GITBADFMT;

    field[0] = obj[0];
    field[1] = sp_scan[0];
    value[0] = sp_scan[0] + 1;               // skip space
    value[1] = nl;

    obj[0] = nl;
    if (has_nl) u8csUsed(obj, 1);            // skip '\n'
    done;
}

ok64 GITu8sCommitTree(u8cs commit, u8 tree_sha[20]) {
    sane($ok(commit) && tree_sha);
    u8cs body = {commit[0], commit[1]};
    u8cs field = {}, value = {};
    while (GITu8sDrainCommit(body, field, value) == OK) {
        if ($empty(field)) break;
        if ($len(field) == 4 && memcmp(field[0], "tree", 4) == 0) {
            if ($len(value) < 40) return GITBADFMT;
            u8s bin = {tree_sha, tree_sha + 20};
            u8cs hex = {value[0], value[0] + 40};
            return HEXu8sDrainSome(bin, hex);
        }
    }
    return GITBADFMT;
}
