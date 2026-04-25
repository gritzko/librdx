//  POST: commit the current worktree state.
//
//  Inputs at commit time: the worktree on disk and the ULOG.  The most
//  recent `get` / `post` / `patch` row names a baseline tree URI
//  (single hash → keeper, multiple → graf — graf is not wired yet and
//  defaults to keeper-single-hash-only for now).  `put` / `delete`
//  rows since the last `post` are the explicit staging intent.
//
//  Per-file change-set at commit time:
//    * path matches a `put <path>` row (since last post)   → rewrite
//    * path matches a `delete <path>` row                  → drop
//    * no explicit row for the path, any put/delete exists → carry
//      over from baseline (or drop if missing from wt)
//    * no put/delete rows at all since last post           → implicit
//      all-dirty: mtime ∉ stamp-set → rewrite; missing → drop.
//
//  Pack layout: one keeper pack with `strict_order=NO`, fed in the
//  order commit → trees → blobs (forward refs permitted).
//
#include "POST.h"

#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/IGNO.h"
#include "dog/QURY.h"
#include "keeper/GIT.h"
#include "keeper/REFS.h"
#include "keeper/SHA1.h"
#include "keeper/WALK.h"

#include "AT.h"

// --- Per-path state ---
//
//  Parallel arrays keyed by the path-registry index (SNIFFIntern).
//  Allocated once per POSTCommit invocation and freed at the end.

enum post_flag {
    POST_IN_BASE  = 1 << 0,  // path had an entry in the baseline tree
    POST_ON_DISK  = 1 << 1,  // path currently exists on disk
    POST_EXPL_PUT = 1 << 2,  // explicit `put <path>` since last post
    POST_EXPL_DEL = 1 << 3,  // explicit `delete <path>` since last post
    POST_REWRITE  = 1 << 4,  // fate: pull content from wt, emit new blob
    POST_KEEP     = 1 << 5,  // fate: reuse baseline sha
    POST_DROP     = 1 << 6,  // fate: omit from new tree
};

typedef struct {
    sha1   old_sha;     // from baseline (empty if POST_IN_BASE unset)
    sha1   new_sha;     // final sha (empty if dropped)
    u16    old_mode;    // from baseline
    u16    new_mode;    // final
    Bu8    content;     // blob content for rewrites (freed at end)
} post_rec;

#define POST_MAX_DIR_ROWS 64

typedef struct {
    keeper     *k;
    u8cs        reporoot;
    post_rec   *rec;
    u8         *flag;
    u32         cap;
    b8          any_pd;     // any put/delete rows since last post
    b8          base_is_patch;  // baseline row is a `patch`, not get/post
    b8          has_base;   // baseline row exists (any get/post/patch)
    ron60       last_post_ts;
    ok64        error;
    //  Dir-level put/delete prefixes (trailing '/').  Expanded after
    //  the wt scan so baseline / on-disk presence is known.
    u8cs        dir_puts[POST_MAX_DIR_ROWS];
    u32         n_dir_puts;
    u8cs        dir_dels[POST_MAX_DIR_ROWS];
    u32         n_dir_dels;
} post_ctx;

// --- git mode helpers ---

static u16 post_mode_of_kind(u8 kind) {
    switch (kind) {
        case WALK_KIND_REG: return 0100644;
        case WALK_KIND_EXE: return 0100755;
        case WALK_KIND_LNK: return 0120000;
        case WALK_KIND_DIR: return 040000;
        case WALK_KIND_SUB: return 0160000;
    }
    return 0100644;
}

static void post_mode_feed(Bu8 tree, u16 mode) {
    //  Git modes are printed in octal without leading zeros.  All four
    //  values we emit are 5- or 6-digit strings.
    char buf[8];
    int n = snprintf(buf, sizeof(buf), "%o", (unsigned)mode);
    u8cs m = {(u8cp)buf, (u8cp)buf + n};
    u8bFeed(tree, m);
}

// --- ULOG scans ---

static ok64 post_pd_cb(ron60 verb, u8cs path, ron60 ts, void *vctx) {
    sane(vctx);
    (void)ts;
    post_ctx *c = (post_ctx *)vctx;
    c->any_pd = YES;
    u32 idx = SNIFFIntern(path);
    if (idx >= c->cap) return OK;
    if (verb == SNIFFAtVerbPut())    c->flag[idx] |= POST_EXPL_PUT;
    if (verb == SNIFFAtVerbDelete()) c->flag[idx] |= POST_EXPL_DEL;

    //  Trailing-slash paths are dir markers; stash the prefix so the
    //  expansion pass can walk baseline / wt under it.  Exact-path
    //  flags above are harmless on dir indices — post_decide skips
    //  them via SNIFFIsDir.
    if (!$empty(path) && *u8csLast(path) == '/') {
        if (verb == SNIFFAtVerbPut() && c->n_dir_puts < POST_MAX_DIR_ROWS) {
            c->dir_puts[c->n_dir_puts][0] = path[0];
            c->dir_puts[c->n_dir_puts][1] = path[1];
            c->n_dir_puts++;
        }
        if (verb == SNIFFAtVerbDelete() && c->n_dir_dels < POST_MAX_DIR_ROWS) {
            c->dir_dels[c->n_dir_dels][0] = path[0];
            c->dir_dels[c->n_dir_dels][1] = path[1];
            c->n_dir_dels++;
        }
    }
    return OK;
}

// --- Baseline walk ---

typedef struct {
    post_ctx *c;
} base_ctx;

static ok64 post_base_visit(u8cs path, u8 kind, u8cp esha, u8cs blob,
                             void0p vctx) {
    sane(vctx);
    (void)blob;
    base_ctx *b = (base_ctx *)vctx;
    post_ctx *c = b->c;
    if (kind == WALK_KIND_DIR) {
        //  Root dir arrives with empty path; inner dirs get recorded
        //  only if we plan to emit them (we'll derive that from the
        //  set of files during tree-build).  We don't need per-dir
        //  records here.
        return OK;
    }
    u32 idx = SNIFFIntern(path);
    if (idx >= c->cap) return OK;
    c->flag[idx] |= POST_IN_BASE;
    memcpy(c->rec[idx].old_sha.data, esha, 20);
    c->rec[idx].old_mode = post_mode_of_kind(kind);
    if (kind == WALK_KIND_SUB) {
        //  Gitlink (submodule).  Carry it through to the new tree
        //  verbatim — POST is not in the business of advancing
        //  submodule shas (that's what `git submodule update` does).
        //  Without this entry, the wt scan + post_decide treat the
        //  baseline gitlink as "missing on disk" and DROP it,
        //  silently removing the submodule from history.
        c->flag[idx] |= POST_KEEP;
        return WALKSKIP;
    }
    return OK;
}

static ok64 post_load_baseline(post_ctx *c, sha1 *root_out, b8 *has_out) {
    sane(c && root_out && has_out);
    *has_out = NO;

    ron60 base_ts = 0, base_verb = 0;
    uri base_u = {};
    ok64 br = SNIFFAtBaseline(&base_ts, &base_verb, &base_u);
    if (br == ULOGNONE) done;  // fresh repo
    if (br != OK) return br;
    c->has_base      = YES;
    c->base_is_patch = (base_verb == SNIFFAtVerbPatch());

    //  Baseline query carries the version info (see dog/QURY): one
    //  branch REF plus 1-to-N SHAs.  For a squash-merge POST we only
    //  need the ours tree as the baseline — patched files are
    //  mtime-dirty (PATCH does not stamp), added files aren't in ours
    //  and fall into POST_REWRITE via the implicit-dirty rule, and
    //  deleted files were unlinked by PATCH so they vanish via the
    //  implicit-drop rule.  So take the first SHA spec as "ours".
    u8 hex40[40];
    if (SNIFFAtQueryFirstSha(&base_u, hex40) != OK) done;

    sha1 commit_sha = {};
    a_raw(csha_bin, commit_sha);
    u8cs h40 = {hex40, hex40 + 40};
    HEXu8sDrainSome(csha_bin, h40);

    Bu8 cbuf = {};
    call(u8bAllocate, cbuf, 1UL << 24);
    u8 ctype = 0;
    ok64 go = KEEPGetExact(c->k, &commit_sha, cbuf, &ctype);
    if (go != OK || ctype != DOG_OBJ_COMMIT) {
        u8bFree(cbuf);
        done;
    }

    sha1 tree_sha = {};
    u8cs body = {u8bDataHead(cbuf), u8bIdleHead(cbuf)};
    ok64 to = GITu8sCommitTree(body, tree_sha.data);
    u8bFree(cbuf);
    if (to != OK) done;

    base_ctx bctx = {.c = c};
    ok64 wo = WALKTreeLazy(c->k, tree_sha.data, post_base_visit, &bctx);
    if (wo != OK) return wo;

    *root_out = tree_sha;
    *has_out = YES;
    done;
}

// --- Worktree scan ---

static ok64 post_wt_callback(void *varg, path8bp path) {
    sane(varg && path);
    post_ctx *c = (post_ctx *)varg;
    a_dup(u8c, full, u8bData(path));

    u8cs rel = {};
    if (!SNIFFRelFromFull(&rel, c->reporoot, full)) return OK;
    if (SNIFFSkipMeta(rel))                         return OK;

    //  lstat the file to capture mode + mtime.
    struct stat lsb = {};
    if (lstat((char const *)full[0], &lsb) != 0) return OK;

    u16 mode;
    if (S_ISLNK(lsb.st_mode))               mode = 0120000;
    else if (lsb.st_mode & S_IXUSR)         mode = 0100755;
    else                                    mode = 0100644;

    u32 idx = SNIFFIntern(rel);
    if (idx >= c->cap) return OK;

    c->flag[idx] |= POST_ON_DISK;
    c->rec[idx].new_mode = mode;
    return OK;
}

static ok64 post_scan_wt(post_ctx *c) {
    sane(c);
    a_path(root_path);
    u8bFeed(root_path, c->reporoot);
    call(PATHu8bTerm, root_path);
    return FILEScan(root_path,
                    (FILE_SCAN)(FILE_SCAN_FILES | FILE_SCAN_LINKS |
                                FILE_SCAN_DEEP),
                    post_wt_callback, c);
}

// --- Dir-level put/delete expansion ---
//
//  `be put dir/` and `be delete dir/` record one ULOG row with a
//  trailing slash.  The expansion pass turns each such row into a
//  set of file-level flag updates so post_decide can do its usual
//  per-path logic.  Runs AFTER baseline load + wt scan so both
//  sources of truth are available.
//
//  Rules (v1):
//    * delete: flag every baseline path strictly under the prefix as
//      POST_EXPL_DEL.  POST's unlink loop then drops them from disk.
//    * put, existing dir (any baseline file under prefix): flag those
//      baseline paths as POST_EXPL_PUT.  Untracked on-disk siblings
//      under the same prefix stay unflagged and fall through
//      post_decide's "selective, no explicit rule, not in base" case
//      — i.e. ignored.
//    * put, new dir (no baseline hit): walk the wt under the prefix
//      and flag each non-ignored file as POST_EXPL_PUT.  IGNO is
//      loaded once from the wt root (no nested cascade).

//  True iff `p` lives strictly beneath `prefix` (prefix must end '/').
fun b8 post_path_under(u8cs p, u8cs prefix) {
    size_t plen = $len(prefix);
    if (plen == 0) return NO;
    if ($len(p) <= plen) return NO;
    return memcmp(p[0], prefix[0], plen) == 0;
}

typedef struct {
    post_ctx *c;
    ignocp    ig;      // NULL if no .gitignore at wt root
} wt_put_ctx;

static ok64 post_expand_put_wt_cb(void *varg, path8bp path) {
    sane(varg && path);
    wt_put_ctx *w = (wt_put_ctx *)varg;
    post_ctx *c = w->c;
    a_dup(u8c, full, u8bData(path));

    u8cs rel = {};
    if (!SNIFFRelFromFull(&rel, c->reporoot, full)) return OK;
    if (SNIFFSkipMeta(rel))                         return OK;
    if (w->ig && IGNOMatch(w->ig, rel, NO))         return OK;

    u32 idx = SNIFFIntern(rel);
    if (idx >= c->cap) return OK;

    //  Capture mode + on-disk flag here — the file may have been
    //  interned only now (post_scan_wt sees it too, but scan order
    //  isn't contractual with the registry).
    struct stat lsb = {};
    if (lstat((char const *)full[0], &lsb) == 0) {
        u16 mode;
        if (S_ISLNK(lsb.st_mode))           mode = 0120000;
        else if (lsb.st_mode & S_IXUSR)     mode = 0100755;
        else                                mode = 0100644;
        c->rec[idx].new_mode = mode;
        c->flag[idx] |= POST_ON_DISK;
    }
    c->flag[idx] |= POST_EXPL_PUT;
    return OK;
}

static ok64 post_expand_dir_rows(post_ctx *c, ignocp ig) {
    sane(c);

    //  DELETE: baseline-only.
    for (u32 di = 0; di < c->n_dir_dels; di++) {
        a_dup(u8c, pfx, c->dir_dels[di]);
        u32 n = SNIFFCount();
        for (u32 i = 0; i < n && i < c->cap; i++) {
            if (!(c->flag[i] & POST_IN_BASE)) continue;
            if (SNIFFIsDir(i))                continue;
            u8cs p = {};
            if (SNIFFPath(p, i) != OK)        continue;
            if (!post_path_under(p, pfx))     continue;
            c->flag[i] |= POST_EXPL_DEL;
        }
    }

    //  PUT: try baseline expansion first; fall through to wt walk
    //  only when the prefix names a dir that wasn't tracked yet.
    for (u32 di = 0; di < c->n_dir_puts; di++) {
        a_dup(u8c, pfx, c->dir_puts[di]);
        b8 any_base = NO;
        u32 n = SNIFFCount();
        for (u32 i = 0; i < n && i < c->cap; i++) {
            if (!(c->flag[i] & POST_IN_BASE)) continue;
            if (SNIFFIsDir(i))                continue;
            u8cs p = {};
            if (SNIFFPath(p, i) != OK)        continue;
            if (!post_path_under(p, pfx))     continue;
            c->flag[i] |= POST_EXPL_PUT;
            any_base = YES;
        }
        if (any_base) continue;

        //  New dir — walk the wt subtree with wt-root IGNO applied.
        a_path(full);
        if (SNIFFFullpath(full, c->reporoot, pfx) != OK) continue;
        wt_put_ctx wc = {.c = c, .ig = ig};
        (void)FILEScan(full,
                       (FILE_SCAN)(FILE_SCAN_FILES | FILE_SCAN_LINKS |
                                   FILE_SCAN_DEEP),
                       post_expand_put_wt_cb, &wc);
    }
    done;
}

// --- Change-set resolution ---

static ok64 post_decide(post_ctx *c, u32 idx) {
    sane(c);
    u8 f = c->flag[idx];

    //  Skip directory entries in the registry — they are reconstructed
    //  during tree-build rather than selected here.
    if (SNIFFIsDir(idx)) return OK;
    if (!(f & (POST_IN_BASE | POST_ON_DISK))) return OK;

    //  Gitlinks (submodule entries, mode 0160000) are carried through
    //  verbatim — POST_KEEP was already set by post_base_visit.  The
    //  on-disk presence rule below would otherwise see "no file at
    //  path" (the gitlink references a different repo, not a file in
    //  this one) and drop the entry.  Skip the rest of the logic.
    if (c->rec[idx].old_mode == 0160000) {
        c->flag[idx] |= POST_KEEP;
        return OK;
    }

    //  Explicit rules win.
    if (f & POST_EXPL_DEL) {
        c->flag[idx] |= POST_DROP;
        return OK;
    }
    if (f & POST_EXPL_PUT) {
        if (!(f & POST_ON_DISK)) {   // explicit put of a missing file
            c->flag[idx] |= POST_DROP;
            return OK;
        }
        c->flag[idx] |= POST_REWRITE;
        return OK;
    }

    //  Missing from wt — only drop when implicit mode and the path is
    //  visible to the wt scan.  Gitignored baseline files (e.g. `.be`,
    //  `.gitignore`-listed) are filtered out by `post_wt_callback` →
    //  `SNIFFSkipMeta`, so they never get POST_ON_DISK even though the
    //  bytes are right there on disk.  Treating them as deletions
    //  silently strips them from history.  Honor the gitignore by
    //  keeping the baseline entry verbatim.
    if (!(f & POST_ON_DISK)) {
        u8cs rel_chk = {};
        if ((f & POST_IN_BASE) &&
            SNIFFPath(rel_chk, idx) == OK &&
            SNIFFSkipMeta(rel_chk)) {
            c->flag[idx] |= POST_KEEP;
            return OK;
        }
        if (c->any_pd) {
            //  No explicit rule for this path and we are in selective
            //  mode: keep the baseline entry unchanged.
            if (f & POST_IN_BASE) c->flag[idx] |= POST_KEEP;
        } else {
            //  Implicit mode: a missing file is a deletion.
            c->flag[idx] |= POST_DROP;
        }
        return OK;
    }

    //  On disk, no explicit rule.  Selective and implicit modes share
    //  the same logic here: tracked files get the auto-mtime check
    //  (KEEP if clean, REWRITE if dirty), untracked files are ignored
    //  unless `be put` named them (handled above as POST_EXPL_PUT).
    //  In selective mode this is critical — without it, `be put X` on
    //  one new file would silently drop in-flight modifications to
    //  every other tracked file.
    if (!(f & POST_IN_BASE) && c->any_pd) {
        //  Untracked + selective mode + no put = ignore.  (Implicit
        //  mode falls through to handle fresh-repo first-commit and
        //  base-is-patch cases below.)
        return OK;
    }

    a_path(fp);
    u8cs rel = {};
    call(SNIFFPath, rel, idx);
    call(SNIFFFullpath, fp, c->reporoot, rel);
    struct stat sb = {};
    if (lstat((char const *)u8bDataHead(fp), &sb) != 0) {
        c->flag[idx] |= POST_DROP;
        return OK;
    }
    //  Squash-merge case: baseline is a `patch` row, which stamps its
    //  merge output with the row's own ts.  Those stamps make mtimes
    //  "known", but the disk bytes are the merge result — NOT the
    //  ours-tree baseline — so KEEP (use old_sha) would regress the
    //  new commit's tree to pre-merge content.  Force REWRITE from
    //  disk for every on-disk file.
    if (c->base_is_patch) {
        c->flag[idx] |= POST_REWRITE;
        return OK;
    }

    struct timespec ts = {.tv_sec = sb.st_mtim.tv_sec,
                          .tv_nsec = sb.st_mtim.tv_nsec};
    ron60 mtime_r = SNIFFAtOfTimespec(ts);
    if (SNIFFAtKnown(mtime_r)) {
        //  Clean: keep baseline sha if present.
        if (f & POST_IN_BASE) c->flag[idx] |= POST_KEEP;
        //  Otherwise: untracked clean (shouldn't happen but ignore).
    } else if (f & POST_IN_BASE) {
        //  Tracked + dirty → restage from disk.
        c->flag[idx] |= POST_REWRITE;
    } else if (!c->has_base) {
        //  Fresh-repo first commit (no baseline exists yet): auto-stage
        //  every dirty file.  Once any baseline lands, implicit
        //  `be post -m` reverts to "tracked-only" semantics — a new
        //  untracked file then needs an explicit `be put`.
        c->flag[idx] |= POST_REWRITE;
    }
    //  Else: untracked + dirty + already-have-a-baseline → ignore in
    //  implicit mode.  Side-checkouts (abc/, build/, scratch *.diff)
    //  don't leak into commits that way; the user names them via
    //  `be put <path>` if they really want to add new tracked paths.
    return OK;
}

// --- Hashing new blobs ---

static ok64 post_hash_one(post_ctx *c, u32 idx) {
    sane(c);
    post_rec *r = &c->rec[idx];
    u8 f = c->flag[idx];
    if (!(f & POST_REWRITE)) done;

    u8cs rel = {};
    call(SNIFFPath, rel, idx);
    a_path(fp);
    call(SNIFFFullpath, fp, c->reporoot, rel);

    struct stat lsb = {};
    if (lstat((char const *)u8bDataHead(fp), &lsb) != 0) {
        //  Disappeared between scan and hash — treat as drop.
        c->flag[idx] &= ~POST_REWRITE;
        c->flag[idx] |= POST_DROP;
        done;
    }

    call(u8bAllocate, r->content, 1UL << 20);

    if (S_ISLNK(lsb.st_mode)) {
        char target[1024];
        ssize_t tlen = readlink((char const *)u8bDataHead(fp),
                                target, sizeof(target));
        if (tlen > 0) {
            u8cs tv = {(u8cp)target, (u8cp)target + tlen};
            u8bFeed(r->content, tv);
        }
    } else {
        int fd = -1;
        ok64 oo = FILEOpen(&fd, $path(fp), O_RDONLY);
        if (oo != OK) return oo;
        FILEdrainall(u8bIdle(r->content), fd);
        FILEClose(&fd);
    }

    KEEPObjSha(&r->new_sha, DOG_OBJ_BLOB, u8bDataC(r->content));
    done;
}

// --- Tree building (bottom-up from sorted paths) ---

typedef struct {
    u32    lo, hi;    // sorted-index range
    u8cs   prefix;    // directory prefix these entries live under (with trailing '/')
} tree_range;

//  Locate the end of the range whose sorted paths all start with
//  `prefix` (exclusive).  Caller guarantees [lo..hi) is sorted.
static u32 post_range_end(u32 lo, u32 hi, u8cs prefix) {
    u32 end = lo;
    while (end < hi) {
        u8cs p = {};
        u32 idx = *u32bDataAtP(SNIFF.sorted, end);
        if (SNIFFPath(p, idx) != OK) { end++; continue; }
        size_t plen = $len(prefix);
        if ($len(p) < plen) break;
        if (memcmp(p[0], prefix[0], plen) != 0) break;
        end++;
    }
    return end;
}

static ok64 post_build_tree(post_ctx *c, u32 lo, u32 hi, u8cs prefix,
                            sha1 *tree_out, Bu8 tree_body_list,
                            u32 *emit_count) {
    //  Recursively build a tree for paths in [lo, hi) under `prefix`.
    //  Emits serialized tree body bytes (prefixed by u32 length) into
    //  `tree_body_list`.  The caller replays the list later to feed
    //  keeper in the pack's expected commit→trees→blobs order.
    sane(c && tree_out);

    Bu8 tree = {};
    call(u8bAllocate, tree, (u64)(hi - lo) * 80);

    u32 i = lo;
    while (i < hi) {
        u32 idx = *u32bDataAtP(SNIFF.sorted, i);
        u8cs rel = {};
        if (SNIFFPath(rel, idx) != OK) { i++; continue; }

        size_t plen = $len(prefix);
        if ($len(rel) <= plen) { i++; continue; }
        u8cs rest = {$atp(rel, plen), rel[1]};
        if ($empty(rest)) { i++; continue; }

        //  Find first '/' in rest to distinguish direct-child files
        //  from entries in deeper subtrees.
        u8c const *slash = NULL;
        {
            a_dup(u8c, scan, rest);
            if (u8csFind(scan, '/') == OK) slash = scan[0];
        }

        if (slash) {
            //  Sub-directory of this tree: recurse over children.
            u8cs dirname = {rest[0], slash};
            a_pad(u8, subprefix, 2048);
            u8bFeed(subprefix, prefix);
            u8bFeed(subprefix, dirname);
            u8bFeed1(subprefix, '/');
            u8cs sub = {u8bDataHead(subprefix), subprefix[2]};

            u32 sub_hi = post_range_end(i, hi, sub);

            sha1 sub_sha = {};
            ok64 so = post_build_tree(c, i, sub_hi, sub, &sub_sha,
                                      tree_body_list, emit_count);
            if (so != OK) { u8bFree(tree); return so; }

            if (!sha1empty(&sub_sha)) {
                post_mode_feed(tree, 040000);
                u8bFeed1(tree, ' ');
                u8bFeed(tree, dirname);
                u8bFeed1(tree, 0);
                a_rawc(sr, sub_sha);
                u8bFeed(tree, sr);
            }
            i = sub_hi;
            continue;
        }

        //  Direct-child file entry.
        u8 f = c->flag[idx];
        if (f & POST_DROP) { i++; continue; }
        if (!(f & (POST_KEEP | POST_REWRITE))) { i++; continue; }

        sha1 entry_sha = (f & POST_REWRITE)
            ? c->rec[idx].new_sha
            : c->rec[idx].old_sha;
        if (sha1empty(&entry_sha)) { i++; continue; }

        u16 mode = (f & POST_REWRITE)
            ? c->rec[idx].new_mode
            : c->rec[idx].old_mode;
        if (mode == 0) mode = 0100644;

        post_mode_feed(tree, mode);
        u8bFeed1(tree, ' ');
        u8bFeed(tree, rest);
        u8bFeed1(tree, 0);
        a_rawc(er, entry_sha);
        u8bFeed(tree, er);
        i++;
    }

    if (u8bDataLen(tree) == 0) {
        memset(tree_out, 0, sizeof(*tree_out));
        u8bFree(tree);
        done;
    }

    KEEPObjSha(tree_out, DOG_OBJ_TREE, u8bDataC(tree));

    //  Record (len u32, body bytes) in tree_body_list; the feeder
    //  loop in POSTCommit parses them back out to hand to keeper.
    u32 tlen = (u32)u8bDataLen(tree);
    u8cs tl = {(u8cp)&tlen, (u8cp)&tlen + sizeof(u32)};
    u8bFeed(tree_body_list, tl);
    u8bFeed(tree_body_list, u8bDataC(tree));
    (*emit_count)++;

    u8bFree(tree);
    done;
}

// --- Empty-tree feed (handles "no files to commit" case) ---

static ok64 post_feed_empty_tree(keeper *k, keep_pack *p, sha1 *out) {
    u8cs empty = {};
    u8csc nopath = {NULL, NULL};
    return KEEPPackFeed(k, p, DOG_OBJ_TREE, empty, nopath, 0, out);
}

// --- Resolve parent commit sha for the commit body ---

static ok64 post_parent_sha(keeper *k, u8cs parent_hex, sha1 *out) {
    sane(out && $ok(parent_hex));
    if ($len(parent_hex) != 40) fail(SNIFFFAIL);

    a_raw(bin, *out);
    HEXu8sDrainSome(bin, parent_hex);
    //  Verify the commit actually lives in keeper (sanity check).
    Bu8 tmp = {};
    call(u8bAllocate, tmp, 1UL << 20);
    u8 ctype = 0;
    ok64 go = KEEPGetExact(k, out, tmp, &ctype);
    u8bFree(tmp);
    if (go != OK || ctype != DOG_OBJ_COMMIT) fail(SNIFFFAIL);
    done;
}

// --- Baseline branch query (from ULOG) ---

#define POST_MAX_PARENTS 16

//  Read the latest baseline (`get`/`post`/`patch`) row.  Fills `out`
//  with the first REF spec from its query (the branch name), and
//  `parents[]` with every 40-hex SHA spec — these are the parents the
//  next commit must inherit.  `*nparents_out` is set to the count.
//  `*had_baseline_out` is YES iff a baseline row exists (so callers
//  can distinguish a fresh repo from a corrupt one).
//
//  Returns OK in both the "baseline exists" and "no baseline" cases —
//  callers branch on `*had_baseline_out`.  Non-OK reflects ULOG /
//  parse failures upstream.
static ok64 post_collect_parents(u8bp out, sha1 *parents, u32 *nparents_out,
                                 u32 cap, b8 *had_baseline_out) {
    sane(out && parents && nparents_out && had_baseline_out);
    u8bReset(out);
    *nparents_out = 0;
    *had_baseline_out = NO;
    ron60 ts = 0, verb = 0;
    uri u = {};
    ok64 r = SNIFFAtBaseline(&ts, &verb, &u);
    if (r == ULOGNONE) done;        // fresh repo — root commit allowed
    if (r != OK) return r;
    *had_baseline_out = YES;

    a_dup(u8c, q, u.query);
    while (!$empty(q)) {
        qref spec = {};
        if (QURYu8sDrain(q, &spec) != OK) break;
        if (spec.type == QURY_NONE) {
            //  Empty between separators (leading `&` or trailing).
            //  Don't break — keep draining; the SHAs may be later.
            if ($empty(q)) break;
            continue;
        }
        if (spec.type == QURY_REF && u8bDataLen(out) == 0) {
            u8bFeed(out, spec.body);
        } else if (spec.type == QURY_SHA && $len(spec.body) == 40 &&
                   *nparents_out < cap) {
            sha1 ph = {};
            u8s bin = {ph.data, ph.data + 20};
            a_dup(u8c, hx, spec.body);
            if (HEXu8sDrainSome(bin, hx) == OK && bin[0] == ph.data + 20)
                parents[(*nparents_out)++] = ph;
        }
    }
    done;
}

//  Legacy single-parent variant, kept for callers that only need the
//  branch + first parent hex.
static ok64 post_baseline_branch(u8bp out, u8bp hex_out) {
    sane(out && hex_out);
    u8bReset(out);
    u8bReset(hex_out);
    ron60 ts = 0, verb = 0;
    uri u = {};
    ok64 r = SNIFFAtBaseline(&ts, &verb, &u);
    if (r != OK) done;

    //  Split the baseline query into its first REF spec (branch name,
    //  no leading `?`) and its first 40-hex SHA spec (parent commit).
    //  Additional specs — extra SHAs from an in-progress merge — are
    //  not relevant to the branch/parent lookup here.
    a_dup(u8c, q, u.query);
    while (!$empty(q)) {
        qref spec = {};
        if (QURYu8sDrain(q, &spec) != OK) break;
        if (spec.type == QURY_NONE) break;
        if (spec.type == QURY_REF && u8bDataLen(out) == 0) {
            u8bFeed(out, spec.body);
        } else if (spec.type == QURY_SHA && $len(spec.body) == 40 &&
                   u8bDataLen(hex_out) == 0) {
            u8bFeed(hex_out, spec.body);
        }
    }
    done;
}

// --- Public API ---

ok64 POSTCommit(u8cs reporoot, u8cs message, u8cs author, sha1 *sha_out) {
    sane($ok(message) && $ok(author) && sha_out);
    keeper *k = &KEEP;

    //  1. Resolve baseline (branch, parents).  Three cases:
    //       * no baseline row at all  → root commit (0 parents, OK).
    //       * baseline + 1+ parents   → normal / merge commit.
    //       * baseline + 0 parents    → corrupt at-log; refuse.
    //     The third case used to silently produce a parentless commit,
    //     which on push replaces the peer's history with a single
    //     dangling root.  See AT.md §"Baseline rule" + the patch row's
    //     extending `<prior>&<theirs>` query for why a baseline row
    //     should always carry at least one 40-hex SHA spec.
    a_pad(u8, brbuf, 256);
    sha1  parents[POST_MAX_PARENTS] = {};
    u32   nparents = 0;
    b8    had_baseline = NO;
    ok64  br = post_collect_parents(brbuf, parents, &nparents,
                                    POST_MAX_PARENTS, &had_baseline);
    if (br != OK) return br;
    if (had_baseline && nparents == 0) {
        fprintf(stderr,
                "sniff: post: baseline at-log row has no parent SHA — "
                "refusing parentless commit (would orphan peer history "
                "on push)\n");
        return SNIFFFAIL;
    }
    if (u8bDataLen(brbuf) == 0) {
        a_cstr(def, "heads/master");
        u8bFeed(brbuf, def);
    }

    //  Per-path arrays sized to current registry + head-room for
    //  paths interned during wt scan.  The registry is shared with
    //  keeper, so indices are globally stable once assigned.
    u32 npath0 = SNIFFCount();
    u32 cap = npath0 + (1u << 20);

    post_rec *rec = NULL;
    Bu8 rec_buf = {};
    call(u8bAllocate, rec_buf, (u64)cap * sizeof(post_rec));
    memset(u8bDataHead(rec_buf), 0, (u64)cap * sizeof(post_rec));
    rec = (post_rec *)u8bDataHead(rec_buf);

    Bu8 flag_buf = {};
    call(u8bAllocate, flag_buf, cap);
    memset(u8bDataHead(flag_buf), 0, cap);

    post_ctx ctx = {
        .k = k, .rec = rec,
        .flag = u8bDataHead(flag_buf), .cap = cap,
        .last_post_ts = SNIFFAtLastPostTs(),
    };
    ctx.reporoot[0] = reporoot[0];
    ctx.reporoot[1] = reporoot[1];

    //  2. Baseline walk.
    sha1 base_tree_sha = {};
    b8 have_base = NO;
    ok64 lo = post_load_baseline(&ctx, &base_tree_sha, &have_base);
    if (lo != OK) { u8bFree(rec_buf); u8bFree(flag_buf); return lo; }

    //  3. Put/delete scan since last post.
    call(SNIFFAtScanPutDelete, ctx.last_post_ts, post_pd_cb, &ctx);

    //  4. Worktree scan.
    call(post_scan_wt, &ctx);

    //  4b. Expand dir-level put/delete rows into file-level flags.
    //      The wt-root .gitignore is already loaded into SNIFF.ignores
    //      at SNIFFOpen time, so every op consults the same set.
    {
        ok64 xr = post_expand_dir_rows(&ctx, &SNIFF.ignores);
        if (xr != OK) {
            u8bFree(rec_buf); u8bFree(flag_buf);
            return xr;
        }
    }

    //  5. Decide fate per path.
    {
        u32 n_now = SNIFFCount();
        for (u32 i = 0; i < n_now && i < cap; i++) {
            ok64 dr = post_decide(&ctx, i);
            if (dr != OK) { u8bFree(rec_buf); u8bFree(flag_buf); return dr; }
        }
    }

    //  5b. For every file explicitly deleted since last post, unlink
    //      it from disk — otherwise `be delete foo && be post` leaves
    //      foo on disk, and subsequent auto-stage passes would
    //      re-add it.  This is the mtime-attribution fix for
    //      BEhistory / the "deleted-file re-added" regression.
    {
        u32 n_now = SNIFFCount();
        for (u32 i = 0; i < n_now && i < cap; i++) {
            if (!(ctx.flag[i] & POST_EXPL_DEL)) continue;
            if (!(ctx.flag[i] & POST_ON_DISK)) continue;
            u8cs rel = {};
            if (SNIFFPath(rel, i) != OK) continue;
            a_path(fp);
            if (SNIFFFullpath(fp, reporoot, rel) != OK) continue;
            (void)FILEUnLink($path(fp));
            ctx.flag[i] &= ~POST_ON_DISK;
        }
    }

    //  6. Hash blobs for rewrite entries.
    {
        u32 n_now = SNIFFCount();
        for (u32 i = 0; i < n_now && i < cap; i++) {
            ok64 hr = post_hash_one(&ctx, i);
            if (hr != OK) { u8bFree(rec_buf); u8bFree(flag_buf); return hr; }
        }
    }

    //  7. Sort paths, then build trees bottom-up.
    call(SNIFFSort);

    sha1 root_tree = {};
    b8 have_root = NO;
    Bu8 tree_bodies = {};
    call(u8bAllocate, tree_bodies, 1UL << 20);
    u32 tree_count = 0;

    {
        u32 n_sorted = u32bDataLen(SNIFF.sorted);
        u8cs no_prefix = {};
        ok64 bo = post_build_tree(&ctx, 0, n_sorted, no_prefix,
                                  &root_tree, tree_bodies, &tree_count);
        if (bo != OK) {
            u8bFree(tree_bodies);
            u8bFree(rec_buf); u8bFree(flag_buf);
            return bo;
        }
        have_root = !sha1empty(&root_tree);
    }

    //  8. If the result has no files, fall back to the empty-tree sha.
    keep_pack p = {};
    call(KEEPPackOpen, k, &p);
    p.strict_order = NO;

    if (!have_root) {
        call(post_feed_empty_tree, k, &p, &root_tree);
    }

    //  9. Verify each parent commit exists locally; refuse otherwise.
    //     `parents[]` already holds the decoded sha1 bytes from the
    //     baseline row's QURY scan; `post_parent_sha` re-runs the
    //     keeper lookup as a sanity check.
    for (u32 i = 0; i < nparents; i++) {
        a_pad(u8, hx_buf, 40);
        a_rawc(psha_in, parents[i]);
        HEXu8sFeedSome(hx_buf_idle, psha_in);
        a_dup(u8c, ph, u8bDataC(hx_buf));
        sha1 ps = {};
        if (post_parent_sha(k, ph, &ps) != OK) {
            fprintf(stderr,
                    "sniff: post: parent commit %.*s not found in keeper — "
                    "refusing\n",
                    (int)u8csLen(ph), (char const *)ph[0]);
            return SNIFFFAIL;
        }
        parents[i] = ps;
    }

    //  10. Build commit body.
    Bu8 com = {};
    call(u8bAllocate, com, 4096);
    a_cstr(tree_label, "tree ");
    u8bFeed(com, tree_label);
    a_pad(u8, thex, 40);
    a_rawc(tsha, root_tree);
    HEXu8sFeedSome(thex_idle, tsha);
    u8bFeed(com, u8bDataC(thex));
    u8bFeed1(com, '\n');

    for (u32 i = 0; i < nparents; i++) {
        a_cstr(par_label, "parent ");
        u8bFeed(com, par_label);
        a_pad(u8, par_hex, 40);
        a_rawc(psha, parents[i]);
        HEXu8sFeedSome(par_hex_idle, psha);
        u8bFeed(com, u8bDataC(par_hex));
        u8bFeed1(com, '\n');
    }

    time_t now = time(NULL);
    char tsb[64];
    int tslen = snprintf(tsb, sizeof(tsb), " %lld +0000\n", (long long)now);
    u8cs ts_s = {(u8cp)tsb, (u8cp)tsb + tslen};

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

    //  11. Feed pack: commit first.
    u8csc nopath = {NULL, NULL};
    a_dup(u8c, com_data, u8bData(com));
    ok64 fo = KEEPPackFeed(k, &p, DOG_OBJ_COMMIT, com_data, nopath, 0, sha_out);
    u8bFree(com);
    if (fo != OK) {
        KEEPPackClose(k, &p);
        u8bFree(tree_bodies);
        u8bFree(rec_buf); u8bFree(flag_buf);
        return fo;
    }

    //  12. Feed all rebuilt trees.
    if (have_root) {
        u8c *walk = u8bDataHead(tree_bodies);
        u8c *end_walk = u8bIdleHead(tree_bodies);
        while (walk < end_walk) {
            u32 tlen = 0;
            memcpy(&tlen, walk, sizeof(u32));
            walk += sizeof(u32);
            u8cs tbody = {walk, walk + tlen};
            sha1 tsha_dummy = {};
            ok64 to = KEEPPackFeed(k, &p, DOG_OBJ_TREE, tbody,
                                   nopath, 0, &tsha_dummy);
            walk += tlen;
            if (to != OK) {
                KEEPPackClose(k, &p);
                u8bFree(tree_bodies);
                u8bFree(rec_buf); u8bFree(flag_buf);
                return to;
            }
        }
    }

    //  13. Feed all new blobs.
    {
        u32 n_now = SNIFFCount();
        for (u32 i = 0; i < n_now && i < cap; i++) {
            if (!(ctx.flag[i] & POST_REWRITE)) continue;
            post_rec *r = &ctx.rec[i];
            if (!u8bOK(r->content)) continue;
            u8cs rel = {};
            if (SNIFFPath(rel, i) != OK) continue;
            u8csc bpath = {rel[0], rel[1]};
            a_dup(u8c, body, u8bData(r->content));
            sha1 bsha = {};
            ok64 bo = KEEPPackFeed(k, &p, DOG_OBJ_BLOB, body,
                                   bpath, 0, &bsha);
            if (bo != OK) {
                KEEPPackClose(k, &p);
                u8bFree(tree_bodies);
                u8bFree(rec_buf); u8bFree(flag_buf);
                return bo;
            }
        }
    }

    call(KEEPPackClose, k, &p);

    //  14. Advance keeper REFS for the branch (if we have one).
    //      `brbuf` is the bare ref (e.g. `heads/main`).  Build a
    //      canonical key via DOGCanonURIFeed — collapses the trunk
    //      aliases {master, main, trunk} (with or without `heads/`)
    //      to bare `?`.  Value is bare 40-hex.
    a_pad(u8, out_hex, 40);
    {
        a_rawc(osha, *sha_out);
        HEXu8sFeedSome(out_hex_idle, osha);
    }
    {
        a_dup(u8c, branch, u8bData(brbuf));
        if (!u8csEmpty(branch)) {
            a_path(keepdir, u8bDataC(k->h->root), KEEP_DIR_S);

            uri uk = {};
            uk.query[0] = branch[0];
            uk.query[1] = branch[1];
            a_pad(u8, keybuf, 128);
            call(DOGCanonURIFeed, keybuf, &uk);
            a_dup(u8c, refkey, u8bData(keybuf));

            a_dup(u8c, val, u8bDataC(out_hex));

            (void)REFSAppendVerb($path(keepdir), REFSVerbPost(), refkey, val);
        }
    }

    //  15. Append `post` ULOG row with stamp ts; futimens written
    //      files so they become clean under the new stamp.  Row URI
    //      puts the ref and the new tip sha into the query, chained
    //      as `<branch>&<40hex>` per dog/QURY; fragment stays empty
    //      (reserved for dog/FRAG content-locator syntax).
    a_pad(u8, qbuf, 256);
    {
        a_dup(u8c, branch, u8bData(brbuf));
        if (!u8csEmpty(branch)) {
            u8bFeed(qbuf, branch);
            u8bFeed1(qbuf, '&');
        }
        u8bFeed(qbuf, u8bDataC(out_hex));
    }
    uri urow = {};
    {
        a_dup(u8c, q, u8bData(qbuf));
        urow.query[0] = q[0];
        urow.query[1] = q[1];
    }

    ron60 ts = 0;
    struct timespec tv = {};
    SNIFFAtNow(&ts, &tv);
    ron60 verb = SNIFFAtVerbPost();
    ok64 ar = SNIFFAtAppendAt(ts, verb, &urow);
    (void)ar;

    //  Stamp every file that survived into the new commit (rewrites +
    //  keeps on disk) with the post row's timestamp.
    {
        u32 n_now = SNIFFCount();
        for (u32 i = 0; i < n_now && i < cap; i++) {
            u8 f = ctx.flag[i];
            if (f & POST_DROP) continue;
            if (!(f & POST_ON_DISK)) continue;
            if (SNIFFIsDir(i)) continue;
            u8cs rel = {};
            if (SNIFFPath(rel, i) != OK) continue;
            a_path(fp);
            if (SNIFFFullpath(fp, reporoot, rel) != OK) continue;
            (void)SNIFFAtStampPath(fp, ts);
        }
    }

    //  16. Clean up.
    {
        u32 n_now = SNIFFCount();
        for (u32 i = 0; i < n_now && i < cap; i++) {
            if (u8bOK(ctx.rec[i].content)) u8bFree(ctx.rec[i].content);
        }
    }
    u8bFree(tree_bodies);
    u8bFree(rec_buf); u8bFree(flag_buf);

    fprintf(stderr, "sniff: commit %.*s\n",
            (int)u8bDataLen(out_hex), (char *)u8bDataHead(out_hex));
    done;
}

ok64 POSTSetLabel(u8cs ref_uri, u8cs sha_hex) {
    sane($ok(ref_uri) && !u8csEmpty(ref_uri) && $ok(sha_hex));
    if (u8csLen(sha_hex) != 40) fail(SNIFFFAIL);

    a_path(keepdir, u8bDataC(KEEP.h->root), KEEP_DIR_S);

    //  Canonicalise the caller-supplied ref URI (user input path:
    //  command line `be post ?<label>`).  Lex → canonicalise → feed.
    uri u = {};
    u.data[0] = ref_uri[0];
    u.data[1] = ref_uri[1];
    call(URILexer, &u);
    u.data[0] = ref_uri[0];
    u.data[1] = ref_uri[1];
    a_pad(u8, keybuf, 256);
    call(DOGCanonURIFeed, keybuf, &u);
    a_dup(u8c, key, u8bData(keybuf));

    //  Val is bare 40-hex (canonical).  `post` verb — local ref move.
    return REFSAppendVerb($path(keepdir), REFSVerbPost(), key, sha_hex);
}
