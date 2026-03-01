#include "BE.h"

#include <stdlib.h>
#include <string.h>

#include "abc/POL.h"
#include "abc/PRO.h"

// ---- Key builders ----

ok64 BEKeyHead(u8s into, u8cs project, u8cs path) {
    sane($ok(into) && $ok(project));
    call(u8sFeed, into, project);
    u8sFeed1(into, '/');
    if ($ok(path) && !$empty(path)) {
        call(u8sFeed, into, path);
    }
    done;
}

ok64 BEKeyVer(u8s into, u8cs project, u8cs path, ron60 stamp) {
    sane($ok(into) && $ok(project));
    call(u8sFeed, into, project);
    u8sFeed1(into, '/');
    if ($ok(path) && !$empty(path)) {
        call(u8sFeed, into, path);
    }
    a_cstr(qv, "?v=");
    call(u8sFeed, into, qv);
    call(RONu8sFeedPad, into, stamp, 10);
    into[0] += 10;
    done;
}

ok64 BEKeyBranch(u8s into, u8cs project, u8cs path, u8cs branch) {
    sane($ok(into) && $ok(project) && $ok(branch));
    call(u8sFeed, into, project);
    u8sFeed1(into, '/');
    if ($ok(path) && !$empty(path)) {
        call(u8sFeed, into, path);
    }
    a_cstr(qy, "?y=");
    call(u8sFeed, into, qy);
    call(u8sFeed, into, branch);
    done;
}

ok64 BEKeyCommit(u8s into, u8cs project, ron60 stamp) {
    sane($ok(into) && $ok(project));
    call(u8sFeed, into, project);
    a_cstr(sep, "/?v=");
    call(u8sFeed, into, sep);
    call(RONu8sFeedPad, into, stamp, 10);
    into[0] += 10;
    done;
}

ok64 BEKeyBranchPtr(u8s into, u8cs project, u8cs branch) {
    sane($ok(into) && $ok(project) && $ok(branch));
    call(u8sFeed, into, project);
    a_cstr(sep, "/?y=");
    call(u8sFeed, into, sep);
    call(u8sFeed, into, branch);
    done;
}

ok64 BEKeyConf(u8s into, u8cs project, u8cs confkey) {
    sane($ok(into) && $ok(confkey));
    if ($ok(project) && !$empty(project)) {
        call(u8sFeed, into, project);
    }
    a_cstr(sep, "?conf.");
    call(u8sFeed, into, sep);
    call(u8sFeed, into, confkey);
    done;
}

// Check if BE is on a branch, fill branch name if so
static b8 BEOnBranch(BEp be, u8csp branch) {
    if (!$ok(be->loc.query) || $empty(be->loc.query)) return NO;
    u8cs q = {be->loc.query[0], be->loc.query[1]};
    if ($len(q) <= 2 || q[0][0] != 'y' || q[0][1] != '=') return NO;
    if (branch != NULL) {
        branch[0] = q[0] + 2;
        branch[1] = q[1];
    }
    return YES;
}

ok64 BEKeyCur(u8s into, BEp be, u8cs path) {
    sane(be != NULL && $ok(into));
    u8cs branch = {};
    if (BEOnBranch(be, branch)) {
        return BEKeyBranch(into, be->loc.path, path, branch);
    }
    return BEKeyHead(into, be->loc.path, path);
}

// ---- BASTExport: flatten BASON tree to source text ----

static ok64 BASTExportRec(u8s out, u64bp stack, u8csc data) {
    sane(1);
    u8 type = 0;
    u8cs key = {};
    u8cs val = {};
    while (BASONDrain(stack, data, &type, key, val) == OK) {
        u8 raw = type & ~0x20;  // uppercase
        if (raw == 'S') {
            // string leaf: emit value verbatim
            call(u8sFeed, out, val);
        } else if (raw == 'A' || raw == 'O') {
            // container: recurse
            call(BASONInto, stack, data, val);
            call(BASTExportRec, out, stack, data);
            call(BASONOuto, stack);
        }
        // other types (int, etc.) - skip
    }
    done;
}

ok64 BASTExport(u8s out, u64bp stack, u8csc data) {
    sane($ok(out) && stack != NULL && $ok(data));
    if ($empty(data)) done;
    call(BASONOpen, stack, data);
    call(BASTExportRec, out, stack, data);
    done;
}

// ---- Internal helpers ----

// Write data to a file, creating it
static ok64 BEWriteFile(path8cg path, u8cs data) {
    sane(path != NULL);
    int fd = 0;
    call(FILECreate, &fd, path);
    if (!$empty(data)) {
        ok64 o = FILEFeedall(fd, data);
        FILEClose(&fd);
        return o;
    }
    call(FILEClose, &fd);
    done;
}

// Build $HOME/.be/<repo>/ path
static ok64 BERepoPath(path8g out, u8cs repo) {
    sane(out != NULL && $ok(repo) && !$empty(repo));
    const char *home = getenv("HOME");
    test(home != NULL, BEFAIL);
    a_cstr(homecs, home);
    call(u8sFeed, out + 1, homecs);
    call(path8gTerm, out);
    a_cstr(dotbe, ".be");
    call(path8gPush, out, dotbe);
    call(path8gPush, out, repo);
    done;
}

// Walk up from path looking for .be file, return dir containing it
static ok64 BEFindDotBe(path8g result, path8cg start) {
    sane(result != NULL && $ok(start));
    a_path(cur, "");
    call(path8gDup, path8gIn(cur), start);
    for (int depth = 0; depth < 64; depth++) {
        // Save current and try .be
        a_path(trial, "");
        call(path8gDup, path8gIn(trial), path8cgIn(cur));
        a_cstr(dotbe, ".be");
        call(path8gPush, path8gIn(trial), dotbe);
        struct stat st;
        ok64 o = FILEStat(&st, path8cgIn(trial));
        if (o == OK && S_ISREG(st.st_mode)) {
            call(path8gDup, result, path8cgIn(cur));
            done;
        }
        // Go up one level
        o = path8gPop(path8gIn(cur));
        if (o != OK) break;
        // Check if we reached root
        u8cs d = {cur[1], cur[2]};
        if ($len(d) <= 1) break;
    }
    fail(BEnone);
}

static ok64 BEScratchInit(BEp be) {
    sane(be != NULL);
    for (int i = 0; i < BE_SCRATCH; i++) {
        call(u8bMap, be->scratch[i], BE_SCRATCH_LEN);
    }
    done;
}

static ok64 BEScratchFree(BEp be) {
    sane(be != NULL);
    for (int i = 0; i < BE_SCRATCH; i++) {
        if (be->scratch[i][0] != NULL) {
            u8bUnMap(be->scratch[i]);
        }
    }
    done;
}

// ---- Lifecycle ----

ok64 BEInit(BEp be, u8cs be_uri, path8cg worktree) {
    sane(be != NULL && $ok(be_uri) && worktree != NULL);
    memset(be, 0, sizeof(BE));

    // Copy URI string into backing store
    size_t ulen = $len(be_uri);
    test(ulen < sizeof(be->loc_buf), BEBAD);
    memcpy(be->loc_buf, be_uri[0], ulen);
    be->loc.data[0] = be->loc_buf;
    be->loc.data[1] = be->loc_buf + ulen;
    call(URILexer, &be->loc);

    // Setup worktree path
    call(path8bAlloc, be->work_pp);
    call(path8gDup, path8gIn(be->work_pp), worktree);

    // Build repo path: $HOME/.be/<repo>/
    call(path8bAlloc, be->repo_pp);
    test($ok(be->loc.host) && !$empty(be->loc.host), BEBAD);
    call(BERepoPath, path8gIn(be->repo_pp), be->loc.host);
    call(FILEMakeDirP, path8cgIn(be->repo_pp));

    // Write .be file in worktree
    a_path(dotbe_path, "");
    call(path8gDup, path8gIn(dotbe_path), worktree);
    call(path8gPush, path8gIn(dotbe_path), dotbe);
    call(BEWriteFile, path8cgIn(dotbe_path), be_uri);

    // Open DB
    call(ROCKOpen, &be->db, path8cgIn(be->repo_pp));
    call(BEScratchInit, be);
    done;
}

ok64 BEOpen(BEp be, path8cg worktree) {
    sane(be != NULL && worktree != NULL);
    memset(be, 0, sizeof(BE));

    // Setup worktree path
    call(path8bAlloc, be->work_pp);

    // Find .be file
    call(BEFindDotBe, path8gIn(be->work_pp), worktree);

    // Read .be file
    a_path(dotbe_path, "");
    call(path8gDup, path8gIn(dotbe_path), path8cgIn(be->work_pp));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(dotbe_path), dotbe);

    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(dotbe_path));
    u8cp md0 = mapbuf[1], md1 = mapbuf[2];
    u8cs mapped = {md0, md1};
    size_t ulen = $len(mapped);
    test(ulen < sizeof(be->loc_buf), BEBAD);
    memcpy(be->loc_buf, mapped[0], ulen);
    call(FILEUnMap, mapbuf);

    be->loc.data[0] = be->loc_buf;
    be->loc.data[1] = be->loc_buf + ulen;
    call(URILexer, &be->loc);

    // Build repo path: $HOME/.be/<repo>/
    call(path8bAlloc, be->repo_pp);
    test($ok(be->loc.host) && !$empty(be->loc.host), BEBAD);
    call(BERepoPath, path8gIn(be->repo_pp), be->loc.host);

    // Open DB
    call(ROCKOpen, &be->db, path8cgIn(be->repo_pp));
    call(BEScratchInit, be);
    done;
}

ok64 BEClose(BEp be) {
    if (be == NULL) return OK;
    ROCKClose(&be->db);
    BEScratchFree(be);
    path8bFree(be->repo_pp);
    path8bFree(be->work_pp);
    memset(be, 0, sizeof(BE));
    return OK;
}

// ---- POST (worktree → repo) ----

// Callback context for scanning files
typedef struct {
    BEp be;
    ROCKbatchp wb;
    ron60 stamp;
} BEPostCtx;

// Get file extension as a const slice
static void BEExtOf(u8csp ext, u8cs filename) {
    ext[0] = NULL;
    ext[1] = NULL;
    u8cp p = filename[1];
    while (p > filename[0]) {
        --p;
        if (*p == '.') {
            ext[0] = p;
            ext[1] = filename[1];
            return;
        }
    }
}

static ok64 BEPostFile(BEp be, ROCKbatchp wb, ron60 stamp, path8cg filepath) {
    sane(be != NULL && wb != NULL);
    // Compute relative path from worktree root
    a_path(relpath, "");
    call(path8gRelative, path8gIn(relpath), path8cgIn(be->work_pp),
         filepath);
    u8cs rel = {relpath[1], relpath[2]};

    // Map source file
    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, filepath);
    u8cs source = {mapbuf[1], mapbuf[2]};

    // Get extension for parser
    u8cs basename = {};
    path8gBase(basename, filepath);
    u8cs ext = {};
    BEExtOf(ext, basename);

    // Parse to BASON (skip files with unknown extensions)
    u8bp nbuf = be->scratch[BE_PARSE];
    u8bReset(nbuf);
    aBpad(u64, nidx, 4096);
    ok64 o = BASTParse(nbuf, nidx, source, ext);
    if (o == BADARG) {
        FILEUnMap(mapbuf);
        done;
    }
    if (o != OK) {
        FILEUnMap(mapbuf);
        fail(o);
    }
    u8cs new_bason = {nbuf[1], nbuf[2]};

    // Build head key
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    call(BEKeyHead, key, be->loc.path, rel);
    u8cs head_key = {kbuf, key[0]};

    // Get old BASON from DB
    u8bp obuf = be->scratch[BE_READ];
    u8bReset(obuf);
    ok64 go = ROCKGet(&be->db, obuf, head_key);

    if (go == OK) {
        // Diff old vs new
        u8cs old_bason = {obuf[1], obuf[2]};
        u8bp dbuf = be->scratch[BE_PATCH];
        u8bReset(dbuf);
        aBpad(u64, didx, 4096);
        aBpad(u64, ostk, 256);
        aBpad(u64, nstk, 256);
        o = BASONDiff(dbuf, didx, ostk, old_bason, nstk, new_bason);
        if (o != OK) {
            FILEUnMap(mapbuf);
            fail(o);
        }
        u8cs delta = {dbuf[1], dbuf[2]};
        if ($empty(delta)) {
            // No change
            FILEUnMap(mapbuf);
            done;
        }
        // Store waypoint delta
        u8 vkbuf[512];
        u8s vkey = {vkbuf, vkbuf + sizeof(vkbuf)};
        call(BEKeyVer, vkey, be->loc.path, rel, stamp);
        u8cs wp_key = {vkbuf, vkey[0]};
        call(ROCKBatchPut, wb, wp_key, delta);
    }

    // Store new head
    call(ROCKBatchPut, wb, head_key, new_bason);

    // If branch mode, also store branch key
    u8cs branch = {};
    if (BEOnBranch(be, branch)) {
        u8 tkbuf[512];
        u8s tkey = {tkbuf, tkbuf + sizeof(tkbuf)};
        call(BEKeyBranch, tkey, be->loc.path, rel, branch);
        u8cs branch_key = {tkbuf, tkey[0]};
        call(ROCKBatchPut, wb, branch_key, new_bason);
    }

    FILEUnMap(mapbuf);
    done;
}

static ok64 BEPostScanCB(voidp arg, path8p path) {
    sane(arg != NULL);
    BEPostCtx *ctx = (BEPostCtx *)arg;
    return BEPostFile(ctx->be, ctx->wb, ctx->stamp, path8cgIn(path));
}

ok64 BEPost(BEp be, int pathc, u8cs *paths, u8cs message) {
    sane(be != NULL);
    ron60 stamp = RONNow();

    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    if (pathc > 0 && paths != NULL) {
        // POST specific files
        for (int i = 0; i < pathc; i++) {
            // Build absolute path from worktree + relative path
            a_path(apath, "");
            call(path8gDup, path8gIn(apath), path8cgIn(be->work_pp));
            call(path8gPush, path8gIn(apath), paths[i]);
            ok64 o = BEPostFile(be, &wb, stamp, path8cgIn(apath));
            if (o != OK) {
                ROCKBatchClose(&wb);
                fail(o);
            }
        }
    } else {
        // POST all files (deep scan)
        BEPostCtx ctx = {be, &wb, stamp};
        a_path(spath, "");
        call(path8gDup, path8gIn(spath), path8cgIn(be->work_pp));
        ok64 o = FILEDeepScanFiles(spath, BEPostScanCB, &ctx);
        if (o != OK) {
            ROCKBatchClose(&wb);
            fail(o);
        }
    }

    // Store commit metadata
    u8 ckbuf[512];
    u8s ckey = {ckbuf, ckbuf + sizeof(ckbuf)};
    call(BEKeyCommit, ckey, be->loc.path, stamp);
    u8cs commit_key = {ckbuf, ckey[0]};
    // Commit value: message (or empty)
    if ($ok(message) && !$empty(message)) {
        call(ROCKBatchPut, &wb, commit_key, message);
    } else {
        u8cs empty = {NULL, NULL};
        call(ROCKBatchPut, &wb, commit_key, empty);
    }

    // Update branch pointer if in branch mode
    {
        u8cs branch = {};
        if (BEOnBranch(be, branch)) {
            u8 tpbuf[512];
            u8s tpkey = {tpbuf, tpbuf + sizeof(tpbuf)};
            call(BEKeyBranchPtr, tpkey, be->loc.path, branch);
            u8cs branchptr_key = {tpbuf, tpkey[0]};
            u8cs stamp_raw = {(u8cp)&stamp, (u8cp)&stamp + sizeof(stamp)};
            call(ROCKBatchPut, &wb, branchptr_key, stamp_raw);
        }
    }

    call(ROCKBatchWrite, &be->db, &wb);
    call(ROCKBatchClose, &wb);
    done;
}

// ---- GET (repo → worktree) ----

// Export BASON value to a file in worktree
static ok64 BEExportFile(BEp be, u8cs relpath, u8cs bason) {
    sane(be != NULL && $ok(relpath) && $ok(bason));
    u8bp out = be->scratch[BE_RENDER];
    u8bReset(out);
    aBpad(u64, stk, 256);
    call(BASTExport, u8bIdle(out), stk, bason);
    u8cs source = {out[1], out[2]};

    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(be->work_pp));
    call(path8gPush, path8gIn(fpath), relpath);

    u8cs dir = {};
    path8gDir(dir, path8cgIn(fpath));
    if (!$empty(dir)) {
        a_path(dpath, "");
        call(u8sFeed, u8bIdle(dpath), dir);
        call(path8gTerm, path8gIn(dpath));
        FILEMakeDirP(path8cgIn(dpath));
    }
    call(BEWriteFile, path8cgIn(fpath), source);
    done;
}

static ok64 BEGetFile(BEp be, u8cs relpath) {
    sane(be != NULL);
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    call(BEKeyCur, key, be, relpath);
    u8cs db_key = {kbuf, key[0]};

    u8bp vbuf = be->scratch[BE_READ];
    u8bReset(vbuf);
    ok64 go = ROCKGet(&be->db, vbuf, db_key);
    if (go == ROCKnone) return BEnone;
    if (go != OK) return go;
    u8cs bason = {vbuf[1], vbuf[2]};
    call(BEExportFile, be, relpath, bason);
    done;
}

// Build project prefix "project/" into a key buffer, return as slice
static ok64 BEProjectPrefix(u8csp out, u8s buf, u8cs project) {
    sane(out != NULL && $ok(buf) && $ok(project));
    call(u8sFeed, buf, project);
    u8sFeed1(buf, '/');
    out[0] = buf[1];  // buf is [idle..end], data starts at original buf[0]
    out[1] = buf[0];
    done;
}

// ROCKScan callback for BEGetProject: export head keys
typedef struct {
    BEp be;
    size_t pfxlen;
} BEGetProjectCtx;

static ok64 BEGetProjectCB(voidp arg, u8cs key, u8cs val) {
    BEGetProjectCtx *ctx = (BEGetProjectCtx *)arg;
    u8cs rest = {key[0] + ctx->pfxlen, key[1]};
    if ($empty(rest)) return OK;
    u8cs qscan = {rest[0], rest[1]};
    if (u8csFind(qscan, '?') == OK) return OK;  // skip query keys
    return BEExportFile(ctx->be, rest, val);
}

// Get all head files for a project from DB into worktree
static ok64 BEGetProject(BEp be, u8cs project) {
    sane(be != NULL && $ok(project));
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, project);
    u8sFeed1(pfx, '/');
    u8cs prefix = {pfxbuf, pfx[0]};
    BEGetProjectCtx ctx = {be, $len(prefix)};
    call(ROCKScan, &be->db, prefix, BEGetProjectCB, &ctx);
    done;
}

// Switch BE to a new branch: rebuild URI, reparse, rewrite .be file
ok64 BESwitchBranch(BEp be, u8cs branch) {
    sane(be != NULL && $ok(branch) && !$empty(branch));
    // Build new URI in temp buffer (avoid aliasing loc_buf)
    u8 tmp[512];
    u8s uri_s = {tmp, tmp + sizeof(tmp)};
    // Copy existing URI parts, replace query with y=<branch>
    uri out = {};
    out.scheme[0] = be->loc.scheme[0];
    out.scheme[1] = be->loc.scheme[1];
    out.authority[0] = be->loc.authority[0];
    out.authority[1] = be->loc.authority[1];
    out.host[0] = be->loc.host[0];
    out.host[1] = be->loc.host[1];
    out.path[0] = be->loc.path[0];
    out.path[1] = be->loc.path[1];
    // Build "y=<branch>" query
    u8 qbuf[256];
    u8s qs = {qbuf, qbuf + sizeof(qbuf)};
    a_cstr(yp, "y=");
    call(u8sFeed, qs, yp);
    call(u8sFeed, qs, branch);
    out.query[0] = qbuf;
    out.query[1] = qs[0];
    call(URIutf8Feed, uri_s, &out);
    size_t ulen = uri_s[0] - tmp;
    test(ulen < sizeof(be->loc_buf), BEBAD);
    memcpy(be->loc_buf, tmp, ulen);
    be->loc.data[0] = be->loc_buf;
    be->loc.data[1] = be->loc_buf + ulen;
    call(URILexer, &be->loc);
    // Rewrite .be file
    a_path(dpath, "");
    call(path8gDup, path8gIn(dpath), path8cgIn(be->work_pp));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(dpath), dotbe);
    u8cs new_uri = {be->loc_buf, be->loc_buf + ulen};
    call(BEWriteFile, path8cgIn(dpath), new_uri);
    done;
}

ok64 BEGet(BEp be, int pathc, u8cs *paths, u8cs branch) {
    sane(be != NULL);

    // If branch specified, switch to that branch
    if ($ok(branch) && !$empty(branch)) {
        call(BESwitchBranch, be, branch);
    }

    if (pathc > 0 && paths != NULL) {
        for (int i = 0; i < pathc; i++) {
            call(BEGetFile, be, paths[i]);
        }
    } else {
        // Get all files for this project
        call(BEGetProject, be, be->loc.path);
    }
    done;
}

// ---- PUT (merge branch into head) ----

// ROCKScan callback for BEPut: merge matching branch keys into head
typedef struct {
    BEp be;
    ROCKbatchp wb;
    ron60 stamp;
    u8cs source_branch;
    size_t pfxlen;
} BEPutCtx;

static ok64 BEPutCB(voidp arg, u8cs key, u8cs val) {
    BEPutCtx *ctx = (BEPutCtx *)arg;
    BEp be = ctx->be;
    u8cs rest = {key[0] + ctx->pfxlen, key[1]};
    u8cs qscan = {rest[0], rest[1]};
    if (u8csFind(qscan, '?') != OK) return OK;
    u8cs filepath = {rest[0], qscan[0]};
    u8cs query = {qscan[0] + 1, rest[1]};
    if ($len(query) <= 2 || query[0][0] != 'y' || query[0][1] != '=')
        return OK;
    u8cs tw = {query[0] + 2, query[1]};
    if (!$eq(tw, ctx->source_branch)) return OK;

    // Get target (head) BASON
    u8 hkbuf[512];
    u8s hkey = {hkbuf, hkbuf + sizeof(hkbuf)};
    ok64 o = BEKeyHead(hkey, be->loc.path, filepath);
    if (o != OK) return o;
    u8cs head_key = {hkbuf, hkey[0]};

    u8bp tbuf = be->scratch[BE_READ];
    u8bReset(tbuf);
    ok64 go = ROCKGet(&be->db, tbuf, head_key);
    u8cs target_bason = {};
    if (go == OK) {
        target_bason[0] = tbuf[1];
        target_bason[1] = tbuf[2];
    }

    if ($ok(target_bason) && !$empty(target_bason)) {
        // Merge
        u8bp mbuf = be->scratch[BE_PARSE];
        u8bReset(mbuf);
        aBpad(u64, midx, 4096);
        aBpad(u64, lstk, 256);
        aBpad(u64, rstk, 256);
        o = BASONMerge(mbuf, midx, lstk, target_bason, rstk, val);
        if (o != OK) return o;
        u8cs merged = {mbuf[1], mbuf[2]};

        // Diff for waypoint
        u8bp dbuf = be->scratch[BE_PATCH];
        u8bReset(dbuf);
        aBpad(u64, didx, 4096);
        aBpad(u64, ostk, 256);
        aBpad(u64, nstk, 256);
        o = BASONDiff(dbuf, didx, ostk, target_bason, nstk, merged);
        if (o != OK) return o;
        u8cs delta = {dbuf[1], dbuf[2]};

        o = ROCKBatchPut(ctx->wb, head_key, merged);
        if (o != OK) return o;
        if (!$empty(delta)) {
            u8 vkbuf[512];
            u8s vkey = {vkbuf, vkbuf + sizeof(vkbuf)};
            o = BEKeyVer(vkey, be->loc.path, filepath, ctx->stamp);
            if (o != OK) return o;
            u8cs wp_key = {vkbuf, vkey[0]};
            o = ROCKBatchPut(ctx->wb, wp_key, delta);
            if (o != OK) return o;
        }
    } else {
        // No target, just copy source as head
        o = ROCKBatchPut(ctx->wb, head_key, val);
        if (o != OK) return o;
    }
    return OK;
}

ok64 BEPut(BEp be, u8cs source_branch, u8cs message) {
    sane(be != NULL && $ok(source_branch) && !$empty(source_branch));
    ron60 stamp = RONNow();

    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    // Scan project prefix, merge matching branch keys
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, be->loc.path);
    u8sFeed1(pfx, '/');
    u8cs project_prefix = {pfxbuf, pfx[0]};

    BEPutCtx pctx = {be, &wb, stamp, {source_branch[0], source_branch[1]},
                      $len(project_prefix)};
    call(ROCKScan, &be->db, project_prefix, BEPutCB, &pctx);

    // Commit metadata
    u8 ckbuf[512];
    u8s ckey = {ckbuf, ckbuf + sizeof(ckbuf)};
    call(BEKeyCommit, ckey, be->loc.path, stamp);
    u8cs commit_key = {ckbuf, ckey[0]};
    if ($ok(message) && !$empty(message)) {
        call(ROCKBatchPut, &wb, commit_key, message);
    } else {
        u8cs empty = {NULL, NULL};
        call(ROCKBatchPut, &wb, commit_key, empty);
    }

    call(ROCKBatchWrite, &be->db, &wb);
    call(ROCKBatchClose, &wb);
    done;
}

// ---- DELETE ----

// ROCKScan callback: delete keys matching ?y=<branch>
typedef struct {
    ROCKbatchp wb;
    u8cs branch;
    size_t pfxlen;
} BEDeleteBranchCtx;

static ok64 BEDeleteBranchCB(voidp arg, u8cs key, u8cs val) {
    BEDeleteBranchCtx *ctx = (BEDeleteBranchCtx *)arg;
    u8cs rest = {key[0] + ctx->pfxlen, key[1]};
    u8cs qs = {rest[0], rest[1]};
    if (u8csFind(qs, '?') == OK) {
        u8cs kq = {qs[0] + 1, rest[1]};
        if ($len(kq) > 2 && kq[0][0] == 'y' && kq[0][1] == '=') {
            u8cs kbranch = {kq[0] + 2, kq[1]};
            if ($eq(kbranch, ctx->branch)) {
                return ROCKBatchDel(ctx->wb, key);
            }
        }
    }
    return OK;
}

ok64 BEDelete(BEp be, u8cs target) {
    sane(be != NULL && $ok(target));

    // Parse target to determine what to delete
    // If target contains '?y=', delete branch
    // Otherwise delete file head key
    u8cs tscan = {target[0], target[1]};

    if (u8csFind(tscan, '?') == OK) {
        // Has query — delete branch keys by prefix scan
        u8 pfxbuf[512];
        u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
        call(u8sFeed, pfx, be->loc.path);
        u8sFeed1(pfx, '/');
        u8cs project_prefix = {pfxbuf, pfx[0]};

        u8cs query = {tscan[0] + 1, target[1]};
        // Expect y=<branch>
        test($len(query) > 2 && query[0][0] == 'y' && query[0][1] == '=',
             BEBAD);
        u8cs branch = {query[0] + 2, query[1]};

        ROCKbatch wb = {};
        call(ROCKBatchOpen, &wb);

        BEDeleteBranchCtx dctx = {&wb, {branch[0], branch[1]},
                                   $len(project_prefix)};
        call(ROCKScan, &be->db, project_prefix, BEDeleteBranchCB, &dctx);

        // Also delete branch pointer
        u8 tpbuf[512];
        u8s tpkey = {tpbuf, tpbuf + sizeof(tpbuf)};
        call(BEKeyBranchPtr, tpkey, be->loc.path, branch);
        u8cs branchptr_key = {tpbuf, tpkey[0]};
        call(ROCKBatchDel, &wb, branchptr_key);

        call(ROCKBatchWrite, &be->db, &wb);
        call(ROCKBatchClose, &wb);
    } else {
        // Delete file head key
        u8 kbuf[512];
        u8s key = {kbuf, kbuf + sizeof(kbuf)};
        call(BEKeyHead, key, be->loc.path, target);
        u8cs head_key = {kbuf, key[0]};
        call(ROCKDel, &be->db, head_key);

        // Store tombstone waypoint
        ron60 stamp = RONNow();
        u8 vkbuf[512];
        u8s vkey = {vkbuf, vkbuf + sizeof(vkbuf)};
        call(BEKeyVer, vkey, be->loc.path, target, stamp);
        u8cs wp_key = {vkbuf, vkey[0]};
        u8cs empty = {};
        call(ROCKPut, &be->db, wp_key, empty);
    }
    done;
}

// ---- GET deps (.beget) ----

ok64 BEGetDeps(BEp be, b8 include_opt) {
    sane(be != NULL);

    // Build path to .beget
    a_path(bpath, "");
    call(path8gDup, path8gIn(bpath), path8cgIn(be->work_pp));
    a_cstr(beget, ".beget");
    call(path8gPush, path8gIn(bpath), beget);

    // No .beget = no deps, that's OK
    struct stat st;
    ok64 o = FILEStat(&st, path8cgIn(bpath));
    if (o != OK) done;

    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(bpath));
    u8cp m0 = mapbuf[1], m1 = mapbuf[2];
    u8cs content = {m0, m1};

    // section: 0=none, 1=deps, 2=opt
    int section = 0;
    u8cp pos = content[0];

    while (pos < content[1]) {
        // Find line boundaries
        u8cp line_end = pos;
        while (line_end < content[1] && *line_end != '\n') line_end++;
        u8cs line = {pos, line_end};
        pos = line_end < content[1] ? line_end + 1 : line_end;

        // Skip blank
        if ($empty(line)) continue;

        // Trim leading whitespace
        u8cp lp = line[0];
        while (lp < line[1] && (*lp == ' ' || *lp == '\t')) lp++;
        if (lp >= line[1]) continue;

        // Skip comments
        if (*lp == '#') continue;

        // Trim trailing whitespace
        u8cp le = line[1];
        while (le > lp && (le[-1] == ' ' || le[-1] == '\t' || le[-1] == '\r'))
            le--;
        if (le <= lp) continue;

        // Section header
        if (*lp == '[') {
            a_cstr(deps_hdr, "[deps]");
            a_cstr(opt_hdr, "[opt]");
            u8cs sec = {lp, le};
            if ($eq(sec, deps_hdr))
                section = 1;
            else if ($eq(sec, opt_hdr))
                section = 2;
            continue;
        }

        // Skip if no section, or opt not requested
        if (section == 0) continue;
        if (section == 2 && !include_opt) continue;

        // Parse dep URI to extract project path
        u8cs dep_uri = {lp, le};
        uri dep_loc = {};
        u8 dep_buf[512];
        size_t dlen = $len(dep_uri);
        test(dlen < sizeof(dep_buf), BEBAD);
        memcpy(dep_buf, dep_uri[0], dlen);
        dep_loc.data[0] = dep_buf;
        dep_loc.data[1] = dep_buf + dlen;
        o = URILexer(&dep_loc);
        if (o != OK) continue;

        u8cs dep_project = {dep_loc.path[0], dep_loc.path[1]};
        if (!$ok(dep_project) || $empty(dep_project)) continue;

        // Get all files for this project (same repo for now)
        call(BEGetProject, be, dep_project);
    }

    call(FILEUnMap, mapbuf);
    done;
}

// ---- Checkpoint (fork repo) ----

ok64 BECheckpoint(BEp be, u8cs new_repo) {
    sane(be != NULL && $ok(new_repo) && !$empty(new_repo));
    a_path(dpath, "");
    call(BERepoPath, path8gIn(dpath), new_repo);
    call(ROCKCheckpoint, &be->db, path8cgIn(dpath));
    done;
}
