//  DEL: build a tree with files/dirs removed.
//
#include "DEL.h"

#include <string.h>
#include <sys/stat.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/DPATH.h"
#include "keeper/GIT.h"
#include "keeper/SHA1.h"
#include "keeper/WALK.h"

// --- Reuse post's sha_ctx shape ---

typedef struct {
    sniff *s;
    sha1p  shas;
    u32    capacity;
} del_sha_ctx;

static sha1cp DELGetSha(del_sha_ctx const *ctx, u32 idx) {
    if (idx >= ctx->capacity) return NULL;
    if (sha1empty(&ctx->shas[idx])) return NULL;
    return &ctx->shas[idx];
}

static void DELSetSha(del_sha_ctx *ctx, u32 idx, sha1 const *sha) {
    if (idx < ctx->capacity) ctx->shas[idx] = *sha;
}

// --- Collect old tree SHAs (same as POST) ---

static ok64 DELCollectTree(del_sha_ctx *ctx, keeper *k,
                             sha1 const *tree_sha, u8cs prefix) {
    sane(ctx && k && tree_sha);

    u64 hashlet = WHIFFHashlet60(tree_sha);
    Bu8 buf = {};
    call(u8bAllocate, buf, 1UL << 24);
    u8 otype = 0;
    ok64 o = KEEPGet(k, hashlet, 15, buf, &otype);
    if (o != OK) { u8bFree(buf); fail(o); }
    if (otype != DOG_OBJ_TREE) { u8bFree(buf); fail(SNIFFFAIL); }

    size_t tsz = u8bDataLen(buf);
    Bu8 tcopy = {};
    o = u8bAllocate(tcopy, tsz);
    if (o != OK) { u8bFree(buf); fail(o); }
    u8bFeed(tcopy, u8bDataC(buf));
    u8bFree(buf);

    u8cs tree = {u8bDataHead(tcopy), u8bIdleHead(tcopy)};
    u8cs file = {}, esha = {};

    while (GITu8sDrainTree(tree, file, esha) == OK) {
        u8cs scan = {file[0], file[1]};
        if (u8csFind(scan, ' ') != OK) continue;
        u8cs name_s = {scan[0], file[1]};
        ++name_s[0];
        u8cs mode_s = {file[0], scan[0]};

        if (DPATHVerify(name_s) != OK) {
            fprintf(stderr, "sniff: bad path '%.*s', skip\n",
                    (int)$len(name_s), (char *)name_s[0]);
            continue;
        }

        b8 is_submodule = ($len(mode_s) >= 2 &&
                           $at(mode_s, 0) == '1' && $at(mode_s, 1) == '6');
        if (is_submodule) continue;
        b8 is_dir = ($at(mode_s, 0) == '4');

        a_pad(u8, rel, 2048);
        if (!$empty(prefix)) {
            u8bFeed(rel, prefix);
            u8bFeed1(rel, '/');
        }
        u8bFeed(rel, name_s);
        if (is_dir) u8bFeed1(rel, '/');
        PATHu8bTerm(rel);
        u8cs relpath = {u8bDataHead(rel), rel[2]};

        u32 idx = is_dir ? SNIFFInternDir(ctx->s, relpath)
                         : SNIFFIntern(ctx->s, relpath);
        DELSetSha(ctx, idx, (sha1cp)esha[0]);

        if (is_dir)
            DELCollectTree(ctx, k, (sha1cp)esha[0], relpath);
    }

    u8bFree(tcopy);
    done;
}

static ok64 DELResolveParent(del_sha_ctx *ctx, keeper *k,
                               u8cs parent_hex) {
    sane(ctx && k && $ok(parent_hex));

    size_t hexlen = $len(parent_hex);
    if (hexlen > 15) hexlen = 15;
    u64 hashlet = WHIFFHexHashlet60(parent_hex);

    Bu8 cbuf = {};
    call(u8bAllocate, cbuf, 1UL << 24);
    u8 ctype = 0;
    call(KEEPGet, k, hashlet, hexlen, cbuf, &ctype);

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

    sha1 tree_sha = {};
    u8cs commit_body = {u8bDataHead(cbuf), u8bIdleHead(cbuf)};
    call(WALKCommitTree, commit_body, tree_sha.data);
    u8bFree(cbuf);

    u8cs no_prefix = {};
    call(DELCollectTree, ctx, k, &tree_sha, no_prefix);
    done;
}

// --- Depth-first tree build, excluding del_set entries ---

static ok64 DELBuild(sha1 *tree_out, sniff *s, keeper *k,
                       keep_pack *p, del_sha_ctx *sha_tab,
                       u8cs reporoot, u8cp del_set,
                       u32 lo, u32 hi, u8cs prefix) {
    sane(s && k && p && sha_tab && tree_out && del_set);

    Bu8 tree = {};
    call(u8bAllocate, tree, (u64)(hi - lo) * 80);

    u32 i = lo;
    while (i < hi) {
        u32 idx = *u32bDataAtP(s->sorted, i);

        // Skip deleted entries
        if (del_set[idx]) {
            if (SNIFFIsDir(s, idx)) {
                // Skip entire subtree
                u8cs rel = {};
                SNIFFPath(rel, s, idx);
                u32 sub_hi = i + 1;
                while (sub_hi < hi) {
                    u32 sidx = *u32bDataAtP(s->sorted, sub_hi);
                    u8cs sp = {};
                    SNIFFPath(sp, s, sidx);
                    if ($len(sp) <= $len(rel)) break;
                    if (memcmp(sp[0], rel[0], $len(rel)) != 0) break;
                    sub_hi++;
                }
                i = sub_hi;
            } else {
                i++;
            }
            continue;
        }

        u8cs rel = {};
        if (SNIFFPath(rel, s, idx) != OK) { i++; continue; }

        size_t plen = $len(prefix);
        u8cs rest = {$atp(rel, plen), rel[1]};
        if ($empty(rest)) { i++; continue; }

        b8 is_dir = (*$last(rel) == '/');

        if (is_dir) {
            u32 sub_lo = i + 1;
            u32 sub_hi = sub_lo;
            while (sub_hi < hi) {
                u32 sidx = *u32bDataAtP(s->sorted, sub_hi);
                u8cs sp = {};
                SNIFFPath(sp, s, sidx);
                if ($len(sp) <= $len(rel)) break;
                if (memcmp(sp[0], rel[0], $len(rel)) != 0) break;
                sub_hi++;
            }

            // Check if any child is deleted
            b8 has_del = NO;
            for (u32 j = sub_lo; j < sub_hi; j++) {
                u32 cidx = *u32bDataAtP(s->sorted, j);
                if (del_set[cidx]) { has_del = YES; break; }
            }

            sha1 sub_sha = {};
            if (!has_del) {
                sha1cp old = DELGetSha(sha_tab, idx);
                if (old) {
                    sub_sha = *old;
                } else {
                    ok64 o = DELBuild(&sub_sha, s, k, p, sha_tab,
                                        reporoot, del_set,
                                        sub_lo, sub_hi, rel);
                    if (o != OK) { u8bFree(tree); return o; }
                }
            } else {
                ok64 o = DELBuild(&sub_sha, s, k, p, sha_tab,
                                    reporoot, del_set,
                                    sub_lo, sub_hi, rel);
                if (o != OK) { u8bFree(tree); return o; }
            }

            if (!sha1empty(&sub_sha)) {
                u8cs name = {rest[0], $last(rest)};
                a_cstr(mode, "40000");
                u8bFeed(tree, mode);
                u8bFeed1(tree, ' ');
                u8bFeed(tree, name);
                u8bFeed1(tree, 0);
                a_rawc(sha_raw, sub_sha);
                u8bFeed(tree, sha_raw);
            }

            i = sub_hi;
        } else {
            // Unchanged file — reuse old SHA
            sha1cp old = DELGetSha(sha_tab, idx);
            if (!old) { i++; continue; }

            u8cs name = {rest[0], rest[1]};

            // Recover mode from stat
            u8cs full_rel = {};
            SNIFFPath(full_rel, s, idx);
            a_path(fp);
            SNIFFFullpath(fp, reporoot, full_rel);
            struct stat lsb = {};
            if (lstat((char *)u8bDataHead(fp), &lsb) != 0) {
                i++; continue;
            }

            if (S_ISLNK(lsb.st_mode)) {
                a_cstr(m, "120000");
                u8bFeed(tree, m);
            } else if (lsb.st_mode & S_IXUSR) {
                a_cstr(m, "100755");
                u8bFeed(tree, m);
            } else {
                a_cstr(m, "100644");
                u8bFeed(tree, m);
            }
            u8bFeed1(tree, ' ');
            u8bFeed(tree, name);
            u8bFeed1(tree, 0);
            a_rawc(sha_raw, *old);
            u8bFeed(tree, sha_raw);

            i++;
        }
    }

    if (u8bDataLen(tree) == 0) {
        memset(tree_out, 0, sizeof(*tree_out));
        u8bFree(tree);
        done;
    }

    u8cs tree_data = {u8bDataHead(tree), u8bIdleHead(tree)};
    call(KEEPPackFeed, k, p, DOG_OBJ_TREE, tree_data, tree_out);
    u8bFree(tree);
    done;
}

// --- Public API ---

ok64 DELTree(sha1 *tree_out, sniff *s, keeper *k, keep_pack *p,
             u8cs reporoot, u8cs parent_hex, u8cp del_set) {
    sane(s && k && p && tree_out && del_set);

    call(SNIFFSort, s);

    u32 npath = SNIFFCount(s);
    u32 cap = npath + SNIFF_HASH_SIZE;
    Bsha1 sha_mem = {};
    call(sha1bAllocate, sha_mem, cap);
    memset(sha1bHead(sha_mem), 0, (u64)cap * sizeof(sha1));
    del_sha_ctx sha_tab = {.s = s, .shas = sha1bHead(sha_mem),
                           .capacity = cap};

    call(DELResolveParent, &sha_tab, k, parent_hex);

    u8cs no_prefix = {};
    ok64 o = DELBuild(tree_out, s, k, p, &sha_tab, reporoot,
                        del_set, 0, u32bDataLen(s->sorted), no_prefix);

    sha1bFree(sha_mem);
    return o;
}
