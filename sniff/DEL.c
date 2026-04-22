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
#include "keeper/KEEP.h"
#include "keeper/SHA1.h"

#include "STAGE.h"

// --- idx → sha1 lookup ---

static sha1cp DELGetSha(sha1p shas, u32 capacity, u32 idx) {
    if (idx >= capacity) return NULL;
    if (sha1empty(&shas[idx])) return NULL;
    return &shas[idx];
}

// --- Depth-first tree build, excluding del_set entries ---

static ok64 DELBuild(sha1 *tree_out, sniff *s, keeper *k,
                       keep_pack *p, sha1p sha_tab, u32 sha_cap,
                       u8cs reporoot, u8cp del_set,
                       u32 lo, u32 hi, u8cs prefix) {
    sane(p && sha_tab && tree_out && del_set);

    Bu8 tree = {};
    call(u8bAllocate, tree, (u64)(hi - lo) * 80);

    u32 root_idx = SNIFFRootIdx();
    u32 i = lo;
    while (i < hi) {
        u32 idx = *u32bDataAtP(s->sorted, i);
        if (idx == root_idx) { i++; continue; }  // skip root self-entry

        // Skip deleted entries
        if (del_set[idx]) {
            if (SNIFFIsDir(idx)) {
                // Skip entire subtree
                u8cs rel = {};
                SNIFFPath(rel, idx);
                u32 sub_hi = i + 1;
                while (sub_hi < hi) {
                    u32 sidx = *u32bDataAtP(s->sorted, sub_hi);
                    u8cs sp = {};
                    SNIFFPath(sp, sidx);
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
        if (SNIFFPath(rel, idx) != OK) { i++; continue; }

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
                SNIFFPath(sp, sidx);
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
                sha1cp old = DELGetSha(sha_tab, sha_cap, idx);
                if (old) {
                    sub_sha = *old;
                } else {
                    ok64 o = DELBuild(&sub_sha, s, k, p, sha_tab, sha_cap,
                                        reporoot, del_set,
                                        sub_lo, sub_hi, rel);
                    if (o != OK) { u8bFree(tree); return o; }
                }
            } else {
                ok64 o = DELBuild(&sub_sha, s, k, p, sha_tab, sha_cap,
                                    reporoot, del_set,
                                    sub_lo, sub_hi, rel);
                if (o != OK) { u8bFree(tree); return o; }
            }

            if (!sha1empty(&sub_sha)) {
                SNIFFRecord(SNIFF_TREE, idx,
                            WHIFFHashlet40(&sub_sha));
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
            sha1cp old = DELGetSha(sha_tab, sha_cap, idx);
            if (!old) { i++; continue; }

            u8cs name = {rest[0], rest[1]};

            // Recover mode from stat
            u8cs full_rel = {};
            SNIFFPath(full_rel, idx);
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
    u8csc nopath = {NULL, NULL};
    call(KEEPPackFeed, k, p, DOG_OBJ_TREE, tree_data, nopath, 0, tree_out);
    u8bFree(tree);
    done;
}

// Auto-scan: mark tracked files missing from disk.  Caller owns buf.
static void del_auto_scan(u8p del_set, sniff *s, u8cs reporoot) {
    u32 n = SNIFFCount();
    for (u32 i = 0; i < n; i++) {
        if (SNIFFIsDir(i)) continue;
        u8cs rel = {};
        if (SNIFFPath(rel, i) != OK) continue;
        if ($empty(rel)) continue;
        if (SNIFFGet(SNIFF_BLOB, i) == 0) continue;  // not tracked

        a_path(fp);
        if (SNIFFFullpath(fp, reporoot, rel) != OK) continue;
        struct stat lsb = {};
        if (lstat((char *)u8bDataHead(fp), &lsb) != 0) del_set[i] = 1;
    }
}

// --- Public API ---

ok64 DELStage(sha1 *tree_out, u8cs reporoot, u8cp del_set) {
    sane(tree_out);
    sniff *s = &SNIFF; keeper *k = &KEEP; (void)s; (void)k;

    a_pad(u8, brbuf, 256);
    call(STAGEBranch, brbuf);
    a_dup(u8c, branch, u8bData(brbuf));
    keep_pack pk = {};
    keep_pack *p = &pk;
    call(STAGEOpen, p, branch);

    ok64 so = SNIFFSort();
    if (so != OK) { STAGEClose(p, branch); return so; }

    u32 npath = SNIFFCount();
    u32 cap = npath + SNIFF_HASH_SIZE;

    // If caller didn't pass a set, auto-build one covering every
    // tracked file missing from disk.
    Bu8 dset_mem = {};
    u8cp effective_set = del_set;
    if (!effective_set) {
        call(u8bAllocate, dset_mem, npath);
        memset(u8bDataHead(dset_mem), 0, npath);
        del_auto_scan(u8bDataHead(dset_mem), s, reporoot);
        effective_set = u8bDataHead(dset_mem);
    }

    Bsha1 sha_mem = {};
    ok64 o = sha1bAllocate(sha_mem, cap);
    if (o != OK) { if (u8bData(dset_mem)[0]) u8bFree(dset_mem); return o; }
    memset(sha1bHead(sha_mem), 0, (u64)cap * sizeof(sha1));
    sha1p sha_tab = sha1bHead(sha_mem);

    o = SNIFFCollectBaseTree(sha_tab, cap);
    (void)o;  // advisory — see PUT.c

    u8cs no_prefix = {};
    o = DELBuild(tree_out, s, k, p, sha_tab, cap, reporoot,
                  effective_set, 0, u32bDataLen(s->sorted), no_prefix);

    if (o == OK && !sha1empty(tree_out)) {
        SNIFFRecord(SNIFF_TREE, SNIFFRootIdx(),
                    WHIFFHashlet40(tree_out));
    }
    //  Note: we intentionally leave SNIFF_BLOB / SNIFF_CHECKOUT intact
    //  for deleted paths.  A subsequent GETCheckout of the delete
    //  commit needs those fields non-zero so GETPrune can unlink the
    //  stale worktree file.  The file is still physically on disk.

    sha1bFree(sha_mem);
    if (u8bData(dset_mem)[0]) u8bFree(dset_mem);
    if (o == OK) o = STAGEClose(p, branch);
    else STAGEClose(p, branch);
    return o;
}
