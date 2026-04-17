//  WALK: tree walker on KEEP.
//
#include "WALK.h"

#include <stdio.h>
#include <string.h>

#include "abc/PRO.h"
#include "dog/DOG.h"
#include "dog/DPATH.h"
#include "GIT.h"

u8 WALKu8sModeKind(u8cs mode) {
    if ($empty(mode)) return 0;
    u8 c0 = $at(mode, 0);
    if (c0 == '4') return WALK_KIND_DIR;
    if (c0 != '1' || $len(mode) < 2) return 0;
    u8 c1 = $at(mode, 1);
    if (c1 == '6') return WALK_KIND_SUB;
    if (c1 == '2') return WALK_KIND_LNK;
    if (c1 == '0') {
        // 100644 vs 100755
        return ($len(mode) >= 6 && $at(mode, 3) == '7')
             ? WALK_KIND_EXE : WALK_KIND_REG;
    }
    return 0;
}

//  Depth-first dive through one tree.  `pathbuf` carries the current
//  path (no leading/trailing '/'), shared across recursion levels.
//  Each level owns its own `tbuf` and per-entry `bbuf` (blob) so
//  nested KEEPGetExact calls don't clobber parent bytes.
static ok64 walk_tree_dive(keeper *k, sha1 const *tree_sha,
                            u8bp pathbuf, b8 eager,
                            walk_tree_fn visit, void0p ctx) {
    sane(k && tree_sha && visit);

    Bu8 tbuf = {};
    call(u8bAllocate, tbuf, 1UL << 20);
    u8 otype = 0;
    ok64 o = KEEPGetExact(k, tree_sha, tbuf, &otype);
    if (o != OK) { u8bFree(tbuf); return o; }
    if (otype != DOG_OBJ_TREE) { u8bFree(tbuf); return WALKBADFMT; }

    u8cs tree_s = {u8bDataHead(tbuf), u8bIdleHead(tbuf)};
    u8cs file = {}, esha = {};
    ok64 result = OK;

    while (GITu8sDrainTree(tree_s, file, esha) == OK) {
        // Parse "<mode> <name>".
        u8cs scan = {file[0], file[1]};
        if (u8csFind(scan, ' ') != OK) continue;
        u8cs mode_s = {file[0], scan[0]};
        u8cs name_s = {scan[0] + 1, file[1]};
        if ($empty(mode_s) || $empty(name_s)) continue;
        u8 kind = WALKu8sModeKind(mode_s);
        if (kind == 0) continue;
        if (DPATHVerify(name_s) != OK) {
            fprintf(stderr, "walk: bad path '%.*s', skip\n",
                    (int)$len(name_s), (char *)name_s[0]);
            continue;
        }

        // Push "/name" (or just "name" at root) onto pathbuf.
        size_t pre_len = u8bDataLen(pathbuf);
        if (pre_len > 0) {
            if (u8bFeed1(pathbuf, '/') != OK) { result = WALKNOROOM; break; }
        }
        if (u8bFeed(pathbuf, name_s) != OK) { result = WALKNOROOM; break; }
        u8cs path = {u8bDataHead(pathbuf), pathbuf[2]};

        // Eager blob resolve for file-like kinds.
        Bu8 bbuf = {};
        u8cs blob = {};
        b8 is_file = (kind == WALK_KIND_REG || kind == WALK_KIND_EXE ||
                      kind == WALK_KIND_LNK);
        if (eager && is_file) {
            if (u8bAllocate(bbuf, 1UL << 20) == OK) {
                sha1 entry_sha = {};
                memcpy(entry_sha.data, esha[0], 20);
                u8 btype = 0;
                if (KEEPGetExact(k, &entry_sha, bbuf, &btype) == OK &&
                    btype == DOG_OBJ_BLOB) {
                    blob[0] = u8bDataHead(bbuf);
                    blob[1] = u8bIdleHead(bbuf);
                }
            }
        }

        ok64 vo = visit(path, kind, esha[0], blob, ctx);
        if (bbuf[0]) u8bFree(bbuf);

        if (vo == OK && kind == WALK_KIND_DIR) {
            sha1 sub = {};
            memcpy(sub.data, esha[0], 20);
            vo = walk_tree_dive(k, &sub, pathbuf, eager, visit, ctx);
        }

        // Rewind pathbuf to pre-entry length.
        size_t cur_len = u8bDataLen(pathbuf);
        if (cur_len > pre_len) u8bShed(pathbuf, cur_len - pre_len);

        if (vo == WALKSKIP) continue;
        if (vo == WALKSTOP) { result = WALKSTOP; break; }
        if (vo != OK) { result = vo; break; }
    }

    u8bFree(tbuf);
    return result;
}

static ok64 walk_tree_entry(keeper *k, u8cp tree_sha, b8 eager,
                             walk_tree_fn visit, void0p ctx) {
    sane(k && tree_sha && visit);
    a_pad(u8, pathbuf, 2048);
    sha1 root = {};
    memcpy(root.data, tree_sha, 20);

    u8cs empty_path = {}, empty_blob = {};
    ok64 vo = visit(empty_path, WALK_KIND_DIR, tree_sha, empty_blob, ctx);
    if (vo == WALKSTOP) return OK;
    if (vo == WALKSKIP) return OK;
    if (vo != OK) return vo;

    ok64 o = walk_tree_dive(k, &root, pathbuf, eager, visit, ctx);
    if (o == WALKSTOP) return OK;
    return o;
}

ok64 WALKTree(keeper *k, u8cp tree_sha, walk_tree_fn visit, void0p ctx) {
    return walk_tree_entry(k, tree_sha, YES, visit, ctx);
}

ok64 WALKTreeLazy(keeper *k, u8cp tree_sha, walk_tree_fn visit, void0p ctx) {
    return walk_tree_entry(k, tree_sha, NO, visit, ctx);
}
