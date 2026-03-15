#include "BE.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <utime.h>

#include "abc/ANSI.h"
#include "ast/HILI.h"
#include "abc/POL.h"
#include "abc/PRO.h"
#include "IGNO.h"

// ---- File metadata helpers (BASON-based) ----

ok64 BEMetaFromStat(BEmeta *m, struct stat *st, u8cs ext) {
    sane(m != NULL && st != NULL);
    m->mtime = (u32)st->st_mtime;
    u32 mode = st->st_mode & 07777;
    u32 ftype = BASTFtype(ext);
    m->modeftype = (mode << 20) | ((ftype & 0x3FFFF) << 2);
    done;
}

// Build BASON object { "m": <mtime>, "f": <modeftype> }
ok64 BEMetaFeedBason(u8bp buf, u64bp idx, BEmeta m) {
    sane(buf != NULL);
    u8cs nokey = {(u8cp)"", (u8cp)""};
    call(BASONFeedInto, idx, buf, 'O', nokey);
    // mtime
    u8cs mk = {(u8cp)"m", (u8cp)"m" + 1};
    u8 mbytes[4];
    mbytes[0] = m.mtime & 0xff;
    mbytes[1] = (m.mtime >> 8) & 0xff;
    mbytes[2] = (m.mtime >> 16) & 0xff;
    mbytes[3] = (m.mtime >> 24) & 0xff;
    u8cs mv = {mbytes, mbytes + 4};
    call(BASONFeed, idx, buf, 'N', mk, mv);
    // modeftype
    u8cs fk = {(u8cp)"f", (u8cp)"f" + 1};
    u8 fbytes[4];
    fbytes[0] = m.modeftype & 0xff;
    fbytes[1] = (m.modeftype >> 8) & 0xff;
    fbytes[2] = (m.modeftype >> 16) & 0xff;
    fbytes[3] = (m.modeftype >> 24) & 0xff;
    u8cs fv = {fbytes, fbytes + 4};
    call(BASONFeed, idx, buf, 'N', fk, fv);
    call(BASONFeedOuto, idx, buf);
    done;
}

// Drain BASON metadata object into BEmeta
ok64 BEMetaDrainBason(BEmeta *m, u8cs bason) {
    sane(m != NULL && $ok(bason));
    m->mtime = 0;
    m->modeftype = 0;
    if ($empty(bason)) done;
    aBpad(u64, stk, 32);
    call(BASONOpen, stk, bason);
    // First drain: expect Object container
    u8 type = 0;
    u8cs key = {};
    u8cs val = {};
    test(BASONDrain(stk, bason, &type, key, val) == OK, BEBAD);
    test(type == 'O', BEBAD);
    call(BASONInto, stk, bason, val);
    // Drain children
    while (BASONDrain(stk, bason, &type, key, val) == OK) {
        if (type == 'N' && $len(val) == 4) {
            u32 v = val[0][0] | ((u32)val[0][1] << 8) |
                    ((u32)val[0][2] << 16) | ((u32)val[0][3] << 24);
            if ($len(key) == 1 && key[0][0] == 'm') {
                m->mtime = v;
            } else if ($len(key) == 1 && key[0][0] == 'f') {
                m->modeftype = v;
            }
        }
    }
    call(BASONOuto, stk);
    done;
}

// ---- Key builder ----

ok64 BEKeyBuild(u8s into, u8cs scheme, u8cs path, u8cs query, u8cs fragment) {
    sane($ok(into));
    call(URIMake, into, scheme, 0, path, query, fragment);
    done;
}

// ---- Query sub-structure helpers ----

ok64 BEQueryBuild(u8s into, ron60 stamp, ron60 branch) {
    sane($ok(into));
    call(RONu8sFeedPad, into, stamp, 10);
    into[0] += 10;
    u8sFeed1(into, '-');
    if (branch != 0) {
        call(RONutf8sFeed, into, branch);
    }
    done;
}

// Forward declaration: trigram POST helper
static ok64 BETriPost(BEp be, ROCKbatchp wb, u8cs bason, u8cs fpath);
// Forward declaration: symbol index POST helper
static ok64 BESymPost(BEp be, ROCKbatchp wb, u8cs bason, u8cs fpath);

// ---- Key parsers ----

ok64 BEKeyBranch(ron60 *branch, u8cs key) {
    sane(branch != NULL && $ok(key));
    uri u = {};
    call(URIutf8Drain, key, &u);
    if ($empty(u.query)) fail(BEnone);
    ron120 ver = {};
    call(VERParse, &ver, u.query);
    *branch = VEROrigin(&ver);
    done;
}

ok64 BEKeyStamp(ron60 *stamp, u8cs key) {
    sane(stamp != NULL && $ok(key));
    uri u = {};
    call(URIutf8Drain, key, &u);
    if ($empty(u.query)) fail(BEnone);
    ron120 ver = {};
    call(VERParse, &ver, u.query);
    *stamp = VERTime(&ver);
    done;
}

// ---- BASTExport: flatten BASON tree to source text ----

static ok64 BASTExportRec(u8s out, u64bp stack, u8csc data) {
    sane(1);
    u8 type = 0;
    u8cs key = {};
    u8cs val = {};
    while (BASONDrain(stack, data, &type, key, val) == OK) {
        if (!BASONPlex(type)) {
            call(u8sFeed, out, val);
        } else {
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

// ---- BASTCat: syntax-highlighted export ----

static ok64 BASTCatRec(u8s out, u64bp stack, u8csc data,
                        u8 *cstk, int depth) {
    sane(1);
    u8 type = 0;
    u8cs key = {};
    u8cs val = {};
    while (BASONDrain(stack, data, &type, key, val) == OK) {
        if (!BASONPlex(type)) {
            b8 styled = HILILeaf(out, type);
            call(u8sFeed, out, val);
            if (styled) HILIRestore(out, cstk, depth);
        } else {
            int d = depth < HILI_MAXDEPTH ? depth : HILI_MAXDEPTH - 1;
            cstk[d] = type;
            HILIContainer(out, type);
            call(BASONInto, stack, data, val);
            call(BASTCatRec, out, stack, data, cstk, depth + 1);
            call(BASONOuto, stack);
            HILIRestore(out, cstk, depth);
        }
    }
    done;
}

ok64 BASTCat(u8s out, u64bp stack, u8csc data) {
    sane($ok(out) && stack != NULL && $ok(data));
    if ($empty(data)) done;
    call(BASONOpen, stack, data);
    u8 cstk[HILI_MAXDEPTH];
    memset(cstk, 0, sizeof(cstk));
    call(BASTCatRec, out, stack, data, cstk, 0);
    done;
}

// ---- Internal helpers ----

// Forward declaration
static void BEExtOf(u8csp ext, u8cs filename);

// Build "project/relpath" into a slice
static ok64 BEProjPath(u8s into, u8csc project, u8csc relpath) {
    sane($ok(into));
    call(u8sFeed, into, project);
    u8sFeed1(into, '/');
    if ($ok(relpath) && !$empty(relpath)) call(u8sFeed, into, relpath);
    done;
}

// Flush rocksdb memtable
static void BEFlushDB(BEp be) {
    rocksdb_flushoptions_t *fopt = rocksdb_flushoptions_create();
    if (fopt) {
        char *ferr = NULL;
        rocksdb_flush(be->db.db, fopt, &ferr);
        if (ferr) free(ferr);
        rocksdb_flushoptions_destroy(fopt);
    }
}

// Batch write + clear (no per-file flush; let RocksDB manage compaction)
static ok64 BEBatchFlush(BEp be, ROCKbatchp wb) {
    sane(be != NULL && wb != NULL);
    call(ROCKBatchWrite, &be->db, wb);
    rocksdb_writebatch_clear(wb->wb);
    done;
}

// Build worktree path from relpath
static ok64 BEWorkPath(path8g out, BEp be, u8cs relpath) {
    sane(out != NULL && be != NULL);
    call(path8gDup, out, path8cgIn(be->work_pp));
    a_path(rp, "");
    call(u8sFeed, u8bIdle(rp), relpath);
    call(path8gTerm, path8gIn(rp));
    call(path8gAdd, out, path8cgIn(rp));
    done;
}

// Get ext + codec from a filename/relpath
static void BEFileInfo(u8csp ext, u8csp codec, u8cs relpath) {
    u8cs basename = {};
    path8gBase(basename, (path8cg){relpath[0], relpath[1], relpath[1]});
    BEExtOf(ext, basename);
    BASTCodec(codec, ext);
}

// Check if a key's query matches branch formula
static b8 BEWaypointMatch(ron120cs formcs, u8cs query) {
    ron120 ver = {};
    if (VERParse(&ver, query) != OK) return NO;
    return VERFormMatch(formcs, VERTime(&ver), VEROrigin(&ver));
}

// Parse file to BASON (empty file → empty array, else BASTParse)
static ok64 BEParseFile(u8bp nbuf, u8bp *mapbuf, path8cg filepath,
                         struct stat *fst, u8cs ext) {
    sane(nbuf != NULL && fst != NULL);
    u8bReset(nbuf);
    *mapbuf = NULL;
    if (fst->st_size == 0) {
        u8cs nokey = {(u8cp)"", (u8cp)""};
        call(BASONFeedInto, NULL, nbuf, 'A', nokey);
        call(BASONFeedOuto, NULL, nbuf);
    } else {
        call(FILEMapRO, mapbuf, filepath);
        u8cs source = {(*mapbuf)[1], (*mapbuf)[2]};
        call(BASTParse, nbuf, NULL, source, ext);
    }
    done;
}

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
    call(ROCKSetMerge, db, BASONMergeY);
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
    rocksdb_options_set_write_buffer_size(db->opt, 16 * MB);
    rocksdb_options_set_max_write_buffer_number(db->opt, 2);
    rocksdb_writeoptions_disable_WAL(db->wopt, 1);
    if (db->cache) {
        rocksdb_cache_set_capacity(db->cache, 64 * MB);
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
        ron60 origin = 0;
        call(RONutf8sDrain, &origin, m);
        be->branches[0] = VERMake(0, origin, VER_ANY);
        be->branchc = 1;
        done;
    }
    u8cp p = be->loc.query[0];
    u8cp end = be->loc.query[1];
    while (p < end && be->branchc < BE_MAX_BRANCHES) {
        u8cp start = p;
        while (p < end && *p != '&') p++;
        if (p > start) {
            u8cs entry = {start, p};
            ron60 origin = 0;
            ok64 o = RONutf8sDrain(&origin, entry);
            if (o == OK) {
                be->branches[be->branchc] = VERMake(0, origin, VER_ANY);
                be->branchc++;
            }
        }
        if (p < end) p++;  // skip '&'
    }
    // Active branch is always #0
    done;
}

// Check if a branch origin is in the branch filter list
static b8 BEBranchVisible(BEp be, ron60 origin) {
    for (int i = 0; i < be->branchc; i++) {
        if (VEROrigin(&be->branches[i]) == origin) return YES;
    }
    return NO;
}

// ---- BEScan / BEScanChanged ----

// Internal: flush accumulated base + waypoints for one file
static ok64 BEScanFlush(BEp be, ron120cs form, u8cs relpath,
                         b8 has_base, int wpc, u8cs *waypoints,
                         BEScanCBf cb, voidp arg) {
    sane(be != NULL && cb != NULL);
    u8cs base_bason = {};
    if (has_base) {
        u8cp b0 = be->scratch[BE_READ][1];
        u8cp b1 = be->scratch[BE_READ][2];
        base_bason[0] = b0;
        base_bason[1] = b1;
    }

    // Merge base + waypoints into BE_PATCH
    u8bp mbuf = be->scratch[BE_PATCH];
    u8bReset(mbuf);

    if (wpc == 0 && has_base) {
        call(u8sFeed, u8bIdle(mbuf), base_bason);
    } else if (wpc > 0) {
        int total = (has_base ? 1 : 0) + wpc;
        u8cs inputs[total];
        int idx = 0;
        if (has_base) {
            $mv(inputs[idx], base_bason);
            idx++;
        }
        for (int w = 0; w < wpc; w++) {
            $mv(inputs[idx], waypoints[w]);
            idx++;
        }
        u8css in_css = {inputs, inputs + total};
        call(BASONMergeN, mbuf, NULL, in_css);
    } else {
        done;  // no base, no waypoints
    }

    u8cp m0 = mbuf[1], m1 = mbuf[2];
    u8cs merged = {m0, m1};
    if ($empty(merged)) done;

    // Fetch stat: metadata (base + matching waypoints)
    BEmeta meta = {};
    a_pad(u8, fp, 256);
    call(BEProjPath, fp_idle, be->loc.path, relpath);

    a_cstr(sch_stat, BE_SCHEME_STAT);
    // Read stat: base
    a_uri(stat_base, sch_stat, 0, fp_datac, 0, 0);
    u8bp sbuf = be->scratch[BE_WRAP];
    u8bReset(sbuf);
    if (ROCKGet(&be->db, sbuf, stat_base) == OK) {
        u8cp s0 = sbuf[1], s1 = sbuf[2];
        u8cs sv = {s0, s1};
        BEMetaDrainBason(&meta, sv);
    }
    // Prefix scan stat: waypoints
    a_pad(u8, spfx, 512);
    call(BEKeyBuild, spfx_idle, sch_stat, fp_datac, 0, 0);
    u8sFeed1(spfx_idle, '?');

    ROCKiter sit = {};
    call(ROCKIterOpen, &sit, &be->db);
    call(ROCKIterSeek, &sit, spfx_datac);
    while (ROCKIterValid(&sit)) {
        u8cs sk = {};
        ROCKIterKey(&sit, sk);
        if ($len(sk) < $len(spfx_datac) ||
            memcmp(sk[0], spfx_datac[0], $len(spfx_datac)) != 0)
            break;
        uri sku = {};
        ok64 o = URIutf8Drain(sk, &sku);
        if (o == OK && !$empty(sku.query) &&
            BEWaypointMatch(form, sku.query)) {
            u8cs sv = {};
            ROCKIterVal(&sit, sv);
            BEMetaDrainBason(&meta, sv);
        }
        call(ROCKIterNext, &sit);
    }
    call(ROCKIterClose, &sit);

    call(cb, arg, relpath, merged, meta);
    done;
}

// Core scan: iterate be:<path>/ prefix, accumulate per-file, flush
static ok64 BEScanCore(BEp be, uricp loc, BEScanCBf cb, voidp arg,
                         b8 changed_only) {
    sane(be != NULL && loc != NULL && cb != NULL);

    // Parse formula from loc->query (or be->branches[])
    ron120 fbuf[VER_MAX];
    ron120s form = {fbuf, fbuf + VER_MAX};
    u8cs locq = {loc->query[0], loc->query[1]};
    if ($ok(locq) && !$empty(locq)) {
        call(VERFormParse, form, locq);
    } else {
        call(VERFormFromBranches, form, be->branchc, be->branches);
    }
    ron120cs formcs = {fbuf, form[0]};

    // Build be:<path>/ prefix
    a_cstr(sch_be, BE_SCHEME_BE);
    a_pad(u8, fp, 256);
    u8cs norp = {};
    call(BEProjPath, fp_idle, loc->path, norp);
    a_pad(u8, pfx, 512);
    call(BEKeyBuild, pfx_idle, sch_be, fp_datac, 0, 0);
    size_t path_off = $len(loc->path) + 1;  // "project/"

    // Current file state
    u8 cur_rp_buf[512];
    u8s cur_rp_s = {cur_rp_buf, cur_rp_buf + sizeof(cur_rp_buf)};
    u8cs cur_relpath = {};
    b8 cur_has_base = NO;
    u8cs cur_waypoints[256];
    int cur_wpc = 0;

    u8bp readbuf = be->scratch[BE_READ];
    u8bp wpbuf = be->scratch[BE_RENDER];

    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be->db);
    call(ROCKIterSeek, &it, pfx_datac);

    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(pfx_datac) ||
            memcmp(k[0], pfx_datac[0], $len(pfx_datac)) != 0)
            break;

        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o != OK) {
            call(ROCKIterNext, &it);
            continue;
        }
        // Skip fragment keys (commit messages etc)
        if (!$empty(ku.fragment)) {
            call(ROCKIterNext, &it);
            continue;
        }

        // Extract relpath from path component
        if ($len(ku.path) <= path_off) {
            call(ROCKIterNext, &it);
            continue;
        }
        u8cs relpath = {ku.path[0] + path_off, ku.path[1]};

        // Check if relpath changed → flush previous
        if ($ok(cur_relpath) && !$empty(cur_relpath) && !$eq(relpath, cur_relpath)) {
            if (!changed_only || cur_wpc > 0) {
                ok64 fo = BEScanFlush(be, formcs, cur_relpath,
                                       cur_has_base, cur_wpc,
                                       cur_waypoints, cb, arg);
                if (fo != OK && fo != BEnone) {
                    ROCKIterClose(&it);
                    fail(fo);
                }
            }
            // Reset for new file
            cur_relpath[0] = NULL;
            cur_relpath[1] = NULL;
            cur_has_base = NO;
            cur_wpc = 0;
        }

        // Start new file if needed
        if (!$ok(cur_relpath) || $empty(cur_relpath)) {
            cur_rp_s[0] = cur_rp_buf;
            o = u8sFeed(cur_rp_s, relpath);
            if (o != OK) {
                call(ROCKIterNext, &it);
                continue;
            }
            cur_relpath[0] = cur_rp_buf;
            cur_relpath[1] = cur_rp_s[0];
            cur_has_base = NO;
            cur_wpc = 0;
            u8bReset(readbuf);
            u8bReset(wpbuf);
        }

        if ($empty(ku.query)) {
            // Base key: copy value into BE_READ
            u8bReset(readbuf);
            u8cs v = {};
            ROCKIterVal(&it, v);
            o = u8sFeed(u8bIdle(readbuf), v);
            if (o == OK) cur_has_base = YES;
        } else {
            // Waypoint key: check formula match
            if (BEWaypointMatch(formcs, ku.query) && cur_wpc < 256) {
                u8cs v = {};
                ROCKIterVal(&it, v);
                u8cp start = wpbuf[2];
                o = u8sFeed(u8bIdle(wpbuf), v);
                if (o == OK) {
                    cur_waypoints[cur_wpc][0] = start;
                    cur_waypoints[cur_wpc][1] = wpbuf[2];
                    cur_wpc++;
                }
            }
        }

        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);

    // Flush last file
    if ($ok(cur_relpath) && !$empty(cur_relpath)) {
        if (!changed_only || cur_wpc > 0) {
            call(BEScanFlush, be, formcs, cur_relpath,
                 cur_has_base, cur_wpc,
                 cur_waypoints, cb, arg);
        }
    }
    done;
}

ok64 BEScan(BEp be, uricp loc, BEScanCBf cb, voidp arg) {
    sane(be != NULL && loc != NULL && cb != NULL);
    return BEScanCore(be, loc, cb, arg, NO);
}

ok64 BEScanChanged(BEp be, uricp loc, BEScanCBf cb, voidp arg) {
    sane(be != NULL && loc != NULL && cb != NULL);
    return BEScanCore(be, loc, cb, arg, YES);
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
            call(RONutf8sFeed, into, VEROrigin(&be->branches[i]));
        }
    }
    done;
}

// Rewrite .be file from current state
static ok64 BERewriteDotBe(BEp be) {
    sane(be != NULL);
    a_pad(u8, ubuf, 512);
    call(BEBuildURI, ubuf_idle, be);
    u8bReset(be->loc_buf);
    call(u8bFeed, be->loc_buf, ubuf_datac);
    u8csMv(be->loc.data, u8bDataC(be->loc_buf));
    call(URILexer, &be->loc);
    call(BEParseBranches, be);
    // Write file
    a_path(dpath, "");
    call(path8gDup, path8gIn(dpath), path8cgIn(be->work_pp));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(dpath), dotbe);
    u8cs new_uri;
    u8csMv(new_uri, u8bDataC(be->loc_buf));
    call(BEWriteFile, path8cgIn(dpath), new_uri);
    done;
}

// ---- Lifecycle ----

ok64 BEInit(BEp be, u8cs be_uri, path8cg worktree) {
    sane(be != NULL && $ok(be_uri) && worktree != NULL);
    memset(be, 0, sizeof(BE));

    call(u8bAllocate, be->loc_buf, 512);
    call(u8bFeed, be->loc_buf, be_uri);
    u8csMv(be->loc.data, u8bDataC(be->loc_buf));
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
    be->initial = YES;
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
    call(u8bAllocate, be->loc_buf, 512);
    call(u8bFeed, be->loc_buf, mapped);
    call(FILEUnMap, mapbuf);

    u8csMv(be->loc.data, u8bDataC(be->loc_buf));
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
    u8bFree(be->loc_buf);
    memset(be, 0, sizeof(BE));
    return OK;
}

ok64 BEAddBranch(BEp be, u8cs branch) {
    sane(be != NULL && $ok(branch) && !$empty(branch));
    ron60 origin = 0;
    call(RONutf8sDrain, &origin, branch);
    // Check not already present
    if (BEBranchVisible(be, origin)) done;
    test(be->branchc < BE_MAX_BRANCHES, BEBAD);
    be->branches[be->branchc] = VERMake(0, origin, VER_ANY);
    be->branchc++;
    call(BERewriteDotBe, be);
    done;
}

ok64 BERemoveBranch(BEp be, u8cs branch) {
    sane(be != NULL && $ok(branch) && !$empty(branch));
    ron60 origin = 0;
    call(RONutf8sDrain, &origin, branch);
    for (int i = 0; i < be->branchc; i++) {
        if (VEROrigin(&be->branches[i]) == origin) {
            for (int j = i; j + 1 < be->branchc; j++)
                be->branches[j] = be->branches[j + 1];
            be->branchc--;
            call(BERewriteDotBe, be);
            done;
        }
    }
    done;
}

ok64 BESetActive(BEp be, u8cs branch) {
    sane(be != NULL && $ok(branch) && !$empty(branch));
    ron60 origin = 0;
    call(RONutf8sDrain, &origin, branch);
    for (int i = 0; i < be->branchc; i++) {
        if (VEROrigin(&be->branches[i]) == origin) {
            if (i != 0) {
                ron120 tmp = be->branches[0];
                be->branches[0] = be->branches[i];
                be->branches[i] = tmp;
            }
            done;
        }
    }
    // Not in list; add it first, then swap to 0
    call(BEAddBranch, be, branch);
    int last = be->branchc - 1;
    if (last != 0) {
        ron120 tmp = be->branches[0];
        be->branches[0] = be->branches[last];
        be->branches[last] = tmp;
    }
    done;
}

// ---- GET (repo -> worktree) ----

// Get file extension as a const slice
static void BEExtOf(u8csp ext, u8cs filename) {
    ext[0] = NULL;
    ext[1] = NULL;
    u8cp p = filename[1];
    while (p > filename[0] + 1) {
        --p;
        if (*p == '.') {
            ext[0] = p;
            ext[1] = filename[1];
            return;
        }
    }
}

// Export BASON content + metadata to a file in worktree
static ok64 BEExportFile(BEp be, u8cs relpath, u8cs bason, BEmeta meta) {
    sane(be != NULL && $ok(relpath) && $ok(bason));

    u8bp out = be->scratch[BE_RENDER];
    u8bReset(out);
    aBpad(u64, stk, 256);
    call(BASTExport, u8bIdle(out), stk, bason);
    u8cs source = {out[1], out[2]};

    if (be->to_stdout) {
        if (!$empty(source)) {
            fwrite(source[0], 1, $len(source), stdout);
        }
        done;
    }

    a_path(fpath, "");
    call(BEWorkPath, path8gIn(fpath), be, relpath);

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

// Scan base + formula-matching waypoints for a single file.
// Calls cb(arg, key, val) for each matching record.
// Returns BEnone if no records found. Non-OK from cb stops scan.
ok64 BEScanFile(ROCKdbp db, u8cs project, u8cs relpath,
                ron120cs formcs, BEFileCBf cb, voidp arg) {
    sane(db != NULL && cb != NULL);

    // Build file path component: project/relpath
    a_pad(u8, fp, 256);
    call(BEProjPath, fp_idle, project, relpath);

    // Build base key — iterator seek starts here
    a_cstr(sch_be, BE_SCHEME_BE);
    a_uri(base_key, sch_be, 0, fp_datac, 0, 0);
    b8 any = NO;

    ROCKiter it = {};
    call(ROCKIterOpen, &it, db);
    call(ROCKIterSeek, &it, base_key);
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);

        // Stop when past base_key prefix (covers base + ?waypoints)
        if ($len(k) < $len(base_key) ||
            memcmp(k[0], base_key[0], $len(base_key)) != 0)
            break;
        // Exact match or ?query — reject longer paths (BAST.c vs BAST.cli.c)
        if ($len(k) > $len(base_key) && k[0][$len(base_key)] != '?')
            break;

        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o != OK) {
            call(ROCKIterNext, &it);
            continue;
        }

        // Skip fragment keys
        if (!$empty(ku.fragment)) {
            call(ROCKIterNext, &it);
            continue;
        }

        // Extract stamp+branch; base key has stamp=0, origin=0
        ron60 stamp = 0;
        ron60 br_val = 0;
        if (!$empty(ku.query)) {
            ron120 ver = {};
            o = VERParse(&ver, ku.query);
            if (o == OK) {
                stamp = VERTime(&ver);
                br_val = VEROrigin(&ver);
            }
        }

        if (VERFormMatch(formcs, stamp, br_val)) {
            u8cs v = {};
            ROCKIterVal(&it, v);
            if (!$empty(v)) {
                any = YES;
                o = cb(arg, k, v);
                if (o != OK) {
                    ROCKIterClose(&it);
                    return o;
                }
            }
        }
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);

    if (!any) fail(BEnone);
    done;
}

// Collect callback for BEMergeFile
typedef struct {
    u8p buf[4];
    u8cs pieces[257];
    int count;
} BEMergeCtx_;

static ok64 BEMergeCB_(voidp arg, u8cs key, u8cs val) {
    BEMergeCtx_ *ctx = (BEMergeCtx_ *)arg;
    if (ctx->count >= 257) return BEFAIL;
    u8cp start = ctx->buf[2];
    ok64 o = u8sFeed(u8bIdle(ctx->buf), val);
    if (o != OK) return o;
    ctx->pieces[ctx->count][0] = start;
    ctx->pieces[ctx->count][1] = ctx->buf[2];
    ctx->count++;
    return OK;
}

// Merge file content: formula-filtered, thread-safe (allocates own buffers)
ok64 BEMergeFile(ROCKdbp db, u8cs project, u8cs relpath,
                 ron120cs formcs, u8bp result) {
    sane(db != NULL && result != NULL);

    BEMergeCtx_ ctx = {};
    call(u8bAllocate, ctx.buf, 1 << 18);

    ok64 o = BEScanFile(db, project, relpath, formcs, BEMergeCB_, &ctx);
    if (o != OK) {
        u8bFree(ctx.buf);
        return o;
    }
    if (ctx.count == 1) {
        o = u8sFeed(u8bIdle(result), ctx.pieces[0]);
        u8bFree(ctx.buf);
        return o;
    }

    u8css in_css = {ctx.pieces, ctx.pieces + ctx.count};
    o = BASONMergeN(result, NULL, in_css);
    u8bFree(ctx.buf);
    return o;
}

// GET single file: read metadata from stat:, content from be:, merge
// result receives pure BASON. meta_out receives latest metadata.
ok64 BEGetFileMerged(BEp be, u8cs project, u8cs relpath,
                     u8bp result, BEmeta *meta_out) {
    sane(be != NULL);

    // Build formula from branches
    ron120 form[VER_MAX];
    ron120s form_s = {form, form + VER_MAX};
    call(VERFormFromBranches, form_s, be->branchc, be->branches);
    ron120cs formcs = {form, form_s[0]};

    // Merge be: content
    call(BEMergeFile, &be->db, project, relpath, formcs, result);

    // Scan stat: metadata if requested
    if (meta_out) {
        a_pad(u8, fp, 256);
        call(BEProjPath, fp_idle, project, relpath);

        a_cstr(sch_stat, BE_SCHEME_STAT);
        BEmeta latest_meta = {};

        // Read stat: base key
        a_uri(stat_key, sch_stat, 0, fp_datac, 0, 0);
        u8bp sbuf = be->scratch[BE_WRAP];
        u8bReset(sbuf);
        if (ROCKGet(&be->db, sbuf, stat_key) == OK) {
            u8cp s0 = sbuf[1], s1 = sbuf[2];
            u8cs stat_val = {s0, s1};
            BEMetaDrainBason(&latest_meta, stat_val);
        }

        // Scan stat: waypoints
        a_pad(u8, spfx, 512);
        call(BEKeyBuild, spfx_idle, sch_stat, fp_datac, 0, 0);
        u8sFeed1(spfx_idle, '?');

        ROCKiter sit = {};
        call(ROCKIterOpen, &sit, &be->db);
        call(ROCKIterSeek, &sit, spfx_datac);
        while (ROCKIterValid(&sit)) {
            u8cs sk = {};
            ROCKIterKey(&sit, sk);
            if ($len(sk) < $len(spfx_datac) ||
                memcmp(sk[0], spfx_datac[0], $len(spfx_datac)) != 0)
                break;
            uri sku = {};
            ok64 o = URIutf8Drain(sk, &sku);
            if (o == OK && !$empty(sku.query)) {
                ron120 ver = {};
                o = VERParse(&ver, sku.query);
                if (o == OK && BEBranchVisible(be, VEROrigin(&ver))) {
                    u8cs sv = {};
                    ROCKIterVal(&sit, sv);
                    BEMetaDrainBason(&latest_meta, sv);
                }
            }
            call(ROCKIterNext, &sit);
        }
        call(ROCKIterClose, &sit);

        *meta_out = latest_meta;
    }
    done;
}

static void BEPostReport(u8cs rel, u8cs codec, const char *status, u8 color);

static ok64 BEGetFile(BEp be, u8cs relpath) {
    sane(be != NULL);

    u8cs ext = {};
    u8cs codec = {};
    BEFileInfo(ext, codec, relpath);

    u8bp mbuf = be->scratch[BE_PATCH];
    u8bReset(mbuf);

    BEmeta meta = {};
    ok64 go = BEGetFileMerged(be, be->loc.path, relpath, mbuf, &meta);
    if (go == BEnone) return BEnone;
    if (go != OK) {
        BEPostReport(relpath, codec, "FAIL", DARK_RED);
        return go;
    }

    u8cs merged = {mbuf[1], mbuf[2]};
    if ($empty(merged)) return BEnone;

    ok64 eo = BEExportFile(be, relpath, merged, meta);
    if (eo != OK) {
        BEPostReport(relpath, codec, "FAIL", DARK_RED);
        return eo;
    }
    BEPostReport(relpath, codec, "OK", DARK_GREEN);
    done;
}

// BEScan callback for GET: export each file to worktree
static ok64 BEExportCB(voidp arg, u8cs relpath, u8cs bason, BEmeta meta) {
    BEp be = (BEp)arg;
    u8cs ext = {};
    u8cs codec = {};
    BEFileInfo(ext, codec, relpath);
    ok64 o = BEExportFile(be, relpath, bason, meta);
    if (o == OK)
        BEPostReport(relpath, codec, "OK", DARK_GREEN);
    else
        BEPostReport(relpath, codec, "FAIL", DARK_RED);
    return o;
}

static ok64 BEGetProject(BEp be, u8cs project) {
    sane(be != NULL && $ok(project));
    uri scan_loc = be->loc;
    $mv(scan_loc.path, project);
    return BEScan(be, &scan_loc, BEExportCB, be);
}

ok64 BEGet(BEp be, int pathc, u8cs *paths, u8cs branch) {
    sane(be != NULL);

    // If branch specified, set it as active (may be "&"-separated list)
    if ($ok(branch) && !$empty(branch)) {
        // Reset branch list, parse all entries from query
        be->branchc = 0;
        u8cp p = branch[0];
        u8cp end = branch[1];
        while (p < end && be->branchc < BE_MAX_BRANCHES) {
            u8cp start = p;
            while (p < end && *p != '&') p++;
            if (p > start) {
                u8cs entry = {start, p};
                ron60 origin = 0;
                ok64 o = RONutf8sDrain(&origin, entry);
                if (o == OK) {
                    be->branches[be->branchc] = VERMake(0, origin, VER_ANY);
                    be->branchc++;
                }
            }
            if (p < end) p++;
        }
        call(BERewriteDotBe, be);
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

// ---- STATUS (compare worktree vs DB) ----

// BEStatCBf and BEScanStatMerged declared in BE.h

static ok64 BEStatusCB(voidp arg, u8cs relpath, BEmeta merged_meta) {
    sane(arg != NULL);
    BEp be = (BEp)arg;

    u8cs ext = {};
    u8cs codec = {};
    BEFileInfo(ext, codec, relpath);

    a_path(fpath, "");
    call(BEWorkPath, path8gIn(fpath), be, relpath);

    struct stat fst;
    if (FILEStat(&fst, path8cgIn(fpath)) != OK) {
        BEPostReport(relpath, codec, "DEL", DARK_RED);
    } else if ((u32)fst.st_mtime != merged_meta.mtime) {
        BEPostReport(relpath, codec, "MOD", DARK_YELLOW);
    }
    done;
}

ok64 BEStatusFiles(BEp be) {
    sane(be != NULL);
    u8cs empty_pfx = {};
    call(BEScanStatMerged, be, empty_pfx, BEStatusCB, be);
    done;
}

// ---- DIFF (worktree vs repo, colored output) ----

// Check if relpath matches one of the given filter paths
static b8 BEDiffPathMatch(u8cs relpath, int pathc, u8cs *paths) {
    if (pathc == 0 || paths == NULL) return YES;
    for (int i = 0; i < pathc; i++) {
        if ($eq(relpath, paths[i])) return YES;
    }
    return NO;
}

typedef struct {
    BEp be;
    int pathc;
    u8cs *paths;
    b8 any_output;
} BEDiffCtx;

static ok64 BEDiffCB(voidp arg, u8cs relpath, BEmeta merged_meta) {
    sane(arg != NULL);
    BEDiffCtx *ctx = (BEDiffCtx *)arg;
    BEp be = ctx->be;

    u8cs HDRESC = $u8str("\033[1m");
    a_pad(u8, _delesc, 16);
    escfeedBG256(_delesc_idle, HILI_DEL_BG);
    u8cs DELESC = {_delesc[1], _delesc[2]};
    u8cs RST = $u8str("\033[0m");
    u8cs NL = $u8str("\n");
    u8cs SEP = $u8str("\033[34m---\033[0m\n");

    // Filter by requested paths
    if (!BEDiffPathMatch(relpath, ctx->pathc, ctx->paths))
        return OK;

    // Build worktree path and stat
    a_path(fpath, "");
    call(BEWorkPath, path8gIn(fpath), be, relpath);

    struct stat fst;
    b8 deleted = NO;
    b8 modified = NO;
    if (FILEStat(&fst, path8cgIn(fpath)) != OK) {
        deleted = YES;
    } else if ((u32)fst.st_mtime != merged_meta.mtime) {
        modified = YES;
    }

    if (!deleted && !modified) return OK;

    // Get old BASON from repo (use BE_PATHS to avoid conflict
    // with BE_READ which BEGetFileMerged uses internally)
    u8bp obuf = be->scratch[BE_PATHS];
    u8bReset(obuf);
    ok64 go = BEGetFileMerged(be, be->loc.path, relpath, obuf, NULL);
    u8cs old_bason = {};
    if (go == OK) {
        u8cp ob0 = obuf[1], ob1 = obuf[2];
        old_bason[0] = ob0;
        old_bason[1] = ob1;
    }

    if (deleted) {
        // Show entire old content in red strikethrough
        if (!$empty(old_bason)) {
            u8bp rbuf = be->scratch[BE_RENDER];
            u8bReset(rbuf);
            if (ctx->any_output) call(u8bFeed, rbuf, SEP);
            call(u8bFeed, rbuf, HDRESC);
            call(u8bFeed, rbuf, relpath);
            call(u8bFeed, rbuf, RST);
            call(u8bFeed, rbuf, NL);
            call(u8bFeed, rbuf, DELESC);
            aBpad(u64, stk, 256);
            call(BASTExport, u8bIdle(rbuf), stk, old_bason);
            call(u8bFeed, rbuf, RST);
            call(u8bFeed, rbuf, NL);
            ctx->any_output = YES;
            u8cp r0 = rbuf[1], r1 = rbuf[2];
            u8cs rendered = {r0, r1};
            call(FILEout, rendered);
        }
    } else {
        // Modified: diff→merge→render pipeline
        u8cs ext = {};
        BEExtOf(ext, relpath);
        u8bp nbuf = be->scratch[BE_PARSE];
        u8bp mapbuf = NULL;
        call(BEParseFile, nbuf, &mapbuf, path8cgIn(fpath), &fst, ext);
        u8cp n0 = nbuf[1], n1 = nbuf[2];
        u8cs new_bason = {n0, n1};

        // Step 1: diff(old, new) → patch
        u8bp dbuf = be->scratch[BE_READ];
        u8bReset(dbuf);
        u8bp wbuf = be->scratch[BE_WRAP];
        u8bReset(wbuf);
        u64b hb = {(u64p)wbuf[0], (u64p)wbuf[1],
                   (u64p)wbuf[2], (u64p)wbuf[3]};
        aBpad(u64, ostk, 256);
        aBpad(u64, nstk, 256);
        call(BASONDiff, dbuf, NULL, ostk, old_bason, nstk,
             new_bason, hb);
        u8cp d0 = dbuf[1], d1 = dbuf[2];
        u8cs patch = {d0, d1};

        // Step 2: merge(old, patch) → merged
        u8bp mbuf = be->scratch[BE_PATCH];
        u8bReset(mbuf);
        aBpad(u64, lstk, 256);
        aBpad(u64, rstk, 256);
        call(BASONMerge, mbuf, NULL, lstk, old_bason, rstk, patch);
        u8cp m0 = mbuf[1], m1 = mbuf[2];
        u8cs merged = {m0, m1};

        // Step 3: render old vs merged (same key space)
        u8bp rbuf = be->scratch[BE_RENDER];
        u8bReset(rbuf);
        call(BASONDiffPrint, u8bIdle(rbuf), old_bason, merged, 3,
             relpath);
        u8cp r0 = rbuf[1], r1 = rbuf[2];
        u8cs rendered = {r0, r1};
        if (!$empty(rendered)) {
            // Assemble header + rendered into BE_WRAP
            u8bReset(wbuf);
            if (ctx->any_output) call(u8bFeed, wbuf, SEP);
            call(u8bFeed, wbuf, HDRESC);
            call(u8bFeed, wbuf, relpath);
            call(u8bFeed, wbuf, RST);
            call(u8bFeed, wbuf, NL);
            call(u8bFeed, wbuf, rendered);
            if (rendered[1][-1] != '\n')
                call(u8bFeed, wbuf, NL);
            ctx->any_output = YES;
            u8cs out = {wbuf[1], wbuf[2]};
            call(FILEout, out);
        }
        if (mapbuf) FILEUnMap(mapbuf);
    }
    done;
}

ok64 BEDiffFiles(BEp be, int pathc, u8cs *paths) {
    sane(be != NULL);
    BEDiffCtx ctx = {.be = be, .pathc = pathc, .paths = paths};
    u8cs empty_pfx = {};
    call(BEScanStatMerged, be, empty_pfx, BEDiffCB, &ctx);
    done;
}

// ---- POST (worktree -> repo) ----

typedef struct {
    BEp be;
    ROCKbatchp wb;
    ron60 stamp;
    ignop ig;
    b8 is_base;
    u32 file_count;
} BEPostCtx;

// Report file status: "STAT codc path\n" (4+1+4+1 = 10 char prefix)
static void BEPostReport(u8cs rel, u8cs codec, const char *status, u8 color) {
    a_pad(u8, lb, 512);
    // Status: up to 4 chars, colored, padded
    escfeed(lb_idle, color);
    int si = 0;
    while (status[si] && si < 4) {
        u8sFeed1(lb_idle, (u8)status[si]);
        si++;
    }
    escfeed(lb_idle, 0);
    while (si < 4) { u8sFeed1(lb_idle, ' '); si++; }
    u8sFeed1(lb_idle, ' ');
    // Codec: up to 4 chars, gray, padded
    escfeed(lb_idle, GRAY);
    int ci = 0;
    u8cp ce = codec[0];
    while (ce < codec[1] && ci < 4) {
        u8sFeed1(lb_idle, *ce);
        ce++;
        ci++;
    }
    escfeed(lb_idle, 0);
    while (ci < 4) { u8sFeed1(lb_idle, ' '); ci++; }
    u8sFeed1(lb_idle, ' ');
    // Path
    u8sFeed(lb_idle, rel);
    u8sFeed1(lb_idle, '\n');
    FILEout(lb_datac);
}

// Write be: and stat: keys for a file.
// query empty → base key; non-empty → waypoint key.
static ok64 BEPostWriteKeys(BEp be, ROCKbatchp wb,
                             u8cs fpath, u8cs query,
                             u8cs content, BEmeta meta) {
    sane(be != NULL && wb != NULL);
    a_cstr(sch_be, BE_SCHEME_BE);
    a_cstr(sch_stat, BE_SCHEME_STAT);
    a_uri(be_key, sch_be, 0, fpath, query, 0);
    call(ROCKBatchPut, wb, be_key, content);
    a_uri(stat_key, sch_stat, 0, fpath, query, 0);
    u8bp mbuf = be->scratch[BE_WRAP];
    u8bReset(mbuf);
    call(BEMetaFeedBason, mbuf, NULL, meta);
    u8cs stat_val = {mbuf[1], mbuf[2]};
    call(ROCKBatchPut, wb, stat_key, stat_val);
    done;
}

// Parse file, write keys. is_base=YES → base keys, else waypoint keys.
// do_diff=YES → diff against old merged state, write delta.
static ok64 BEPostFile(BEp be, ROCKbatchp wb, ron60 stamp,
                        b8 is_base, b8 do_diff, path8cg filepath) {
    sane(be != NULL && wb != NULL);
    // Compute relative path from worktree root
    a_path(relpath, "");
    call(path8gRelative, path8gIn(relpath), path8cgIn(be->work_pp), filepath);
    u8cs rel = {relpath[1], relpath[2]};

    // Stat source file for metadata
    struct stat fst;
    call(FILEStat, &fst, filepath);

    // Get extension and codec
    u8cs basename = {};
    path8gBase(basename, filepath);
    u8cs ext = {};
    BEExtOf(ext, basename);
    u8cs codec = {};
    BASTCodec(codec, ext);

    // Build metadata
    BEmeta meta = {};
    BEMetaFromStat(&meta, &fst, ext);

    // Parse to BASON
    u8bp nbuf = be->scratch[BE_PARSE];
    u8bReset(nbuf);
    u8bp mapbuf = NULL;
    ok64 o = OK;

    if (fst.st_size == 0) {
        u8cs nokey = {(u8cp)"", (u8cp)""};
        call(BASONFeedInto, NULL, nbuf, 'A', nokey);
        call(BASONFeedOuto, NULL, nbuf);
    } else {
        call(FILEMapRO, &mapbuf, filepath);
        u8cs source = {mapbuf[1], mapbuf[2]};
        o = BASTParse(nbuf, NULL, source, ext);
        if (o != OK) {
            BEPostReport(rel, codec, "SKIP", DARK_YELLOW);
            FILEUnMap(mapbuf);
            done;
        }
    }
    u8cp nb0 = nbuf[1], nb1 = nbuf[2];
    u8cs new_bason = {nb0, nb1};

    // Build file path component for keys
    a_pad(u8, fp, 256);
    call(BEProjPath, fp_idle, be->loc.path, rel);

    // Build query: empty for base, stamp-branch for waypoint
    u8cs query = {};
    a_pad(u8, wq, 128);
    if (!is_base) {
        call(BEQueryBuild, wq_idle, stamp, VEROrigin(&be->branches[0]));
        query[0] = wq_datac[0];
        query[1] = wq_datac[1];
    }

    // Determine content: full BASON or diff delta
    u8cs content = {};
    $mv(content, new_bason);

    if (do_diff && !is_base) {
        u8bp pbuf = be->scratch[BE_PATCH];
        u8bReset(pbuf);
        ok64 go = BEGetFileMerged(be, be->loc.path, rel, pbuf, NULL);
        u8cs old_bason = {};
        if (go == OK) {
            u8cp md0 = pbuf[1], md1 = pbuf[2];
            old_bason[0] = md0;
            old_bason[1] = md1;
        }
        if ($ok(old_bason) && !$empty(old_bason)) {
            u8bp obuf = be->scratch[BE_READ];
            u8bReset(obuf);
            aBpad(u64, ostk, 256);
            aBpad(u64, nstk, 256);
            o = BASONDiff(obuf, NULL, ostk, old_bason, nstk,
                          new_bason, NULL);
            if (o != OK) {
                if (mapbuf) FILEUnMap(mapbuf);
                fail(o);
            }
            u8cs delta = {obuf[1], obuf[2]};
            if ($empty(delta)) {
                if (mapbuf) FILEUnMap(mapbuf);
                done;
            }
            $mv(content, delta);
        }
    }

    call(BEPostWriteKeys, be, wb, fp_datac, query, content, meta);

    // Extract trigrams and write tri: keys
    call(BETriPost, be, wb, new_bason, fp_datac);
    // Extract symbol names and write sym: keys
    call(BESymPost, be, wb, new_bason, fp_datac);

    // Commit per file to bound memory
    call(BEBatchFlush, be, wb);

    BEPostReport(rel, codec, "OK", DARK_GREEN);

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
            if (IGNOMatch(ctx->ig, rel, YES)) {
                u8cs dircodec = {(u8cp)"dir", (u8cp)"dir" + 3};
                BEPostReport(rel, dircodec, "IGN", DARK_YELLOW);
                return FILEskip;
            }
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
            if (IGNOMatch(ctx->ig, rel, NO)) {
                u8cs ext = {};
                u8cs codec = {};
                BEFileInfo(ext, codec, rel);
                BEPostReport(rel, codec, "IGN", DARK_YELLOW);
                return OK;
            }
        }
    }
    ok64 po = BEPostFile(ctx->be, ctx->wb, ctx->stamp, ctx->is_base, NO,
                         path8cgIn(path));
    if (po != OK) {
        rocksdb_writebatch_clear(ctx->wb->wb);
        a_path(relpath, "");
        path8gRelative(path8gIn(relpath), path8cgIn(ctx->be->work_pp),
                        path8cgIn(path));
        u8cs rel = {relpath[1], relpath[2]};
        u8cs ext = {};
        u8cs codec = {};
        BEFileInfo(ext, codec, rel);
        BEPostReport(rel, codec, "FAIL", DARK_RED);
    } else if (++ctx->file_count % 64 == 0) {
        BEFlushDB(ctx->be);
    }
    return OK;
}

// ---- stat: iterator with formula merge (cases 2, 3a) ----

// Iterate stat: keys, group by relpath, merge metadata via branch formula,
// call cb for each file. prefix_filter limits to a relpath subtree.
ok64 BEScanStatMerged(BEp be, u8cs prefix_filter,
                      BEStatCBf cb, voidp arg) {
    sane(be != NULL && cb != NULL);

    // Build branch formula
    ron120 fbuf[VER_MAX];
    ron120s form = {fbuf, fbuf + VER_MAX};
    call(VERFormFromBranches, form, be->branchc, be->branches);
    ron120cs formcs = {fbuf, form[0]};

    // Build stat:<project>/ prefix
    a_cstr(sch_stat, BE_SCHEME_STAT);
    a_pad(u8, fp, 256);
    call(BEProjPath, fp_idle, be->loc.path, prefix_filter);
    a_pad(u8, pfx, 512);
    call(BEKeyBuild, pfx_idle, sch_stat, fp_datac, 0, 0);
    size_t path_off = $len(be->loc.path) + 1;  // skip "project/"

    // Current file state
    u8 cur_rp_buf[512];
    u8s cur_rp_s = {cur_rp_buf, cur_rp_buf + sizeof(cur_rp_buf)};
    u8cs cur_relpath = {};
    BEmeta cur_meta = {};

    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be->db);
    call(ROCKIterSeek, &it, pfx_datac);

    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(pfx_datac) ||
            memcmp(k[0], pfx_datac[0], $len(pfx_datac)) != 0)
            break;

        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o != OK) {
            call(ROCKIterNext, &it);
            continue;
        }
        // Skip fragment keys (commits, milestones)
        if (!$empty(ku.fragment)) {
            call(ROCKIterNext, &it);
            continue;
        }

        // Extract relpath
        if ($len(ku.path) <= path_off) {
            call(ROCKIterNext, &it);
            continue;
        }
        u8cs relpath = {ku.path[0] + path_off, ku.path[1]};

        // New file → flush previous
        if ($ok(cur_relpath) && !$empty(cur_relpath) &&
            !$eq(relpath, cur_relpath)) {
            ok64 fo = cb(arg, cur_relpath, cur_meta);
            if (fo != OK && fo != BEnone) {
                ROCKIterClose(&it);
                fail(fo);
            }
            cur_relpath[0] = NULL;
            cur_relpath[1] = NULL;
            cur_meta = (BEmeta){};
        }

        // Start new file
        if (!$ok(cur_relpath) || $empty(cur_relpath)) {
            cur_rp_s[0] = cur_rp_buf;
            o = u8sFeed(cur_rp_s, relpath);
            if (o != OK) {
                call(ROCKIterNext, &it);
                continue;
            }
            cur_relpath[0] = cur_rp_buf;
            cur_relpath[1] = cur_rp_s[0];
            cur_meta = (BEmeta){};
        }

        // Drain metadata
        u8cs v = {};
        ROCKIterVal(&it, v);
        if ($empty(ku.query)) {
            // Base key: initialize metadata
            BEMetaDrainBason(&cur_meta, v);
        } else {
            // Waypoint: check formula match
            if (BEWaypointMatch(formcs, ku.query)) {
                BEMetaDrainBason(&cur_meta, v);
            }
        }

        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);

    // Flush last file
    if ($ok(cur_relpath) && !$empty(cur_relpath)) {
        ok64 fo = cb(arg, cur_relpath, cur_meta);
        if (fo != OK && fo != BEnone) fail(fo);
    }
    done;
}

// ---- DB-driven incremental post callback (case 2, 3a) ----

typedef struct {
    BEp be;
    ROCKbatchp wb;
    ron60 stamp;
} BEPostDiffCtx;

static ok64 BEPostDiffCB(voidp arg, u8cs relpath, BEmeta merged_meta) {
    sane(arg != NULL);
    BEPostDiffCtx *ctx = (BEPostDiffCtx *)arg;
    BEp be = ctx->be;

    // Build worktree path from relpath
    a_path(fpath, "");
    call(BEWorkPath, path8gIn(fpath), be, relpath);

    // Stat file on disk
    struct stat fst;
    if (FILEStat(&fst, path8cgIn(fpath)) != OK) {
        // File deleted from disk — skip
        return OK;
    }

    // Get extension and codec for reporting
    u8cs ext = {};
    u8cs codec = {};
    BEFileInfo(ext, codec, relpath);

    // Build current metadata
    BEmeta meta = {};
    BEMetaFromStat(&meta, &fst, ext);

    // Fast-path: skip if mtime unchanged
    if (merged_meta.mtime > 0 && meta.mtime == merged_meta.mtime)
        return OK;

    // Parse new file to BASON
    u8bp nbuf = be->scratch[BE_PARSE];
    u8bReset(nbuf);
    u8bp mapbuf = NULL;

    if (fst.st_size == 0) {
        u8cs nokey = {(u8cp)"", (u8cp)""};
        call(BASONFeedInto, NULL, nbuf, 'A', nokey);
        call(BASONFeedOuto, NULL, nbuf);
    } else {
        call(FILEMapRO, &mapbuf, path8cgIn(fpath));
        u8cs source = {mapbuf[1], mapbuf[2]};
        __ = BASTParse(nbuf, NULL, source, ext);
        if (__ != OK) {
            BEPostReport(relpath, codec, "SKIP", DARK_YELLOW);
            FILEUnMap(mapbuf);
            return OK;
        }
    }
    u8cp nb0 = nbuf[1], nb1 = nbuf[2];
    u8cs new_bason = {nb0, nb1};

    // Get old merged BASON from repo
    u8bp pbuf = be->scratch[BE_PATCH];
    u8bReset(pbuf);
    ok64 go = BEGetFileMerged(be, be->loc.path, relpath, pbuf, NULL);
    u8cs old_bason = {};
    if (go == OK) {
        u8cp md0 = pbuf[1], md1 = pbuf[2];
        old_bason[0] = md0;
        old_bason[1] = md1;
    }

    // Build file path component for keys
    a_pad(u8, kp, 256);
    __ = BEProjPath(kp_idle, be->loc.path, relpath);
    if (__ != OK) { if (mapbuf) FILEUnMap(mapbuf); return __; }

    // Build query: stamp-branch
    ron60 active = VEROrigin(&be->branches[0]);
    a_pad(u8, wq, 128);
    __ = BEQueryBuild(wq_idle, ctx->stamp, active);
    if (__ != OK) { if (mapbuf) FILEUnMap(mapbuf); return __; }

    u8cs content = {};
    if ($ok(old_bason) && !$empty(old_bason)) {
        // Diff old vs new
        u8bp obuf = be->scratch[BE_READ];
        u8bReset(obuf);
        aBpad(u64, ostk, 256);
        aBpad(u64, nstk, 256);
        __ = BASONDiff(obuf, NULL, ostk, old_bason, nstk,
                       new_bason, NULL);
        if (__ != OK) {
            if (mapbuf) FILEUnMap(mapbuf);
            return __;
        }
        u8cs delta = {obuf[1], obuf[2]};
        if ($empty(delta)) {
            // Content unchanged but mtime differs — update stat: mtime
            a_cstr(sch_stat, BE_SCHEME_STAT);
            a_uri(stat_key, sch_stat, 0, kp_datac, wq_datac, 0);
            u8bp sbuf = be->scratch[BE_WRAP];
            u8bReset(sbuf);
            call(BEMetaFeedBason, sbuf, NULL, meta);
            u8cp s0 = sbuf[1], s1 = sbuf[2];
            u8cs stat_val = {s0, s1};
            call(ROCKBatchPut, ctx->wb, stat_key, stat_val);
            call(BEBatchFlush, be, ctx->wb);
            if (mapbuf) FILEUnMap(mapbuf);
            return OK;
        }
        $mv(content, delta);
    } else {
        // New file (no old): full BASON as waypoint
        $mv(content, new_bason);
    }

    __ = BEPostWriteKeys(be, ctx->wb, kp_datac, wq_datac, content, meta);
    if (__ != OK) { if (mapbuf) FILEUnMap(mapbuf); return __; }

    // Extract trigrams
    __ = BETriPost(be, ctx->wb, new_bason, kp_datac);
    if (__ != OK) { if (mapbuf) FILEUnMap(mapbuf); return __; }
    // Extract symbols
    __ = BESymPost(be, ctx->wb, new_bason, kp_datac);
    if (__ != OK) { if (mapbuf) FILEUnMap(mapbuf); return __; }

    // Commit per file to bound memory
    __ = BEBatchFlush(be, ctx->wb);
    if (__ != OK) { if (mapbuf) FILEUnMap(mapbuf); return __; }

    BEPostReport(relpath, codec, "OK", DARK_GREEN);

    if (mapbuf) FILEUnMap(mapbuf);
    return OK;
}

// ---- Prefix probe (case 3 decision) ----

// Check if any stat: records exist for a relpath prefix.
static b8 BEPostHasRecords(BEp be, u8cs relpath_prefix) {
    a_cstr(sch_stat, BE_SCHEME_STAT);
    a_pad(u8, fp, 256);
    if (BEProjPath(fp_idle, be->loc.path, relpath_prefix) != OK) return NO;
    a_pad(u8, pfx, 512);
    if (BEKeyBuild(pfx_idle, sch_stat, fp_datac, 0, 0) != OK) return NO;

    ROCKiter it = {};
    if (ROCKIterOpen(&it, &be->db) != OK) return NO;
    if (ROCKIterSeek(&it, pfx_datac) != OK) {
        ROCKIterClose(&it);
        return NO;
    }
    b8 found = NO;
    if (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) >= $len(pfx_datac) &&
            memcmp(k[0], pfx_datac[0], $len(pfx_datac)) == 0)
            found = YES;
    }
    ROCKIterClose(&it);
    return found;
}

// Helper: resolve CLI path argument to absolute path
static ok64 BEPostResolvePath(path8g out, BEp be, u8cs arg) {
    sane(out != NULL && be != NULL);
    call(path8gDup, out, path8cgIn(be->work_pp));
    a_path(rp, "");
    call(u8sFeed, u8bIdle(rp), arg);
    call(path8gTerm, path8gIn(rp));
    call(path8gAdd, out, path8cgIn(rp));
    done;
}

ok64 BEPost(BEp be, int pathc, u8cs *paths, u8cs message) {
    sane(be != NULL);
    ron60 stamp = RONNow();

    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    if (be->initial) {
        // CASE 1: Initial import — FS scan, base keys
        igno gi = {};
        u8cs workdir;
        u8csMv(workdir, u8bDataC(be->work_pp));
        IGNOLoad(&gi, workdir);
        BEPostCtx ctx = {be, &wb, stamp, &gi, YES};
        if (pathc > 0 && paths != NULL) {
            for (int i = 0; i < pathc; i++) {
                a_path(apath, "");
                call(BEPostResolvePath, path8gIn(apath), be, paths[i]);
                struct stat dst;
                ok64 so = FILEStat(&dst, path8cgIn(apath));
                if (so == OK && S_ISDIR(dst.st_mode)) {
                    ok64 o = FILEScan(
                        apath,
                        (FILE_SCAN)(FILE_SCAN_FILES | FILE_SCAN_DIRS |
                                    FILE_SCAN_DEEP),
                        BEPostScanCB, &ctx);
                    if (o != OK) {
                        IGNOFree(&gi);
                        ROCKBatchClose(&wb);
                        fail(o);
                    }
                } else {
                    ok64 o = BEPostFile(be, &wb, stamp, YES, NO,
                                        path8cgIn(apath));
                    if (o != OK) {
                        IGNOFree(&gi);
                        ROCKBatchClose(&wb);
                        fail(o);
                    }
                }
            }
        } else {
            a_path(spath, "");
            call(path8gDup, path8gIn(spath), path8cgIn(be->work_pp));
            ok64 o = FILEScan(
                spath,
                (FILE_SCAN)(FILE_SCAN_FILES | FILE_SCAN_DIRS |
                            FILE_SCAN_DEEP),
                BEPostScanCB, &ctx);
            if (o != OK) {
                IGNOFree(&gi);
                ROCKBatchClose(&wb);
                fail(o);
            }
        }
        IGNOFree(&gi);
    } else if (pathc == 0 || paths == NULL) {
        // CASE 2: Incremental post — DB scan for tracked files
        BEPostDiffCtx dctx = {be, &wb, stamp};
        u8cs empty_pfx = {};
        call(BEScanStatMerged, be, empty_pfx, BEPostDiffCB, &dctx);
    } else {
        // CASE 3: Per-prefix decision
        for (int i = 0; i < pathc; i++) {
            a_pad(u8, rp, 512);

            a_path(apath, "");
            call(BEPostResolvePath, path8gIn(apath), be, paths[i]);

            struct stat dst;
            ok64 so = FILEStat(&dst, path8cgIn(apath));

            if (so == OK && S_ISDIR(dst.st_mode)) {
                // Directory path — build relpath prefix
                a_path(relp, "");
                call(path8gRelative, path8gIn(relp),
                     path8cgIn(be->work_pp), path8cgIn(apath));
                u8cs rel = {relp[1], relp[2]};
                // Add trailing /
                call(u8sFeed, rp_idle, rel);
                u8sFeed1(rp_idle, '/');

                if (BEPostHasRecords(be, rp_datac)) {
                    // CASE 3a: DB scan with prefix filter
                    BEPostDiffCtx dctx = {be, &wb, stamp};
                    ok64 o = BEScanStatMerged(be, rp_datac,
                                              BEPostDiffCB, &dctx);
                    if (o != OK) {
                        ROCKBatchClose(&wb);
                        fail(o);
                    }
                } else {
                    // CASE 3b: New prefix — FS scan, waypoint keys
                    igno gi = {};
                    u8cs workdir;
                    u8csMv(workdir, u8bDataC(be->work_pp));
                    IGNOLoad(&gi, workdir);
                    BEPostCtx ctx = {be, &wb, stamp, &gi, NO};
                    ok64 o = FILEScan(
                        apath,
                        (FILE_SCAN)(FILE_SCAN_FILES | FILE_SCAN_DIRS |
                                    FILE_SCAN_DEEP),
                        BEPostScanCB, &ctx);
                    IGNOFree(&gi);
                    if (o != OK) {
                        ROCKBatchClose(&wb);
                        fail(o);
                    }
                }
            } else {
                // Single file — diff-based post (no mtime skip)
                ok64 o = BEPostFile(be, &wb, stamp, NO, YES,
                                    path8cgIn(apath));
                if (o != OK) {
                    ROCKBatchClose(&wb);
                    fail(o);
                }
            }
        }
    }

    // Store commit metadata: be:project/?stamp-branch#commit
    ron60 active = VEROrigin(&be->branches[0]);
    a_pad(u8, cp, 256);
    u8cs norp = {};
    call(BEProjPath, cp_idle, be->loc.path, norp);
    a_pad(u8, cq, 128);
    call(BEQueryBuild, cq_idle, stamp, active);
    a_cstr(sch_be, BE_SCHEME_BE);
    a_cstr(meta_commit, "commit");
    a_uri(commit_key, sch_be, 0, cp_datac, cq_datac, meta_commit);
    if ($ok(message) && !$empty(message)) {
        call(ROCKBatchPut, &wb, commit_key, message);
    } else {
        u8cs empty = {NULL, NULL};
        call(ROCKBatchPut, &wb, commit_key, empty);
    }

    call(ROCKBatchWrite, &be->db, &wb);
    call(ROCKBatchClose, &wb);
    be->initial = NO;
    done;
}

// ---- POST data (HTTP / programmatic) ----

ok64 BEPostData(BEp be, u8cs relpath, u8cs source, u8cs branch, u8cs message) {
    sane(be != NULL && $ok(relpath) && !$empty(relpath));
    ron60 stamp = RONNow();

    // Resolve branch: use provided or fall back to active
    ron60 active = 0;
    if ($ok(branch) && !$empty(branch)) {
        call(RONutf8sDrain, &active, branch);
    } else {
        active = VEROrigin(&be->branches[0]);
    }

    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    // Get extension and codec
    u8cs ext = {};
    u8cs codec = {};
    BEFileInfo(ext, codec, relpath);

    // Parse source to BASON
    u8bp nbuf = be->scratch[BE_PARSE];
    u8bReset(nbuf);
    if ($empty(source)) {
        u8cs nokey = {(u8cp)"", (u8cp)""};
        call(BASONFeedInto, NULL, nbuf, 'A', nokey);
        call(BASONFeedOuto, NULL, nbuf);
    } else {
        ok64 po = BASTParse(nbuf, NULL, source, ext);
        if (po != OK) {
            ROCKBatchClose(&wb);
            fail(po);
        }
    }
    u8cp nb0 = nbuf[1], nb1 = nbuf[2];
    u8cs new_bason = {nb0, nb1};

    // Get old merged BASON from repo
    u8bp pbuf = be->scratch[BE_PATCH];
    u8bReset(pbuf);
    ok64 go = BEGetFileMerged(be, be->loc.path, relpath, pbuf, NULL);
    u8cs old_bason = {};
    if (go == OK) {
        u8cp md0 = pbuf[1], md1 = pbuf[2];
        old_bason[0] = md0;
        old_bason[1] = md1;
    }

    // Build file path component for keys
    a_pad(u8, fp, 256);
    call(BEProjPath, fp_idle, be->loc.path, relpath);

    // Build query: stamp-branch
    a_pad(u8, wq, 128);
    call(BEQueryBuild, wq_idle, stamp, active);

    // Determine content: diff delta or full BASON
    u8cs content = {};
    if ($ok(old_bason) && !$empty(old_bason)) {
        u8bp obuf = be->scratch[BE_READ];
        u8bReset(obuf);
        aBpad(u64, ostk, 256);
        aBpad(u64, nstk, 256);
        ok64 do_ = BASONDiff(obuf, NULL, ostk, old_bason, nstk,
                             new_bason, NULL);
        if (do_ != OK) {
            ROCKBatchClose(&wb);
            fail(do_);
        }
        u8cs delta = {obuf[1], obuf[2]};
        if ($empty(delta)) {
            // No content change
            ROCKBatchClose(&wb);
            done;
        }
        $mv(content, delta);
    } else {
        $mv(content, new_bason);
    }

    // Build metadata: current time, 0644, ftype from ext
    BEmeta meta = {};
    meta.mtime = (u32)stamp;
    u32 mode = 0644;
    u32 ftype = BASTFtype(ext);
    meta.modeftype = (mode << 20) | ((ftype & 0x3FFFF) << 2);

    // Write be: and stat: keys
    call(BEPostWriteKeys, be, &wb, fp_datac, wq_datac, content, meta);

    // Extract trigrams
    call(BETriPost, be, &wb, new_bason, fp_datac);
    // Extract symbols
    call(BESymPost, be, &wb, new_bason, fp_datac);

    // Commit metadata: be:project/?stamp-branch#commit
    a_pad(u8, cp, 256);
    u8cs norp = {};
    call(BEProjPath, cp_idle, be->loc.path, norp);
    a_cstr(sch_be, BE_SCHEME_BE);
    a_cstr(meta_commit, "commit");
    a_uri(commit_key, sch_be, 0, cp_datac, wq_datac, meta_commit);
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

// Helper: re-key all waypoints of a scheme from source branch to active branch
static ok64 BEPutScheme(BEp be, ROCKbatchp wb, u8cs scheme,
                         ron60 src_val, ron60 active) {
    sane(be != NULL && wb != NULL);
    a_pad(u8, fp, 256);
    u8cs norp = {};
    call(BEProjPath, fp_idle, be->loc.path, norp);
    a_pad(u8, pfx, 512);
    call(BEKeyBuild, pfx_idle, scheme, fp_datac, 0, 0);

    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be->db);
    call(ROCKIterSeek, &it, pfx_datac);

    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(pfx_datac) ||
            memcmp(k[0], pfx_datac[0], $len(pfx_datac)) != 0)
            break;

        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o != OK || $empty(ku.query) || !$empty(ku.fragment)) {
            call(ROCKIterNext, &it);
            continue;
        }

        ron120 ver = {};
        o = VERParse(&ver, ku.query);
        ron60 wp_stamp = VERTime(&ver);
        if (o != OK || VEROrigin(&ver) != src_val) {
            call(ROCKIterNext, &it);
            continue;
        }

        u8cs v = {};
        ROCKIterVal(&it, v);

        // Build new key under active branch
        a_pad(u8, nq, 128);
        call(BEQueryBuild, nq_idle, wp_stamp, active);
        a_uri(new_key, scheme, 0, ku.path, nq_datac, 0);
        call(ROCKBatchPut, wb, new_key, v);
        call(ROCKBatchDel, wb, k);

        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);
    done;
}

ok64 BEPut(BEp be, u8cs source_branch, u8cs message) {
    sane(be != NULL && $ok(source_branch) && !$empty(source_branch));
    ron60 stamp = RONNow();
    ron60 active = VEROrigin(&be->branches[0]);

    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    // Re-key both stat: and be: waypoints from source → active
    a_cstr(sch_be, BE_SCHEME_BE);
    a_cstr(sch_stat, BE_SCHEME_STAT);
    ron60 src_val = 0;
    call(RONutf8sDrain, &src_val, source_branch);
    call(BEPutScheme, be, &wb, sch_be, src_val, active);
    call(BEPutScheme, be, &wb, sch_stat, src_val, active);

    // Merge metadata: be:project/?stamp-branch#merge
    a_pad(u8, mp, 256);
    u8cs norp = {};
    call(BEProjPath, mp_idle, be->loc.path, norp);
    a_pad(u8, mq, 128);
    call(BEQueryBuild, mq_idle, stamp, active);
    a_cstr(meta_merge, "merge");
    a_uri(merge_key, sch_be, 0, mp_datac, mq_datac, meta_merge);
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

// Check if any waypoint key in the project has a matching branch suffix
static b8 BEHasBranch(BEp be, u8cs name) {
    a_cstr(sch_be, BE_SCHEME_BE);
    a_pad(u8, fp, 256);
    u8cs norp = {};
    if (BEProjPath(fp_idle, be->loc.path, norp) != OK) return NO;
    a_pad(u8, pfx, 512);
    if (BEKeyBuild(pfx_idle, sch_be, fp_datac, 0, 0) != OK) return NO;

    ron60 name_val = 0;
    if (RONutf8sDrain(&name_val, name) != OK) return NO;

    ROCKiter it = {};
    if (ROCKIterOpen(&it, &be->db) != OK) return NO;
    if (ROCKIterSeek(&it, pfx_datac) != OK) {
        ROCKIterClose(&it);
        return NO;
    }

    b8 found = NO;
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(pfx_datac) ||
            memcmp(k[0], pfx_datac[0], $len(pfx_datac)) != 0)
            break;
        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o == OK && !$empty(ku.query)) {
            ron120 ver = {};
            o = VERParse(&ver, ku.query);
            if (o == OK && VEROrigin(&ver) == name_val) {
                found = YES;
                break;
            }
        }
        ROCKIterNext(&it);
    }
    ROCKIterClose(&it);
    return found;
}

// Delete waypoint keys for a branch under a scheme prefix.
// keep_frags=YES preserves fragment keys (commit/milestone metadata).
static ok64 BEDeleteBranchScheme(BEp be, ROCKbatchp wb,
                                  u8cs scheme, u8cs name,
                                  b8 keep_frags) {
    sane(be != NULL && wb != NULL);
    a_pad(u8, fp, 256);
    u8cs norp = {};
    call(BEProjPath, fp_idle, be->loc.path, norp);
    a_pad(u8, pfx, 512);
    call(BEKeyBuild, pfx_idle, scheme, fp_datac, 0, 0);

    ron60 name_val = 0;
    call(RONutf8sDrain, &name_val, name);

    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be->db);
    call(ROCKIterSeek, &it, pfx_datac);
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(pfx_datac) ||
            memcmp(k[0], pfx_datac[0], $len(pfx_datac)) != 0)
            break;
        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o == OK && !$empty(ku.query)) {
            if (!keep_frags || $empty(ku.fragment)) {
                ron120 ver = {};
                o = VERParse(&ver, ku.query);
                if (o == OK && VEROrigin(&ver) == name_val) {
                    call(ROCKBatchDel, wb, k);
                }
            }
        }
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);
    done;
}

static ok64 BEDeleteBranch(BEp be, u8cs name) {
    sane(be != NULL && $ok(name) && !$empty(name));
    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);
    a_cstr(sch_be, BE_SCHEME_BE);
    a_cstr(sch_stat, BE_SCHEME_STAT);
    call(BEDeleteBranchScheme, be, &wb, sch_be, name, NO);
    call(BEDeleteBranchScheme, be, &wb, sch_stat, name, NO);
    call(ROCKBatchWrite, &be->db, &wb);
    call(ROCKBatchClose, &wb);
    done;
}

static ok64 BEDeleteFile(BEp be, u8cs target) {
    sane(be != NULL && $ok(target) && !$empty(target));

    ron60 stamp = RONNow();
    ron60 active = VEROrigin(&be->branches[0]);

    // Build path component
    a_pad(u8, dp, 256);
    call(BEProjPath, dp_idle, be->loc.path, target);

    // Build query
    a_pad(u8, dq, 128);
    call(BEQueryBuild, dq_idle, stamp, active);
    u8cs empty_val = {NULL, NULL};

    // Tombstone: empty value in be: key
    a_cstr(sch_be, BE_SCHEME_BE);
    a_uri(be_key, sch_be, 0, dp_datac, dq_datac, 0);
    call(ROCKPut, &be->db, be_key, empty_val);

    // Zero metadata in stat: key
    a_cstr(sch_stat, BE_SCHEME_STAT);
    a_uri(stat_key, sch_stat, 0, dp_datac, dq_datac, 0);
    aBpad(u8, mbuf, 256);
    BEmeta zero = {};
    call(BEMetaFeedBason, mbuf, NULL, zero);
    u8cs tomb = {mbuf[1], mbuf[2]};
    call(ROCKPut, &be->db, stat_key, tomb);
    done;
}

ok64 BEDelete(BEp be, u8cs target) {
    sane(be != NULL && $ok(target) && !$empty(target));

    // ?prefix → explicit branch
    if (target[0][0] == '?') {
        u8cs name = {target[0] + 1, target[1]};
        call(BEDeleteBranch, be, name);
        done;
    }

    // stat() the worktree path — if it exists, it's a file
    a_path(fpath, "");
    call(BEWorkPath, path8gIn(fpath), be, target);

    struct stat st;
    ok64 so = FILEStat(&st, path8cgIn(fpath));
    if (so == OK) {
        call(BEDeleteFile, be, target);
        done;
    }

    // Not on disk — check if a branch with this name exists in the DB
    if (BEHasBranch(be, target)) {
        call(BEDeleteBranch, be, target);
        done;
    }

    // Default: treat as file tombstone
    call(BEDeleteFile, be, target);
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

// BEScanChanged callback: write merged BASON as new base
typedef struct {
    BEp be;
    ROCKbatchp wb;
} BEMileCBCtx;

static ok64 BEMileCB(voidp arg, u8cs relpath, u8cs bason, BEmeta meta) {
    sane(arg != NULL);
    BEMileCBCtx *ctx = (BEMileCBCtx *)arg;
    BEp be = ctx->be;

    // Build full path component: project/relpath
    a_pad(u8, fp, 256);
    call(BEProjPath, fp_idle, be->loc.path, relpath);

    // Write new be: base
    a_cstr(sch_be, BE_SCHEME_BE);
    a_uri(base_key, sch_be, 0, fp_datac, 0, 0);
    call(ROCKBatchPut, ctx->wb, base_key, bason);

    // Write new stat: base with metadata
    a_cstr(sch_stat, BE_SCHEME_STAT);
    a_uri(stat_key, sch_stat, 0, fp_datac, 0, 0);
    aBpad(u8, smbuf, 256);
    call(BEMetaFeedBason, smbuf, NULL, meta);
    u8cs stat_val = {smbuf[1], smbuf[2]};
    call(ROCKBatchPut, ctx->wb, stat_key, stat_val);
    done;
}

ok64 BEMilestone(BEp be, u8cs name) {
    sane(be != NULL);

    a_cstr(main_branch, "main");
    a_cstr(sch_be, BE_SCHEME_BE);
    a_cstr(sch_stat, BE_SCHEME_STAT);
    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    // Build main-only formula URI for BEScanChanged
    uri main_loc = be->loc;
    u8cs main_q = {main_branch[0], main_branch[1]};
    $mv(main_loc.query, main_q);

    // Phase 1: write merged base keys via BEScanChanged
    BEMileCBCtx mctx = {be, &wb};
    call(BEScanChanged, be, &main_loc, BEMileCB, &mctx);

    // Phase 2: delete main-branch waypoint keys (keep fragment metadata)
    call(BEDeleteBranchScheme, be, &wb, sch_be, main_branch, YES);
    call(BEDeleteBranchScheme, be, &wb, sch_stat, main_branch, YES);

    // Record milestone metadata: be:project/?stamp-branch#milestone
    if ($ok(name) && !$empty(name)) {
        ron60 stamp = RONNow();
        a_pad(u8, msp, 256);
        u8cs norp = {};
        call(BEProjPath, msp_idle, be->loc.path, norp);
        a_pad(u8, msq, 128);
        ron60 main_val = 0;
        RONutf8sDrain(&main_val, main_branch);
        call(BEQueryBuild, msq_idle, stamp, main_val);
        a_cstr(meta_milestone, "milestone");
        a_uri(ms_key, sch_be, 0, msp_datac, msq_datac, meta_milestone);
        call(ROCKBatchPut, &wb, ms_key, name);
    }

    call(ROCKBatchWrite, &be->db, &wb);
    call(ROCKBatchClose, &wb);
    done;
}

// ---- Trigram index (POST-time indexing) ----

#define BETriChar(c) (RON64_REV[(u8)(c)] != 0xff)

// Trigram collection context for POST
typedef struct {
    u8 seen[32768];  // bit array for dedup (~238K possible trigrams)
    BEp be;
    ROCKbatchp wb;
    u8cs hashlet;
} BETriCtx;

static u32 BETriHash(u8cs tri) {
    return ((u32)tri[0][0] * 127 + (u32)tri[0][1]) * 127 + (u32)tri[0][2];
}

static ok64 BETriCB(voidp arg, u8cs trigram) {
    ok64 __ = OK;
    BETriCtx *ctx = (BETriCtx *)arg;
    u32 h = BETriHash(trigram) & 0x3FFFF;
    u32 byte_idx = h >> 3;
    u8 bit = 1 << (h & 7);
    if (ctx->seen[byte_idx] & bit) return OK;
    ctx->seen[byte_idx] |= bit;

    // Build tri: key
    a_cstr(sch_tri, BE_SCHEME_TRI);
    a_uri(tri_key, sch_tri, 0, ctx->be->loc.path, trigram, 0);

    // Build BASON object { hashlet: "" } as merge operand
    aBpad(u8, obj, 128);
    u8cs noval = {(u8cp)"", (u8cp)""};
    call(BASONFeedInto, NULL, obj, 'O', noval);
    call(BASONFeed, NULL, obj, 'S', ctx->hashlet, noval);
    call(BASONFeedOuto, NULL, obj);
    u8cs obj_val = {obj[1], obj[2]};

    call(ROCKBatchMerge, ctx->wb, tri_key, obj_val);
    return OK;
}

static ok64 BETriPost(BEp be, ROCKbatchp wb, u8cs bason, u8cs fpath) {
    sane(be != NULL && wb != NULL);
    if ($empty(bason)) done;

    a_pad(u8, hl, 4);
    call(BEHashlet, hl_idle, fpath);

    BETriCtx ctx = {};
    memset(ctx.seen, 0, sizeof(ctx.seen));
    ctx.be = be;
    ctx.wb = wb;
    $mv(ctx.hashlet, hl_datac);

    call(BETriExtract, bason, BETriCB, &ctx);
    done;
}

// ---- Symbol index (POST-time indexing) ----
// Values are BASON objects { relpath: "" } — relpaths as keys, merge = union.
// A single ROCKGet on sym:project/?symbol gives the file list directly.

typedef struct {
    BEp be;
    ROCKbatchp wb;
    u8cs relpath;
} BESymCtx;

static ok64 BESymCB(voidp arg, u8cs symbol) {
    ok64 __ = OK;
    BESymCtx *ctx = (BESymCtx *)arg;

    // Build sym: key with symbol in query slot for prefix scan
    a_cstr(sch_sym, BE_SCHEME_SYM);
    a_uri(sym_key, sch_sym, 0, ctx->be->loc.path, symbol, 0);

    // Build BASON object { relpath: "" } as merge operand
    aBpad(u8, obj, 512);
    u8cs noval = {(u8cp)"", (u8cp)""};
    call(BASONFeedInto, NULL, obj, 'O', noval);
    call(BASONFeed, NULL, obj, 'S', ctx->relpath, noval);
    call(BASONFeedOuto, NULL, obj);
    u8cs obj_val = {obj[1], obj[2]};

    call(ROCKBatchMerge, ctx->wb, sym_key, obj_val);
    return OK;
}

static ok64 BESymPost(BEp be, ROCKbatchp wb, u8cs bason, u8cs fpath) {
    sane(be != NULL && wb != NULL);
    if ($empty(bason)) done;

    // Extract relpath from fpath (skip "project/" prefix)
    size_t projlen = $len(be->loc.path);
    u8cs relpath = {fpath[0] + projlen + 1, fpath[1]};
    if ($empty(relpath)) done;

    BESymCtx ctx = {};
    ctx.be = be;
    ctx.wb = wb;
    $mv(ctx.relpath, relpath);

    call(BESymExtract, bason, BESymCB, &ctx);
    done;
}
