//  GIT: parsers for git objects
//
#include "GIT.h"

#include <string.h>

#include "abc/HEX.h"
#include "abc/PRO.h"

ok64 GITu8sDrainTree(u8cs obj, u8csp file, u8csp sha1) {
    sane(u8csOK(obj) && file && sha1);
    if ($empty(obj)) return NODATA;

    // tree entry: <mode> <filename>\0<20-byte-sha1>
    u8cp start = obj[0];
    u8cp p = start;
    u8cp e = obj[1];

    // scan for the NUL separator
    while (p < e && *p != 0) p++;
    if (p >= e) return GITBADFMT;

    // file = mode + name (everything before NUL)
    file[0] = start;
    file[1] = p;

    p++;  // skip NUL

    // need 20 bytes for SHA1
    if (p + GIT_SHA1_LEN > e) return GITBADFMT;

    sha1[0] = p;
    sha1[1] = p + GIT_SHA1_LEN;

    // advance input past this entry
    obj[0] = p + GIT_SHA1_LEN;

    done;
}

ok64 GITu8sDrainCommit(u8cs obj, u8csp field, u8csp value) {
    sane(u8csOK(obj) && field && value);
    if ($empty(obj)) return NODATA;

    u8cp p = obj[0];
    u8cp e = obj[1];

    // blank line: body follows
    if (*p == '\n') {
        field[0] = p;
        field[1] = p;  // empty field
        p++;            // skip the blank line
        value[0] = p;
        value[1] = e;  // rest is the body
        obj[0] = e;    // consumed everything
        return OK;
    }

    // header line: <field> <value>\n
    u8cp fstart = p;
    while (p < e && *p != ' ' && *p != '\n') p++;
    field[0] = fstart;
    field[1] = p;

    if (p >= e || *p != ' ') return GITBADFMT;
    p++;  // skip space

    u8cp vstart = p;
    while (p < e && *p != '\n') p++;
    value[0] = vstart;
    value[1] = p;

    if (p < e) p++;  // skip newline

    obj[0] = p;

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
