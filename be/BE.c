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

ok64 BEKeyTwig(u8s into, u8cs project, u8cs path, u8cs twig) {
    sane($ok(into) && $ok(project) && $ok(twig));
    call(u8sFeed, into, project);
    u8sFeed1(into, '/');
    if ($ok(path) && !$empty(path)) {
        call(u8sFeed, into, path);
    }
    a_cstr(qy, "?y=");
    call(u8sFeed, into, qy);
    call(u8sFeed, into, twig);
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

ok64 BEKeyTwigPtr(u8s into, u8cs project, u8cs twig) {
    sane($ok(into) && $ok(project) && $ok(twig));
    call(u8sFeed, into, project);
    a_cstr(sep, "/?y=");
    call(u8sFeed, into, sep);
    call(u8sFeed, into, twig);
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

ok64 BEKeyCur(u8s into, BEp be, u8cs path) {
    sane(be != NULL && $ok(into));
    if ($ok(be->loc.query) && !$empty(be->loc.query)) {
        // twig mode: query contains "y=<twig>"
        u8cs q = {be->loc.query[0], be->loc.query[1]};
        // skip "y=" prefix
        if ($len(q) > 2 && q[0][0] == 'y' && q[0][1] == '=') {
            u8cs twig = {q[0] + 2, q[1]};
            return BEKeyTwig(into, be->loc.path, path, twig);
        }
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

// Make directory path recursively (like mkdir -p)
static ok64 BEMakeDirP(path8cg path) {
    sane(path != NULL);
    // Try direct first
    ok64 o = FILEMakeDir(path);
    if (o == OK || o == FILEEXIST) return OK;
    // Parent might not exist, try creating it
    u8 pbuf[FILE_PATH_MAX_LEN];
    path8 parent = {pbuf, pbuf, pbuf, pbuf + FILE_PATH_MAX_LEN};
    u8cs dir = {};
    path8gDir(dir, path);
    if ($empty(dir) || $len(dir) == 0) return o;
    call(u8sFeed, u8bIdle(parent), dir);
    call(path8gTerm, path8gIn(parent));
    call(BEMakeDirP, path8cgIn(parent));
    o = FILEMakeDir(path);
    if (o == FILEEXIST) return OK;
    return o;
}

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

// Walk up from path looking for .be file, return dir containing it
static ok64 BEFindDotBe(path8g result, path8cg start) {
    sane(result != NULL && $ok(start));
    u8 pbuf[FILE_PATH_MAX_LEN];
    path8 cur = {pbuf, pbuf, pbuf, pbuf + FILE_PATH_MAX_LEN};
    call(path8gDup, path8gIn(cur), start);
    for (int depth = 0; depth < 64; depth++) {
        // Save current and try .be
        u8 tbuf[FILE_PATH_MAX_LEN];
        path8 trial = {tbuf, tbuf, tbuf, tbuf + FILE_PATH_MAX_LEN};
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
    be->work_pp[0] = be->work_pbuf;
    be->work_pp[1] = be->work_pbuf;
    be->work_pp[2] = be->work_pbuf;
    be->work_pp[3] = be->work_pbuf + FILE_PATH_MAX_LEN;
    call(path8gDup, path8gIn(be->work_pp), worktree);

    // Build repo path: $HOME/.be/<branch>/
    be->repo_pp[0] = be->repo_pbuf;
    be->repo_pp[1] = be->repo_pbuf;
    be->repo_pp[2] = be->repo_pbuf;
    be->repo_pp[3] = be->repo_pbuf + FILE_PATH_MAX_LEN;
    const char *home = getenv("HOME");
    test(home != NULL, BEFAIL);
    a_cstr(homecs, home);
    call(u8sFeed, u8bIdle(be->repo_pp), homecs);
    call(path8gTerm, path8gIn(be->repo_pp));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(be->repo_pp), dotbe);
    // branch = loc.host
    test($ok(be->loc.host) && !$empty(be->loc.host), BEBAD);
    call(path8gPush, path8gIn(be->repo_pp), be->loc.host);

    // Create directories
    call(BEMakeDirP, path8cgIn(be->repo_pp));

    // Write .be file in worktree
    u8 dotbe_pbuf[FILE_PATH_MAX_LEN];
    path8 dotbe_path = {dotbe_pbuf, dotbe_pbuf, dotbe_pbuf,
                        dotbe_pbuf + FILE_PATH_MAX_LEN};
    call(path8gDup, path8gIn(dotbe_path), worktree);
    call(path8gPush, path8gIn(dotbe_path), dotbe);
    call(BEWriteFile, path8cgIn(dotbe_path), be_uri);

    // Open DB
    call(ROCKOpen, &be->db, path8cgIn(be->repo_pp));
    done;
}

ok64 BEOpen(BEp be, path8cg worktree) {
    sane(be != NULL && worktree != NULL);
    memset(be, 0, sizeof(BE));

    // Setup worktree path
    be->work_pp[0] = be->work_pbuf;
    be->work_pp[1] = be->work_pbuf;
    be->work_pp[2] = be->work_pbuf;
    be->work_pp[3] = be->work_pbuf + FILE_PATH_MAX_LEN;

    // Find .be file
    call(BEFindDotBe, path8gIn(be->work_pp), worktree);

    // Read .be file
    u8 dotbe_pbuf[FILE_PATH_MAX_LEN];
    path8 dotbe_path = {dotbe_pbuf, dotbe_pbuf, dotbe_pbuf,
                        dotbe_pbuf + FILE_PATH_MAX_LEN};
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

    // Build repo path: $HOME/.be/<branch>/
    be->repo_pp[0] = be->repo_pbuf;
    be->repo_pp[1] = be->repo_pbuf;
    be->repo_pp[2] = be->repo_pbuf;
    be->repo_pp[3] = be->repo_pbuf + FILE_PATH_MAX_LEN;
    const char *home = getenv("HOME");
    test(home != NULL, BEFAIL);
    a_cstr(homecs, home);
    call(u8sFeed, u8bIdle(be->repo_pp), homecs);
    call(path8gTerm, path8gIn(be->repo_pp));
    a_cstr(dotbe2, ".be");
    call(path8gPush, path8gIn(be->repo_pp), dotbe2);
    test($ok(be->loc.host) && !$empty(be->loc.host), BEBAD);
    call(path8gPush, path8gIn(be->repo_pp), be->loc.host);

    // Open DB
    call(ROCKOpen, &be->db, path8cgIn(be->repo_pp));
    done;
}

ok64 BEClose(BEp be) {
    if (be == NULL) return OK;
    ROCKClose(&be->db);
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
    u8 relbuf[FILE_PATH_MAX_LEN];
    path8 relpath = {relbuf, relbuf, relbuf, relbuf + FILE_PATH_MAX_LEN};
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

    // Parse to BASON
    aBpad(u8, nbuf, 65536);
    aBpad(u64, nidx, 4096);
    ok64 o = BASTParse(nbuf, nidx, source, ext);
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
    aBpad(u8, obuf, 65536);
    ok64 go = ROCKGet(&be->db, obuf, head_key);

    if (go == OK) {
        // Diff old vs new
        u8cs old_bason = {obuf[1], obuf[2]};
        aBpad(u8, dbuf, 65536);
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

    // If twig mode, also store twig key
    if ($ok(be->loc.query) && !$empty(be->loc.query) &&
        $len(be->loc.query) > 2 && be->loc.query[0][0] == 'y' &&
        be->loc.query[0][1] == '=') {
        u8cs twig = {be->loc.query[0] + 2, be->loc.query[1]};
        u8 tkbuf[512];
        u8s tkey = {tkbuf, tkbuf + sizeof(tkbuf)};
        call(BEKeyTwig, tkey, be->loc.path, rel, twig);
        u8cs twig_key = {tkbuf, tkey[0]};
        call(ROCKBatchPut, wb, twig_key, new_bason);
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
            u8 apbuf[FILE_PATH_MAX_LEN];
            path8 apath = {apbuf, apbuf, apbuf, apbuf + FILE_PATH_MAX_LEN};
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
        u8 spbuf[FILE_PATH_MAX_LEN];
        path8 spath = {spbuf, spbuf, spbuf, spbuf + FILE_PATH_MAX_LEN};
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

    // Update twig pointer if in twig mode
    if ($ok(be->loc.query) && !$empty(be->loc.query) &&
        $len(be->loc.query) > 2 && be->loc.query[0][0] == 'y' &&
        be->loc.query[0][1] == '=') {
        u8cs twig = {be->loc.query[0] + 2, be->loc.query[1]};
        u8 tpbuf[512];
        u8s tpkey = {tpbuf, tpbuf + sizeof(tpbuf)};
        call(BEKeyTwigPtr, tpkey, be->loc.path, twig);
        u8cs twigptr_key = {tpbuf, tpkey[0]};
        u8cs stamp_raw = {(u8cp)&stamp, (u8cp)&stamp + sizeof(stamp)};
        call(ROCKBatchPut, &wb, twigptr_key, stamp_raw);
    }

    call(ROCKBatchWrite, &be->db, &wb);
    call(ROCKBatchClose, &wb);
    done;
}

// ---- GET (repo → worktree) ----

static ok64 BEGetFile(BEp be, u8cs relpath) {
    sane(be != NULL);
    // Build key
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    call(BEKeyCur, key, be, relpath);
    u8cs db_key = {kbuf, key[0]};

    // Get BASON from DB
    aBpad(u8, vbuf, 65536);
    ok64 go = ROCKGet(&be->db, vbuf, db_key);
    if (go == ROCKnone) return BEnone;
    if (go != OK) return go;

    u8cs bason = {vbuf[1], vbuf[2]};

    // Export to source text
    aBpad(u8, out, 65536);
    aBpad(u64, stk, 256);
    call(BASTExport, u8bIdle(out), stk, bason);

    u8cs source = {out[1], out[2]};

    // Build worktree file path
    u8 fpbuf[FILE_PATH_MAX_LEN];
    path8 fpath = {fpbuf, fpbuf, fpbuf, fpbuf + FILE_PATH_MAX_LEN};
    call(path8gDup, path8gIn(fpath), path8cgIn(be->work_pp));
    call(path8gPush, path8gIn(fpath), relpath);

    // Ensure parent directory exists
    u8cs dir = {};
    path8gDir(dir, path8cgIn(fpath));
    if (!$empty(dir)) {
        u8 dpbuf[FILE_PATH_MAX_LEN];
        path8 dpath = {dpbuf, dpbuf, dpbuf, dpbuf + FILE_PATH_MAX_LEN};
        call(u8sFeed, u8bIdle(dpath), dir);
        call(path8gTerm, path8gIn(dpath));
        BEMakeDirP(path8cgIn(dpath));
    }

    // Write file
    call(BEWriteFile, path8cgIn(fpath), source);
    done;
}

ok64 BEGet(BEp be, int pathc, u8cs *paths, u8cs twig) {
    sane(be != NULL);

    // If twig specified, switch to that twig
    if ($ok(twig) && !$empty(twig)) {
        // Rebuild URI with new query
        size_t ulen = 0;
        u8 *p = be->loc_buf;
        u8s uri_s = {p, p + sizeof(be->loc_buf)};
        // scheme://host/path?y=twig
        if ($ok(be->loc.scheme) && !$empty(be->loc.scheme)) {
            call(u8sFeed, uri_s, be->loc.scheme);
            a_cstr(sep, "://");
            call(u8sFeed, uri_s, sep);
        }
        if ($ok(be->loc.host) && !$empty(be->loc.host)) {
            call(u8sFeed, uri_s, be->loc.host);
        }
        if ($ok(be->loc.path) && !$empty(be->loc.path)) {
            u8sFeed1(uri_s, '/');
            call(u8sFeed, uri_s, be->loc.path);
        }
        a_cstr(qy, "?y=");
        call(u8sFeed, uri_s, qy);
        call(u8sFeed, uri_s, twig);
        ulen = uri_s[0] - be->loc_buf;
        be->loc.data[0] = be->loc_buf;
        be->loc.data[1] = be->loc_buf + ulen;
        call(URILexer, &be->loc);

        // Rewrite .be file
        u8 dbuf[FILE_PATH_MAX_LEN];
        path8 dpath = {dbuf, dbuf, dbuf, dbuf + FILE_PATH_MAX_LEN};
        call(path8gDup, path8gIn(dpath), path8cgIn(be->work_pp));
        a_cstr(dotbe, ".be");
        call(path8gPush, path8gIn(dpath), dotbe);
        u8cs new_uri = {be->loc_buf, be->loc_buf + ulen};
        call(BEWriteFile, path8cgIn(dpath), new_uri);
    }

    if (pathc > 0 && paths != NULL) {
        for (int i = 0; i < pathc; i++) {
            call(BEGetFile, be, paths[i]);
        }
    } else {
        // Get all: iterate DB prefix
        u8 pfxbuf[512];
        u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
        call(u8sFeed, pfx, be->loc.path);
        u8sFeed1(pfx, '/');
        u8cs prefix = {pfxbuf, pfx[0]};

        ROCKiter it = {};
        call(ROCKIterOpen, &it, &be->db);
        call(ROCKIterSeek, &it, prefix);
        while (ROCKIterValid(&it)) {
            u8cs k = {};
            ROCKIterKey(&it, k);
            // Check prefix match
            if ($len(k) < $len(prefix) ||
                memcmp(k[0], prefix[0], $len(prefix)) != 0)
                break;
            // Skip query keys (? = waypoints, twigs, config)
            u8cs rest = {k[0] + $len(prefix), k[1]};
            b8 has_query = NO;
            u8cp rp = rest[0];
            while (rp < rest[1]) {
                if (*rp == '?') {
                    has_query = YES;
                    break;
                }
                rp++;
            }
            if (!has_query) {
                // bare path = head file, extract relative path
                u8cs relpath = {rest[0], rest[1]};
                call(BEGetFile, be, relpath);
            }
            call(ROCKIterNext, &it);
        }
        call(ROCKIterClose, &it);
    }
    done;
}

// ---- PUT (merge twig/branch into head) ----

ok64 BEPut(BEp be, u8cs source_twig, u8cs message) {
    sane(be != NULL && $ok(source_twig) && !$empty(source_twig));
    ron60 stamp = RONNow();

    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    // Iterate source twig keys
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, be->loc.path);
    u8sFeed1(pfx, '/');
    u8cs project_prefix = {pfxbuf, pfx[0]};

    // Scan all head keys under project
    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be->db);
    call(ROCKIterSeek, &it, project_prefix);

    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(project_prefix) ||
            memcmp(k[0], project_prefix[0], $len(project_prefix)) != 0)
            break;

        // Only process twig keys matching source_twig
        u8cs rest = {k[0] + $len(project_prefix), k[1]};
        // Look for ?y=<source_twig>
        u8cp qp = rest[0];
        b8 is_twig_key = NO;
        u8cs filepath = {};
        while (qp < rest[1]) {
            if (*qp == '?') {
                filepath[0] = rest[0];
                filepath[1] = qp;
                u8cs query = {qp + 1, rest[1]};
                if ($len(query) > 2 && query[0][0] == 'y' &&
                    query[0][1] == '=') {
                    u8cs tw = {query[0] + 2, query[1]};
                    if ($eq(tw, source_twig)) is_twig_key = YES;
                }
                break;
            }
            qp++;
        }
        if (!is_twig_key) {
            call(ROCKIterNext, &it);
            continue;
        }

        // Get source (twig) BASON
        u8cs src_bason = {};
        ROCKIterVal(&it, src_bason);

        // Get target (head) BASON
        u8 hkbuf[512];
        u8s hkey = {hkbuf, hkbuf + sizeof(hkbuf)};
        call(BEKeyHead, hkey, be->loc.path, filepath);
        u8cs head_key = {hkbuf, hkey[0]};

        aBpad(u8, tbuf, 65536);
        ok64 go = ROCKGet(&be->db, tbuf, head_key);
        u8cs target_bason = {};
        if (go == OK) {
            target_bason[0] = tbuf[1];
            target_bason[1] = tbuf[2];
        }

        if ($ok(target_bason) && !$empty(target_bason)) {
            // Merge
            aBpad(u8, mbuf, 65536);
            aBpad(u64, midx, 4096);
            aBpad(u64, lstk, 256);
            aBpad(u64, rstk, 256);
            call(BASONMerge, mbuf, midx, lstk, target_bason, rstk,
                 src_bason);
            u8cs merged = {mbuf[1], mbuf[2]};

            // Diff for waypoint
            aBpad(u8, dbuf, 65536);
            aBpad(u64, didx, 4096);
            aBpad(u64, ostk, 256);
            aBpad(u64, nstk, 256);
            call(BASONDiff, dbuf, didx, ostk, target_bason, nstk,
                 merged);
            u8cs delta = {dbuf[1], dbuf[2]};

            call(ROCKBatchPut, &wb, head_key, merged);
            if (!$empty(delta)) {
                u8 vkbuf[512];
                u8s vkey = {vkbuf, vkbuf + sizeof(vkbuf)};
                call(BEKeyVer, vkey, be->loc.path, filepath, stamp);
                u8cs wp_key = {vkbuf, vkey[0]};
                call(ROCKBatchPut, &wb, wp_key, delta);
            }
        } else {
            // No target, just copy source as head
            call(ROCKBatchPut, &wb, head_key, src_bason);
        }

        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);

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

ok64 BEDelete(BEp be, u8cs target) {
    sane(be != NULL && $ok(target));

    // Parse target to determine what to delete
    // If target contains '?y=', delete twig
    // Otherwise delete file head key
    u8cp qp = target[0];
    while (qp < target[1] && *qp != '?') qp++;

    if (qp < target[1]) {
        // Has query — delete twig keys by prefix scan
        u8 pfxbuf[512];
        u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
        call(u8sFeed, pfx, be->loc.path);
        u8sFeed1(pfx, '/');
        u8cs project_prefix = {pfxbuf, pfx[0]};

        u8cs query = {qp + 1, target[1]};
        // Expect y=<twig>
        test($len(query) > 2 && query[0][0] == 'y' && query[0][1] == '=',
             BEBAD);
        u8cs twig = {query[0] + 2, query[1]};

        ROCKbatch wb = {};
        call(ROCKBatchOpen, &wb);

        ROCKiter it = {};
        call(ROCKIterOpen, &it, &be->db);
        call(ROCKIterSeek, &it, project_prefix);
        while (ROCKIterValid(&it)) {
            u8cs k = {};
            ROCKIterKey(&it, k);
            if ($len(k) < $len(project_prefix) ||
                memcmp(k[0], project_prefix[0], $len(project_prefix)) != 0)
                break;
            // Check if key ends with ?y=<twig>
            u8cs rest = {k[0] + $len(project_prefix), k[1]};
            u8cp kqp = rest[0];
            while (kqp < rest[1] && *kqp != '?') kqp++;
            if (kqp < rest[1]) {
                u8cs kq = {kqp + 1, rest[1]};
                if ($len(kq) > 2 && kq[0][0] == 'y' && kq[0][1] == '=') {
                    u8cs ktwig = {kq[0] + 2, kq[1]};
                    if ($eq(ktwig, twig)) {
                        call(ROCKBatchDel, &wb, k);
                    }
                }
            }
            call(ROCKIterNext, &it);
        }
        call(ROCKIterClose, &it);

        // Also delete twig pointer
        u8 tpbuf[512];
        u8s tpkey = {tpbuf, tpbuf + sizeof(tpbuf)};
        call(BEKeyTwigPtr, tpkey, be->loc.path, twig);
        u8cs twigptr_key = {tpbuf, tpkey[0]};
        call(ROCKBatchDel, &wb, twigptr_key);

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

// ---- Checkpoint (branch) ----

ok64 BECheckpoint(BEp be, u8cs new_branch) {
    sane(be != NULL && $ok(new_branch) && !$empty(new_branch));
    // Build dest path: $HOME/.be/<new_branch>/
    u8 dpbuf[FILE_PATH_MAX_LEN];
    path8 dpath = {dpbuf, dpbuf, dpbuf, dpbuf + FILE_PATH_MAX_LEN};
    const char *home = getenv("HOME");
    test(home != NULL, BEFAIL);
    a_cstr(homecs, home);
    call(u8sFeed, u8bIdle(dpath), homecs);
    call(path8gTerm, path8gIn(dpath));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(dpath), dotbe);
    call(path8gPush, path8gIn(dpath), new_branch);
    call(ROCKCheckpoint, &be->db, path8cgIn(dpath));
    done;
}
