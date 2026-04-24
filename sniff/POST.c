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

typedef struct {
    keeper     *k;
    u8cs        reporoot;
    post_rec   *rec;
    u8         *flag;
    u32         cap;
    b8          any_pd;     // any put/delete rows since last post
    ron60       last_post_ts;
    ok64        error;
} post_ctx;

// --- git mode helpers ---

static u16 post_mode_of_kind(u8 kind) {
    switch (kind) {
        case WALK_KIND_REG: return 0100644;
        case WALK_KIND_EXE: return 0100755;
        case WALK_KIND_LNK: return 0120000;
        case WALK_KIND_DIR: return 040000;
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
    if (kind == WALK_KIND_SUB) return WALKSKIP;
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

    //  Baseline fragment is either `<sha>` (single-hash, from get/post)
    //  or `<ours>,<theirs>[,...]` (post-patch N-hash URI, see AT.h).
    //  For a squash-merge POST we only need the ours tree as the
    //  baseline: patched files are mtime-dirty (PATCH does not stamp),
    //  added files aren't in ours and fall into POST_REWRITE via the
    //  implicit-dirty rule, and deleted files were unlinked by PATCH so
    //  they vanish via the implicit-drop rule.
    a_dup(u8c, frag, base_u.fragment);
    if ($len(frag) < 40) done;
    u8cs h40 = {frag[0], frag[0] + 40};

    sha1 commit_sha = {};
    a_raw(csha_bin, commit_sha);
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

// --- Change-set resolution ---

static ok64 post_decide(post_ctx *c, u32 idx) {
    sane(c);
    u8 f = c->flag[idx];

    //  Skip directory entries in the registry — they are reconstructed
    //  during tree-build rather than selected here.
    if (SNIFFIsDir(idx)) return OK;
    if (!(f & (POST_IN_BASE | POST_ON_DISK))) return OK;

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

    //  Missing from wt — only drop when implicit mode or baseline had it.
    if (!(f & POST_ON_DISK)) {
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

    //  On disk, no explicit rule.
    if (c->any_pd) {
        //  Selective mode: carry over baseline entry; a new on-disk
        //  file that isn't mentioned goes unstaged (ignore).
        if (f & POST_IN_BASE) c->flag[idx] |= POST_KEEP;
        return OK;
    }

    //  Implicit mode: include dirty files (mtime ∉ stamp-set).
    a_path(fp);
    u8cs rel = {};
    call(SNIFFPath, rel, idx);
    call(SNIFFFullpath, fp, c->reporoot, rel);
    struct stat sb = {};
    if (lstat((char const *)u8bDataHead(fp), &sb) != 0) {
        c->flag[idx] |= POST_DROP;
        return OK;
    }
    struct timespec ts = {.tv_sec = sb.st_mtim.tv_sec,
                          .tv_nsec = sb.st_mtim.tv_nsec};
    ron60 mtime_r = SNIFFAtOfTimespec(ts);
    if (SNIFFAtKnown(mtime_r)) {
        //  Clean: keep baseline sha if present.
        if (f & POST_IN_BASE) c->flag[idx] |= POST_KEEP;
        //  Otherwise: untracked clean (shouldn't happen but ignore).
    } else {
        c->flag[idx] |= POST_REWRITE;
    }
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

static ok64 post_baseline_branch(u8bp out, u8bp hex_out) {
    sane(out && hex_out);
    u8bReset(out);
    u8bReset(hex_out);
    ron60 ts = 0, verb = 0;
    uri u = {};
    ok64 r = SNIFFAtBaseline(&ts, &verb, &u);
    if (r != OK) done;
    //  Branches are written to REFS as `?heads/X`; the URI query slice
    //  stores the bare `heads/X` (RFC 3986 — the `?` is the separator).
    //  Prefix it here so downstream callers can treat `out` as a full
    //  ref key ready to hand to REFSAppend.
    if (!u8csEmpty(u.query)) {
        u8bFeed1(out, '?');
        u8bFeed(out, u.query);
    }
    //  Fragment is either `<sha>` or `<ours>,<theirs>[,...]`.  Take the
    //  first 40-hex chunk as the parent commit — see post_load_baseline.
    a_dup(u8c, frag, u.fragment);
    if ($len(frag) >= 40) {
        u8cs h40 = {frag[0], frag[0] + 40};
        u8bFeed(hex_out, h40);
    }
    done;
}

// --- Public API ---

ok64 POSTCommit(u8cs reporoot, u8cs message, u8cs author, sha1 *sha_out) {
    sane($ok(message) && $ok(author) && sha_out);
    keeper *k = &KEEP;

    //  1. Resolve baseline (branch, parent hex).  If no baseline, we're
    //     making a root commit.
    a_pad(u8, brbuf, 256);
    a_pad(u8, phex, 64);
    call(post_baseline_branch, brbuf, phex);
    if (u8bDataLen(brbuf) == 0) {
        a_cstr(def, "?heads/master");
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

    //  9. Resolve parent commit sha (from baseline, if any).
    sha1 parent_sha = {};
    b8 has_parent = NO;
    if (u8bDataLen(phex) == 40) {
        a_dup(u8c, ph, u8bData(phex));
        if (post_parent_sha(k, ph, &parent_sha) == OK) has_parent = YES;
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
    {
        a_dup(u8c, branch, u8bData(brbuf));
        if (!u8csEmpty(branch) && branch[0][0] == '?') {
            a_path(keepdir, u8bDataC(k->h->root), KEEP_DIR_S);
            a_pad(u8, valbuf, 64);
            u8bFeed1(valbuf, '?');
            a_pad(u8, out_hex, 40);
            a_rawc(osha, *sha_out);
            HEXu8sFeedSome(out_hex_idle, osha);
            u8bFeed(valbuf, u8bDataC(out_hex));
            a_dup(u8c, val, u8bData(valbuf));
            (void)REFSAppend($path(keepdir), branch, val);
        }
    }

    //  15. Append `post` ULOG row with stamp ts; futimens written
    //      files so they become clean under the new stamp.  Row URI
    //      is composed via abc/URI: query = branch ref (minus the
    //      leading `?`), fragment = 40-hex of the new commit.
    a_pad(u8, out_hex, 40);
    {
        a_rawc(osha, *sha_out);
        HEXu8sFeedSome(out_hex_idle, osha);
    }
    uri urow = {};
    {
        a_dup(u8c, branch, u8bData(brbuf));
        //  Strip a leading `?` sentinel — URI query slices store the
        //  raw query without it.
        if (!u8csEmpty(branch) && *branch[0] == '?') u8csUsed1(branch);
        if (!u8csEmpty(branch)) {
            urow.query[0] = branch[0];
            urow.query[1] = branch[1];
        }
    }
    {
        a_dup(u8c, oh, u8bData(out_hex));
        urow.fragment[0] = oh[0];
        urow.fragment[1] = oh[1];
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

    a_pad(u8, tbuf, 64);
    u8bFeed1(tbuf, '?');
    u8bFeed(tbuf, sha_hex);
    a_dup(u8c, to, u8bData(tbuf));

    return REFSAppend($path(keepdir), ref_uri, to);
}
