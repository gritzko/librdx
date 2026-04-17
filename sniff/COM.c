//  COM: commit worktree files to keeper.
//
#include "COM.h"

#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/DPATH.h"
#include "keeper/GIT.h"
#include "keeper/SHA1.h"
#include "keeper/WALK.h"

// --- Collect old tree SHAs: path_index → sha1 ---

typedef struct {
    sniff *s;
    sha1s  shas;
    u32    capacity;
} sha_ctx;

static void com_set_sha(sha_ctx *ctx, u32 idx, sha1 const *sha) {
    if (idx >= ctx->capacity) return;
    ctx->shas[0][idx] = *sha;
}

static sha1cp com_get_sha(sha_ctx const *ctx, u32 idx) {
    if (idx >= ctx->capacity) return NULL;
    if (sha1empty(&ctx->shas[0][idx])) return NULL;
    return &ctx->shas[0][idx];
}

static ok64 com_collect_tree(sha_ctx *ctx, keeper *k,
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

        a_pad(u8, rel, 2048);
        if (!$empty(prefix)) {
            u8bFeed(rel, prefix);
            u8bFeed1(rel, '/');
        }
        u8bFeed(rel, name_s);
        PATHu8gTerm(PATHu8gIn(rel));
        u8cs relpath = {u8bDataHead(rel), rel[2]};

        u32 idx = SNIFFIntern(ctx->s, relpath);
        sha1cp esha1 = (sha1cp)esha[0];
        com_set_sha(ctx, idx, esha1);

        b8 is_dir = ($at(mode_s, 0) == '4');
        if (is_dir)
            com_collect_tree(ctx, k, esha1, relpath);
    }

    u8bFree(tcopy);
    done;
}

// --- Tree entry ---

typedef struct {
    u32  path_idx;
    sha1 sha;
    u8   mode[8];
    u8   mode_len;
    b8   is_dir;
} tree_entry;

// Build tree for one directory level.
// commit_set: NULL=all changed, else only commit_set[idx]==1 files.
// Files missing from disk are excluded (deletions).
static ok64 com_build_tree(sniff *s, keeper *k, keep_pack *p,
                           sha_ctx *sha_tab, u8cs reporoot,
                           u8cs dir_prefix, u8cp commit_set,
                           sha1 *sha_out) {
    sane(s && k && p && sha_tab && sha_out);

    tree_entry entries[4096];
    u8cs       names[4096];
    u32 nentries = 0;
    u32 n = SNIFFCount(s);

    for (u32 i = 0; i < n; i++) {
        u8cs rel = {};
        if (SNIFFPath(rel, s, i) != OK) continue;

        // Check if rel is under dir_prefix
        size_t plen = $len(dir_prefix);
        if (plen > 0) {
            if ($len(rel) <= plen) continue;
            if (memcmp(rel[0], dir_prefix[0], plen) != 0) continue;
            if ($at(rel, plen) != '/') continue;
            rel[0] = $atp(rel, plen + 1);
        }

        // Check if direct child (no further /)
        u8cs check = {rel[0], rel[1]};
        b8 has_slash = (u8csFind(check, '/') == OK);

        if (has_slash) {
            u8cs dirname = {rel[0], check[0]};

            // Dedup: check existing, evict file shadows
            b8 dup = NO;
            for (u32 j = 0; j < nentries; j++) {
                if ($len(names[j]) != $len(dirname)) continue;
                if (memcmp(names[j][0], dirname[0], $len(dirname)) != 0)
                    continue;
                if (entries[j].is_dir) { dup = YES; break; }
                entries[j] = entries[nentries - 1];
                names[j][0] = names[nentries - 1][0];
                names[j][1] = names[nentries - 1][1];
                nentries--;
                j--;
            }
            if (dup) continue;
            if (nentries >= 4096) continue;

            a_pad(u8, subdir, 2048);
            if (!$empty(dir_prefix)) {
                u8bFeed(subdir, dir_prefix);
                u8bFeed1(subdir, '/');
            }
            u8bFeed(subdir, dirname);
            PATHu8gTerm(PATHu8gIn(subdir));
            u8cs sub = {u8bDataHead(subdir), subdir[2]};

            tree_entry *e = &entries[nentries];
            e->path_idx = i;
            e->is_dir = YES;
            memcpy(e->mode, "40000", 5);
            e->mode_len = 5;
            names[nentries][0] = dirname[0];
            names[nentries][1] = dirname[1];

            ok64 o = com_build_tree(s, k, p, sha_tab, reporoot,
                                    sub, commit_set, &e->sha);
            if (o != OK) return o;
            // Skip empty subtrees (deleted dirs)
            if (sha1empty(&e->sha)) continue;
            nentries++;
        } else {
            // Direct child file — skip dir duplicates
            b8 is_dup_dir = NO;
            for (u32 j = 0; j < nentries; j++) {
                if (!entries[j].is_dir) continue;
                if ($len(names[j]) == $len(rel) &&
                    memcmp(names[j][0], rel[0], $len(rel)) == 0) {
                    is_dup_dir = YES;
                    break;
                }
            }
            if (is_dup_dir) continue;

            // Check if file exists on disk
            u8cs full_rel = {};
            SNIFFPath(full_rel, s, i);
            a_path(fp);
            SNIFFFullpath(fp, reporoot, full_rel);
            struct stat lsb = {};
            b8 exists = (lstat((char *)u8bDataHead(fp), &lsb) == 0);
            if (!exists) continue;  // deleted file — exclude

            // Determine if this file is "changed" for commit purposes
            b8 is_new = (SNIFFGet(s, SNIFF_HASHLET, i) == 0);
            u64 co = SNIFFGet(s, SNIFF_CHECKOUT, i);
            u64 ch = SNIFFGet(s, SNIFF_CHANGED, i);
            b8 mtime_changed = (ch != 0 && ch != co);
            b8 changed = is_new || mtime_changed;

            // If commit_set specified, only include selected files
            // (changed ones get new blobs, others keep old SHA)
            if (commit_set && commit_set[i]) {
                changed = YES;  // force new blob for selected files
            } else if (commit_set && !commit_set[i] && changed) {
                changed = NO;  // not selected, keep old SHA
            }

            if (nentries >= 4096) continue;
            tree_entry *e = &entries[nentries];
            e->path_idx = i;
            e->is_dir = NO;

            b8 is_link = S_ISLNK(lsb.st_mode);

            if (changed) {
                Bu8 content = {};
                ok64 o = u8bAllocate(content, 1UL << 24);
                if (o != OK) continue;
                if (is_link) {
                    char target[1024];
                    ssize_t tlen = readlink(
                        (char *)u8bDataHead(fp), target, sizeof(target));
                    if (tlen > 0) {
                        u8cs ts = {(u8cp)target, (u8cp)target + tlen};
                        u8bFeed(content, ts);
                    }
                } else {
                    int fd = -1;
                    o = FILEOpen(&fd, PATHu8cgIn(fp), O_RDONLY);
                    if (o != OK) { u8bFree(content); continue; }
                    FILEdrainall(u8bIdle(content), fd);
                    FILEClose(&fd);
                }
                u8cs blob = {u8bDataHead(content), u8bIdleHead(content)};
                o = KEEPPackFeed(k, p, DOG_OBJ_BLOB, blob, &e->sha);
                u8bFree(content);
                if (o != OK) return o;
            } else {
                sha1cp old_sha = com_get_sha(sha_tab, i);
                if (!old_sha) continue;
                e->sha = *old_sha;
            }

            // Mode from lstat
            if (is_link) {
                memcpy(e->mode, "120000", 6);
                e->mode_len = 6;
            } else if (lsb.st_mode & S_IXUSR) {
                memcpy(e->mode, "100755", 6);
                e->mode_len = 6;
            } else {
                memcpy(e->mode, "100644", 6);
                e->mode_len = 6;
            }

            names[nentries][0] = rel[0];
            names[nentries][1] = rel[1];
            nentries++;
        }
    }

    // Sort entries by name (insertion sort)
    for (u32 i = 1; i < nentries; i++) {
        tree_entry etmp = entries[i];
        u8cs ntmp = {names[i][0], names[i][1]};
        u32 j = i;
        while (j > 0) {
            size_t la = $len(names[j - 1]);
            size_t lb = $len(ntmp);
            size_t minl = la < lb ? la : lb;
            int c = memcmp(names[j - 1][0], ntmp[0], minl);
            if (c == 0) c = (la > lb) - (la < lb);
            if (c <= 0) break;
            entries[j] = entries[j - 1];
            names[j][0] = names[j - 1][0];
            names[j][1] = names[j - 1][1];
            j--;
        }
        entries[j] = etmp;
        names[j][0] = ntmp[0];
        names[j][1] = ntmp[1];
    }

    // Empty tree → signal to parent via zeroed sha_out
    if (nentries == 0) {
        memset(sha_out, 0, sizeof(*sha_out));
        done;
    }

    // Serialize tree
    Bu8 tree = {};
    call(u8bAllocate, tree, nentries * 80);
    for (u32 i = 0; i < nentries; i++) {
        tree_entry *e = &entries[i];
        u8cs mode = {e->mode, e->mode + e->mode_len};
        u8bFeed(tree, mode);
        u8bFeed1(tree, ' ');
        u8bFeed(tree, names[i]);
        u8bFeed1(tree, 0);
        a_rawc(sha, e->sha);
        u8bFeed(tree, sha);
    }

    u8cs tree_data = {u8bDataHead(tree), u8bIdleHead(tree)};
    call(KEEPPackFeed, k, p, DOG_OBJ_TREE, tree_data, sha_out);
    u8bFree(tree);
    done;
}

// --- Public API ---

ok64 COMCommit(sniff *s, keeper *k, u8cs reporoot,
               u8cs parent_hex, u8cs message, u8cs author,
               u8cp commit_set, sha1 *sha_out) {
    sane(s && k && $ok(parent_hex) && $ok(message) && $ok(author));

    size_t hexlen = $len(parent_hex);
    if (hexlen > 15) hexlen = 15;
    u64 parent_hashlet = WHIFFHexHashlet60(parent_hex);

    Bu8 cbuf = {};
    call(u8bAllocate, cbuf, 1UL << 24);
    u8 ctype = 0;
    call(KEEPGet, k, parent_hashlet, hexlen, cbuf, &ctype);

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

    // Parent SHA from content
    sha1 parent_sha = {};
    {
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
        SHA1Sum(&parent_sha, _d);
        u8bFree(tmp);
    }

    // Parent's tree SHA
    sha1 parent_tree_sha = {};
    u8cs commit_body = {u8bDataHead(cbuf), u8bIdleHead(cbuf)};
    call(WALKCommitTree, commit_body, parent_tree_sha.data);  // keeper API boundary
    u8bFree(cbuf);

    // Collect old tree SHAs
    u32 npath = SNIFFCount(s);
    u32 cap = npath + SNIFF_HASH_SIZE;
    Bsha1 sha_mem = {};
    call(sha1bAllocate, sha_mem, cap);
    memset(sha1bHead(sha_mem), 0, (u64)cap * sizeof(sha1));
    sha1s sha_slice = {sha1bHead(sha_mem), sha1bHead(sha_mem) + cap};
    sha_ctx sha_tab = {.s = s, .shas = {sha_slice[0], sha_slice[1]},
                       .capacity = cap};

    u8cs no_prefix = {};
    call(com_collect_tree, &sha_tab, k, &parent_tree_sha, no_prefix);

    // Start pack
    keep_pack p = {};
    call(KEEPPackOpen, k, &p);

    // Build tree
    sha1 root_tree_sha = {};
    ok64 o = com_build_tree(s, k, &p, &sha_tab, reporoot,
                            no_prefix, commit_set, &root_tree_sha);
    sha1bFree(sha_mem);
    if (o != OK) { KEEPPackClose(k, &p); return o; }

    // Build commit object
    Bu8 com = {};
    call(u8bAllocate, com, 4096);

    a_cstr(tree_label, "tree ");
    u8bFeed(com, tree_label);
    a_pad(u8, tree_hex, 40);
    a_rawc(tsha, root_tree_sha);
    HEXu8sFeedSome(tree_hex_idle, tsha);
    u8bFeed(com, u8bDataC(tree_hex));
    u8bFeed1(com, '\n');

    a_cstr(par_label, "parent ");
    u8bFeed(com, par_label);
    a_pad(u8, par_hex, 40);
    a_rawc(psha, parent_sha);
    HEXu8sFeedSome(par_hex_idle, psha);
    u8bFeed(com, u8bDataC(par_hex));
    u8bFeed1(com, '\n');

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

    u8cs com_data = {u8bDataHead(com), u8bIdleHead(com)};
    call(KEEPPackFeed, k, &p, DOG_OBJ_COMMIT, com_data, sha_out);
    u8bFree(com);

    call(KEEPPackClose, k, &p);

    a_pad(u8, out_hex, 40);
    u8cs osha = {sha_out->data, sha_out->data + GIT_SHA1_LEN};
    HEXu8sFeedSome(out_hex_idle, osha);
    fprintf(stderr, "sniff: commit %.*s\n",
            (int)u8bDataLen(out_hex), (char *)u8bDataHead(out_hex));
    done;
}
