//  POST: wrap the current base tree into a commit object.
//
#include "POST.h"
#include "PUT.h"

#include <string.h>
#include <time.h>

#include "abc/HEX.h"
#include "abc/PRO.h"
#include "keeper/GIT.h"
#include "keeper/REFS.h"
#include "keeper/SHA1.h"
#include "keeper/WALK.h"

#include "AT.h"
#include "STAGE.h"

// Compute full SHA1 of parent commit for the "parent" line.
static ok64 POSTParentSha(sha1 *out, keeper *k, u8cs parent_hex) {
    sane(out && k && $ok(parent_hex));

    size_t hexlen = $len(parent_hex);
    if (hexlen > 15) hexlen = 15;
    u64 hashlet = WHIFFHexHashlet60(parent_hex);

    Bu8 cbuf = {};
    call(u8bAllocate, cbuf, 1UL << 24);
    u8 ctype = 0;
    call(KEEPGet, k, hashlet, hexlen, cbuf, &ctype);

    // Dereference tag
    if (ctype == DOG_OBJ_TAG) {
        u8cs body = {u8bDataHead(cbuf), u8bIdleHead(cbuf)};
        u8cs field = {}, value = {};
        sha1 tag_sha = {};
        a_raw(tag_bin, tag_sha);
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if ($empty(field)) break;
            if ($len(field) == 6 && memcmp(field[0], "object", 6) == 0 &&
                $len(value) >= 40) {
                u8cs hex40 = {value[0], $atp(value, 40)};
                HEXu8sDrainSome(tag_bin, hex40);
                break;
            }
        }
        u64 ch = WHIFFHashlet60(&tag_sha);
        u8bReset(cbuf);
        call(KEEPGet, k, ch, 15, cbuf, &ctype);
    }
    if (ctype != DOG_OBJ_COMMIT) { u8bFree(cbuf); fail(SNIFFFAIL); }

    // SHA1("commit <len>\0" + content)
    size_t csz = u8bDataLen(cbuf);
    char hdr[64];
    int hlen = snprintf(hdr, sizeof(hdr), "commit %zu", csz);
    Bu8 tmp = {};
    call(u8bAllocate, tmp, (u64)hlen + 1 + csz);
    u8cs hs = {(u8cp)hdr, (u8cp)hdr + hlen};
    u8bFeed(tmp, hs);
    u8bFeed1(tmp, 0);
    u8bFeed(tmp, u8bDataC(cbuf));
    a_dup(u8c, _d, u8bData(tmp));
    SHA1Sum(out, _d);
    u8bFree(tmp);
    u8bFree(cbuf);
    done;
}

// Resolve the worktree's current commit (parent for a new commit)
// from sniff/at.log.  Writes 40 hex bytes into out_hex[0..40].
// Returns NO if at.log is missing/empty.
static b8 post_parent_hex(u8 *out_hex, keeper *k, u8cs reporoot) {
    (void)k; (void)reporoot;
    a_pad(u8, bbuf, 256);
    a_pad(u8, sbuf, 64);
    sniff_at tail = {.branch = bbuf, .sha = sbuf};
    if (SNIFFAtRead(&tail) != OK) return NO;
    if (u8bDataLen(tail.sha) != 40) return NO;
    memcpy(out_hex, u8bDataHead(tail.sha), 40);
    return YES;
}

// Resolve parent commit's tree hashlet for staging.  Returns 0 if
// there's no parent (root commit) or resolution fails.
static u64 post_parent_tree_hashlet(keeper *k, u8cs reporoot) {
    u8 hex[40] = {};
    if (!post_parent_hex(hex, k, reporoot)) return 0;
    u8cs head = {hex, hex + 40};
    sha1 tree_sha = {};
    if (SNIFFParentTreeSha(&tree_sha, head) != OK) return 0;
    return WHIFFHashlet40(&tree_sha);
}

//  Canonical repack: walk the root tree and collect 60-bit hashlets of
//  all staged (tree, blob) objects reachable from it.  Objects already
//  in the main keeper (i.e. unchanged from the parent commit) are
//  skipped — they remain addressable through keeper's existing idx.
//  Hashlets are de-duplicated by linear scan (staged sets are small).
typedef struct {
    u8cs branch;
    Bu64 trees;
    Bu64 blobs;
} repack_ctx;

static b8 u64b_contains(Bu64 buf, u64 v) {
    u64cp h = u64bDataHead(buf);
    u64cp e = (u64cp)u8bIdleHead((u8bp)buf);
    for (; h < e; h++) if (*h == v) return YES;
    return NO;
}

static ok64 repack_walk(repack_ctx *rc, sha1 const *tree_sha) {
    sane(rc && tree_sha);
    u64 tree_hl = WHIFFHashlet60(tree_sha);

    //  If this tree is NOT in staging, it's already in main keeper —
    //  skip entirely (its subtree is also in main).
    u64 val = 0;
    if (STAGELookup(rc->branch, tree_hl, 15, &val) != OK) done;

    if (u64b_contains(rc->trees, tree_hl)) done;

    Bu8 tbuf = {};
    call(u8bAllocate, tbuf, 1UL << 20);
    u8 otype = 0;
    ok64 go = STAGEGet(rc->branch, tree_hl, 15, tbuf, &otype);
    if (go != OK || otype != DOG_OBJ_TREE) { u8bFree(tbuf); done; }

    u64bPush(rc->trees, &tree_hl);

    a_dup(u8c, obj, u8bData(tbuf));
    u8cs file = {}, esha = {};
    while (GITu8sDrainTree(obj, file, esha) == OK) {
        u8cs scan = {file[0], file[1]};
        if (u8csFind(scan, ' ') != OK) continue;
        u8cs mode_s = {file[0], scan[0]};
        u8 kind = WALKu8sModeKind(mode_s);
        if (kind == 0) continue;

        sha1 esha_b = {};
        memcpy(esha_b.data, esha[0], 20);
        u64 entry_hl = WHIFFHashlet60(&esha_b);

        if (kind == WALK_KIND_DIR) {
            (void)repack_walk(rc, &esha_b);
        } else if (kind == WALK_KIND_REG || kind == WALK_KIND_EXE ||
                   kind == WALK_KIND_LNK) {
            u64 bv = 0;
            if (STAGELookup(rc->branch, entry_hl, 15, &bv) == OK &&
                !u64b_contains(rc->blobs, entry_hl))
                u64bPush(rc->blobs, &entry_hl);
        }
    }
    u8bFree(tbuf);
    done;
}

//  Emit every collected object to `mp` in canonical order: trees
//  first, then blobs (commit was already fed by the caller).
static ok64 repack_emit(keep_pack *mp, u8cs branch, repack_ctx *rc) {
    sane(mp && rc);
    keeper *k = &KEEP;

    Bu8 obuf = {};
    call(u8bAllocate, obuf, 1UL << 20);

    //  Trees.
    u64cp th = u64bDataHead(rc->trees);
    u64cp te = (u64cp)u8bIdleHead((u8bp)rc->trees);
    for (; th < te; th++) {
        u8 otype = 0;
        u8bReset(obuf);
        call(STAGEGet, branch, *th, 15, obuf, &otype);
        sha1 tmp = {};
        a_dup(u8c, body, u8bData(obuf));
        u8csc nopath = {NULL, NULL};
        call(KEEPPackFeed, k, mp, DOG_OBJ_TREE, body, nopath, &tmp);
    }
    //  Blobs.  Repack from staging to main log; the original feed in
    //  PUT/COM has already fan-out'd the indexer with the live path,
    //  so pass empty here to avoid re-indexing under a wrong path.
    u64cp bh = u64bDataHead(rc->blobs);
    u64cp be = (u64cp)u8bIdleHead((u8bp)rc->blobs);
    for (; bh < be; bh++) {
        u8 otype = 0;
        u8bReset(obuf);
        call(STAGEGet, branch, *bh, 15, obuf, &otype);
        sha1 tmp = {};
        a_dup(u8c, body, u8bData(obuf));
        u8csc nopath = {NULL, NULL};
        call(KEEPPackFeed, k, mp, DOG_OBJ_BLOB, body, nopath, &tmp);
    }
    u8bFree(obuf);
    done;
}

// --- Public API ---

ok64 POSTCommit(u8cs reporoot,
                u8cs message, u8cs author, sha1 *sha_out) {
    sane($ok(message) && $ok(author));
    sniff *s = &SNIFF; keeper *k = &KEEP; (void)s;

    //  1. Resolve target branch (sniff/at.log tail or "heads/master").
    a_pad(u8, brbuf, 256);
    call(STAGEBranch, brbuf);
    a_dup(u8c, branch, u8bData(brbuf));

    //  2. Auto-stage when nothing has been explicitly PUT/DELETEd.
    u64 base = SNIFFBaseTree();
    u64 head_tree = post_parent_tree_hashlet(k, reporoot);
    sha1 root_tree = {};
    b8 have_root_sha = NO;

    if (base == 0 || base == head_tree) {
        call(PUTStage, &root_tree, reporoot, NULL);
        base = SNIFFBaseTree();
        if (base == 0) {
            //  Empty worktree: write an empty tree into the staging log
            //  so the commit body can reference it and the repack walk
            //  emits it.
            keep_pack sp = {};
            call(STAGEOpen, &sp, branch);
            u8cs empty = {};
            u8csc nopath = {NULL, NULL};
            call(KEEPPackFeed, k, &sp, DOG_OBJ_TREE, empty, nopath, &root_tree);
            call(STAGEClose, &sp, branch);
            base = WHIFFHashlet40(&root_tree);
            SNIFFRecord(SNIFF_TREE, SNIFFRootIdx(), base);
        }
        have_root_sha = !sha1empty(&root_tree);
    }

    //  3. Resolve root_tree sha when a prior explicit PUT already set
    //     base.  Prefer staging; fall back to main keeper (unchanged
    //     subtree of the parent commit).
    if (!have_root_sha) {
        Bu8 tbuf = {};
        call(u8bAllocate, tbuf, 1UL << 24);
        u8 otype = 0;
        ok64 go = STAGEGet(branch, base << 20, 10, tbuf, &otype);
        if (go != OK || otype != DOG_OBJ_TREE) {
            u8bReset(tbuf);
            go = KEEPGet(k, base << 20, 10, tbuf, &otype);
        }
        if (go != OK || otype != DOG_OBJ_TREE) {
            u8bFree(tbuf);
            fail(SNIFFFAIL);
        }
        KEEPObjSha(&root_tree, DOG_OBJ_TREE, u8bDataC(tbuf));
        u8bFree(tbuf);
    }

    //  4. Resolve parent commit sha (if any).
    sha1 parent_sha = {};
    b8 has_parent = NO;
    u8 parent_hex[40] = {};
    if (post_parent_hex(parent_hex, k, reporoot)) {
        u8cs head = {parent_hex, parent_hex + 40};
        if (POSTParentSha(&parent_sha, k, head) == OK) has_parent = YES;
    }

    //  5. Build commit body (tree, optional parent, author, committer,
    //     message).
    Bu8 com = {};
    call(u8bAllocate, com, 4096);

    a_cstr(tree_label, "tree ");
    u8bFeed(com, tree_label);
    a_pad(u8, tree_hex, 40);
    a_rawc(tsha, root_tree);
    HEXu8sFeedSome(tree_hex_idle, tsha);
    u8bFeed(com, u8bDataC(tree_hex));
    u8bFeed1(com, '\n');

    if (has_parent) {
        a_cstr(par_label, "parent ");
        u8bFeed(com, par_label);
        a_pad(u8, par_hex, 40);
        a_rawc(psha, parent_sha);
        HEXu8sFeedSome(par_hex_idle, psha);
        u8bFeed(com, u8bDataC(par_hex));
        u8bFeed1(com, '\n');
    }

    time_t now = time(NULL);
    char ts[64];
    int tslen = snprintf(ts, sizeof(ts), " %lld +0000\n", (long long)now);
    u8cs ts_s = {(u8cp)ts, (u8cp)ts + tslen};

    a_cstr(auth_label, "author ");
    u8bFeed(com, auth_label);
    u8bFeed(com, author);
    u8bFeed(com, ts_s);

    a_cstr(comm_label, "committer ");
    u8bFeed(com, comm_label);
    u8bFeed(com, author);
    u8bFeed(com, ts_s);

    u8bFeed1(com, '\n');
    u8bFeed(com, message);
    u8bFeed1(com, '\n');

    //  6. Canonical pack on MAIN keeper: commit → trees → blobs.
    keep_pack mp = {};
    call(KEEPPackOpen, k, &mp);

    a_dup(u8c, com_data, u8bData(com));
    u8csc nopath = {NULL, NULL};
    ok64 o = KEEPPackFeed(k, &mp, DOG_OBJ_COMMIT, com_data, nopath, sha_out);
    u8bFree(com);
    if (o != OK) { KEEPPackClose(k, &mp); return o; }

    //  7. Walk staged reachable objects and emit trees, then blobs.
    repack_ctx rc = {};
    rc.branch[0] = branch[0]; rc.branch[1] = branch[1];
    call(u64bAllocate, rc.trees, 1U << 14);
    call(u64bAllocate, rc.blobs, 1U << 14);
    o = repack_walk(&rc, &root_tree);
    if (o == OK) o = repack_emit(&mp, branch, &rc);
    u64bFree(rc.trees);
    u64bFree(rc.blobs);
    if (o != OK) { KEEPPackClose(k, &mp); return o; }

    call(KEEPPackClose, k, &mp);

    //  8. Drop staging dir; record new commit in at.log.
    STAGEDrop(branch);

    a_pad(u8, out_hex, 40);
    a_rawc(osha, *sha_out);
    HEXu8sFeedSome(out_hex_idle, osha);
    a_dup(u8c, hex_in, u8bData(out_hex));
    (void)SNIFFAtAppend(branch, hex_in);

    fprintf(stderr, "sniff: commit %.*s\n",
            (int)u8bDataLen(out_hex), (char *)u8bDataHead(out_hex));
    done;
}

ok64 POSTSetLabel(u8cs ref_uri, u8cs sha_hex) {
    sane($ok(ref_uri) && !u8csEmpty(ref_uri) && $ok(sha_hex));
    if (u8csLen(sha_hex) != 40) fail(SNIFFFAIL);

    a_path(keepdir, u8bDataC(KEEP.h->root), KEEP_DIR_S);

    a_pad(u8, tbuf, 64);
    u8bFeed1(tbuf, '?');
    u8bFeed(tbuf, sha_hex);
    a_dup(u8c, to, u8bData(tbuf));

    return REFSAppend($path(keepdir), ref_uri, to);
}
