//  GET: checkout a commit tree from keeper.
//
#include "GET.h"

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/DPATH.h"
#include "keeper/GIT.h"
#include "keeper/REFS.h"
#include "keeper/WALK.h"

// Per-entry visitor context for WALKTreeLazy.
typedef struct {
    sniff  *s;
    keeper *k;
    u8cs    reporoot;
    u8p     seen;
    ok64    error;       // first fatal error encountered, if any
} get_ctx;

static ok64 get_visit(u8cs path, u8 kind, u8cp esha, u8cs blob,
                       void0p vctx) {
    (void)blob;  // lazy mode: blob is always empty, we pull if needed
    get_ctx *g = (get_ctx *)vctx;
    sniff  *s = g->s;
    keeper *k = g->k;

    if (kind == WALK_KIND_SUB) return WALKSKIP;

    u64 entry_hashlet = WHIFFHashlet40((sha1cp)esha);

    if (kind == WALK_KIND_DIR) {
        if ($empty(path)) {
            // Root tree: record base hashlet at the reserved root idx.
            u32 ridx = SNIFFRootIdx(s);
            g->seen[ridx] = 1;
            SNIFFRecord(s, SNIFF_TREE, ridx, entry_hashlet);
            return OK;  // walker recurses into children
        }

        a_path(dp);
        SNIFFFullpath(dp, g->reporoot, path);
        FILEMakeDirP($path(dp));

        u32 idx = SNIFFInternDir(s, path);
        g->seen[idx] = 1;
        SNIFFRecord(s, SNIFF_TREE, idx, entry_hashlet);
        return OK;  // walker recurses
    }

    // File entry (REG/EXE/LNK).
    u32 idx = SNIFFIntern(s, path);
    g->seen[idx] = 1;

    // Fast path: if the hashlet already matches AND the file still
    // exists on disk, skip the rewrite.  Missing-on-disk falls through
    // so checkout can re-materialise a rm'd file.
    u64 old_hashlet = SNIFFGet(s, SNIFF_BLOB, idx);
    if (old_hashlet == entry_hashlet && old_hashlet != 0) {
        a_path(existing);
        if (SNIFFFullpath(existing, g->reporoot, path) == OK) {
            struct stat xb = {};
            if (lstat((char *)u8bDataHead(existing), &xb) == 0)
                return WALKSKIP;
        }
    }

    u64 co = SNIFFGet(s, SNIFF_CHECKOUT, idx);
    u64 ch = SNIFFGet(s, SNIFF_CHANGED, idx);
    if (ch != 0 && ch != co) {
        fprintf(stderr, "sniff: skip dirty %.*s\n",
                (int)$len(path), (char *)path[0]);
        return WALKSKIP;
    }

    // Pull blob content now (lazy mode).
    Bu8 bbuf = {};
    ok64 o = u8bAllocate(bbuf, 1UL << 24);
    if (o != OK) { g->error = o; return o; }
    u8 bt = 0;
    sha1 entry_sha = {};
    memcpy(entry_sha.data, esha, 20);
    o = KEEPGetExact(k, &entry_sha, bbuf, &bt);
    if (o != OK) { u8bFree(bbuf); g->error = o; return o; }

    a_path(fp);
    o = SNIFFFullpath(fp, g->reporoot, path);
    if (o != OK) { u8bFree(bbuf); g->error = o; return o; }

    if (kind == WALK_KIND_LNK) {
        unlink((char *)u8bDataHead(fp));
        u8bFeed1(bbuf, 0);
        symlink((char *)u8bDataHead(bbuf), (char *)u8bDataHead(fp));
    } else {
        int fd = -1;
        o = FILECreate(&fd, $path(fp));
        if (o != OK) { u8bFree(bbuf); g->error = o; return o; }
        u8cs data = {u8bDataHead(bbuf), u8bIdleHead(bbuf)};
        o = FILEFeedAll(fd, data);
        FILEClose(&fd);
        if (o != OK) { u8bFree(bbuf); g->error = o; return o; }
        if (kind == WALK_KIND_EXE)
            chmod((char *)u8bDataHead(fp), 0755);
    }
    u8bFree(bbuf);

    SNIFFRecord(s, SNIFF_BLOB, idx, entry_hashlet);

    struct stat sb = {};
    if (FILEStat(&sb, $path(fp)) == OK)
        SNIFFRecord(s, SNIFF_CHECKOUT, idx, (u64)sb.st_mtim.tv_sec);

    return OK;
}

// Remove tracked files not in the new tree.
// Files first, then dirs (FILERmDir only works on empty dirs).
static ok64 GETPrune(sniff *s, u8cs reporoot, u8cp seen) {
    sane(s);
    u32 n = SNIFFCount(s);
    u32 removed = 0, errors = 0;

    // Pass 1: unlink files
    for (u32 i = 0; i < n; i++) {
        if (seen[i]) continue;
        u64 h = SNIFFGet(s, SNIFF_BLOB, i);
        if (h == 0) continue;
        if (SNIFFIsDir(s, i)) continue;

        u8cs rel = {};
        call(SNIFFPath, rel, s, i);
        if ($empty(rel)) continue;

        a_path(fp);
        call(SNIFFFullpath, fp, reporoot, rel);

        ok64 o = FILEUnLink($path(fp));
        if (o == OK || o == FILENOENT) {
            SNIFFRecord(s, SNIFF_BLOB, i, 0);
            SNIFFRecord(s, SNIFF_CHECKOUT, i, 0);
            removed++;
        } else {
            fprintf(stderr, "sniff: unlink fail %.*s: %s\n",
                    (int)u8csLen(rel), (char *)rel[0], ok64str(o));
            errors++;
        }
    }

    // Pass 2: rmdir stale dirs (reverse order for bottom-up)
    for (u32 i = n; i > 0; ) {
        i--;
        if (seen[i]) continue;
        u64 h = SNIFFGet(s, SNIFF_TREE, i);
        if (h == 0) continue;
        if (!SNIFFIsDir(s, i)) continue;

        u8cs rel = {};
        call(SNIFFPath, rel, s, i);

        a_path(fp);
        call(SNIFFFullpath, fp, reporoot, rel);

        ok64 o = FILERmDir($path(fp), NO);
        if (o == OK || o == FILENOENT || o == FILENOTEMP) {
            SNIFFRecord(s, SNIFF_TREE, i, 0);
            SNIFFRecord(s, SNIFF_CHECKOUT, i, 0);
        }
    }

    if (removed > 0 || errors > 0)
        fprintf(stderr, "sniff: removed %u file(s), %u error(s)\n",
                removed, errors);
    done;
}

// --- Public API ---

ok64 GETCheckout(sniff *s, keeper *k, u8cs reporoot, u8cs hex,
                 u8cs source) {
    sane(s && k && $ok(hex));

    size_t hexlen = $len(hex);
    if (hexlen > 15) hexlen = 15;
    u64 hashlet = WHIFFHexHashlet60(hex);

    Bu8 buf = {};
    call(u8bAllocate, buf, 1UL << 24);
    u8 otype = 0;
    ok64 o = KEEPGet(k, hashlet, hexlen, buf, &otype);
    if (o != OK) {
        u8bFree(buf);
        fprintf(stderr, "sniff: object not found\n");
        fail(SNIFFFAIL);
    }

    // Dereference annotated tag
    if (otype == DOG_OBJ_TAG) {
        u8cs body = {u8bDataHead(buf), u8bIdleHead(buf)};
        u8cs field = {}, value = {};
        sha1 tag_sha = {};
        a_raw(tag_bin, tag_sha);
        b8 found = NO;
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if ($empty(field)) break;
            if ($len(field) == 6 && memcmp(field[0], "object", 6) == 0 &&
                $len(value) >= 40) {
                u8cs hex40 = {value[0], $atp(value, 40)};
                HEXu8sDrainSome(tag_bin, hex40);
                found = YES;
                break;
            }
        }
        if (!found) {
            u8bFree(buf);
            fprintf(stderr, "sniff: bad tag (no object)\n");
            fail(SNIFFFAIL);
        }
        u8bReset(buf);
        o = KEEPGetExact(k, &tag_sha, buf, &otype);
        if (o != OK || otype != DOG_OBJ_COMMIT) {
            u8bFree(buf);
            fprintf(stderr, "sniff: tag target not a commit\n");
            fail(SNIFFFAIL);
        }
    }

    if (otype != DOG_OBJ_COMMIT) {
        u8bFree(buf);
        fprintf(stderr, "sniff: not a commit\n");
        fail(SNIFFFAIL);
    }

    // Extract tree SHA
    sha1 tree_sha = {};
    u8cs commit = {u8bDataHead(buf), u8bIdleHead(buf)};
    o = GITu8sCommitTree(commit, tree_sha.data);
    u8bFree(buf);
    if (o != OK) {
        fprintf(stderr, "sniff: bad commit (no tree)\n");
        fail(SNIFFFAIL);
    }

    // Allocate seen bitmap
    u32 npath = SNIFFCount(s);
    u32 seen_size = npath + 65536;  // padding for new paths during walk
    Bu8 seen_buf = {};
    call(u8bAllocate, seen_buf, seen_size);
    memset(u8bDataHead(seen_buf), 0, seen_size);

    get_ctx ctx = {
        .s = s, .k = k,
        .seen = u8bDataHead(seen_buf), .error = OK,
    };
    ctx.reporoot[0] = reporoot[0];
    ctx.reporoot[1] = reporoot[1];
    o = WALKTreeLazy(k, tree_sha.data, get_visit, &ctx);
    if (o == OK && ctx.error != OK) o = ctx.error;

    if (o == OK)
        o = GETPrune(s, reporoot, u8bDataHead(seen_buf));

    u8bFree(seen_buf);
    if (o == OK) {
        o = SNIFFCompact(s);
    }
    if (o == OK) {
        SNIFFSetHead(s, hex);
        if (!$empty(source)) {
            a_cstr(keepdir, k->dir);
            a_pad(u8, file_uri, 1280);
            a_cstr(scheme, "file://");
            u8bFeed(file_uri, scheme);
            u8bFeed(file_uri, reporoot);
            a_dup(u8c, from, u8bData(file_uri));
            REFSAppend(keepdir, from, source);
        }
        fprintf(stderr, "sniff: checkout done\n");
    }
    return o;
}
