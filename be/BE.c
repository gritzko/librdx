#include "BE.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <utime.h>

#include "abc/POL.h"
#include "abc/PRO.h"
#include "IGNO.h"

// ---- File metadata helpers ----

ok64 BEMetaPack(u8s into, BEmeta m) {
    sane($ok(into) && $len(into) >= BE_META_SIZE);
    call(u8sFeed32, into, &m.mtime);
    call(u8sFeed32, into, &m.modeftype);
    done;
}

ok64 BEMetaUnpack(BEmeta *m, u8cs from) {
    sane(m != NULL && $ok(from) && $len(from) >= BE_META_SIZE);
    call(u8sDrain32, from, &m->mtime);
    call(u8sDrain32, from, &m->modeftype);
    done;
}

ok64 BEMetaFromStat(BEmeta *m, struct stat *st, u8cs ext) {
    sane(m != NULL && st != NULL);
    m->mtime = (u32)st->st_mtime;
    u32 mode = st->st_mode & 07777;
    u32 ftype = BASTFtype(ext);
    m->modeftype = (mode << 20) | ((ftype & 0x3FFFF) << 2);
    done;
}

// ---- Query sub-structure helpers ----

ok64 BEQueryBuild(u8s into, ron60 stamp, u8cs branch) {
    sane($ok(into));
    call(RONu8sFeedPad, into, stamp, 10);
    into[0] += 10;
    u8sFeed1(into, '-');
    if ($ok(branch) && !$empty(branch)) {
        call(u8sFeed, into, branch);
    }
    done;
}

ok64 BEQueryParse(ron60 *stamp, u8csp branch, u8cs query) {
    sane($ok(query));
    if ($len(query) < 11) fail(BEnone);
    if (stamp != NULL) {
        u8cs ts = {query[0], query[0] + 10};
        call(RONutf8sDrain, stamp, ts);
    }
    if (query[0][10] != '-') fail(BEnone);
    if (branch != NULL) {
        branch[0] = query[0] + 11;
        branch[1] = query[1];
    }
    done;
}

// ---- Key parsers ----

ok64 BEKeyBranchSuffix(u8csp branch, u8cs key) {
    sane(branch != NULL && $ok(key));
    uri u = {};
    call(URIutf8Drain, key, &u);
    if ($empty(u.query)) fail(BEnone);
    call(BEQueryParse, NULL, branch, u.query);
    done;
}

ok64 BEKeyStamp(ron60 *stamp, u8cs key) {
    sane(stamp != NULL && $ok(key));
    uri u = {};
    call(URIutf8Drain, key, &u);
    if ($empty(u.query)) fail(BEnone);
    call(BEQueryParse, stamp, NULL, u.query);
    done;
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
            call(u8sFeed, out, val);
        } else if (raw == 'A' || raw == 'O') {
            call(BASONInto, stack, data, val);
            call(BASTExportRec, out, stack, data);
            call(BASONOuto, stack);
        }
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

static ok64 BEFindDotBe(path8g result, path8cg start) {
    sane(result != NULL && $ok(start));
    a_path(cur, "");
    call(path8gDup, path8gIn(cur), start);
    for (int depth = 0; depth < 64; depth++) {
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
        o = path8gPop(path8gIn(cur));
        if (o != OK) break;
        u8cs d = {cur[1], cur[2]};
        if ($len(d) <= 1) break;
    }
    fail(BEnone);
}

static ok64 BEOpenDB(ROCKdbp db, path8cg path) {
    sane(db != NULL && path != NULL);
    call(ROCKInit, db, YES);
    int compression[7] = {
        rocksdb_snappy_compression,
        rocksdb_snappy_compression,
        rocksdb_zstd_compression,
        rocksdb_zstd_compression,
        rocksdb_zstd_compression,
        rocksdb_zstd_compression,
        rocksdb_zstd_compression,
    };
    rocksdb_options_set_num_levels(db->opt, 7);
    rocksdb_options_set_compression_per_level(db->opt, compression, 7);
    rocksdb_options_set_level_compaction_dynamic_level_bytes(db->opt, 1);
    rocksdb_options_set_max_background_jobs(db->opt, 2);
    rocksdb_writeoptions_disable_WAL(db->wopt, 1);
    if (db->cache) {
        rocksdb_cache_set_capacity(db->cache, 128 * MB);
    }
    return ROCKOpenDB(db, path);
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

// ---- Multi-branch query parsing ----

// Parse query "branch1&branch2&main" into be->branches[]
static ok64 BEParseBranches(BEp be) {
    sane(be != NULL);
    be->branchc = 0;
    if (!$ok(be->loc.query) || $empty(be->loc.query)) {
        // No query = just main
        a_cstr(m, "main");
        be->branches[0][0] = m[0];
        be->branches[0][1] = m[1];
        be->branchc = 1;
        done;
    }
    u8cp p = be->loc.query[0];
    u8cp end = be->loc.query[1];
    while (p < end && be->branchc < BE_MAX_BRANCHES) {
        u8cp start = p;
        while (p < end && *p != '&') p++;
        if (p > start) {
            be->branches[be->branchc][0] = start;
            be->branches[be->branchc][1] = p;
            be->branchc++;
        }
        if (p < end) p++;  // skip '&'
    }
    // Active branch is always #0
    done;
}

// Check if a branch name is in the branch filter list
static b8 BEBranchVisible(BEp be, u8cs branch) {
    for (int i = 0; i < be->branchc; i++) {
        if ($eq(be->branches[i], branch)) return YES;
    }
    return NO;
}

// Build .be URI string from current state
static ok64 BEBuildURI(u8s into, BEp be) {
    sane($ok(into) && be != NULL);
    a_cstr(scheme, "be://");
    call(u8sFeed, into, scheme);
    if ($ok(be->loc.host) && !$empty(be->loc.host)) {
        call(u8sFeed, into, be->loc.host);
    }
    if ($ok(be->loc.path) && !$empty(be->loc.path)) {
        call(u8sFeed, into, be->loc.path);
    }
    if (be->branchc > 0) {
        u8sFeed1(into, '?');
        for (int i = 0; i < be->branchc; i++) {
            if (i > 0) u8sFeed1(into, '&');
            call(u8sFeed, into, be->branches[i]);
        }
    }
    done;
}

// Rewrite .be file from current state
static ok64 BERewriteDotBe(BEp be) {
    sane(be != NULL);
    u8 tmp[512];
    u8s uri_s = {tmp, tmp + sizeof(tmp)};
    call(BEBuildURI, uri_s, be);
    size_t ulen = uri_s[0] - tmp;
    test(ulen < sizeof(be->loc_buf), BEBAD);
    memcpy(be->loc_buf, tmp, ulen);
    be->loc.data[0] = be->loc_buf;
    be->loc.data[1] = be->loc_buf + ulen;
    call(URILexer, &be->loc);
    call(BEParseBranches, be);
    // Write file
    a_path(dpath, "");
    call(path8gDup, path8gIn(dpath), path8cgIn(be->work_pp));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(dpath), dotbe);
    u8cs new_uri = {be->loc_buf, be->loc_buf + ulen};
    call(BEWriteFile, path8cgIn(dpath), new_uri);
    done;
}

// ---- Lifecycle ----

ok64 BEInit(BEp be, u8cs be_uri, path8cg worktree) {
    sane(be != NULL && $ok(be_uri) && worktree != NULL);
    memset(be, 0, sizeof(BE));

    size_t ulen = $len(be_uri);
    test(ulen < sizeof(be->loc_buf), BEBAD);
    memcpy(be->loc_buf, be_uri[0], ulen);
    be->loc.data[0] = be->loc_buf;
    be->loc.data[1] = be->loc_buf + ulen;
    call(URILexer, &be->loc);
    call(BEParseBranches, be);

    call(path8bAlloc, be->work_pp);
    call(path8gDup, path8gIn(be->work_pp), worktree);

    call(path8bAlloc, be->repo_pp);
    test($ok(be->loc.host) && !$empty(be->loc.host), BEBAD);
    call(BERepoPath, path8gIn(be->repo_pp), be->loc.host);
    call(FILEMakeDirP, path8cgIn(be->repo_pp));

    a_path(dotbe_path, "");
    call(path8gDup, path8gIn(dotbe_path), worktree);
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(dotbe_path), dotbe);
    call(BEWriteFile, path8cgIn(dotbe_path), be_uri);

    call(BEOpenDB, &be->db, path8cgIn(be->repo_pp));
    call(BEScratchInit, be);
    done;
}

ok64 BEOpen(BEp be, path8cg worktree) {
    sane(be != NULL && worktree != NULL);
    memset(be, 0, sizeof(BE));

    call(path8bAlloc, be->work_pp);
    call(BEFindDotBe, path8gIn(be->work_pp), worktree);

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
    call(BEParseBranches, be);

    call(path8bAlloc, be->repo_pp);
    test($ok(be->loc.host) && !$empty(be->loc.host), BEBAD);
    call(BERepoPath, path8gIn(be->repo_pp), be->loc.host);

    call(BEOpenDB, &be->db, path8cgIn(be->repo_pp));
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

ok64 BEAddBranch(BEp be, u8cs branch) {
    sane(be != NULL && $ok(branch) && !$empty(branch));
    // Check not already present
    if (BEBranchVisible(be, branch)) done;
    test(be->branchc < BE_MAX_BRANCHES, BEBAD);
    // Copy branch name into loc_buf (append after existing URI data)
    size_t blen = $len(branch);
    u8cp loc_end = be->loc.data[1];
    size_t used = loc_end - be->loc_buf;
    test(used + 1 + blen < sizeof(be->loc_buf), BEBAD);
    memcpy(be->loc_buf + used, branch[0], blen);
    be->branches[be->branchc][0] = be->loc_buf + used;
    be->branches[be->branchc][1] = be->loc_buf + used + blen;
    be->branchc++;
    call(BERewriteDotBe, be);
    done;
}

ok64 BERemoveBranch(BEp be, u8cs branch) {
    sane(be != NULL && $ok(branch) && !$empty(branch));
    for (int i = 0; i < be->branchc; i++) {
        if ($eq(be->branches[i], branch)) {
            // Shift remaining
            for (int j = i; j + 1 < be->branchc; j++) {
                be->branches[j][0] = be->branches[j + 1][0];
                be->branches[j][1] = be->branches[j + 1][1];
            }
            be->branchc--;
            call(BERewriteDotBe, be);
            done;
        }
    }
    done;
}

ok64 BESetActive(BEp be, u8cs branch) {
    sane(be != NULL && $ok(branch) && !$empty(branch));
    for (int i = 0; i < be->branchc; i++) {
        if ($eq(be->branches[i], branch)) {
            // Swap to position 0
            if (i != 0) {
                u8cs tmp = {be->branches[0][0], be->branches[0][1]};
                $mv(be->branches[0], be->branches[i]);
                $mv(be->branches[i], tmp);
            }
            done;
        }
    }
    // Not in list; add it first, then swap to 0
    call(BEAddBranch, be, branch);
    int last = be->branchc - 1;
    if (last != 0) {
        u8cs tmp = {be->branches[0][0], be->branches[0][1]};
        $mv(be->branches[0], be->branches[last]);
        $mv(be->branches[last], tmp);
    }
    done;
}

// ---- GET (repo -> worktree) ----

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

// Export BASON value (with meta prefix) to a file in worktree
static ok64 BEExportFile(BEp be, u8cs relpath, u8cs metabason) {
    sane(be != NULL && $ok(relpath) && $ok(metabason));
    test($len(metabason) >= BE_META_SIZE, BEBAD);

    // Unpack metadata
    BEmeta meta = {};
    u8cs mslice = {metabason[0], metabason[1]};
    call(BEMetaUnpack, &meta, mslice);
    u8cs bason = {metabason[0] + BE_META_SIZE, metabason[1]};

    u8bp out = be->scratch[BE_RENDER];
    u8bReset(out);
    aBpad(u64, stk, 256);
    call(BASTExport, u8bIdle(out), stk, bason);
    u8cs source = {out[1], out[2]};

    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(be->work_pp));
    // relpath may contain '/' (e.g. "abc/01.h"), use path8gAdd
    a_path(rpath, "");
    call(u8sFeed, u8bIdle(rpath), relpath);
    call(path8gTerm, path8gIn(rpath));
    call(path8gAdd, path8gIn(fpath), path8cgIn(rpath));

    u8cs dir = {};
    path8gDir(dir, path8cgIn(fpath));
    if (!$empty(dir)) {
        a_path(dpath, "");
        call(u8sFeed, u8bIdle(dpath), dir);
        call(path8gTerm, path8gIn(dpath));
        FILEMakeDirP(path8cgIn(dpath));
    }
    call(BEWriteFile, path8cgIn(fpath), source);

    // Restore file metadata (path is 0-terminated by path8gPush)
    u32 mode = meta.modeftype >> 20;
    if (mode != 0) {
        chmod((const char *)fpath[1], mode);
    }
    if (meta.mtime != 0) {
        struct utimbuf ut;
        ut.actime = time(NULL);
        ut.modtime = meta.mtime;
        utime((const char *)fpath[1], &ut);
    }
    done;
}

// GET single file: read base + scan waypoints + merge
static ok64 BEGetFileMerged(BEp be, u8cs relpath, u8bp result) {
    sane(be != NULL);

    // 1. Build file path component (shared by base key and prefix)
    u8 fpbuf[256];
    u8s fps = {fpbuf, fpbuf + sizeof(fpbuf)};
    call(u8sFeed, fps, be->loc.path);
    u8sFeed1(fps, '/');
    if ($ok(relpath) && !$empty(relpath)) call(u8sFeed, fps, relpath);
    u8cs fpath = {fpbuf, fps[0]};

    // Read base (path-only URI)
    uri bu = {};
    u8csMv(bu.path, fpath);
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    call(URIutf8Feed, key, &bu);
    u8cs base_key = {kbuf, key[0]};

    u8bp bbuf = be->scratch[BE_READ];
    u8bReset(bbuf);
    ok64 go = ROCKGet(&be->db, bbuf, base_key);
    b8 has_base = (go == OK);

    // 2. Prefix scan for waypoints (path + '?' delimiter)
    uri pu = {};
    u8csMv(pu.path, fpath);
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(URIutf8Feed, pfx, &pu);
    u8sFeed1(pfx, '?');
    u8cs prefix = {pfxbuf, pfx[0]};

    // Collect matching waypoint values into a staging buffer
    // (iterator vals are only valid until next iter op)
    u8bp wpbuf = be->scratch[BE_RENDER];
    u8bReset(wpbuf);
    u8cs waypoints[256];
    int wpc = 0;

    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be->db);
    call(ROCKIterSeek, &it, prefix);
    while (ROCKIterValid(&it) && wpc < 256) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(prefix) ||
            memcmp(k[0], prefix[0], $len(prefix)) != 0)
            break;

        u8cs branch = {};
        ok64 o = BEKeyBranchSuffix(branch, k);
        if (o == OK && BEBranchVisible(be, branch)) {
            u8cs v = {};
            ROCKIterVal(&it, v);
            u8cp start = wpbuf[2];  // current idle start
            o = u8sFeed(u8bIdle(wpbuf), v);
            if (o == OK) {
                waypoints[wpc][0] = start;
                waypoints[wpc][1] = wpbuf[2];
                wpc++;
            }
        }
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);

    if (!has_base && wpc == 0) fail(BEnone);

    // Track latest metadata (last waypoint wins, or base if no waypoints)
    BEmeta latest_meta = {};
    b8 has_meta = NO;

    // Strip meta prefix from base
    u8cs base_bason = {};
    if (has_base) {
        u8cs base_raw = {bbuf[1], bbuf[2]};
        if ($len(base_raw) >= BE_META_SIZE) {
            u8cs mslice = {base_raw[0], base_raw[1]};
            BEMetaUnpack(&latest_meta, mslice);
            has_meta = YES;
            base_bason[0] = base_raw[0] + BE_META_SIZE;
            base_bason[1] = base_raw[1];
        } else {
            base_bason[0] = base_raw[0];
            base_bason[1] = base_raw[1];
        }
    }

    // Strip meta prefix from waypoints
    u8cs wp_bason[256];
    for (int i = 0; i < wpc; i++) {
        if ($len(waypoints[i]) >= BE_META_SIZE) {
            u8cs mslice = {waypoints[i][0], waypoints[i][1]};
            BEMetaUnpack(&latest_meta, mslice);
            has_meta = YES;
            wp_bason[i][0] = waypoints[i][0] + BE_META_SIZE;
            wp_bason[i][1] = waypoints[i][1];
        } else {
            wp_bason[i][0] = waypoints[i][0];
            wp_bason[i][1] = waypoints[i][1];
        }
    }

    // 3. Merge: base + waypoints (timestamp-ordered, later wins)
    // Prepend latest metadata to result
    if (has_meta) {
        call(BEMetaPack, u8bIdle(result), latest_meta);
    } else {
        // Zero metadata
        BEmeta zero = {};
        call(BEMetaPack, u8bIdle(result), zero);
    }

    if (wpc == 0) {
        // No waypoints, just return base bason after meta
        call(u8sFeed, u8bIdle(result), base_bason);
        done;
    }

    // Build inputs array: [base, wp0, wp1, ...]
    int total = (has_base ? 1 : 0) + wpc;
    u8cs inputs[total];
    int idx = 0;
    if (has_base) {
        inputs[idx][0] = base_bason[0];
        inputs[idx][1] = base_bason[1];
        idx++;
    }
    for (int i = 0; i < wpc; i++) {
        inputs[idx][0] = wp_bason[i][0];
        inputs[idx][1] = wp_bason[i][1];
        idx++;
    }

    u8css in_css = {inputs, inputs + total};
    call(BASONMergeN, result, NULL, in_css);
    done;
}

static ok64 BEGetFile(BEp be, u8cs relpath) {
    sane(be != NULL);

    u8bp mbuf = be->scratch[BE_PATCH];
    u8bReset(mbuf);

    ok64 go = BEGetFileMerged(be, relpath, mbuf);
    if (go == BEnone) return BEnone;
    if (go != OK) {
        fprintf(stderr, "MERGED %.*s\n", (int)$len(relpath), relpath[0]);
        return go;
    }

    u8cs merged = {mbuf[1], mbuf[2]};
    // Empty or meta-only = tombstone, file is deleted
    if ($len(merged) <= BE_META_SIZE) return BEnone;

    ok64 eo = BEExportFile(be, relpath, merged);
    if (eo != OK) {
        fprintf(stderr, "EXPORT %.*s\n", (int)$len(relpath), relpath[0]);
        return eo;
    }
    done;
}

// Build project prefix "project/" into a key buffer, return as slice
static ok64 BEProjectPrefix(u8csp out, u8s buf, u8cs project) {
    sane(out != NULL && $ok(buf) && $ok(project));
    call(u8sFeed, buf, project);
    u8sFeed1(buf, '/');
    out[0] = buf[1];
    out[1] = buf[0];
    done;
}

// First pass: collect unique file relpaths
typedef struct {
    u8bp pathbuf;       // buffer to store relpath strings
    u8cs *paths;        // array of relpath slices
    int count;
    int max;
    size_t plen;        // length of project path
} BEGetProjectCtx;

static ok64 BEGetProjectCB(voidp arg, u8cs key, u8cs val) {
    BEGetProjectCtx *ctx = (BEGetProjectCtx *)arg;
    uri ku = {};
    if (URIutf8Drain(key, &ku) != OK) return OK;
    // Skip metadata keys (have fragment)
    if (!$empty(ku.fragment)) return OK;
    // Extract relpath: strip "project/" prefix from path
    size_t plen = ctx->plen;
    if ($len(ku.path) <= plen + 1) return OK;
    u8cs rest = {ku.path[0] + plen + 1, ku.path[1]};
    if ($empty(rest)) return OK;
    // Keys are sorted, so same relpath groups together — skip dups
    if (ctx->count > 0 && $eq(ctx->paths[ctx->count - 1], rest))
        return OK;
    if (ctx->count >= ctx->max) return OK;
    // Copy relpath into persistent buffer (iter keys are transient)
    u8cp start = ctx->pathbuf[2];
    ok64 o = u8sFeed(u8bIdle(ctx->pathbuf), rest);
    if (o != OK) return OK;
    ctx->paths[ctx->count][0] = start;
    ctx->paths[ctx->count][1] = ctx->pathbuf[2];
    ctx->count++;
    return OK;
}

static ok64 BEGetProject(BEp be, u8cs project) {
    sane(be != NULL && $ok(project));
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, project);
    u8sFeed1(pfx, '/');
    u8cs prefix = {pfxbuf, pfx[0]};
    // Use scratch buffer to store collected relpaths
    u8bp sbuf = be->scratch[BE_WRAP];
    u8bReset(sbuf);
    int maxfiles = 8192;
    u8cs *filepaths = calloc(maxfiles, sizeof(u8cs));
    test(filepaths != NULL, BEFAIL);
    BEGetProjectCtx ctx = {sbuf, filepaths, 0, maxfiles, $len(project)};
    ok64 o = ROCKScan(&be->db, prefix, BEGetProjectCB, &ctx);
    if (o != OK) {
        free(filepaths);
        fail(o);
    }
    // Second pass: export each file
    for (int i = 0; i < ctx.count; i++) {
        ok64 fo = BEGetFile(be, filepaths[i]);
        if (fo != OK) {
            u8 errbuf[12] = {};
            u8s es = {errbuf, errbuf + 11};
            RONutf8sFeed(es, fo);
            fprintf(stderr, "FAIL %.*s (%.*s)\n",
                    (int)$len(filepaths[i]), filepaths[i][0],
                    (int)(es[0] - errbuf), errbuf);
        }
    }
    free(filepaths);
    done;
}

ok64 BEGet(BEp be, int pathc, u8cs *paths, u8cs branch) {
    sane(be != NULL);

    // If branch specified, set it as active
    if ($ok(branch) && !$empty(branch)) {
        call(BESetActive, be, branch);
    }

    if (pathc > 0 && paths != NULL) {
        for (int i = 0; i < pathc; i++) {
            call(BEGetFile, be, paths[i]);
        }
    } else {
        call(BEGetProject, be, be->loc.path);
    }
    done;
}

// ---- POST (worktree -> repo) ----

typedef struct {
    BEp be;
    ROCKbatchp wb;
    ron60 stamp;
    ignop ig;
} BEPostCtx;

static ok64 BEPostFile(BEp be, ROCKbatchp wb, ron60 stamp, path8cg filepath) {
    sane(be != NULL && wb != NULL);
    // Compute relative path from worktree root
    a_path(relpath, "");
    call(path8gRelative, path8gIn(relpath), path8cgIn(be->work_pp), filepath);
    u8cs rel = {relpath[1], relpath[2]};

    // Stat source file for metadata
    struct stat fst;
    call(FILEStat, &fst, filepath);

    // Get extension for parser
    u8cs basename = {};
    path8gBase(basename, filepath);
    u8cs ext = {};
    BEExtOf(ext, basename);

    // Build metadata
    BEmeta meta = {};
    BEMetaFromStat(&meta, &fst, ext);

    // Parse to BASON
    u8bp nbuf = be->scratch[BE_PARSE];
    u8bReset(nbuf);
    u8bp mapbuf = NULL;
    ok64 o = OK;

    if (fst.st_size == 0) {
        // Empty file: store as empty container
        u8cs nokey = {(u8cp)"", (u8cp)""};
        call(BASONFeedInto, NULL, nbuf, 'A', nokey);
        call(BASONFeedOuto, NULL, nbuf);
    } else {
        call(FILEMapRO, &mapbuf, filepath);
        u8cs source = {mapbuf[1], mapbuf[2]};
        o = BASTParse(nbuf, NULL, source, ext);
        if (o != OK) {
            u8 errbuf[12] = {};
            u8s es = {errbuf, errbuf + 11};
            RONutf8sFeed(es, o);
            fprintf(stderr, "SKIP %.*s (%.*s)\n",
                    (int)$len(rel), rel[0],
                    (int)(es[0] - errbuf), errbuf);
            FILEUnMap(mapbuf);
            done;
        }
    }
    u8cp nb0 = nbuf[1], nb1 = nbuf[2];
    u8cs new_bason = {nb0, nb1};

    // Get current merged state (base + visible waypoints)
    u8bp obuf = be->scratch[BE_READ];
    u8bReset(obuf);

    // We need a separate merge buffer since BE_READ will be used inside
    u8bp pbuf = be->scratch[BE_PATCH];
    u8bReset(pbuf);

    ok64 go = BEGetFileMerged(be, rel, pbuf);
    u8cs old_merged = {};
    if (go == OK) {
        u8cp md0 = pbuf[1], md1 = pbuf[2];
        old_merged[0] = md0;
        old_merged[1] = md1;
    }

    // Strip metadata prefix from old merged for diff
    u8cs old_bason = {};
    if ($ok(old_merged) && $len(old_merged) > BE_META_SIZE) {
        old_bason[0] = old_merged[0] + BE_META_SIZE;
        old_bason[1] = old_merged[1];
    }

    // Build waypoint key via URI API
    u8cs active = {be->branches[0][0],
                   be->branches[0][1]};
    u8 wpbuf[256];
    u8s wps = {wpbuf, wpbuf + sizeof(wpbuf)};
    call(u8sFeed, wps, be->loc.path);
    u8sFeed1(wps, '/');
    if ($ok(rel) && !$empty(rel)) call(u8sFeed, wps, rel);
    u8 wqbuf[128];
    u8s wqs = {wqbuf, wqbuf + sizeof(wqbuf)};
    call(BEQueryBuild, wqs, stamp, active);
    uri wu = {};
    u8cs wpath = {wpbuf, wps[0]};
    u8cs wquery = {wqbuf, wqs[0]};
    u8csMv(wu.path, wpath);
    u8csMv(wu.query, wquery);
    u8 wkbuf[512];
    u8s wkey = {wkbuf, wkbuf + sizeof(wkbuf)};
    call(URIutf8Feed, wkey, &wu);
    u8cs wp_key = {wkbuf, wkey[0]};

    if ($ok(old_bason) && !$empty(old_bason)) {
        // Diff old merged vs new parsed
        u8bReset(obuf);  // reuse for diff output
        aBpad(u64, ostk, 256);
        aBpad(u64, nstk, 256);
        o = BASONDiff(obuf, NULL, ostk, old_bason, nstk, new_bason);
        if (o != OK) {
            if (mapbuf) FILEUnMap(mapbuf);
            fail(o);
        }
        u8cs delta = {obuf[1], obuf[2]};
        if ($empty(delta)) {
            // No change
            if (mapbuf) FILEUnMap(mapbuf);
            done;
        }
        // Build [meta][delta] in wrap buffer
        u8bp sbuf = be->scratch[BE_WRAP];
        u8bReset(sbuf);
        call(BEMetaPack, u8bIdle(sbuf), meta);
        call(u8sFeed, u8bIdle(sbuf), delta);
        u8cs wp_val = {sbuf[1], sbuf[2]};
        call(ROCKBatchPut, wb, wp_key, wp_val);
    } else {
        // No existing state; store [meta][full BASON] as waypoint
        u8bp sbuf = be->scratch[BE_WRAP];
        u8bReset(sbuf);
        call(BEMetaPack, u8bIdle(sbuf), meta);
        call(u8sFeed, u8bIdle(sbuf), new_bason);
        u8cs wp_val = {sbuf[1], sbuf[2]};
        call(ROCKBatchPut, wb, wp_key, wp_val);
    }

    if (mapbuf) FILEUnMap(mapbuf);
    done;
}

static ok64 BEPostScanCB(voidp arg, path8p path) {
    sane(arg != NULL);
    BEPostCtx *ctx = (BEPostCtx *)arg;
    struct stat st;
    if (lstat((const char *)*path, &st) == 0 && S_ISDIR(st.st_mode)) {
        u8cs basename = {};
        path8gBase(basename, path8cgIn(path));
        // Always skip dot-directories
        if (!$empty(basename) && basename[0][0] == '.') return FILEskip;
        // Check .gitignore
        if (ctx->ig) {
            a_path(relpath, "");
            call(path8gRelative, path8gIn(relpath), path8cgIn(ctx->be->work_pp),
                 path8cgIn(path));
            u8cs rel = {relpath[1], relpath[2]};
            if (IGNOMatch(ctx->ig, rel, YES)) return FILEskip;
        }
        return OK;
    }
    // Check .gitignore for files
    if (ctx->ig) {
        a_path(relpath, "");
        ok64 ro = path8gRelative(path8gIn(relpath), path8cgIn(ctx->be->work_pp),
                                  path8cgIn(path));
        if (ro == OK) {
            u8cs rel = {relpath[1], relpath[2]};
            if (IGNOMatch(ctx->ig, rel, NO)) return OK;
        }
    }
    ok64 po = BEPostFile(ctx->be, ctx->wb, ctx->stamp, path8cgIn(path));
    if (po != OK) {
        a_path(relpath, "");
        path8gRelative(path8gIn(relpath), path8cgIn(ctx->be->work_pp),
                        path8cgIn(path));
        u8cs rel = {relpath[1], relpath[2]};
        u8 errbuf[12] = {};
        u8s es = {errbuf, errbuf + 11};
        RONutf8sFeed(es, po);
        fprintf(stderr, "FAIL %.*s (%.*s)\n",
                (int)$len(rel), rel[0],
                (int)(es[0] - errbuf), errbuf);
    }
    return OK;
}

ok64 BEPost(BEp be, int pathc, u8cs *paths, u8cs message) {
    sane(be != NULL);
    ron60 stamp = RONNow();

    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    if (pathc > 0 && paths != NULL) {
        for (int i = 0; i < pathc; i++) {
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
        igno gi = {};
        u8cs workdir = {be->work_pp[1], be->work_pp[2]};
        IGNOLoad(&gi, workdir);
        BEPostCtx ctx = {be, &wb, stamp, &gi};
        a_path(spath, "");
        call(path8gDup, path8gIn(spath), path8cgIn(be->work_pp));
        ok64 o = FILEScan(spath,
                          (FILE_SCAN)(FILE_SCAN_FILES | FILE_SCAN_DIRS |
                                      FILE_SCAN_DEEP),
                          BEPostScanCB, &ctx);
        IGNOFree(&gi);
        if (o != OK) {
            ROCKBatchClose(&wb);
            fail(o);
        }
    }

    // Store commit metadata via URI: project/?stamp-branch#commit
    u8cs active = {be->branches[0][0],
                   be->branches[0][1]};
    u8 cpbuf[256];
    u8s cps = {cpbuf, cpbuf + sizeof(cpbuf)};
    call(u8sFeed, cps, be->loc.path);
    u8sFeed1(cps, '/');
    u8 cqbuf[128];
    u8s cqs = {cqbuf, cqbuf + sizeof(cqbuf)};
    call(BEQueryBuild, cqs, stamp, active);
    a_cstr(meta_commit, "commit");
    uri cu = {};
    u8cs cp = {cpbuf, cps[0]};
    u8cs cq = {cqbuf, cqs[0]};
    u8csMv(cu.path, cp);
    u8csMv(cu.query, cq);
    u8csMv(cu.fragment, meta_commit);
    u8 ckbuf[512];
    u8s ckey = {ckbuf, ckbuf + sizeof(ckbuf)};
    call(URIutf8Feed, ckey, &cu);
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

// ---- PUT (merge source branch into active) ----

ok64 BEPut(BEp be, u8cs source_branch, u8cs message) {
    sane(be != NULL && $ok(source_branch) && !$empty(source_branch));
    ron60 stamp = RONNow();
    u8cs active = {be->branches[0][0],
                   be->branches[0][1]};

    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    // Scan all waypoint keys for this project
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, be->loc.path);
    u8sFeed1(pfx, '/');
    u8cs project_prefix = {pfxbuf, pfx[0]};

    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be->db);
    call(ROCKIterSeek, &it, project_prefix);

    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(project_prefix) ||
            memcmp(k[0], project_prefix[0], $len(project_prefix)) != 0)
            break;

        // Parse key as URI
        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        // Skip non-waypoint keys (no query) and metadata keys (has fragment)
        if (o != OK || $empty(ku.query) || !$empty(ku.fragment)) {
            call(ROCKIterNext, &it);
            continue;
        }

        // Extract branch and timestamp from query
        u8cs branch = {};
        ron60 wp_stamp = 0;
        o = BEQueryParse(&wp_stamp, branch, ku.query);
        if (o != OK || !$eq(branch, source_branch)) {
            call(ROCKIterNext, &it);
            continue;
        }

        u8cs v = {};
        ROCKIterVal(&it, v);

        // Extract file path (path minus project prefix)
        u8cs filepath = {ku.path[0] + $len(project_prefix), ku.path[1]};

        // Build new waypoint key under active branch via URI
        u8 npbuf[256];
        u8s nps = {npbuf, npbuf + sizeof(npbuf)};
        call(u8sFeed, nps, be->loc.path);
        u8sFeed1(nps, '/');
        if (!$empty(filepath)) call(u8sFeed, nps, filepath);
        u8 nqbuf[128];
        u8s nqs = {nqbuf, nqbuf + sizeof(nqbuf)};
        call(BEQueryBuild, nqs, wp_stamp, active);
        uri nu = {};
        u8cs np = {npbuf, nps[0]};
        u8cs nq = {nqbuf, nqs[0]};
        u8csMv(nu.path, np);
        u8csMv(nu.query, nq);
        u8 nkbuf[512];
        u8s nkey = {nkbuf, nkbuf + sizeof(nkbuf)};
        o = URIutf8Feed(nkey, &nu);
        if (o != OK) {
            call(ROCKIterNext, &it);
            continue;
        }
        u8cs new_key = {nkbuf, nkey[0]};
        call(ROCKBatchPut, &wb, new_key, v);

        // Delete old source branch waypoint
        call(ROCKBatchDel, &wb, k);

        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);

    // Commit metadata via URI: project/?stamp-branch#merge
    u8 mpbuf[256];
    u8s mps = {mpbuf, mpbuf + sizeof(mpbuf)};
    call(u8sFeed, mps, be->loc.path);
    u8sFeed1(mps, '/');
    u8 mqbuf[128];
    u8s mqs = {mqbuf, mqbuf + sizeof(mqbuf)};
    call(BEQueryBuild, mqs, stamp, active);
    a_cstr(meta_merge, "merge");
    uri mu = {};
    u8cs mp = {mpbuf, mps[0]};
    u8cs mq = {mqbuf, mqs[0]};
    u8csMv(mu.path, mp);
    u8csMv(mu.query, mq);
    u8csMv(mu.fragment, meta_merge);
    u8 mkbuf[512];
    u8s mkey = {mkbuf, mkbuf + sizeof(mkbuf)};
    call(URIutf8Feed, mkey, &mu);
    u8cs merge_key = {mkbuf, mkey[0]};
    if ($ok(message) && !$empty(message)) {
        call(ROCKBatchPut, &wb, merge_key, message);
    } else {
        u8cs empty = {NULL, NULL};
        call(ROCKBatchPut, &wb, merge_key, empty);
    }

    call(ROCKBatchWrite, &be->db, &wb);
    call(ROCKBatchClose, &wb);
    done;
}

// ---- DELETE ----

ok64 BEDelete(BEp be, u8cs target) {
    sane(be != NULL && $ok(target));

    // Check if target is a branch name (no path separators)
    u8cs tscan = {target[0], target[1]};
    b8 has_slash = (u8csFind(tscan, '/') == OK);

    if (!has_slash) {
        // Delete all waypoints for this branch
        u8 pfxbuf[512];
        u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
        call(u8sFeed, pfx, be->loc.path);
        u8sFeed1(pfx, '/');
        u8cs project_prefix = {pfxbuf, pfx[0]};

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

            u8cs branch = {};
            ok64 o = BEKeyBranchSuffix(branch, k);
            if (o == OK && $eq(branch, target)) {
                call(ROCKBatchDel, &wb, k);
            }
            call(ROCKIterNext, &it);
        }
        call(ROCKIterClose, &it);

        call(ROCKBatchWrite, &be->db, &wb);
        call(ROCKBatchClose, &wb);
    } else {
        // Delete file: write meta-only waypoint (tombstone)
        ron60 stamp = RONNow();
        u8cs active = {be->branches[0][0],
                   be->branches[0][1]};
        u8 dpbuf[256];
        u8s dps = {dpbuf, dpbuf + sizeof(dpbuf)};
        call(u8sFeed, dps, be->loc.path);
        u8sFeed1(dps, '/');
        call(u8sFeed, dps, target);
        u8 dqbuf[128];
        u8s dqs = {dqbuf, dqbuf + sizeof(dqbuf)};
        call(BEQueryBuild, dqs, stamp, active);
        uri du = {};
        u8cs dp = {dpbuf, dps[0]};
        u8cs dq = {dqbuf, dqs[0]};
        u8csMv(du.path, dp);
        u8csMv(du.query, dq);
        u8 wkbuf[512];
        u8s wkey = {wkbuf, wkbuf + sizeof(wkbuf)};
        call(URIutf8Feed, wkey, &du);
        u8cs wp_key = {wkbuf, wkey[0]};
        u8 mbuf[BE_META_SIZE];
        u8s ms = {mbuf, mbuf + BE_META_SIZE};
        BEmeta zero = {};
        call(BEMetaPack, ms, zero);
        u8cs tomb = {mbuf, ms[0]};
        call(ROCKPut, &be->db, wp_key, tomb);
    }
    done;
}

// ---- GET deps (.beget) ----

ok64 BEGetDeps(BEp be, b8 include_opt) {
    sane(be != NULL);

    a_path(bpath, "");
    call(path8gDup, path8gIn(bpath), path8cgIn(be->work_pp));
    a_cstr(beget, ".beget");
    call(path8gPush, path8gIn(bpath), beget);

    struct stat st;
    ok64 o = FILEStat(&st, path8cgIn(bpath));
    if (o != OK) done;

    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(bpath));
    u8cp m0 = mapbuf[1], m1 = mapbuf[2];
    u8cs content = {m0, m1};

    int section = 0;
    u8cp pos = content[0];

    while (pos < content[1]) {
        u8cp line_end = pos;
        while (line_end < content[1] && *line_end != '\n') line_end++;
        u8cs line = {pos, line_end};
        pos = line_end < content[1] ? line_end + 1 : line_end;

        if ($empty(line)) continue;

        u8cp lp = line[0];
        while (lp < line[1] && (*lp == ' ' || *lp == '\t')) lp++;
        if (lp >= line[1]) continue;
        if (*lp == '#') continue;

        u8cp le = line[1];
        while (le > lp && (le[-1] == ' ' || le[-1] == '\t' || le[-1] == '\r'))
            le--;
        if (le <= lp) continue;

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

        if (section == 0) continue;
        if (section == 2 && !include_opt) continue;

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

// ---- Milestone (fold main waypoints into base) ----

// Collect unique file paths that have main waypoints
typedef struct {
    u8bp pathbuf;  // staging for file path strings
    u8cs *paths;   // array of file path slices
    int *pathc;
    int maxpaths;
    u8cs main_branch;
    size_t pfxlen;
} BEMScanCtx;

static ok64 BEMScanCB(voidp arg, u8cs key, u8cs val) {
    BEMScanCtx *ctx = (BEMScanCtx *)arg;
    // Parse key as URI
    uri ku = {};
    if (URIutf8Drain(key, &ku) != OK) return OK;
    // Only waypoint keys (has query), skip metadata (has fragment)
    if ($empty(ku.query) || !$empty(ku.fragment)) return OK;
    u8cs branch = {};
    if (BEQueryParse(NULL, branch, ku.query) != OK) return OK;
    if (!$eq(branch, ctx->main_branch)) return OK;
    // Extract file path (path minus project prefix)
    u8cs filepath = {ku.path[0] + ctx->pfxlen, ku.path[1]};
    // Check if already collected
    for (int i = 0; i < *ctx->pathc; i++) {
        if ($eq(ctx->paths[i], filepath)) return OK;
    }
    if (*ctx->pathc >= ctx->maxpaths) return OK;
    u8cp s = ctx->pathbuf[2];
    ok64 o = u8sFeed(u8bIdle(ctx->pathbuf), filepath);
    if (o != OK) return OK;
    ctx->paths[*ctx->pathc][0] = s;
    ctx->paths[*ctx->pathc][1] = ctx->pathbuf[2];
    (*ctx->pathc)++;
    return OK;
}

ok64 BEMilestone(BEp be, u8cs name) {
    sane(be != NULL);

    a_cstr(main_branch, "main");
    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, be->loc.path);
    u8sFeed1(pfx, '/');
    u8cs project_prefix = {pfxbuf, pfx[0]};

    // Collect unique file paths that have main waypoints
    u8bp fpbuf = be->scratch[BE_RENDER];
    u8bReset(fpbuf);
    u8cs file_paths[1024];
    int filec = 0;

    BEMScanCtx sctx = {fpbuf, file_paths, &filec, 1024,
                        {main_branch[0], main_branch[1]},
                        $len(project_prefix)};
    call(ROCKScan, &be->db, project_prefix, BEMScanCB, &sctx);

    // For each file with main waypoints: merge and create base
    for (int f = 0; f < filec; f++) {
        u8cs filepath = {file_paths[f][0], file_paths[f][1]};

        // Build file path component (shared by base key and prefix)
        u8 ffbuf[256];
        u8s ffs = {ffbuf, ffbuf + sizeof(ffbuf)};
        call(u8sFeed, ffs, be->loc.path);
        u8sFeed1(ffs, '/');
        if (!$empty(filepath)) call(u8sFeed, ffs, filepath);
        u8cs ffpath = {ffbuf, ffs[0]};

        // Read base (path-only URI)
        uri buri = {};
        u8csMv(buri.path, ffpath);
        u8 bkbuf[512];
        u8s bkey = {bkbuf, bkbuf + sizeof(bkbuf)};
        call(URIutf8Feed, bkey, &buri);
        u8cs base_key = {bkbuf, bkey[0]};

        u8bp bbuf = be->scratch[BE_READ];
        u8bReset(bbuf);
        ok64 go = ROCKGet(&be->db, bbuf, base_key);
        b8 has_base = (go == OK);

        // Scan main waypoints (path + '?' prefix)
        uri wpuri = {};
        u8csMv(wpuri.path, ffpath);
        u8 wpfxbuf[512];
        u8s wpfx = {wpfxbuf, wpfxbuf + sizeof(wpfxbuf)};
        call(URIutf8Feed, wpfx, &wpuri);
        u8sFeed1(wpfx, '?');
        u8cs wp_prefix = {wpfxbuf, wpfx[0]};

        u8bp wvbuf = be->scratch[BE_PARSE];
        u8bReset(wvbuf);
        u8cs main_wps[256];
        u8cs main_wp_keys[256];
        int mwpc = 0;

        // Use ROCKScan-like loop but we need keys too
        ROCKiter wit = {};
        call(ROCKIterOpen, &wit, &be->db);
        call(ROCKIterSeek, &wit, wp_prefix);

        while (ROCKIterValid(&wit) && mwpc < 256) {
            u8cs wk = {};
            ROCKIterKey(&wit, wk);
            if ($len(wk) < $len(wp_prefix) ||
                memcmp(wk[0], wp_prefix[0], $len(wp_prefix)) != 0)
                break;

            u8cs branch = {};
            ok64 o = BEKeyBranchSuffix(branch, wk);
            if (o == OK && $eq(branch, main_branch)) {
                u8cs v = {};
                ROCKIterVal(&wit, v);
                u8cp vs = wvbuf[2];
                o = u8sFeed(u8bIdle(wvbuf), v);
                if (o != OK) break;
                main_wps[mwpc][0] = vs;
                main_wps[mwpc][1] = wvbuf[2];
                u8cp ks = wvbuf[2];
                o = u8sFeed(u8bIdle(wvbuf), wk);
                if (o != OK) break;
                main_wp_keys[mwpc][0] = ks;
                main_wp_keys[mwpc][1] = wvbuf[2];
                mwpc++;
            }
            call(ROCKIterNext, &wit);
        }
        call(ROCKIterClose, &wit);

        if (mwpc == 0) continue;

        // Strip 8B meta prefix, track latest metadata
        BEmeta latest_meta = {};
        u8cs base_bason = {};
        if (has_base) {
            u8cs braw = {bbuf[1], bbuf[2]};
            if ($len(braw) >= BE_META_SIZE) {
                u8cs ms = {braw[0], braw[1]};
                BEMetaUnpack(&latest_meta, ms);
                base_bason[0] = braw[0] + BE_META_SIZE;
                base_bason[1] = braw[1];
            } else {
                base_bason[0] = braw[0];
                base_bason[1] = braw[1];
            }
        }
        u8cs wp_bason[256];
        for (int w = 0; w < mwpc; w++) {
            if ($len(main_wps[w]) >= BE_META_SIZE) {
                u8cs ms = {main_wps[w][0], main_wps[w][1]};
                BEMetaUnpack(&latest_meta, ms);
                wp_bason[w][0] = main_wps[w][0] + BE_META_SIZE;
                wp_bason[w][1] = main_wps[w][1];
            } else {
                wp_bason[w][0] = main_wps[w][0];
                wp_bason[w][1] = main_wps[w][1];
            }
        }

        // Merge: base + main waypoints (BASON only, no meta)
        int total = (has_base ? 1 : 0) + mwpc;
        u8cs inputs[total];
        int idx = 0;
        if (has_base) {
            inputs[idx][0] = base_bason[0];
            inputs[idx][1] = base_bason[1];
            idx++;
        }
        for (int w = 0; w < mwpc; w++) {
            inputs[idx][0] = wp_bason[w][0];
            inputs[idx][1] = wp_bason[w][1];
            idx++;
        }

        u8bp mbuf = be->scratch[BE_PATCH];
        u8bReset(mbuf);

        // Prepend latest metadata to new base
        call(BEMetaPack, u8bIdle(mbuf), latest_meta);

        u8css in_css = {inputs, inputs + total};
        call(BASONMergeN, mbuf, NULL, in_css);
        u8cs merged = {mbuf[1], mbuf[2]};

        // Write new base (meta + merged BASON)
        call(ROCKBatchPut, &wb, base_key, merged);

        // Delete folded main waypoints
        for (int w = 0; w < mwpc; w++) {
            call(ROCKBatchDel, &wb, main_wp_keys[w]);
        }
    }

    // Record milestone metadata via URI: project/?stamp-branch#milestone
    if ($ok(name) && !$empty(name)) {
        ron60 stamp = RONNow();
        u8 mspbuf[256];
        u8s msps = {mspbuf, mspbuf + sizeof(mspbuf)};
        call(u8sFeed, msps, be->loc.path);
        u8sFeed1(msps, '/');
        u8 msqbuf[128];
        u8s msqs = {msqbuf, msqbuf + sizeof(msqbuf)};
        call(BEQueryBuild, msqs, stamp, main_branch);
        a_cstr(meta_milestone, "milestone");
        uri msu = {};
        u8cs msp = {mspbuf, msps[0]};
        u8cs msq = {msqbuf, msqs[0]};
        u8csMv(msu.path, msp);
        u8csMv(msu.query, msq);
        u8csMv(msu.fragment, meta_milestone);
        u8 mkbuf[512];
        u8s mkey = {mkbuf, mkbuf + sizeof(mkbuf)};
        call(URIutf8Feed, mkey, &msu);
        u8cs ms_key = {mkbuf, mkey[0]};
        call(ROCKBatchPut, &wb, ms_key, name);
    }

    call(ROCKBatchWrite, &be->db, &wb);
    call(ROCKBatchClose, &wb);
    done;
}
