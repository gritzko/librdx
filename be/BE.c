#include "BE.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <utime.h>

#include "abc/ANSI.h"
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
    call(BASONFeed, idx, buf, 'I', mk, mv);
    // modeftype
    u8cs fk = {(u8cp)"f", (u8cp)"f" + 1};
    u8 fbytes[4];
    fbytes[0] = m.modeftype & 0xff;
    fbytes[1] = (m.modeftype >> 8) & 0xff;
    fbytes[2] = (m.modeftype >> 16) & 0xff;
    fbytes[3] = (m.modeftype >> 24) & 0xff;
    u8cs fv = {fbytes, fbytes + 4};
    call(BASONFeed, idx, buf, 'I', fk, fv);
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
        if (type == 'I' && $len(val) == 4) {
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

// Forward declaration: trigram POST helper
static ok64 BETriPost(BEp be, ROCKbatchp wb, u8cs bason, u8cs fpath);

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
        if (type == 'S') {
            call(u8sFeed, out, val);
        } else if (BASONPlex(type)) {
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

// ---- Branch formula parsing ----

ok64 BEFormParse(BEFormp form, u8cs query) {
    sane(form != NULL);
    form->count = 0;
    if (!$ok(query) || $empty(query)) done;
    u8cp p = query[0];
    u8cp end = query[1];
    while (p < end && form->count < BE_MAX_BRANCHES) {
        u8cp start = p;
        while (p < end && *p != '&') p++;
        if (p > start) {
            u8cs entry = {start, p};
            // Scan for '-' or '+' separator (not in RON64 alphabet)
            u8cp sep = start;
            while (sep < p && *sep != '-' && *sep != '+') sep++;
            if (sep < p && sep > start) {
                // time-origin or time+origin
                u8cs ts = {start, sep};
                u8cs orig = {sep + 1, p};
                ron60 time_val = 0;
                ron60 orig_val = 0;
                ok64 o = RONutf8sDrain(&time_val, ts);
                if (o == OK) o = RONutf8sDrain(&orig_val, orig);
                if (o == OK) {
                    form->entries[form->count].time = time_val;
                    form->entries[form->count].origin = orig_val;
                    form->count++;
                }
            } else {
                // Just origin, time=0 (match all waypoints)
                ron60 orig_val = 0;
                ok64 o = RONutf8sDrain(&orig_val, entry);
                if (o == OK) {
                    form->entries[form->count].time = 0;
                    form->entries[form->count].origin = orig_val;
                    form->count++;
                }
            }
        }
        if (p < end) p++;  // skip '&'
    }
    done;
}

// Build BEForm from be->branches[] (time=0 for all = match all waypoints)
static ok64 BEFormFromBranches(BEFormp form, BEp be) {
    sane(form != NULL && be != NULL);
    form->count = 0;
    for (int i = 0; i < be->branchc && form->count < BE_MAX_BRANCHES; i++) {
        ron60 orig = 0;
        ok64 o = RONutf8sDrain(&orig, be->branches[i]);
        if (o == OK) {
            form->entries[form->count].time = 0;
            form->entries[form->count].origin = orig;
            form->count++;
        }
    }
    done;
}

b8 BEFormMatch(BEFormp form, BERev wp) {
    for (int i = 0; i < form->count; i++) {
        if (form->entries[i].origin == wp.origin) {
            if (form->entries[i].time == 0) return YES;
            if (wp.time <= form->entries[i].time) return YES;
        }
    }
    return NO;
}

// ---- BEScan / BEScanChanged ----

// Internal: flush accumulated base + waypoints for one file
static ok64 BEScanFlush(BEp be, BEFormp form, u8cs relpath,
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
    u8 fpbuf[256];
    u8s fps = {fpbuf, fpbuf + sizeof(fpbuf)};
    call(u8sFeed, fps, be->loc.path);
    u8sFeed1(fps, '/');
    call(u8sFeed, fps, relpath);
    u8cs fpath = {fpbuf, fps[0]};

    a_cstr(sch_stat, BE_SCHEME_STAT);
    // Read stat: base
    a_uri(stat_base, sch_stat, 0, fpath, 0, 0);
    u8bp sbuf = be->scratch[BE_WRAP];
    u8bReset(sbuf);
    if (ROCKGet(&be->db, sbuf, stat_base) == OK) {
        u8cp s0 = sbuf[1], s1 = sbuf[2];
        u8cs sv = {s0, s1};
        BEMetaDrainBason(&meta, sv);
    }
    // Prefix scan stat: waypoints
    u8 spfxbuf[512];
    u8s spfx = {spfxbuf, spfxbuf + sizeof(spfxbuf)};
    call(BEKeyBuild, spfx, sch_stat, fpath, 0, 0);
    u8sFeed1(spfx, '?');
    u8cs stat_prefix = {spfxbuf, spfx[0]};

    ROCKiter sit = {};
    call(ROCKIterOpen, &sit, &be->db);
    call(ROCKIterSeek, &sit, stat_prefix);
    while (ROCKIterValid(&sit)) {
        u8cs sk = {};
        ROCKIterKey(&sit, sk);
        if ($len(sk) < $len(stat_prefix) ||
            memcmp(sk[0], stat_prefix[0], $len(stat_prefix)) != 0)
            break;
        uri sku = {};
        ok64 o = URIutf8Drain(sk, &sku);
        if (o == OK && !$empty(sku.query)) {
            ron60 stamp = 0;
            u8cs br = {};
            o = BEQueryParse(&stamp, br, sku.query);
            if (o == OK) {
                ron60 br_val = 0;
                ok64 o2 = RONutf8sDrain(&br_val, br);
                if (o2 == OK) {
                    BERev wp_id = {stamp, br_val};
                    if (BEFormMatch(form, wp_id)) {
                        u8cs sv = {};
                        ROCKIterVal(&sit, sv);
                        BEMetaDrainBason(&meta, sv);
                    }
                }
            }
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
    BEForm form = {};
    u8cs locq = {loc->query[0], loc->query[1]};
    if ($ok(locq) && !$empty(locq)) {
        call(BEFormParse, &form, locq);
    } else {
        call(BEFormFromBranches, &form, be);
    }

    // Build be:<path>/ prefix
    a_cstr(sch_be, BE_SCHEME_BE);
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, sch_be);
    u8sFeed1(pfx, ':');
    call(u8sFeed, pfx, loc->path);
    u8sFeed1(pfx, '/');
    u8cs prefix = {pfxbuf, pfx[0]};
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
    call(ROCKIterSeek, &it, prefix);

    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(prefix) ||
            memcmp(k[0], prefix[0], $len(prefix)) != 0)
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
                ok64 fo = BEScanFlush(be, &form, cur_relpath,
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
            ron60 stamp = 0;
            u8cs branch = {};
            o = BEQueryParse(&stamp, branch, ku.query);
            if (o == OK) {
                ron60 br_val = 0;
                ok64 o2 = RONutf8sDrain(&br_val, branch);
                if (o2 == OK) {
                    BERev wp_id = {stamp, br_val};
                    if (BEFormMatch(&form, wp_id) && cur_wpc < 256) {
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
            }
        }

        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);

    // Flush last file
    if ($ok(cur_relpath) && !$empty(cur_relpath)) {
        if (!changed_only || cur_wpc > 0) {
            call(BEScanFlush, be, &form, cur_relpath,
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

// Export BASON content + metadata to a file in worktree
static ok64 BEExportFile(BEp be, u8cs relpath, u8cs bason, BEmeta meta) {
    sane(be != NULL && $ok(relpath) && $ok(bason));

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

// GET single file: read metadata from stat:, content from be:, merge
// result receives pure BASON. meta_out receives latest metadata.
ok64 BEGetFileMerged(BEp be, u8cs project, u8cs relpath,
                     u8bp result, BEmeta *meta_out) {
    sane(be != NULL);

    // 1. Build file path component
    u8 fpbuf[256];
    u8s fps = {fpbuf, fpbuf + sizeof(fpbuf)};
    call(u8sFeed, fps, project);
    u8sFeed1(fps, '/');
    if ($ok(relpath) && !$empty(relpath)) call(u8sFeed, fps, relpath);
    u8cs fpath = {fpbuf, fps[0]};
    // Read base from be: key
    a_cstr(sch_be, BE_SCHEME_BE);
    a_uri(base_key, sch_be, 0, fpath, 0, 0);

    u8bp bbuf = be->scratch[BE_READ];
    u8bReset(bbuf);
    ok64 go = ROCKGet(&be->db, bbuf, base_key);
    b8 has_base = (go == OK);

    // Read latest metadata from stat: base key
    a_cstr(sch_stat, BE_SCHEME_STAT);
    a_uri(stat_key, sch_stat, 0, fpath, 0, 0);

    BEmeta latest_meta = {};
    u8bp sbuf = be->scratch[BE_WRAP];
    u8bReset(sbuf);
    if (ROCKGet(&be->db, sbuf, stat_key) == OK) {
        u8cp s0 = sbuf[1], s1 = sbuf[2];
        u8cs stat_val = {s0, s1};
        BEMetaDrainBason(&latest_meta, stat_val);
    }

    // 2. Prefix scan for be: waypoints (be:path?...)
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(BEKeyBuild, pfx, sch_be, fpath, 0, 0);
    u8sFeed1(pfx, '?');
    u8cs prefix = {pfxbuf, pfx[0]};

    // Collect matching waypoint values into a staging buffer
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

        // Extract branch from the key (strip scheme prefix first)
        u8cs branch = {};
        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o == OK && !$empty(ku.query)) {
            o = BEQueryParse(NULL, branch, ku.query);
        }
        if (o == OK && BEBranchVisible(be, branch)) {
            u8cs v = {};
            ROCKIterVal(&it, v);
            u8cp start = wpbuf[2];
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

    // Scan stat: waypoints for latest metadata
    u8 spfxbuf[512];
    u8s spfx = {spfxbuf, spfxbuf + sizeof(spfxbuf)};
    call(BEKeyBuild, spfx, sch_stat, fpath, 0, 0);
    u8sFeed1(spfx, '?');
    u8cs stat_prefix = {spfxbuf, spfx[0]};

    ROCKiter sit = {};
    call(ROCKIterOpen, &sit, &be->db);
    call(ROCKIterSeek, &sit, stat_prefix);
    while (ROCKIterValid(&sit)) {
        u8cs sk = {};
        ROCKIterKey(&sit, sk);
        if ($len(sk) < $len(stat_prefix) ||
            memcmp(sk[0], stat_prefix[0], $len(stat_prefix)) != 0)
            break;
        uri sku = {};
        ok64 o = URIutf8Drain(sk, &sku);
        if (o == OK && !$empty(sku.query)) {
            u8cs br = {};
            o = BEQueryParse(NULL, br, sku.query);
            if (o == OK && BEBranchVisible(be, br)) {
                u8cs sv = {};
                ROCKIterVal(&sit, sv);
                BEMetaDrainBason(&latest_meta, sv);
            }
        }
        call(ROCKIterNext, &sit);
    }
    call(ROCKIterClose, &sit);

    if (!has_base && wpc == 0) fail(BEnone);

    if (meta_out) *meta_out = latest_meta;

    // Pure BASON values — no prefix stripping needed
    u8cs base_bason = {};
    if (has_base) {
        u8cp b0 = bbuf[1], b1 = bbuf[2];
        base_bason[0] = b0;
        base_bason[1] = b1;
    }

    if (wpc == 0) {
        call(u8sFeed, u8bIdle(result), base_bason);
        done;
    }

    // Build inputs array: [base, wp0, wp1, ...]
    int total = (has_base ? 1 : 0) + wpc;
    u8cs inputs[total];
    int idx = 0;
    if (has_base) {
        $mv(inputs[idx], base_bason);
        idx++;
    }
    for (int i = 0; i < wpc; i++) {
        $mv(inputs[idx], waypoints[i]);
        idx++;
    }

    u8css in_css = {inputs, inputs + total};
    call(BASONMergeN, result, NULL, in_css);
    done;
}

static void BEPostReport(u8cs rel, u8cs ext, const char *status, u8 color);

static ok64 BEGetFile(BEp be, u8cs relpath) {
    sane(be != NULL);

    u8cs basename = {};
    path8gBase(basename, (path8cg){relpath[0], relpath[1], relpath[1]});
    u8cs ext = {};
    BEExtOf(ext, basename);

    u8bp mbuf = be->scratch[BE_PATCH];
    u8bReset(mbuf);

    BEmeta meta = {};
    ok64 go = BEGetFileMerged(be, be->loc.path, relpath, mbuf, &meta);
    if (go == BEnone) return BEnone;
    if (go != OK) {
        BEPostReport(relpath, ext, "FAIL", DARK_RED);
        return go;
    }

    u8cs merged = {mbuf[1], mbuf[2]};
    if ($empty(merged)) return BEnone;

    ok64 eo = BEExportFile(be, relpath, merged, meta);
    if (eo != OK) {
        BEPostReport(relpath, ext, "FAIL", DARK_RED);
        return eo;
    }
    BEPostReport(relpath, ext, "OK", DARK_GREEN);
    done;
}

// BEScan callback for GET: export each file to worktree
static ok64 BEExportCB(voidp arg, u8cs relpath, u8cs bason, BEmeta meta) {
    BEp be = (BEp)arg;
    u8cs basename = {};
    path8gBase(basename, (path8cg){relpath[0], relpath[1], relpath[1]});
    u8cs ext = {};
    BEExtOf(ext, basename);
    ok64 o = BEExportFile(be, relpath, bason, meta);
    if (o == OK)
        BEPostReport(relpath, ext, "OK", DARK_GREEN);
    else
        BEPostReport(relpath, ext, "FAIL", DARK_RED);
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

// ---- STATUS (compare worktree vs DB) ----

ok64 BEStatusFiles(BEp be) {
    sane(be != NULL);

    // Build stat: prefix for this project
    a_cstr(sch_stat, BE_SCHEME_STAT);
    u8 spfxbuf[512];
    u8s spfx = {spfxbuf, spfxbuf + sizeof(spfxbuf)};
    call(u8sFeed, spfx, sch_stat);
    u8sFeed1(spfx, ':');
    call(u8sFeed, spfx, be->loc.path);
    u8sFeed1(spfx, '/');
    u8cs stat_prefix = {spfxbuf, spfx[0]};

    ROCKiter sit = {};
    call(ROCKIterOpen, &sit, &be->db);
    call(ROCKIterSeek, &sit, stat_prefix);
    while (ROCKIterValid(&sit)) {
        u8cs sk = {};
        ROCKIterKey(&sit, sk);
        if ($len(sk) < $len(stat_prefix) ||
            memcmp(sk[0], stat_prefix[0], $len(stat_prefix)) != 0)
            break;

        // Parse the stat: URI key
        uri sku = {};
        ok64 o = URIutf8Drain(sk, &sku);
        if (o != OK || !$empty(sku.query)) {
            ROCKIterNext(&sit);
            continue;  // skip waypoints, only base keys
        }

        // Extract relpath: strip project prefix from path
        u8cs relpath = {};
        u8cp rp = sku.path[0] + $len(be->loc.path);
        if (rp < sku.path[1] && *rp == '/') rp++;
        relpath[0] = rp;
        relpath[1] = sku.path[1];
        if ($empty(relpath)) {
            ROCKIterNext(&sit);
            continue;
        }

        // Get stored mtime
        u8cs sv = {};
        ROCKIterVal(&sit, sv);
        BEmeta meta = {};
        BEMetaDrainBason(&meta, sv);

        // Get extension for report
        u8cs basename = {};
        path8gBase(basename, (path8cg){relpath[0], relpath[1], relpath[1]});
        u8cs ext = {};
        BEExtOf(ext, basename);

        // Build worktree path and stat
        a_path(fpath, "");
        call(path8gDup, path8gIn(fpath), path8cgIn(be->work_pp));
        a_path(rp_path, "");
        call(u8sFeed, u8bIdle(rp_path), relpath);
        call(path8gTerm, path8gIn(rp_path));
        call(path8gAdd, path8gIn(fpath), path8cgIn(rp_path));

        struct stat fst;
        if (FILEStat(&fst, path8cgIn(fpath)) != OK) {
            BEPostReport(relpath, ext, "DEL", DARK_RED);
        } else if ((u32)fst.st_mtime != meta.mtime) {
            BEPostReport(relpath, ext, "MOD", DARK_YELLOW);
        }

        ROCKIterNext(&sit);
    }
    ROCKIterClose(&sit);
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

ok64 BEDiffFiles(BEp be, int pathc, u8cs *paths) {
    sane(be != NULL);

    // Build stat: prefix for this project
    a_cstr(sch_stat, BE_SCHEME_STAT);
    u8 spfxbuf[512];
    u8s spfx = {spfxbuf, spfxbuf + sizeof(spfxbuf)};
    call(u8sFeed, spfx, sch_stat);
    u8sFeed1(spfx, ':');
    call(u8sFeed, spfx, be->loc.path);
    u8sFeed1(spfx, '/');
    u8cs stat_prefix = {spfxbuf, spfx[0]};

    u8cs HDRESC = $u8str("\033[1m");
    u8cs DELESC = $u8str("\033[9;31m");
    u8cs RST = $u8str("\033[0m");
    u8cs SEP = $u8str("\033[34m---\033[0m\n");

    b8 any_output = NO;

    ROCKiter sit = {};
    call(ROCKIterOpen, &sit, &be->db);
    call(ROCKIterSeek, &sit, stat_prefix);
    while (ROCKIterValid(&sit)) {
        u8cs sk = {};
        ROCKIterKey(&sit, sk);
        if ($len(sk) < $len(stat_prefix) ||
            memcmp(sk[0], stat_prefix[0], $len(stat_prefix)) != 0)
            break;

        // Parse the stat: URI key — skip waypoints (only base keys)
        uri sku = {};
        ok64 o = URIutf8Drain(sk, &sku);
        if (o != OK || !$empty(sku.query)) {
            ROCKIterNext(&sit);
            continue;
        }

        // Extract relpath
        u8cs relpath = {};
        u8cp rp = sku.path[0] + $len(be->loc.path);
        if (rp < sku.path[1] && *rp == '/') rp++;
        relpath[0] = rp;
        relpath[1] = sku.path[1];
        if ($empty(relpath)) {
            ROCKIterNext(&sit);
            continue;
        }

        // Filter by requested paths
        if (!BEDiffPathMatch(relpath, pathc, paths)) {
            ROCKIterNext(&sit);
            continue;
        }

        // Get stored mtime
        u8cs sv = {};
        ROCKIterVal(&sit, sv);
        BEmeta meta = {};
        BEMetaDrainBason(&meta, sv);

        // Build worktree path and stat
        a_path(fpath, "");
        call(path8gDup, path8gIn(fpath), path8cgIn(be->work_pp));
        a_path(rp_path, "");
        call(u8sFeed, u8bIdle(rp_path), relpath);
        call(path8gTerm, path8gIn(rp_path));
        call(path8gAdd, path8gIn(fpath), path8cgIn(rp_path));

        struct stat fst;
        b8 deleted = NO;
        b8 modified = NO;
        if (FILEStat(&fst, path8cgIn(fpath)) != OK) {
            deleted = YES;
        } else if ((u32)fst.st_mtime != meta.mtime) {
            modified = YES;
        }

        if (!deleted && !modified) {
            ROCKIterNext(&sit);
            continue;
        }

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

        // File header
        if (any_output) call(FILEout, SEP);
        call(FILEout, HDRESC);
        call(FILEout, relpath);
        call(FILEout, RST);
        call(FILEout, NL);
        any_output = YES;

        if (deleted) {
            // Show entire old content in red strikethrough
            if (!$empty(old_bason)) {
                u8bp rbuf = be->scratch[BE_RENDER];
                u8bReset(rbuf);
                aBpad(u64, stk, 256);
                call(BASTExport, u8bIdle(rbuf), stk, old_bason);
                u8cp r0 = rbuf[1], r1 = rbuf[2];
                u8cs rendered = {r0, r1};
                call(FILEout, DELESC);
                call(FILEout, rendered);
                call(FILEout, RST);
                call(FILEout, NL);
            }
        } else {
            // Modified: line-level diff of old text vs worktree
            u8bp mapbuf = NULL;
            call(FILEMapRO, &mapbuf, path8cgIn(fpath));
            u8cs new_text = {mapbuf[1], mapbuf[2]};

            // Export old BASON to text
            u8cs old_text = {};
            if (!$empty(old_bason)) {
                u8bp rbuf = be->scratch[BE_RENDER];
                u8bReset(rbuf);
                aBpad(u64, stk, 256);
                call(BASTExport, u8bIdle(rbuf), stk, old_bason);
                u8cp r0 = rbuf[1], r1 = rbuf[2];
                old_text[0] = r0;
                old_text[1] = r1;
            }

            // Line-level diff with 3 context lines
            u8bp dbuf = be->scratch[BE_PATCH];
            u8bReset(dbuf);
            call(BASTTextDiff, u8bIdle(dbuf), old_text, new_text, 3);
            u8cp d0 = dbuf[1], d1 = dbuf[2];
            u8cs rendered = {d0, d1};
            if (!$empty(rendered)) {
                call(FILEout, rendered);
                if (rendered[1][-1] != '\n')
                    call(FILEout, NL);
            }

            FILEUnMap(mapbuf);
        }

        ROCKIterNext(&sit);
    }
    ROCKIterClose(&sit);
    done;
}

// ---- POST (worktree -> repo) ----

typedef struct {
    BEp be;
    ROCKbatchp wb;
    ron60 stamp;
    ignop ig;
} BEPostCtx;

// Report file status: "STAT codc path\n" (4+1+4+1 = 10 char prefix)
static void BEPostReport(u8cs rel, u8cs ext, const char *status, u8 color) {
    u8 lbuf[512];
    u8s ls = {lbuf, lbuf + sizeof(lbuf)};
    // Status: up to 4 chars, colored, padded
    escfeed(ls, color);
    int si = 0;
    while (status[si] && si < 4) {
        u8sFeed1(ls, (u8)status[si]);
        si++;
    }
    escfeed(ls, 0);
    while (si < 4) { u8sFeed1(ls, ' '); si++; }
    u8sFeed1(ls, ' ');
    // Codec: up to 4 chars, gray, padded
    escfeed(ls, GRAY);
    int ci = 0;
    if (!$empty(ext)) {
        u8cp ce = ext[0];
        if (*ce == '.') ce++;
        while (ce < ext[1] && ci < 4) {
            u8sFeed1(ls, *ce);
            ce++;
            ci++;
        }
    } else {
        u8cs tc = {(u8cp)"text", (u8cp)"text" + 4};
        u8sFeed(ls, tc);
        ci = 4;
    }
    escfeed(ls, 0);
    while (ci < 4) { u8sFeed1(ls, ' '); ci++; }
    u8sFeed1(ls, ' ');
    // Path
    u8sFeed(ls, rel);
    u8sFeed1(ls, '\n');
    u8cs line = {lbuf, ls[0]};
    FILEout(line);
}

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
        u8cs nokey = {(u8cp)"", (u8cp)""};
        call(BASONFeedInto, NULL, nbuf, 'A', nokey);
        call(BASONFeedOuto, NULL, nbuf);
    } else {
        call(FILEMapRO, &mapbuf, filepath);
        u8cs source = {mapbuf[1], mapbuf[2]};
        o = BASTParse(nbuf, NULL, source, ext);
        if (o != OK) {
            BEPostReport(rel, ext, "SKIP", DARK_YELLOW);
            FILEUnMap(mapbuf);
            done;
        }
    }
    u8cp nb0 = nbuf[1], nb1 = nbuf[2];
    u8cs new_bason = {nb0, nb1};

    // Build file path component for keys
    u8 fpbuf[256];
    u8s fps = {fpbuf, fpbuf + sizeof(fpbuf)};
    call(u8sFeed, fps, be->loc.path);
    u8sFeed1(fps, '/');
    if ($ok(rel) && !$empty(rel)) call(u8sFeed, fps, rel);
    u8cs fpath = {fpbuf, fps[0]};

    // Build query: stamp-branch
    u8cs active = {be->branches[0][0], be->branches[0][1]};
    u8 wqbuf[128];
    u8s wqs = {wqbuf, wqbuf + sizeof(wqbuf)};
    call(BEQueryBuild, wqs, stamp, active);
    u8cs wquery = {wqbuf, wqs[0]};

    // Get current merged state for diff
    u8bp pbuf = be->scratch[BE_PATCH];
    u8bReset(pbuf);
    ok64 go = BEGetFileMerged(be, be->loc.path, rel, pbuf, NULL);
    u8cs old_bason = {};
    if (go == OK) {
        u8cp md0 = pbuf[1], md1 = pbuf[2];
        old_bason[0] = md0;
        old_bason[1] = md1;
    }

    a_cstr(sch_be, BE_SCHEME_BE);
    a_cstr(sch_stat, BE_SCHEME_STAT);

    if (be->initial) {
        // Initial import: write base keys (no query)
        a_uri(be_key, sch_be, 0, fpath, 0, 0);
        call(ROCKBatchPut, wb, be_key, new_bason);
        a_uri(stat_key, sch_stat, 0, fpath, 0, 0);
        u8bp mbuf = be->scratch[BE_WRAP];
        u8bReset(mbuf);
        call(BEMetaFeedBason, mbuf, NULL, meta);
        u8cs stat_val = {mbuf[1], mbuf[2]};
        call(ROCKBatchPut, wb, stat_key, stat_val);
    } else if ($ok(old_bason) && !$empty(old_bason)) {
        // Existing file changed: diff and write waypoint
        u8bp obuf = be->scratch[BE_READ];
        u8bReset(obuf);
        aBpad(u64, ostk, 256);
        aBpad(u64, nstk, 256);
        o = BASONDiff(obuf, NULL, ostk, old_bason, nstk, new_bason);
        if (o != OK) {
            if (mapbuf) FILEUnMap(mapbuf);
            fail(o);
        }
        u8cs delta = {obuf[1], obuf[2]};
        if ($empty(delta)) {
            if (mapbuf) FILEUnMap(mapbuf);
            done;
        }
        a_uri(wp_key, sch_be, 0, fpath, wquery, 0);
        call(ROCKBatchPut, wb, wp_key, delta);
        a_uri(stat_key, sch_stat, 0, fpath, wquery, 0);
        u8bp mbuf = be->scratch[BE_WRAP];
        u8bReset(mbuf);
        call(BEMetaFeedBason, mbuf, NULL, meta);
        u8cs stat_val = {mbuf[1], mbuf[2]};
        call(ROCKBatchPut, wb, stat_key, stat_val);
    } else {
        // New file after initial import: full BASON as waypoint
        a_uri(wp_key, sch_be, 0, fpath, wquery, 0);
        call(ROCKBatchPut, wb, wp_key, new_bason);
        a_uri(stat_key, sch_stat, 0, fpath, wquery, 0);
        u8bp mbuf = be->scratch[BE_WRAP];
        u8bReset(mbuf);
        call(BEMetaFeedBason, mbuf, NULL, meta);
        u8cs stat_val = {mbuf[1], mbuf[2]};
        call(ROCKBatchPut, wb, stat_key, stat_val);
    }

    // Extract trigrams and write tri: keys
    call(BETriPost, be, wb, new_bason, fpath);

    // Commit per file to bound memory
    call(ROCKBatchWrite, &be->db, wb);
    rocksdb_writebatch_clear(wb->wb);

    // Force memtable flush to prevent merge-operand buildup
    rocksdb_flushoptions_t *fopt = rocksdb_flushoptions_create();
    if (fopt) {
        char *ferr = NULL;
        rocksdb_flush(be->db.db, fopt, &ferr);
        if (ferr) free(ferr);
        rocksdb_flushoptions_destroy(fopt);
    }

    BEPostReport(rel, ext, "OK", DARK_GREEN);

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
                u8cs noext = {};
                BEPostReport(rel, noext, "IGN", DARK_YELLOW);
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
                u8cs basename = {};
                path8gBase(basename, path8cgIn(path));
                u8cs ext = {};
                BEExtOf(ext, basename);
                BEPostReport(rel, ext, "IGN", DARK_YELLOW);
                return OK;
            }
        }
    }
    ok64 po = BEPostFile(ctx->be, ctx->wb, ctx->stamp, path8cgIn(path));
    if (po != OK) {
        a_path(relpath, "");
        path8gRelative(path8gIn(relpath), path8cgIn(ctx->be->work_pp),
                        path8cgIn(path));
        u8cs rel = {relpath[1], relpath[2]};
        u8cs basename = {};
        path8gBase(basename, path8cgIn(path));
        u8cs ext = {};
        BEExtOf(ext, basename);
        BEPostReport(rel, ext, "FAIL", DARK_RED);
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
            // Check if arg contains '/' — use path8gAdd for multi-component
            u8cs argscan = {paths[i][0], paths[i][1]};
            if (u8csFind(argscan, '/') == OK) {
                // Strip trailing '/' if present
                u8cs arg = {paths[i][0], paths[i][1]};
                while (!$empty(arg) && *(arg[1] - 1) == '/') arg[1]--;
                a_path(rp, "");
                call(u8sFeed, u8bIdle(rp), arg);
                call(path8gTerm, path8gIn(rp));
                call(path8gAdd, path8gIn(apath), path8cgIn(rp));
            } else {
                call(path8gPush, path8gIn(apath), paths[i]);
            }
            // Check if directory — scan it
            struct stat dst;
            ok64 so = FILEStat(&dst, path8cgIn(apath));
            if (so == OK && S_ISDIR(dst.st_mode)) {
                igno gi = {};
                u8cs workdir = {be->work_pp[1], be->work_pp[2]};
                IGNOLoad(&gi, workdir);
                BEPostCtx ctx = {be, &wb, stamp, &gi};
                ok64 o = FILEScan(apath,
                                  (FILE_SCAN)(FILE_SCAN_FILES | FILE_SCAN_DIRS |
                                              FILE_SCAN_DEEP),
                                  BEPostScanCB, &ctx);
                IGNOFree(&gi);
                if (o != OK) {
                    ROCKBatchClose(&wb);
                    fail(o);
                }
            } else {
                ok64 o = BEPostFile(be, &wb, stamp, path8cgIn(apath));
                if (o != OK) {
                    ROCKBatchClose(&wb);
                    fail(o);
                }
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

    // Store commit metadata: be:project/?stamp-branch#commit
    u8cs active = {be->branches[0][0],
                   be->branches[0][1]};
    u8 cpbuf[256];
    u8s cps = {cpbuf, cpbuf + sizeof(cpbuf)};
    call(u8sFeed, cps, be->loc.path);
    u8sFeed1(cps, '/');
    u8 cqbuf[128];
    u8s cqs = {cqbuf, cqbuf + sizeof(cqbuf)};
    call(BEQueryBuild, cqs, stamp, active);
    a_cstr(sch_be, BE_SCHEME_BE);
    a_cstr(meta_commit, "commit");
    u8cs cp = {cpbuf, cps[0]};
    u8cs cq = {cqbuf, cqs[0]};
    a_uri(commit_key, sch_be, 0, cp, cq, meta_commit);
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

// ---- PUT (merge source branch into active) ----

// Helper: re-key all waypoints of a scheme from source branch to active branch
static ok64 BEPutScheme(BEp be, ROCKbatchp wb, u8cs scheme,
                         u8cs source_branch, u8cs active) {
    sane(be != NULL && wb != NULL);
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, scheme);
    u8sFeed1(pfx, ':');
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

        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o != OK || $empty(ku.query) || !$empty(ku.fragment)) {
            call(ROCKIterNext, &it);
            continue;
        }

        u8cs branch = {};
        ron60 wp_stamp = 0;
        o = BEQueryParse(&wp_stamp, branch, ku.query);
        if (o != OK || !$eq(branch, source_branch)) {
            call(ROCKIterNext, &it);
            continue;
        }

        u8cs v = {};
        ROCKIterVal(&it, v);

        // Build new key under active branch
        u8 nqbuf[128];
        u8s nqs = {nqbuf, nqbuf + sizeof(nqbuf)};
        call(BEQueryBuild, nqs, wp_stamp, active);
        u8cs nq = {nqbuf, nqs[0]};
        a_uri(new_key, scheme, 0, ku.path, nq, 0);
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
    u8cs active = {be->branches[0][0],
                   be->branches[0][1]};

    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);

    // Re-key both stat: and be: waypoints from source → active
    a_cstr(sch_be, BE_SCHEME_BE);
    a_cstr(sch_stat, BE_SCHEME_STAT);
    call(BEPutScheme, be, &wb, sch_be, source_branch, active);
    call(BEPutScheme, be, &wb, sch_stat, source_branch, active);

    // Merge metadata: be:project/?stamp-branch#merge
    u8 mpbuf[256];
    u8s mps = {mpbuf, mpbuf + sizeof(mpbuf)};
    call(u8sFeed, mps, be->loc.path);
    u8sFeed1(mps, '/');
    u8 mqbuf[128];
    u8s mqs = {mqbuf, mqbuf + sizeof(mqbuf)};
    call(BEQueryBuild, mqs, stamp, active);
    a_cstr(meta_merge, "merge");
    u8cs mp = {mpbuf, mps[0]};
    u8cs mq = {mqbuf, mqs[0]};
    a_uri(merge_key, sch_be, 0, mp, mq, meta_merge);
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
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    a_cstr(sch_be, BE_SCHEME_BE);
    if (u8sFeed(pfx, sch_be) != OK) return NO;
    u8sFeed1(pfx, ':');
    if (u8sFeed(pfx, be->loc.path) != OK) return NO;
    u8sFeed1(pfx, '/');
    u8cs project_prefix = {pfxbuf, pfx[0]};

    ROCKiter it = {};
    if (ROCKIterOpen(&it, &be->db) != OK) return NO;
    if (ROCKIterSeek(&it, project_prefix) != OK) {
        ROCKIterClose(&it);
        return NO;
    }

    b8 found = NO;
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(project_prefix) ||
            memcmp(k[0], project_prefix[0], $len(project_prefix)) != 0)
            break;
        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o == OK && !$empty(ku.query)) {
            u8cs branch = {};
            o = BEQueryParse(NULL, branch, ku.query);
            if (o == OK && $eq(branch, name)) {
                found = YES;
                break;
            }
        }
        ROCKIterNext(&it);
    }
    ROCKIterClose(&it);
    return found;
}

// Delete all waypoint keys for a branch under a specific scheme prefix
static ok64 BEDeleteBranchScheme(BEp be, ROCKbatchp wb,
                                  u8cs scheme, u8cs name) {
    sane(be != NULL && wb != NULL);
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, scheme);
    u8sFeed1(pfx, ':');
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
        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o == OK && !$empty(ku.query)) {
            u8cs branch = {};
            o = BEQueryParse(NULL, branch, ku.query);
            if (o == OK && $eq(branch, name)) {
                call(ROCKBatchDel, wb, k);
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
    call(BEDeleteBranchScheme, be, &wb, sch_be, name);
    call(BEDeleteBranchScheme, be, &wb, sch_stat, name);
    call(ROCKBatchWrite, &be->db, &wb);
    call(ROCKBatchClose, &wb);
    done;
}

static ok64 BEDeleteFile(BEp be, u8cs target) {
    sane(be != NULL && $ok(target) && !$empty(target));

    ron60 stamp = RONNow();
    u8cs active = {be->branches[0][0], be->branches[0][1]};

    // Build path component
    u8 dpbuf[256];
    u8s dps = {dpbuf, dpbuf + sizeof(dpbuf)};
    call(u8sFeed, dps, be->loc.path);
    u8sFeed1(dps, '/');
    call(u8sFeed, dps, target);
    u8cs fpath = {dpbuf, dps[0]};

    // Build query
    u8 dqbuf[128];
    u8s dqs = {dqbuf, dqbuf + sizeof(dqbuf)};
    call(BEQueryBuild, dqs, stamp, active);
    u8cs wquery = {dqbuf, dqs[0]};
    u8cs empty_val = {NULL, NULL};

    // Tombstone: empty value in be: key
    a_cstr(sch_be, BE_SCHEME_BE);
    a_uri(be_key, sch_be, 0, fpath, wquery, 0);
    call(ROCKPut, &be->db, be_key, empty_val);

    // Zero metadata in stat: key
    a_cstr(sch_stat, BE_SCHEME_STAT);
    a_uri(stat_key, sch_stat, 0, fpath, wquery, 0);
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
    call(path8gDup, path8gIn(fpath), path8cgIn(be->work_pp));
    // Use path8gAdd for multi-component paths (may contain /)
    a_path(rpath, "");
    call(u8sFeed, u8bIdle(rpath), target);
    call(path8gTerm, path8gIn(rpath));
    call(path8gAdd, path8gIn(fpath), path8cgIn(rpath));

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
    ok64 __ = OK;
    BEMileCBCtx *ctx = (BEMileCBCtx *)arg;
    BEp be = ctx->be;

    // Build full path component: project/relpath
    u8 fpbuf[256];
    u8s fps = {fpbuf, fpbuf + sizeof(fpbuf)};
    __ = u8sFeed(fps, be->loc.path);
    if (__ != OK) return __;
    u8sFeed1(fps, '/');
    if (!$empty(relpath)) {
        __ = u8sFeed(fps, relpath);
        if (__ != OK) return __;
    }
    u8cs fpath = {fpbuf, fps[0]};

    // Write new be: base
    a_cstr(sch_be, BE_SCHEME_BE);
    a_uri(base_key, sch_be, 0, fpath, 0, 0);
    __ = ROCKBatchPut(ctx->wb, base_key, bason);
    if (__ != OK) return __;

    // Write new stat: base with metadata
    a_cstr(sch_stat, BE_SCHEME_STAT);
    a_uri(stat_key, sch_stat, 0, fpath, 0, 0);
    aBpad(u8, smbuf, 256);
    __ = BEMetaFeedBason(smbuf, NULL, meta);
    if (__ != OK) return __;
    u8cs stat_val = {smbuf[1], smbuf[2]};
    __ = ROCKBatchPut(ctx->wb, stat_key, stat_val);
    return __;
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

    // Phase 2: delete main-branch waypoint keys (be: and stat:)
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, sch_be);
    u8sFeed1(pfx, ':');
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
        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o == OK && !$empty(ku.query) && $empty(ku.fragment)) {
            u8cs branch = {};
            o = BEQueryParse(NULL, branch, ku.query);
            if (o == OK && $eq(branch, main_branch)) {
                call(ROCKBatchDel, &wb, k);
            }
        }
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);

    // Also delete stat: waypoints for main branch
    u8 spfxbuf[512];
    u8s spfx = {spfxbuf, spfxbuf + sizeof(spfxbuf)};
    call(u8sFeed, spfx, sch_stat);
    u8sFeed1(spfx, ':');
    call(u8sFeed, spfx, be->loc.path);
    u8sFeed1(spfx, '/');
    u8cs stat_prefix = {spfxbuf, spfx[0]};

    ROCKiter sit = {};
    call(ROCKIterOpen, &sit, &be->db);
    call(ROCKIterSeek, &sit, stat_prefix);
    while (ROCKIterValid(&sit)) {
        u8cs sk = {};
        ROCKIterKey(&sit, sk);
        if ($len(sk) < $len(stat_prefix) ||
            memcmp(sk[0], stat_prefix[0], $len(stat_prefix)) != 0)
            break;
        uri sku = {};
        ok64 o = URIutf8Drain(sk, &sku);
        if (o == OK && !$empty(sku.query) && $empty(sku.fragment)) {
            u8cs br = {};
            o = BEQueryParse(NULL, br, sku.query);
            if (o == OK && $eq(br, main_branch)) {
                call(ROCKBatchDel, &wb, sk);
            }
        }
        call(ROCKIterNext, &sit);
    }
    call(ROCKIterClose, &sit);

    // Record milestone metadata: be:project/?stamp-branch#milestone
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
        u8cs msp = {mspbuf, msps[0]};
        u8cs msq = {msqbuf, msqs[0]};
        a_uri(ms_key, sch_be, 0, msp, msq, meta_milestone);
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

    u8 hlbuf[4];
    u8s hls = {hlbuf, hlbuf + sizeof(hlbuf)};
    call(BEHashlet, hls, fpath);
    u8cs hashlet = {hlbuf, hls[0]};

    BETriCtx ctx = {};
    memset(ctx.seen, 0, sizeof(ctx.seen));
    ctx.be = be;
    ctx.wb = wb;
    $mv(ctx.hashlet, hashlet);

    call(BETriExtract, bason, BETriCB, &ctx);
    done;
}
