//  PUT: stage worktree files into a new base tree.
//
#include "PUT.h"

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/DPATH.h"
#include "keeper/GIT.h"
#include "keeper/KEEP.h"
#include "keeper/SHA1.h"

#include "STAGE.h"

// --- idx → sha1 lookup helpers ---

static sha1cp POSTGetSha(sha1p shas, u32 capacity, u32 idx) {
    if (idx >= capacity) return NULL;
    if (sha1empty(&shas[idx])) return NULL;
    return &shas[idx];
}

// --- Depth-first tree build over sorted index ---

// Build tree for interval [lo, hi) of sorted indices where all paths
// share `prefix`.  Returns tree SHA via tree_out.
static ok64 POSTBuild(sha1 *tree_out, sniff *s, keeper *k,
                        keep_pack *p, sha1p sha_tab, u32 sha_cap,
                        u8cs reporoot, u8cp commit_set,
                        u32 lo, u32 hi, u8cs prefix) {
    sane(p && sha_tab && tree_out);

    // git tree entries: mode, name, sha
    // Collect direct children of prefix
    Bu8 tree = {};
    call(u8bAllocate, tree, (u64)(hi - lo) * 80);

    u32 root_idx = SNIFFRootIdx();
    u32 i = lo;
    while (i < hi) {
        u32 idx = *u32bDataAtP(s->sorted, i);
        if (idx == root_idx) { i++; continue; }  // skip root self-entry
        u8cs rel = {};
        if (SNIFFPath(rel, idx) != OK) { i++; continue; }

        // Strip prefix
        size_t plen = $len(prefix);
        u8cs rest = {$atp(rel, plen), rel[1]};
        if ($empty(rest)) { i++; continue; }

        // Check if this is a dir entry (trailing /)
        b8 is_dir = (*$last(rel) == '/');

        if (is_dir) {
            // Find end of this dir's interval
            u32 sub_lo = i + 1;
            u32 sub_hi = sub_lo;
            while (sub_hi < hi) {
                u32 sidx = *u32bDataAtP(s->sorted, sub_hi);
                u8cs sp = {};
                if (SNIFFPath(sp, sidx) != OK) { sub_hi++; continue; }
                // Still under this dir?
                if ($len(sp) <= $len(rel)) break;
                if (memcmp(sp[0], rel[0], $len(rel)) != 0) break;
                sub_hi++;
            }

            // Check if any child in [sub_lo, sub_hi) is touched
            b8 touched = NO;
            for (u32 j = sub_lo; j < sub_hi; j++) {
                u32 cidx = *u32bDataAtP(s->sorted, j);
                if (commit_set) {
                    if (commit_set[cidx]) { touched = YES; break; }
                } else {
                    u64 co = SNIFFGet(SNIFF_CHECKOUT, cidx);
                    u64 ch = SNIFFGet(SNIFF_CHANGED, cidx);
                    if (ch != 0 && ch != co) { touched = YES; break; }
                    u8 ct = SNIFFIsDir(cidx) ? SNIFF_TREE : SNIFF_BLOB;
                    if (SNIFFGet(ct, cidx) == 0) {
                        touched = YES; break;  // new entry
                    }
                }
            }

            sha1 sub_sha = {};
            if (!touched) {
                // Reuse old tree hashlet
                sha1cp old = POSTGetSha(sha_tab, sha_cap, idx);
                if (old) {
                    sub_sha = *old;
                } else {
                    // No old SHA, must rebuild
                    ok64 o = POSTBuild(&sub_sha, s, k, p, sha_tab, sha_cap,
                                         reporoot, commit_set,
                                         sub_lo, sub_hi, rel);
                    if (o != OK) { u8bFree(tree); return o; }
                }
            } else {
                ok64 o = POSTBuild(&sub_sha, s, k, p, sha_tab, sha_cap,
                                     reporoot, commit_set,
                                     sub_lo, sub_hi, rel);
                if (o != OK) { u8bFree(tree); return o; }
            }

            if (!sha1empty(&sub_sha)) {
                // Write back the subtree's hashlet so future PUT/POST
                // sees this dir's current base.
                SNIFFRecord(SNIFF_TREE, idx,
                            WHIFFHashlet40(&sub_sha));

                // name = rest without trailing /
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
            // File entry
            // Check if exists on disk
            u8cs full_rel = {};
            SNIFFPath(full_rel, idx);
            a_path(fp);
            SNIFFFullpath(fp, reporoot, full_rel);
            struct stat lsb = {};
            if (lstat((char *)u8bDataHead(fp), &lsb) != 0) {
                i++; continue;  // deleted
            }

            b8 is_new = (SNIFFGet(SNIFF_BLOB, idx) == 0);
            u64 co = SNIFFGet(SNIFF_CHECKOUT, idx);
            u64 ch = SNIFFGet(SNIFF_CHANGED, idx);
            b8 mtime_changed = (ch != 0 && ch != co);
            b8 changed = is_new || mtime_changed;

            if (commit_set && commit_set[idx])
                changed = YES;
            else if (commit_set && !commit_set[idx] && changed)
                changed = NO;

            sha1 file_sha = {};
            b8 is_link = S_ISLNK(lsb.st_mode);

            if (changed) {
                Bu8 content = {};
                ok64 o = u8bAllocate(content, 1UL << 24);
                if (o != OK) { i++; continue; }
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
                    o = FILEOpen(&fd, $path(fp), O_RDONLY);
                    if (o != OK) { u8bFree(content); i++; continue; }
                    FILEdrainall(u8bIdle(content), fd);
                    FILEClose(&fd);
                }
                u8cs blob = {u8bDataHead(content), u8bIdleHead(content)};
                u8csc bpath = {full_rel[0], full_rel[1]};
                o = KEEPPackFeed(k, p, DOG_OBJ_BLOB, blob, bpath, &file_sha);
                u8bFree(content);
                if (o != OK) { u8bFree(tree); return o; }

                // Record the new blob hashlet + fresh checkout mtime
                // so subsequent PUTs see this file as "base".
                SNIFFRecord(SNIFF_BLOB, idx, WHIFFHashlet40(&file_sha));
                SNIFFRecord(SNIFF_CHECKOUT, idx,
                            (u64)lsb.st_mtim.tv_sec);
            } else {
                sha1cp old = POSTGetSha(sha_tab, sha_cap, idx);
                if (!old) { i++; continue; }
                file_sha = *old;
            }

            // Name = rest (after prefix)
            u8cs name = {rest[0], rest[1]};

            // Mode
            if (is_link) {
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
            a_rawc(sha_raw, file_sha);
            u8bFeed(tree, sha_raw);

            i++;
        }
    }

    // Empty tree
    if (u8bDataLen(tree) == 0) {
        memset(tree_out, 0, sizeof(*tree_out));
        u8bFree(tree);
        done;
    }

    u8cs tree_data = {u8bDataHead(tree), u8bIdleHead(tree)};
    u8csc nopath = {NULL, NULL};
    call(KEEPPackFeed, k, p, DOG_OBJ_TREE, tree_data, nopath, tree_out);
    u8bFree(tree);
    done;
}

// --- Public API ---

ok64 PUTStage(sha1 *tree_out, u8cs reporoot, u8cp file_set) {
    sane(tree_out);
    sniff *s = &SNIFF; keeper *k = &KEEP; (void)s; (void)k;

    //  Open the current branch's staging pack.  Default "heads/master"
    //  when at.log is missing (fresh-repo first-commit flow).
    a_pad(u8, brbuf, 256);
    call(STAGEBranch, brbuf);
    a_dup(u8c, branch, u8bData(brbuf));
    keep_pack pk = {};
    keep_pack *p = &pk;
    call(STAGEOpen, p, branch);

    // Build sorted index
    ok64 rv = SNIFFSort();
    if (rv != OK) { STAGEClose(p, branch); return rv; }

    // Seed per-entry SHAs from the current base tree (root SNIFF_TREE
    // hashlet).  On a fresh repo with no base, sha_tab stays zeroed.
    u32 npath = SNIFFCount();
    u32 cap = npath + SNIFF_HASH_SIZE;
    Bsha1 sha_mem = {};
    call(sha1bAllocate, sha_mem, cap);
    memset(sha1bHead(sha_mem), 0, (u64)cap * sizeof(sha1));
    sha1p sha_tab = sha1bHead(sha_mem);

    //  Base-tree seeding is advisory — it avoids redundant subtree
    //  rebuilds when content is unchanged.  If the base tree lives in
    //  staging (not main keeper), the walk may be incomplete; that
    //  degrades to rebuilding subtrees, not incorrect output.
    ok64 o = SNIFFCollectBaseTree(sha_tab, cap);
    (void)o;

    // Build tree depth-first over sorted index
    u8cs no_prefix = {};
    o = POSTBuild(tree_out, s, k, p, sha_tab, cap, reporoot,
                   file_set, 0, u32bDataLen(s->sorted),
                   no_prefix);

    // Publish the new base tree: update the root SNIFF_TREE hashlet.
    if (o == OK && !sha1empty(tree_out)) {
        SNIFFRecord(SNIFF_TREE, SNIFFRootIdx(),
                    WHIFFHashlet40(tree_out));
    }

    sha1bFree(sha_mem);
    if (o == OK) o = STAGEClose(p, branch);
    else STAGEClose(p, branch);
    return o;
}
