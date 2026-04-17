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
    sha1p sha_tab = sha1bHead(sha_mem);

    call(SNIFFCollectParentTree, s, k, parent_hex, sha_tab, cap);

    u8cs no_prefix = {};
    ok64 o = DELBuild(tree_out, s, k, p, sha_tab, cap, reporoot,
                        del_set, 0, u32bDataLen(s->sorted), no_prefix);

    sha1bFree(sha_mem);
    return o;
}
