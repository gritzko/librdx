//  KEEP: local git object store.
//
//  Stores git packfiles under .dogs/keeper/, indexed by u64→w64
//  in LSM sorted runs of wh128 entries.
//
#include "KEEP.h"
#include "REFS.h"

#include "DELT.h"
#include "PACK.h"
#include "SHA1.h"
#include "ZINF.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/RON.h"
#include "dog/HOME.h"

// wh128 templates for LSM index runs and waiter buffers
#define X(M, name) M##wh128##name
#include "abc/QSORTx.h"
#undef X

#define KEEP_BUFSZ (1ULL << 30)  // 1 GB working buffer (mmap'd, pages on demand)

// --- Helpers ---

static ok64 keep_resolve_dir(path8b out, u8cs reporoot) {
    sane(out);
    if ($empty(reporoot)) {
        // Try HOMEFindDogs (.git-based); if that fails, walk up
        // looking for .dogs/ directory directly.
        ok64 o = HOMEFindDogs(out);
        if (o != OK) {
            // Walk up from cwd looking for .dogs/
            a_pad(u8, cwdbuf, FILE_PATH_MAX_LEN);
            test(getcwd((char *)u8bIdleHead(cwdbuf),
                        u8bIdleLen(cwdbuf)) != NULL, KEEPFAIL);
            u8bFed(cwdbuf, strlen((char *)u8bIdleHead(cwdbuf)));
            a_path(cur);
            a_dup(u8c, cwds, u8bData(cwdbuf));
            call(PATHu8bFeed, cur, cwds);

            a_cstr(dotdogs, ".dogs");
            for (;;) {
                a_path(probe);
                a_dup(u8c, curslice, u8bData(cur));
                call(PATHu8bFeed, probe, curslice);
                call(PATHu8bPush, probe, dotdogs);
                if (FILEisdir(PATHu8cgIn(probe)) == OK) {
                    a_dup(u8c, found, u8bData(cur));
                    call(PATHu8bFeed, out, found);
                    goto found_root;
                }
                size_t before = u8bDataLen(cur);
                call(PATHu8bPop, cur);
                if (u8bDataLen(cur) >= before) fail(KEEPFAIL);
            }
        }
    } else {
        call(PATHu8bFeed, out, reporoot);
    }
found_root:;
    a_cstr(rel, "/" KEEP_DIR);
    call(u8bFeed, out, rel);
    call(PATHu8gTerm, PATHu8gIn(out));
    done;
}

// --- Scan helper: find and mmap files matching extension ---

static ok64 keep_scan_dir(u8csc dir, char const *ext,
                          u8bp *maps, u32 *count, u32 max) {
    a_path(dpat);
    PATHu8bFeed(dpat, dir);
    DIR *dp = opendir((char *)u8bDataHead(dpat));
    if (!dp) return OK;  // dir doesn't exist yet

    size_t extlen = strlen(ext);
    char names[KEEP_MAX_FILES][64];
    u32 nfound = 0;
    struct dirent *e;
    while ((e = readdir(dp)) != NULL && nfound < max) {
        size_t nlen = strlen(e->d_name);
        if (nlen <= extlen || nlen > 63) continue;
        if (strcmp(e->d_name + nlen - extlen, ext) != 0) continue;
        memcpy(names[nfound], e->d_name, nlen + 1);
        nfound++;
    }
    closedir(dp);

    // Sort by name
    for (u32 i = 0; i + 1 < nfound; i++)
        for (u32 j = i + 1; j < nfound; j++)
            if (strcmp(names[i], names[j]) > 0) {
                char tmp[64];
                memcpy(tmp, names[i], 64);
                memcpy(names[i], names[j], 64);
                memcpy(names[j], tmp, 64);
            }

    for (u32 i = 0; i < nfound && *count < max; i++) {
        u8cs fn = {(u8cp)names[i], (u8cp)names[i] + strlen(names[i])};
        a_path(fpath, dir, fn);
        u8bp mapped = NULL;
        if (FILEMapRO(&mapped, PATHu8cgIn(fpath)) == OK) {
            maps[*count] = mapped;
            (*count)++;
        }
    }
    return OK;
}

static ok64 keep_scan_packs(keeper *k, u8csc keepdir) {
    // New layout: log/*.pack
    a_pad(u8, logdir, 1024);
    u8bFeed(logdir, keepdir);
    a_cstr(logsep, "/" KEEP_LOG_DIR);
    u8bFeed(logdir, logsep);
    PATHu8gTerm(PATHu8gIn(logdir));
    a_dup(u8c, ld, u8bData(logdir));
    keep_scan_dir(ld, KEEP_PACK_EXT, k->packs, &k->npacks, KEEP_MAX_FILES);

    // Old layout: keeper/*.packs
    keep_scan_dir(keepdir, ".packs", k->packs, &k->npacks, KEEP_MAX_FILES);

    // New layout: idx/*.idx
    a_pad(u8, idxdir, 1024);
    u8bFeed(idxdir, keepdir);
    a_cstr(idxsep, "/" KEEP_IDX_DIR);
    u8bFeed(idxdir, idxsep);
    PATHu8gTerm(PATHu8gIn(idxdir));
    a_dup(u8c, id, u8bData(idxdir));

    u8bp idx_maps[KEEP_MAX_LEVELS] = {};
    u32 nidx = 0;
    keep_scan_dir(id, KEEP_IDX_EXT, idx_maps, &nidx, KEEP_MAX_LEVELS);
    // Old layout: keeper/*.idx
    keep_scan_dir(keepdir, KEEP_IDX_EXT, idx_maps, &nidx, KEEP_MAX_LEVELS);

    for (u32 i = 0; i < nidx && k->nruns < KEEP_MAX_LEVELS; i++) {
        wh128cp base = (wh128cp)u8bDataHead(idx_maps[i]);
        u32 n = (u32)(u8bDataLen(idx_maps[i]) / sizeof(wh128));
        k->runs[k->nruns][0] = base;
        k->runs[k->nruns][1] = base + n;
        k->run_maps[k->nruns] = idx_maps[i];
        k->nruns++;
    }
    return OK;
}

// --- Open: mmap pack files + load index runs ---

ok64 KEEPOpen(keeper *k, u8cs reporoot) {
    sane(k);
    memset(k, 0, sizeof(*k));

    a_path(dir);
    call(keep_resolve_dir, dir, reporoot);
    a_dup(u8c, dirdata, u8bData(dir));
    size_t dlen = u8csLen(dirdata);
    if (dlen >= sizeof(k->dir)) dlen = sizeof(k->dir) - 1;
    memcpy(k->dir, dirdata[0], dlen);
    k->dir[dlen] = 0;
    a_cstr(keepdir, k->dir);

    // Ensure directories exist
    call(FILEMakeDirP, PATHu8cgIn(dir));
    {
        a_pad(u8, logdir, 1024);
        u8bFeed(logdir, keepdir);
        a_cstr(logrel, "/" KEEP_LOG_DIR);
        u8bFeed(logdir, logrel);
        PATHu8gTerm(PATHu8gIn(logdir));
        FILEMakeDirP(PATHu8cgIn(logdir));
    }
    {
        a_pad(u8, idxdir, 1024);
        u8bFeed(idxdir, keepdir);
        a_cstr(idxrel, "/" KEEP_IDX_DIR);
        u8bFeed(idxdir, idxrel);
        PATHu8gTerm(PATHu8gIn(idxdir));
        FILEMakeDirP(PATHu8cgIn(idxdir));
    }

    // Scan pack files: log/*.pack (new) + keeper/*.packs (old compat)
    // Scan idx files: idx/*.idx (new) + keeper/*.idx (old compat)
    call(keep_scan_packs, k, keepdir);

    // Pre-allocate working buffers for KEEPGet (mmap, reset per call)
    call(u8bMap, k->buf1, KEEP_BUFSZ);
    call(u8bMap, k->buf2, KEEP_BUFSZ);
    call(u8bMap, k->buf3, KEEP_BUFSZ);
    call(u8bMap, k->buf4, KEEP_BUFSZ);

    done;
}

// --- Close ---

ok64 KEEPClose(keeper *k) {
    sane(k);
    for (u32 i = 0; i < k->npacks; i++)
        if (k->packs[i]) FILEUnMap(k->packs[i]);
    for (u32 i = 0; i < k->nruns; i++)
        if (k->run_maps[i]) FILEUnMap(k->run_maps[i]);
    if (k->buf1[0]) u8bUnMap(k->buf1);
    if (k->buf2[0]) u8bUnMap(k->buf2);
    if (k->buf3[0]) u8bUnMap(k->buf3);
    if (k->buf4[0]) u8bUnMap(k->buf4);
    memset(k, 0, sizeof(*k));
    done;
}

// --- Lookup: hashlet → wh64 val ---
// hexlen: number of significant hex chars in the hashlet (6-10).
// With 60-bit hashlets, max is 15 hex chars.

ok64 KEEPLookup(keeper *k, u64 hashlet60, size_t hexlen, u64p val) {
    sane(k && val);

    // Build range for prefix matching.
    // key = hashlet60[60] | type[4].
    // key_lo: hashlet prefix with low bits zeroed, type=0.
    // key_hi: hashlet prefix with low bits all-ones, type=0xf.
    if (hexlen > 15) hexlen = 15;
    u64 nbits = hexlen * 4;
    u64 shift = 60 - nbits;
    u64 hmask = shift < 60 ? (WHIFF_HASHLET60_MASK >> shift) << shift : WHIFF_HASHLET60_MASK;
    u64 hpre = hashlet60 & hmask;

    u64 key_lo = keepKeyPack(0, hpre);
    u64 key_hi = keepKeyPack(0xf, hpre | (~hmask & WHIFF_HASHLET60_MASK));

    for (u32 r = 0; r < k->nruns; r++) {
        wh128cp base = k->runs[r][0];
        size_t len = (size_t)(k->runs[r][1] - base);
        if (len == 0) continue;
        size_t lo = 0, hi = len;
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (base[mid].key < key_lo) lo = mid + 1;
            else hi = mid;
        }
        if (lo < len && base[lo].key >= key_lo && base[lo].key <= key_hi) {
            *val = base[lo].val;
            done;
        }
    }
    return KEEPNONE;
}

// --- Has ---

ok64 KEEPHas(keeper *k, u64 hashlet60, size_t hexlen) {
    u64 val = 0;
    return KEEPLookup(k, hashlet60, hexlen, &val);
}

// --- Resolve: inflate object at pack val (file_id + offset) ---

static ok64 KEEPGetPacked(keeper *k, u64 val, u8bp out, u8p out_type) {
    u32 file_id = wh64Id(val);
    u64 offset  = wh64Off(val);

    if (file_id < 1 || file_id > k->npacks) return KEEPNONE;
    u8bp pack_map = k->packs[file_id - 1];
    u8cp pack = u8bDataHead(pack_map);
    u64 packlen = (u64)(u8bIdleHead(pack_map) - pack);

    if (offset >= packlen) return KEEPFAIL;

    // Chase delta chain, resolve to base object
    u64 chain[256];
    int depth = 0;
    u64 cur = offset;
    u8 obj_type = 0;

    // Use pre-allocated working buffers, reset each call
    u8bReset(k->buf1);
    u8bReset(k->buf2);
    u8p buf1 = u8bHead(k->buf1);
    u8p buf2 = u8bHead(k->buf2);

    u64 outsz = 0;
    u8p result = NULL;
    ok64 rc = OK;

    for (;;) {
        pack_obj obj = {};
        u8cs from = {pack + cur, pack + packlen};
        rc = PACKDrainObjHdr(from, &obj);
        if (rc != OK) goto cleanup;

        if (obj.type >= 1 && obj.type <= 4) {
            // Base object: inflate directly
            obj_type = obj.type;
            if (obj.size > KEEP_BUFSZ) { rc = KEEPNOROOM; goto cleanup; }
            u8s into = {buf1, buf1 + KEEP_BUFSZ};
            rc = PACKInflate(from, into, obj.size);
            if (rc != OK) goto cleanup;
            result = buf1;
            outsz = obj.size;
            break;
        }

        if (depth >= 256) { rc = KEEPFAIL; goto cleanup; }
        chain[depth++] = cur;

        if (obj.type == PACK_OBJ_OFS_DELTA) {
            cur = cur - obj.ofs_delta;
        } else if (obj.type == PACK_OBJ_REF_DELTA) {
            // Look up base by SHA-1 prefix
            u64 base_hashlet = WHIFFHashlet60((sha1cp)obj.ref_delta[0]);
            u64 base_val = 0;
            rc = KEEPLookup(k, base_hashlet, 10, &base_val);
            if (rc != OK) goto cleanup;
            // Base might be in a different pack file
            u32 bfile = wh64Id(base_val);
            if (bfile != file_id) {
                // Cross-file: get base recursively, apply delta chain
                u8bReset(k->buf3);
                u8 btype = 0;
                rc = KEEPGet(k, base_hashlet, 15, k->buf3, &btype);
                if (rc != OK) goto cleanup;
                obj_type = btype;
                // buf3 has the base content; copy to buf1
                a_dup(u8c, bdata, u8bData(k->buf3));
                memcpy(buf1, bdata[0], u8csLen(bdata));
                result = buf1;
                outsz = u8csLen(bdata);
                break;  // apply delta chain from here
            }
            cur = wh64Off(base_val);
        } else {
            rc = KEEPFAIL;
            goto cleanup;
        }
    }

    // Apply delta chain bottom-up
    u8p src = buf1;
    u8p dst = buf2;

    for (int i = depth - 1; i >= 0; i--) {
        pack_obj dobj = {};
        u8cs from = {pack + chain[i], pack + packlen};
        rc = PACKDrainObjHdr(from, &dobj);
        if (rc != OK) goto cleanup;

        u8p dinst = dst + KEEP_BUFSZ / 2;
        if (dobj.size > KEEP_BUFSZ / 2) { rc = KEEPNOROOM; goto cleanup; }
        u8s dinto = {dinst, dinst + KEEP_BUFSZ / 2};
        rc = PACKInflate(from, dinto, dobj.size);
        if (rc != OK) goto cleanup;

        u8cs delta = {dinst, dinst + dobj.size};
        u8cs base = {src, src + outsz};
        u8g apply_out = {dst, dst, dst + KEEP_BUFSZ / 2};
        rc = DELTApply(delta, base, apply_out);
        if (rc != OK) goto cleanup;
        outsz = u8gLeftLen(apply_out);

        u8p tmp = src; src = dst; dst = tmp;
    }

    result = src;
    if (out_type) *out_type = obj_type;

    // Copy result into caller's output buffer
    {
        u8cs content = {result, result + outsz};
        rc = u8bFeed(out, content);
    }

cleanup:
    return rc;
}

// --- Get: inflate object from pack by hashlet ---

ok64 KEEPGet(keeper *k, u64 hashlet, size_t hexlen, u8bp out, u8p out_type) {
    sane(k && out);

    u64 val = 0;
    ok64 lo = KEEPLookup(k, hashlet, hexlen, &val);
    if (lo != OK) return lo;

    return KEEPGetPacked(k, val, out, out_type);
}

// --- GetSha: inflate object, verify full SHA-1 ---

// Forward declaration (defined below with KEEPPackFeed)
static void keep_obj_sha(sha1 *out, u8 type, u8csc content);

ok64 KEEPGetExact(keeper *k, sha1 const *sha, u8bp out, u8p out_type) {
    sane(k && sha && out);

    u64 hashlet60 = WHIFFHashlet60(sha);
    u64 key_lo = keepKeyPack(0, hashlet60);
    u64 key_hi = keepKeyPack(0xf, hashlet60);

    for (u32 r = 0; r < k->nruns; r++) {
        wh128cp base = k->runs[r][0];
        size_t len = (size_t)(k->runs[r][1] - base);
        if (len == 0) continue;

        // Binary search for first entry >= key_lo
        size_t lo = 0, hi = len;
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (base[mid].key < key_lo) lo = mid + 1;
            else hi = mid;
        }

        // Scan all entries with matching hashlet
        for (size_t i = lo; i < len && base[i].key <= key_hi; i++) {
            u8bReset(out);
            u8 otype = 0;
            ok64 rc = KEEPGetPacked(k, base[i].val, out, &otype);
            if (rc != OK) continue;

            // Verify full SHA-1
            sha1 actual = {};
            u8cs content = {u8bDataHead(out), u8bIdleHead(out)};
            keep_obj_sha(&actual, otype, content);
            if (sha1eq(&actual, sha)) {
                if (out_type) *out_type = otype;
                done;
            }
        }
    }
    return KEEPNONE;
}

// --- Verify: get object, check SHA-1, recurse into tree/commit ---

#include "GIT.h"

// Simple visited-set: linear scan (good enough for <10K objects)
#define VERIFY_MAX_VISITED 16384
static u64 verify_visited[VERIFY_MAX_VISITED];
static u32 verify_nvisited = 0;

static b8 verify_seen(u64 hashlet) {
    for (u32 i = 0; i < verify_nvisited; i++)
        if (verify_visited[i] == hashlet) return YES;
    return NO;
}

static void verify_mark(u64 hashlet) {
    if (verify_nvisited < VERIFY_MAX_VISITED)
        verify_visited[verify_nvisited++] = hashlet;
}

static ok64 keep_verify_sha(keeper *k, sha1 expected_sha,
                             u32 *checked, u32 *failed) {
    u64 hashlet = WHIFFHashlet60(&expected_sha);
    if (verify_seen(hashlet)) return OK;  // already verified
    verify_mark(hashlet);

    #define VERIFY_BUFSZ (1ULL << 24)  // 16 MB
    u8p objmem = malloc(VERIFY_BUFSZ);
    if (!objmem) return KEEPNOROOM;
    Bu8 obj = {};
    obj[0] = obj[1] = obj[2] = objmem;
    obj[3] = objmem + VERIFY_BUFSZ;
    u8 obj_type = 0;

    ok64 rc = KEEPGet(k, hashlet, 15, obj, &obj_type);
    if (rc != OK) {
        a_pad(u8, hex, 16);
        WHIFFHexFeed60(hex_idle, hashlet);
        u8bFeed1(hex, 0);
        fprintf(stderr, "  MISS: %s\n", (char *)u8bDataHead(hex));
        (*failed)++;
        free(objmem);
        return rc;
    }

    // Recompute SHA-1
    u8cp content = u8bDataHead(obj);
    u64 content_sz = u8bDataLen(obj);

    static char const *type_names[] = {
        [1] = "commit", [2] = "tree", [3] = "blob", [4] = "tag"};
    if (obj_type < 1 || obj_type > 4) {
        fprintf(stderr, "  BAD TYPE: %u\n", obj_type);
        (*failed)++;
        u8bUnMap(obj);
        return KEEPFAIL;
    }

    char hdr[64];
    int hlen = snprintf(hdr, sizeof(hdr), "%s %lu",
                        type_names[obj_type], (unsigned long)content_sz);
    u8p tmp = malloc((size_t)(hlen + 1 + content_sz));
    if (!tmp) { u8bUnMap(obj); return KEEPNOROOM; }
    memcpy(tmp, hdr, hlen);
    tmp[hlen] = 0;
    memcpy(tmp + hlen + 1, content, content_sz);

    sha1 actual_sha = {};
    { u8csc _d = {tmp, tmp + hlen + 1 + content_sz}; SHA1Sum(&actual_sha, _d); };
    free(tmp);

    if (sha1cmp(&actual_sha, &expected_sha) != 0) {
        a_pad(u8, hex_exp, 16);
        WHIFFHexFeed60(hex_exp_idle, WHIFFHashlet60(&expected_sha));
        u8bFeed1(hex_exp, 0);
        a_pad(u8, hex_got, 16);
        WHIFFHexFeed60(hex_got_idle, WHIFFHashlet60(&actual_sha));
        u8bFeed1(hex_got, 0);
        fprintf(stderr, "  HASH MISMATCH: expected %s got %s\n",
                (char *)u8bDataHead(hex_exp), (char *)u8bDataHead(hex_got));
        (*failed)++;
        u8bUnMap(obj);
        return KEEPFAIL;
    }

    (*checked)++;

    // Recurse based on type
    if (obj_type == KEEP_OBJ_COMMIT) {
        // Parse tree SHA from commit
        u8cs body = {content, content + content_sz};
        u8cs field = {}, value = {};
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if ($empty(field)) break;
            if ($len(field) == 4 && memcmp(field[0], "tree", 4) == 0 &&
                $len(value) >= 40) {
                sha1 tree_sha = {};
                u8s sb = {tree_sha.data, tree_sha.data + 20};
                u8cs hx = {value[0], value[0] + 40};
                HEXu8sDrainSome(sb, hx);
                ok64 o = keep_verify_sha(k, tree_sha, checked, failed);
                if (o != OK) {
                    a_pad(u8, hex, 16);
                    WHIFFHexFeed60(hex_idle, WHIFFHashlet60(&tree_sha));
                    u8bFeed1(hex, 0);
                    fprintf(stderr, "  tree %s verify failed\n",
                            (char *)u8bDataHead(hex));
                }
                break;
            }
        }
    } else if (obj_type == KEEP_OBJ_TREE) {
        // Parse tree entries: each is "mode name\0<20-byte sha>"
        u8cs body = {content, content + content_sz};
        while (!$empty(body)) {
            u8cs entry_field = {}, entry_sha = {};
            ok64 o = GITu8sDrainTree(body, entry_field, entry_sha);
            if (o != OK) break;
            if ($len(entry_sha) != 20) continue;
            // Skip gitlinks (submodule refs) — mode 160000, commit in another repo
            if ($len(entry_field) > 6 && memcmp(entry_field[0], "160000", 6) == 0)
                continue;
            sha1 child_sha = {};
            memcpy(child_sha.data, entry_sha[0], 20);
            o = keep_verify_sha(k, child_sha, checked, failed);
            if (o != OK) {
                a_pad(u8, hex, 16);
                WHIFFHexFeed60(hex_idle, WHIFFHashlet60(&child_sha));
                u8bFeed1(hex, 0);
                fprintf(stderr, "  child %s verify failed\n",
                        (char *)u8bDataHead(hex));
            }
        }
    } else if (obj_type == KEEP_OBJ_TAG) {
        // Parse "object <sha>" from tag body, recurse
        u8cs body = {content, content + content_sz};
        u8cs field = {}, value = {};
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if ($empty(field)) break;
            if ($len(field) == 6 && memcmp(field[0], "object", 6) == 0 &&
                $len(value) >= 40) {
                sha1 target_sha = {};
                u8s sb = {target_sha.data, target_sha.data + 20};
                u8cs hx = {value[0], value[0] + 40};
                HEXu8sDrainSome(sb, hx);
                keep_verify_sha(k, target_sha, checked, failed);
                break;
            }
        }
    }
    // blobs: hash check is sufficient

    free(objmem);
    return OK;
}

ok64 KEEPVerify(keeper *k, u8cs hex_sha) {
    sane(k && $ok(hex_sha));
    verify_nvisited = 0;
    if ($len(hex_sha) < 40) {
        fprintf(stderr, "keeper: verify requires full 40-char SHA\n");
        return KEEPFAIL;
    }

    sha1 sha = {};
    u8s sb = {sha.data, sha.data + 20};
    u8cs hx = {hex_sha[0], hex_sha[0] + 40};
    HEXu8sDrainSome(sb, hx);

    u32 checked = 0, failed = 0;
    ok64 rc = keep_verify_sha(k, sha, &checked, &failed);

    fprintf(stderr, "keeper: verified %u objects, %u failed\n", checked, failed);
    return (failed == 0 && rc == OK) ? OK : KEEPFAIL;
}

// --- Scan ---

ok64 KEEPScan(keeper *k, u64 from_val, keep_cb cb, void *ctx) {
    sane(k && cb);

    u32 file_id = wh64Id(from_val);
    u64 offset  = wh64Off(from_val);

    if (file_id < 1 || file_id > k->npacks) return KEEPNONE;
    u8cp pack = u8bDataHead(k->packs[file_id - 1]);
    u64 packlen = (u64)(u8bIdleHead(k->packs[file_id - 1]) - pack);

    // Skip PACK header if at start
    if (offset == 0 && packlen >= 12) {
        if (memcmp(pack, "PACK", 4) == 0)
            offset = 12;  // skip PACK + version + count
    }

    u8p buf = (u8p)malloc(KEEP_BUFSZ);
    if (!buf) return KEEPNOROOM;

    while (offset < packlen) {
        pack_obj obj = {};
        u8cs from = {pack + offset, pack + packlen};
        ok64 o = PACKDrainObjHdr(from, &obj);
        if (o != OK) break;

        if (obj.type >= 1 && obj.type <= 4 && obj.size <= KEEP_BUFSZ) {
            u8s into = {buf, buf + KEEP_BUFSZ};
            if (PACKInflate(from, into, obj.size) == OK) {
                // Compute SHA-1 for u64
                // git object = "<type> <size>\0<content>"
                char hdr[64];
                int hlen = snprintf(hdr, sizeof(hdr), "%s %llu",
                    obj.type == 1 ? "commit" :
                    obj.type == 2 ? "tree" :
                    obj.type == 3 ? "blob" : "tag",
                    (unsigned long long)obj.size);
                sha1 sha = {};
                // FIXME: proper SHA requires inflate+hash
                // FIXME: compute proper git object SHA-1 (header + content)

                u64 hashlet = WHIFFHashlet60(&sha);
                u8cs content = {buf, buf + obj.size};
                o = cb(obj.type, content, hashlet, ctx);
                if (o != OK) break;
            }
        }

        // Advance past this object
        offset = (u64)(from[0] - pack) + obj.size;
        // Account for zlib overhead (approximate)
        // FIXME: track exact consumed bytes from PACKInflate
    }

    free(buf);
    done;
}

// --- Pack writer: incremental API ---

static ok64 keep_build_pack_path(u8bp path, u8csc dir, u32 file_id) {
    u8bFeed(path, dir);
    u8bFeed1(path, '/');
    RONu8sFeedPad(u8bIdle(path), (u64)file_id, KEEP_SEQNO_W);
    ((u8 **)path)[2] += KEEP_SEQNO_W;
    return OK;
}

con char *keep_type_names[] = {
    [KEEP_OBJ_COMMIT] = "commit",
    [KEEP_OBJ_TREE] = "tree",
    [KEEP_OBJ_BLOB] = "blob",
    [KEEP_OBJ_TAG] = "tag",
};

// Compute git object SHA-1: SHA1("type size\0" + content)
static void keep_obj_sha(sha1 *out, u8 type, u8csc content) {
    a_pad(u8, hdr, 64);
    a_cstr(tname, keep_type_names[type]);
    u8bFeed(hdr, tname);
    u8bFeed1(hdr, ' ');
    char tmp[32];
    int nlen = snprintf(tmp, sizeof(tmp), "%llu",
                        (unsigned long long)u8csLen(content));
    u8cs ns = {(u8cp)tmp, (u8cp)tmp + nlen};
    u8bFeed(hdr, ns);
    u8bFeed1(hdr, 0);

    SHA1state ctx;
    SHA1Open(&ctx);
    a_dup(u8c, hdata, u8bData(hdr));
    SHA1Feed(&ctx, hdata);
    SHA1Feed(&ctx, content);
    SHA1Close(&ctx, out);
}

// Encode pack object varint header into buffer
static void keep_feed_obj_hdr(u8bp buf, u8 type, u64 size) {
    u8 first = (u8)((type << 4) | (size & 0x0f));
    size >>= 4;
    if (size > 0) first |= 0x80;
    u8bFeed1(buf, first);
    while (size > 0) {
        u8 c = (u8)(size & 0x7f);
        size >>= 7;
        if (size > 0) c |= 0x80;
        u8bFeed1(buf, c);
    }
}

ok64 KEEPPackOpen(keeper *k, keep_pack *p) {
    sane(k && p);
    memset(p, 0, sizeof(*p));
    p->file_id = k->npacks + 1;

    call(wh128bAllocate, p->entries, KEEP_PACK_MAX_OBJS);

    // Build path: dir/log/NNNNNNNNNN.pack
    a_cstr(kdir, k->dir);
    a_cstr(logdir, "/" KEEP_LOG_DIR);
    a_pad(u8, logpath, 1024);
    u8bFeed(logpath, kdir);
    u8bFeed(logpath, logdir);
    PATHu8gTerm(PATHu8gIn(logpath));
    FILEMakeDirP(PATHu8cgIn(logpath));

    a_pad(u8, packpath, 1024);
    u8bFeed(packpath, kdir);
    u8bFeed(packpath, logdir);
    u8bFeed1(packpath, '/');
    RONu8sFeedPad(u8bIdle(packpath), (u64)p->file_id, KEEP_SEQNO_W);
    ((u8 **)packpath)[2] += KEEP_SEQNO_W;
    a_cstr(pext, KEEP_PACK_EXT);
    u8bFeed(packpath, pext);
    PATHu8gTerm(PATHu8gIn(packpath));

    // FILEBook: reserve 1GB VA, create with 4KB initial
    call(FILEBookCreate, &p->log, PATHu8cgIn(packpath),
         1ULL << 30, 4096);

    // Write PACK header directly into mmap
    u8 hdr_bytes[12] = {'P','A','C','K', 0,0,0,2, 0,0,0,0};
    u8cs hdr_s = {hdr_bytes, hdr_bytes + 12};
    u8bFeed(p->log, hdr_s);

    done;
}

ok64 KEEPPackFeed(keeper *k, keep_pack *p,
                  u8 type, u8csc content, sha1 *sha_out) {
    sane(k && p && p->log && type >= 1 && type <= 4);

    keep_obj_sha(sha_out, type, content);

    u64 obj_offset = u8bDataLen(p->log);

    // Encode varint header into log
    call(FILEBookEnsure, p->log, 16);
    a_pad(u8, ohdr, 16);
    keep_feed_obj_hdr(ohdr, type, u8csLen(content));
    a_dup(u8c, oh, u8bData(ohdr));
    u8bFeed(p->log, oh);

    // Deflate content directly into log
    u64 clen = u8csLen(content);
    u64 need = clen + 256;
    call(FILEBookEnsure, p->log, need);
    u64 idle_before = u8bIdleLen(p->log);
    a_dup(u8c, zsrc, content);
    call(ZINFDeflate, u8bIdle(p->log), zsrc);
    u8bFed(p->log, idle_before - u8bIdleLen(p->log));

    // Build index entry
    u64 hashlet = WHIFFHashlet60(sha_out);
    wh128 entry = {
        .key = keepKeyPack(type, hashlet),
        .val = wh64Pack(KEEP_VAL_FLAGS, p->file_id, obj_offset),
    };
    wh128bPush(p->entries, &entry);

    p->nobjs++;
    done;
}

ok64 KEEPPackClose(keeper *k, keep_pack *p) {
    sane(k && p && p->log);

    // Patch object count in header (offset 8, 4 bytes big-endian)
    u8p hdr = u8bDataHead(p->log);
    hdr[8]  = (u8)(p->nobjs >> 24);
    hdr[9]  = (u8)(p->nobjs >> 16);
    hdr[10] = (u8)(p->nobjs >> 8);
    hdr[11] = (u8)(p->nobjs);

    // Compute trailing SHA-1 and append
    sha1 pack_sha = {};
    a_dup(u8c, pack_data, u8bData(p->log));
    SHA1Sum(&pack_sha, pack_data);
    call(FILEBookEnsure, p->log, 20);
    a_rawc(sha_s, pack_sha);
    u8bFeed(p->log, sha_s);

    // Trim file to actual data, keep mmap for reading
    call(FILETrimBook, p->log);

    // Add to keeper's pack array
    if (k->npacks < KEEP_MAX_FILES) {
        k->packs[k->npacks] = p->log;
        k->npacks++;
        p->log = NULL;  // ownership transferred
    } else {
        FILEUnBook(p->log);
    }

    // Sort index entries, write .idx file
    a_dup(wh128, sorted, wh128bData(p->entries));
    wh128sSort(sorted);

    a_cstr(kdir, k->dir);
    a_cstr(idxdir, "/" KEEP_IDX_DIR);
    a_pad(u8, idxdirpath, 1024);
    u8bFeed(idxdirpath, kdir);
    u8bFeed(idxdirpath, idxdir);
    PATHu8gTerm(PATHu8gIn(idxdirpath));
    FILEMakeDirP(PATHu8cgIn(idxdirpath));

    a_pad(u8, idxpath, 1024);
    u8bFeed(idxpath, kdir);
    u8bFeed(idxpath, idxdir);
    u8bFeed1(idxpath, '/');
    RONu8sFeedPad(u8bIdle(idxpath), (u64)p->file_id, KEEP_SEQNO_W);
    ((u8 **)idxpath)[2] += KEEP_SEQNO_W;
    a_cstr(iext, KEEP_IDX_EXT);
    u8bFeed(idxpath, iext);
    PATHu8gTerm(PATHu8gIn(idxpath));

    int ifd = -1;
    FILECreate(&ifd, PATHu8cgIn(idxpath));
    if (ifd >= 0) {
        u8cs raw = {(u8cp)sorted[0], (u8cp)sorted[1]};
        FILEFeedAll(ifd, raw);
        close(ifd);
    }

    // Mmap index
    u8bp imapped = NULL;
    if (FILEMapRO(&imapped, PATHu8cgIn(idxpath)) == OK &&
        k->nruns < KEEP_MAX_LEVELS) {
        wh128cp base = (wh128cp)u8bDataHead(imapped);
        u32 n = (u32)(u8bDataLen(imapped) / sizeof(wh128));
        k->runs[k->nruns][0] = base;
        k->runs[k->nruns][1] = base + n;
        k->run_maps[k->nruns] = imapped;
        k->nruns++;
    }

    wh128bFree(p->entries);
    done;
}

// --- KEEPPut: convenience wrapper ---

ok64 KEEPPut(keeper *k, u8csc *objects, wh64 *whiffs, u32 nobjs) {
    sane(k && objects && whiffs && nobjs > 0);

    keep_pack p = {};
    call(KEEPPackOpen, k, &p);

    for (u32 i = 0; i < nobjs; i++) {
        u8 type = wh64Type(whiffs[i]);
        sha1 sha = {};
        ok64 o = KEEPPackFeed(k, &p, type, objects[i], &sha);
        if (o != OK) {
            if (p.log) FILEUnBook(p.log);
            wh128bFree(p.entries);
            return o;
        }
        u64 hashlet = WHIFFHashlet60(&sha);
        whiffs[i] = wh64Pack(type, p.file_id, hashlet);
    }

    call(KEEPPackClose, k, &p);
    done;
}

// --- Walk: recursive tree traversal with URI callbacks ---

// Resolve ref string to a 20-byte tree SHA.
// Handles: 40-char hex SHA (commit or tree), ?refname via REFS.
static ok64 keep_resolve_tree(keeper *k, uricp target, sha1 *tree_sha) {
    sane(k);

    sha1 commit_sha = {};

    // Try fragment (#hash) or query (?ref)
    if (!u8csEmpty(target->fragment)) {
        // Fragment = hex SHA prefix
        u64 hashlet = WHIFFHexHashlet60(target->fragment);
        u8 type = 0;
        u8bReset(k->buf1);
        call(KEEPGet, k, hashlet, u8csLen(target->fragment), k->buf1, &type);
        if (type == KEEP_OBJ_TREE) {
            // Already a tree — compute its SHA
            a_dup(u8c, content, u8bData(k->buf1));
            keep_obj_sha(tree_sha, KEEP_OBJ_TREE, content);
            done;
        }
        if (type != KEEP_OBJ_COMMIT) fail(KEEPFAIL);
        // Parse tree SHA from commit
        a_dup(u8c, body, u8bData(k->buf1));
        u8cs field = {}, value = {};
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if (u8csEmpty(field)) break;
            if (u8csLen(field) == 4 && memcmp(field[0], "tree", 4) == 0 &&
                u8csLen(value) >= 40) {
                u8s sb = {tree_sha->data, tree_sha->data + 20};
                u8cs hx = {value[0], value[0] + 40};
                HEXu8sDrainSome(sb, hx);
                done;
            }
        }
        fail(KEEPFAIL);
    }

    if (!u8csEmpty(target->query)) {
        // Resolve ?ref via REFS
        a_cstr(keepdir, k->dir);
        a_pad(u8, qbuf, 256);
        u8bFeed1(qbuf, '?');
        u8bFeed(qbuf, target->query);
        a_dup(u8c, qkey, u8bData(qbuf));

        u8bp rmap = NULL;
        ref rarr[REFS_MAX_REFS];
        u32 rn = 0;
        REFSLoad(rarr, &rn, REFS_MAX_REFS, &rmap, keepdir);

        b8 found = NO;
        for (u32 i = 0; i < rn; i++) {
            if (REFMatch(&rarr[i], qkey)) {
                a_dup(u8c, val, rarr[i].val);
                if (!u8csEmpty(val) && *val[0] == '?')
                    u8csUsed(val, 1);
                // val is hex SHA of commit
                if (u8csLen(val) >= 40) {
                    u8s sb = {commit_sha.data, commit_sha.data + 20};
                    u8cs hx = {val[0], val[0] + 40};
                    HEXu8sDrainSome(sb, hx);
                    found = YES;
                }
                break;
            }
        }
        if (rmap) u8bUnMap(rmap);
        if (!found) fail(KEEPNONE);

        // Get commit, extract tree SHA
        u64 hashlet = WHIFFHashlet60(&commit_sha);
        u8 type = 0;
        u8bReset(k->buf1);
        call(KEEPGet, k, hashlet, 15, k->buf1, &type);
        if (type != KEEP_OBJ_COMMIT && type != KEEP_OBJ_TAG) fail(KEEPFAIL);

        // If tag, get the commit it points to
        if (type == KEEP_OBJ_TAG) {
            a_dup(u8c, tbody, u8bData(k->buf1));
            u8cs tf = {}, tv = {};
            while (GITu8sDrainCommit(tbody, tf, tv) == OK) {
                if (u8csEmpty(tf)) break;
                if (u8csLen(tf) == 6 && memcmp(tf[0], "object", 6) == 0 &&
                    u8csLen(tv) >= 40) {
                    u8s sb2 = {commit_sha.data, commit_sha.data + 20};
                    u8cs hx2 = {tv[0], tv[0] + 40};
                    HEXu8sDrainSome(sb2, hx2);
                    break;
                }
            }
            hashlet = WHIFFHashlet60(&commit_sha);
            u8bReset(k->buf1);
            call(KEEPGet, k, hashlet, 15, k->buf1, &type);
        }

        a_dup(u8c, body, u8bData(k->buf1));
        u8cs field = {}, value = {};
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if (u8csEmpty(field)) break;
            if (u8csLen(field) == 4 && memcmp(field[0], "tree", 4) == 0 &&
                u8csLen(value) >= 40) {
                u8s sb = {tree_sha->data, tree_sha->data + 20};
                u8cs hx = {value[0], value[0] + 40};
                HEXu8sDrainSome(sb, hx);
                done;
            }
        }
        fail(KEEPFAIL);
    }

    fail(KEEPFAIL);  // no ref or hash in URI
}

// Recursive tree walk
static ok64 keep_walk_tree(keeper *k, u8csc tree_sha,
                           u8bp pathbuf, uricp base_uri,
                           KEEP_WALK mode, keep_walk_f cb, void0p ctx) {
    u64 hashlet = WHIFFHashlet60((sha1cp)tree_sha[0]);
    u8 type = 0;
    u8bReset(k->buf3);
    ok64 o = KEEPGet(k, hashlet, 15, k->buf3, &type);
    if (o != OK) return o;
    if (type != KEEP_OBJ_TREE) return KEEPFAIL;

    a_dup(u8c, body, u8bData(k->buf3));
    size_t path_save = u8bDataLen(pathbuf);

    while (!u8csEmpty(body)) {
        u8cs entry_field = {}, entry_sha = {};
        o = GITu8sDrainTree(body, entry_field, entry_sha);
        if (o != OK) break;
        if (u8csLen(entry_sha) != 20) continue;

        // Skip gitlinks (submodule refs, mode 160000)
        if (u8csLen(entry_field) > 6 && memcmp(entry_field[0], "160000", 6) == 0)
            continue;

        // Extract name: after mode + space
        u8cs name = {};
        a_dup(u8c, ef, entry_field);
        if (u8csFind(ef, ' ') == OK) {
            u8csUsed(ef, 1);
            u8csMv(name, ef);
        } else {
            u8csMv(name, entry_field);
        }

        // Is this a subtree? mode starts with "40" (40000)
        b8 is_tree = (entry_field[0][0] == '4');

        // Build path: append /name to pathbuf
        if (path_save > 0) u8bFeed1(pathbuf, '/');
        u8bFeed(pathbuf, name);
        PATHu8gTerm(PATHu8gIn(pathbuf));

        if (is_tree) {
            if (mode & KEEP_WALK_TREES) {
                uri entry = *base_uri;
                a_dup(u8c, pslice, u8bData(pathbuf));
                entry.path[0] = pslice[0];
                entry.path[1] = pslice[1];
                u8cs empty = {};
                o = cb(ctx, &entry, KEEP_OBJ_TREE, empty);
                if (o != OK) return o;
            }
            if (mode & KEEP_WALK_DEEP) {
                // Save buf3 state — recursive call will overwrite it
                size_t b3_save = u8bDataLen(k->buf3);
                Bu8 saved_tree = {};
                u8bMap(saved_tree, b3_save);
                a_dup(u8c, b3data, u8bData(k->buf3));
                u8bFeed(saved_tree, b3data);

                o = keep_walk_tree(k, entry_sha, pathbuf,
                                   base_uri, mode, cb, ctx);

                // Restore buf3
                u8bReset(k->buf3);
                a_dup(u8c, sdata, u8bData(saved_tree));
                u8bFeed(k->buf3, sdata);
                u8bUnMap(saved_tree);
                // Restore body scan position
                body[0] = u8bDataHead(k->buf3) + (body[0] - b3data[0]);
                body[1] = u8bDataHead(k->buf3) + u8bDataLen(k->buf3);

                if (o != OK) return o;
            }
        } else if (mode & KEEP_WALK_BLOBS) {
            uri entry = *base_uri;
            a_dup(u8c, pslice, u8bData(pathbuf));
            entry.path[0] = pslice[0];
            entry.path[1] = pslice[1];

            u8cs content = {};
            if (mode & KEEP_WALK_CONTENT) {
                u8bReset(k->buf4);
                u8 btype = 0;
                u64 bhash = WHIFFHashlet60((sha1cp)entry_sha[0]);
                ok64 go = KEEPGet(k, bhash, 15, k->buf4, &btype);
                if (go == OK) {
                    content[0] = u8bDataHead(k->buf4);
                    content[1] = u8bDataHead(k->buf4) + u8bDataLen(k->buf4);
                }
            }
            o = cb(ctx, &entry, KEEP_OBJ_BLOB, content);
            if (o != OK) return o;
        }

        // Restore pathbuf to saved length
        ((u8 **)pathbuf)[2] = u8bDataHead(pathbuf) + path_save;
        PATHu8gTerm(PATHu8gIn(pathbuf));
    }

    return OK;
}

ok64 KEEPWalk(keeper *k, uricp target, KEEP_WALK mode,
              keep_walk_f cb, void0p ctx) {
    sane(k && target && cb);

    sha1 tree_sha = {};
    call(keep_resolve_tree, k, target, &tree_sha);

    a_pad(u8, pathbuf, 4096);
    a_rawc(ts, tree_sha);
    return keep_walk_tree(k, ts, pathbuf, target, mode, cb, ctx);
}

// --- Import: read git .idx v2 file alongside .pack, build wh128 index ---
//
// Git pack index v2 format:
//   magic (ff744f63), version (2), 256 fanout entries (u32 BE),
//   N×20 SHA-1, N×u32 CRC, N×u32 offset (BE), [8-byte offsets for >2GB]
//   pack SHA-1, index SHA-1

ok64 KEEPImport(keeper *k, u8cs pack_path) {
    sane(k && $ok(pack_path));

    // NUL-terminate pack_path for FILEMapRO
    a_path(pack_pp, pack_path);

    // Derive .idx path from .pack path (replace extension)
    a_pad(u8, idx_path_buf, 1024);
    call(u8bFeed, idx_path_buf, pack_path);
    size_t plen = u8bDataLen(idx_path_buf);
    if (plen >= 5 && memcmp(u8bDataHead(idx_path_buf) + plen - 5, ".pack", 5) == 0)
        ((u8 **)idx_path_buf)[2] -= 5;
    else {
        fprintf(stderr, "keeper: expected .pack file\n");
        fail(KEEPFAIL);
    }
    a_cstr(idx_ext, ".idx");
    call(u8bFeed, idx_path_buf, idx_ext);
    call(PATHu8gTerm, PATHu8gIn(idx_path_buf));

    // Map both files
    u8bp pack_map = NULL, idx_map = NULL;
    call(FILEMapRO, &pack_map, PATHu8cgIn(pack_pp));
    ok64 io = FILEMapRO(&idx_map, PATHu8cgIn(idx_path_buf));
    if (io != OK) { FILEUnMap(pack_map); return io; }

    u8cp idx = u8bDataHead(idx_map);
    u64 idx_len = (u64)(u8bIdleHead(idx_map) - idx);

    // Verify idx v2 header
    if (idx_len < 8 + 256*4 || idx[0] != 0xff || idx[1] != 0x74 ||
        idx[2] != 0x4f || idx[3] != 0x63) {
        fprintf(stderr, "keeper: not a git pack index v2\n");
        FILEUnMap(pack_map); FILEUnMap(idx_map);
        fail(KEEPFAIL);
    }

    // Read total object count from fanout[255]
    u8cp fanout = idx + 8;
    u32 nobjects = (fanout[255*4] << 24) | (fanout[255*4+1] << 16) |
                   (fanout[255*4+2] << 8) | fanout[255*4+3];

    // Pointers to the three tables
    u8cp sha_table = idx + 8 + 256 * 4;               // N × 20 bytes
    u8cp crc_table = sha_table + (u64)nobjects * 20;   // N × 4 bytes
    u8cp off_table = crc_table + (u64)nobjects * 4;    // N × 4 bytes

    if ((u64)(off_table + (u64)nobjects * 4 - idx) > idx_len) {
        fprintf(stderr, "keeper: index file too small\n");
        FILEUnMap(pack_map); FILEUnMap(idx_map);
        fail(KEEPFAIL);
    }

    fprintf(stderr, "keeper: importing %u objects\n", nobjects);

    // Determine file_id (1-based, matching filename NNNN.packs)
    u32 file_id = k->npacks + 1;
    a_cstr(kdir, k->dir);
    {
        a_pad(u8, dst, 1024);
        call(u8bFeed, dst, kdir);
        a_cstr(logsep, "/" KEEP_LOG_DIR "/");
        call(u8bFeed, dst, logsep);
        call(RONu8sFeedPad, u8bIdle(dst), (u64)file_id, KEEP_SEQNO_W);
        ((u8 **)dst)[2] += KEEP_SEQNO_W;
        a_cstr(ext, KEEP_PACK_EXT);
        call(u8bFeed, dst, ext);
        call(PATHu8gTerm, PATHu8gIn(dst));

        int fd = -1;
        call(FILECreate, &fd, PATHu8cgIn(dst));
        a_dup(u8c, data, u8bData(pack_map));
        call(FILEFeedAll, fd, data);
        close(fd);
    }

    // Build wh128 entries from the idx tables
    wh128 *entries = (wh128 *)malloc((u64)nobjects * sizeof(wh128));
    if (!entries) { FILEUnMap(pack_map); FILEUnMap(idx_map); failc(KEEPNOROOM); }

    for (u32 i = 0; i < nobjects; i++) {
        sha1cp sha = (sha1cp)(sha_table + (u64)i * 20);
        u64 hashlet = WHIFFHashlet60(sha);

        // 4-byte offset (BE), high bit = large offset flag
        u8cp offp = off_table + (u64)i * 4;
        u64 off = ((u64)offp[0] << 24) | ((u64)offp[1] << 16) |
                  ((u64)offp[2] << 8) | offp[3];

        if (off & 0x80000000ULL) {
            // Large offset: index into 8-byte offset table
            // FIXME: handle >2GB packs
            off &= 0x7FFFFFFF;
        }

        // git idx v2 doesn't carry type; use 0 (lookup spans all types)
        entries[i].key = keepKeyPack(0, hashlet);
        entries[i].val = wh64Pack(KEEP_VAL_FLAGS, file_id, off);
    }

    // Sort and dedup (wh128 dedup: same key+val only)
    wh128s sorted = {entries, entries + nobjects};
    wh128sSort(sorted);
    wh128sDedup(sorted);
    u32 nentries = (u32)(sorted[1] - sorted[0]);

    // Write .idx file to idx/
    {
        u64 seqno = (u64)file_id;
        a_pad(u8, idxpath, 1024);
        call(u8bFeed, idxpath, kdir);
        a_cstr(idxsep, "/" KEEP_IDX_DIR "/");
        call(u8bFeed, idxpath, idxsep);
        call(RONu8sFeedPad, u8bIdle(idxpath), seqno, KEEP_SEQNO_W);
        ((u8 **)idxpath)[2] += KEEP_SEQNO_W;
        a_cstr(ext2, KEEP_IDX_EXT);
        call(u8bFeed, idxpath, ext2);
        call(PATHu8gTerm, PATHu8gIn(idxpath));

        int fd = -1;
        call(FILECreate, &fd, PATHu8cgIn(idxpath));
        u8cs data = {(u8cp)entries, (u8cp)(entries + nentries)};
        call(FILEFeedAll, fd, data);
        close(fd);
    }

    free(entries);
    FILEUnMap(pack_map);
    FILEUnMap(idx_map);

    fprintf(stderr, "keeper: indexed %u objects\n", nentries);
    done;
}

// --- Sync: clone or update from remote via git-upload-pack ---

#include "PKT.h"
#include "ZINF.h"
#include <sys/wait.h>

// Compute git object SHA-1: hash("<type> <size>\0<content>")
// Incremental: feeds header and content separately (no copy needed).
static void keep_git_sha1(sha1 *out, u8 type, u8csc content) {
    static char const *type_str[] = {
        [1] = "commit", [2] = "tree", [3] = "blob", [4] = "tag"};
    a_pad(u8, hdr, 64);
    a_cstr(tname, type_str[type]);
    u8bFeed(hdr, tname);
    u8bFeed1(hdr, ' ');
    char tmp[32];
    int nlen = snprintf(tmp, sizeof(tmp), "%llu",
                        (unsigned long long)u8csLen(content));
    u8cs ns = {(u8cp)tmp, (u8cp)tmp + nlen};
    u8bFeed(hdr, ns);
    u8bFeed1(hdr, 0);

    SHA1state ctx;
    SHA1Open(&ctx);
    a_dup(u8c, hdata, u8bData(hdr));
    SHA1Feed(&ctx, hdata);
    SHA1Feed(&ctx, content);
    SHA1Close(&ctx, out);
}

// DFS tree node for pack indexing (used by KEEPSync)
typedef struct { u64 offset; u32 child; u32 sibling; } pack_node;

// Drain REF_DELTA waiters: binary search + scan in sorted wh128 array.
// Links matching waiters as children of parent_idx in the DFS tree.
static void keep_drain_waiters(wh128cp wbuf, size_t wlen,
                               pack_node *nodes, b8 *resolved,
                               u64 sha_key, u32 parent_idx) {
    // Binary search for first entry with matching key
    size_t lo = 0, hi = wlen;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (wbuf[mid].key < sha_key) lo = mid + 1;
        else hi = mid;
    }
    // Scan all entries with matching key
    for (size_t j = lo; j < wlen && wbuf[j].key == sha_key; j++) {
        u32 w = (u32)wbuf[j].val;
        if (resolved[w]) continue;
        nodes[w].sibling = nodes[parent_idx].child;
        nodes[parent_idx].child = w;
    }
}

ok64 KEEPSync(keeper *k, u8cs remote,
              char const *const *wants, char const *const *haves) {
    sane(k);

    if ($empty(remote)) {
        fprintf(stderr, "keeper: sync requires a remote\n");
        return KEEPFAIL;
    }

    // Parse remote: "host /path" or just "/path" for local
    // Build command: ssh host git-upload-pack 'path'
    // or local: git-upload-pack 'path'
    a_pad(u8, cmdbuf, 2048);
    a_dup(u8c, rscan, remote);
    if (u8csFind(rscan, ' ') == OK) {
        u8cs host = {remote[0], rscan[0]};
        u8cs rpath = {rscan[0] + 1, remote[1]};
        a_cstr(ssh_pre, "ssh ");
        a_cstr(gup, " git-upload-pack '");
        a_cstr(sq, "'");
        u8bFeed(cmdbuf, ssh_pre);
        u8bFeed(cmdbuf, host);
        u8bFeed(cmdbuf, gup);
        u8bFeed(cmdbuf, rpath);
        u8bFeed(cmdbuf, sq);
    } else {
        a_cstr(gup, "git-upload-pack '");
        a_cstr(sq, "'");
        u8bFeed(cmdbuf, gup);
        u8bFeed(cmdbuf, remote);
        u8bFeed(cmdbuf, sq);
    }
    u8bFeed1(cmdbuf, 0);  // NUL terminate for popen
    char *cmd = (char *)u8bDataHead(cmdbuf);

    fprintf(stderr, "keeper: connecting: %s\n", cmd);

    // Fork + pipe
    int to_child[2], from_child[2];
    if (pipe(to_child) != 0 || pipe(from_child) != 0) return KEEPFAIL;

    pid_t pid = fork();
    if (pid < 0) return KEEPFAIL;

    if (pid == 0) {
        close(to_child[1]);
        close(from_child[0]);
        dup2(to_child[0], 0);
        dup2(from_child[1], 1);
        close(to_child[0]);
        close(from_child[1]);
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        _exit(127);
    }

    close(to_child[0]);
    close(from_child[1]);
    int wfd = to_child[1];
    int rfd = from_child[0];

    // Read ref advertisement
    #define SYNC_BUFSZ KEEP_BUFSZ  // same as KEEPGet buffers (1 GB mmap)
    // mmap large buffers (virtual space only, pages on demand)
    Bu8 rbuf_b = {}, objbuf_b = {};
    u8bMap(rbuf_b, SYNC_BUFSZ);
    u8bMap(objbuf_b, SYNC_BUFSZ);
    u8p rbuf = u8bHead(rbuf_b);
    u8p buf1 = u8bHead(k->buf1);  // reuse keeper's pre-allocated buffers
    u8p buf2 = u8bHead(k->buf2);
    u8p objbuf = u8bHead(objbuf_b);
    if (!rbuf || !buf1 || !buf2 || !objbuf) {
        if (rbuf_b[0]) u8bUnMap(rbuf_b);
        if (objbuf_b[0]) u8bUnMap(objbuf_b);
        close(wfd); close(rfd);
        kill(pid, SIGTERM); waitpid(pid, NULL, 0);
        return KEEPNOROOM;
    }

    u64 rlen = 0;
    {
        ssize_t n = read(rfd, rbuf, SYNC_BUFSZ);
        if (n <= 0) goto sync_fail;
        rlen = (u64)n;
    }

    // Parse ref advertisement — collect all refs
    u8cs adv = {rbuf, rbuf + rlen};
    u8cs line = {};
    ok64 po = PKTu8sDrain(adv, line);
    if (po != OK || $len(line) < 40) goto sync_fail;

    // First line = HEAD sha
    sha1hex head_hex;
    memcpy(head_hex.data, line[0], 40);

    // Drain remaining refs until flush
    #define MAX_REFS 2048
    sha1hex  refs[MAX_REFS];        // original SHA (for wants)
    sha1hex  refrec[MAX_REFS];      // peeled SHA if available (for REFS)
    char     refnames[MAX_REFS][256];
    u32 nrefs = 0;
    refs[nrefs] = head_hex;
    refrec[nrefs] = head_hex;
    snprintf(refnames[nrefs], 256, "HEAD");
    nrefs++;

    for (;;) {
        ok64 o = PKTu8sDrain(adv, line);
        if (o == PKTFLUSH) break;
        if (o == NODATA) {
            ssize_t n = read(rfd, rbuf + rlen, SYNC_BUFSZ - rlen);
            if (n <= 0) break;
            rlen += (u64)n;
            adv[1] = rbuf + rlen;
            continue;
        }
        if (o != OK) break;
        if ($len(line) >= 42 && nrefs < MAX_REFS) {
            // Extract ref name: after SHA + space, until NUL/space/newline
            u8cp namestart = line[0] + 41;
            u8cp nameend = namestart;
            while (nameend < line[1] && *nameend != 0 &&
                   *nameend != ' ' && *nameend != '\n')
                nameend++;
            size_t namelen = (size_t)(nameend - namestart);
            if (namelen == 0 || namelen >= 256) continue;

            // Peeled tag (^{}): update refrec with dereferenced SHA.
            // refs[] keeps original (for wants), refrec[] gets peeled (for REFS).
            if (namelen > 3 && nameend[-1] == '}' && nameend[-2] == '{' && nameend[-3] == '^') {
                size_t base_namelen = namelen - 3;
                for (u32 pi = 0; pi < nrefs; pi++) {
                    if (strlen(refnames[pi]) == base_namelen &&
                        memcmp(refnames[pi], namestart, base_namelen) == 0) {
                        memcpy(refrec[pi].data, line[0], 40);
                        break;
                    }
                }
                continue;
            }

            memcpy(refs[nrefs].data, line[0], 40);
            memcpy(refrec[nrefs].data, line[0], 40);
            memcpy(refnames[nrefs], namestart, namelen);
            refnames[nrefs][namelen] = 0;
            nrefs++;
        }
    }

    fprintf(stderr, "keeper: %u ref(s), HEAD=%.12s\n", nrefs, head_hex.data);

    // Build want/have negotiation
    {
        #define NEGBUF (1 << 20)  // 1MB for want/have pkt-lines
        u8 *wbuf = malloc(NEGBUF);
        if (!wbuf) goto sync_fail;
        u8s ws = {wbuf, wbuf + NEGBUF};
        b8 first_want = YES;

        if (wants) {
            // Specific wants: match by SHA or ref name against advertised
            for (int wi = 0; wants[wi]; wi++) {
                sha1hex const *sha = NULL;
                size_t wlen = strlen(wants[wi]);
                for (u32 j = 0; j < nrefs; j++) {
                    if (wlen == 40 && memcmp(refs[j].data, wants[wi], 40) == 0) {
                        sha = &refs[j]; break;
                    }
                    if (strcmp(refnames[j], wants[wi]) == 0) {
                        sha = &refs[j]; break;
                    }
                }
                if (!sha) {
                    fprintf(stderr, "keeper: want %s not advertised, skipping\n",
                            wants[wi]);
                    continue;
                }
                char pay[256];
                int plen;
                if (first_want) {
                    plen = snprintf(pay, sizeof(pay),
                        "want %.40s no-progress\n", sha->data);
                    first_want = NO;
                } else {
                    plen = snprintf(pay, sizeof(pay),
                        "want %.40s\n", sha->data);
                }
                u8cs payload = {(u8cp)pay, (u8cp)pay + plen};
                PKTu8sFeed(ws, payload);
            }
        } else {
            // Want all advertised refs
            for (u32 i = 0; i < nrefs; i++) {
                char pay[256];
                int plen;
                if (first_want) {
                    plen = snprintf(pay, sizeof(pay),
                        "want %.40s no-progress\n", refs[i].data);
                    first_want = NO;
                } else {
                    plen = snprintf(pay, sizeof(pay),
                        "want %.40s\n", refs[i].data);
                }
                u8cs payload = {(u8cp)pay, (u8cp)pay + plen};
                PKTu8sFeed(ws, payload);
            }
        }

        if (first_want) {
            fprintf(stderr, "keeper: nothing to want\n");
            goto sync_done;
        }

        PKTu8sFeedFlush(ws);

        // Send have lines
        int nhave_sent = 0;
        if (haves) {
            for (int hi = 0; haves[hi]; hi++) {
                char pay[256];
                int plen = snprintf(pay, sizeof(pay),
                    "have %.40s\n", haves[hi]);
                u8cs payload = {(u8cp)pay, (u8cp)pay + plen};
                PKTu8sFeed(ws, payload);
                nhave_sent++;
            }
        }
        if (nhave_sent > 0)
            fprintf(stderr, "keeper: sent %d have(s)\n", nhave_sent);

        u8 donepay[] = "done\n";
        u8cs donecs = {donepay, donepay + 5};
        PKTu8sFeed(ws, donecs);

        // Write negotiation in chunks (pipe buffer is ~64K)
        u64 wlen = ws[0] - wbuf;
        u64 written = 0;
        while (written < wlen) {
            u64 chunk = wlen - written;
            if (chunk > 32768) chunk = 32768;
            ssize_t n = write(wfd, wbuf + written, chunk);
            if (n <= 0) { free(wbuf); goto sync_fail; }
            written += (u64)n;
        }
        free(wbuf);
    }
    close(wfd);
    wfd = -1;

    // Read response into rbuf (may need multiple reads for ACK sequences)
    rlen = 0;
    for (;;) {
        ssize_t n = read(rfd, rbuf + rlen, SYNC_BUFSZ - rlen);
        if (n <= 0) { if (rlen == 0) goto sync_fail; break; }
        rlen += (u64)n;
        // Stop once we see PACK magic in the buffer
        if (rlen >= 16) {
            for (u64 i = 0; i + 4 <= rlen; i++) {
                if (memcmp(rbuf + i, "PACK", 4) == 0) goto got_pack;
            }
        }
        // Safety: stop after 1MB of ACK chatter
        if (rlen >= (1 << 20)) break;
    }
got_pack:

    // Parse response: NAK (full clone) or ACK <sha> (incremental)
    u8cs resp = {rbuf, rbuf + rlen};
    po = PKTu8sDrain(resp, line);
    if (po != OK) goto sync_fail;
    if ($len(line) >= 3 && memcmp(line[0], "NAK", 3) == 0) {
        // Full clone response
    } else if ($len(line) >= 3 && memcmp(line[0], "ACK", 3) == 0) {
        // Incremental: drain ACK/NAK lines until we reach the pack data
        for (;;) {
            // Check if resp[0] is at PACK magic
            if (resp[1] - resp[0] >= 4 && memcmp(resp[0], "PACK", 4) == 0)
                break;
            ok64 ao = PKTu8sDrain(resp, line);
            if (ao == PKTFLUSH) break;
            if (ao != OK) break;
            if ($len(line) >= 3 && memcmp(line[0], "NAK", 3) == 0) break;
        }
    } else {
        fprintf(stderr, "keeper: unexpected response: %.*s\n",
                (int)$len(line), (char *)line[0]);
        goto sync_fail;
    }

    // Parse PACK header from the initial read
    u8cp pack_start = resp[0];  // where PACK data begins in rbuf
    pack_hdr hdr = {};
    po = PACKDrainHdr(resp, &hdr);
    if (po != OK || hdr.version != 2) goto sync_fail;
    u64 initial_pack = rlen - (u64)(pack_start - rbuf);  // pack bytes in rbuf

    // Open or create pack log file for appending.
    // Estimate VA reservation from object count (~256 bytes/obj).
    b8 appending = (k->npacks > 0);
    u32 file_id = appending ? k->npacks : 1;
    u64 pack_book = 16ULL << 30;  // 16GB VA reservation
    u8bp packbuf = NULL;
    u64 append_offset = 0;  // where new objects start in the log
    {
        a_pad(u8, dst, 1024);
        a_cstr(kdir, k->dir);
        u8bFeed(dst, kdir);
        a_cstr(logsep, "/" KEEP_LOG_DIR "/");
        u8bFeed(dst, logsep);
        RONu8sFeedPad(u8bIdle(dst), (u64)file_id, KEEP_SEQNO_W);
        ((u8 **)dst)[2] += KEEP_SEQNO_W;
        a_cstr(ext, KEEP_PACK_EXT);
        u8bFeed(dst, ext);
        PATHu8gTerm(PATHu8gIn(dst));

        if (appending) {
            ok64 o = FILEBook(&packbuf, PATHu8cgIn(dst), pack_book);
            if (o != OK) goto sync_fail;
            // Mark all existing data as DATA (FILEBook leaves it as IDLE)
            ((u8 **)packbuf)[2] = packbuf[3];
            append_offset = u8bDataLen(packbuf);
        } else {
            // New log — create
            size_t init = initial_pack;
            if (init < 4096) init = 4096;
            ok64 o = FILEBookCreate(&packbuf, PATHu8cgIn(dst),
                                    pack_book, init);
            if (o != OK) goto sync_fail;
        }
    }

    // Copy initial pack data from rbuf into booked file.
    // On append, skip the PACK header (12 bytes) — only append objects.
    {
        u8cp copy_start = pack_start;
        if (appending) {
            // resp[0] already points past the PACK header (PACKDrainHdr consumed it)
            copy_start = resp[0];
        }
        u8cs init_data = {copy_start, rbuf + rlen};
        FILEBookEnsure(packbuf, u8csLen(init_data));
        u8bFeed(packbuf, init_data);
    }

    // Stream remaining pack data from rfd directly into booked file
    {
        for (;;) {
            call(FILEBookEnsure, packbuf, 1 << 20);
            ssize_t n = read(rfd, u8bIdleHead(packbuf), u8bIdleLen(packbuf));
            if (n <= 0) break;
            u8bFed(packbuf, (size_t)n);
        }
    }
    close(rfd);
    rfd = -1;

    u64 new_bytes = u8bDataLen(packbuf) - append_offset;

    fprintf(stderr, "keeper: received %u objects, %llu bytes\n",
            hdr.count, (unsigned long long)new_bytes);

    // Patch object count in PACK header (offset 8, 4 bytes big-endian).
    // On append, add new objects to existing count. On fresh clone, already correct.
    if (appending) {
        u8p phdr = u8bDataHead(packbuf);
        u32 old_count = ((u32)phdr[8] << 24) | ((u32)phdr[9] << 16) |
                        ((u32)phdr[10] << 8) | phdr[11];
        u32 new_count = old_count + hdr.count;
        phdr[8]  = (u8)(new_count >> 24);
        phdr[9]  = (u8)(new_count >> 16);
        phdr[10] = (u8)(new_count >> 8);
        phdr[11] = (u8)(new_count);
    }

    // Trim booked file to actual size, then re-mmap read-only for indexing
    FILETrimBook(packbuf);
    FILEUnBook(packbuf);
    packbuf = NULL;

    // Re-mmap the pack read-only into keeper
    // (on append, unmap the old RO mapping first)
    if (appending && k->npacks > 0) {
        FILEUnMap(k->packs[file_id - 1]);
        k->packs[file_id - 1] = NULL;
        k->npacks--;
    }
    {
        a_pad(u8, pp, 1024);
        a_cstr(kdir, k->dir);
        u8bFeed(pp, kdir);
        a_cstr(logsep, "/" KEEP_LOG_DIR "/");
        u8bFeed(pp, logsep);
        RONu8sFeedPad(u8bIdle(pp), (u64)file_id, KEEP_SEQNO_W);
        ((u8 **)pp)[2] += KEEP_SEQNO_W;
        a_cstr(pext, KEEP_PACK_EXT);
        u8bFeed(pp, pext);
        PATHu8gTerm(PATHu8gIn(pp));
        u8bp mapped = NULL;
        if (FILEMapRO(&mapped, PATHu8cgIn(pp)) == OK) {
            k->packs[k->npacks] = mapped;
            k->npacks++;
        }
    }

    u8cp packbase = u8bDataHead(k->packs[k->npacks - 1]);
    u64 packlen = (u64)(u8bIdleHead(k->packs[k->npacks - 1]) - packbase);

    // Scan pack: record offsets + types
    {
        u64 *offsets = calloc(hdr.count, sizeof(u64));
        u8 *types_arr = calloc(hdr.count, 1);
        if (!offsets || !types_arr) { free(offsets); free(types_arr); goto sync_fail; }

        // Start scanning where new objects begin.
        // Fresh pack has 12-byte header + objects + 20-byte SHA1 trailer.
        // Appended data is raw objects, no header or trailer.
        u64 scan_start = appending ? append_offset : 12;
        u64 data_end = appending ? packlen : (packlen >= 20 ? packlen - 20 : packlen);
        u8cs scan = {packbase + scan_start, packbase + data_end};
        u32 scanned = 0;
        for (u32 i = 0; i < hdr.count; i++) {
            offsets[i] = scan[0] - packbase;
            pack_obj obj = {};
            if (PACKDrainObjHdr(scan, &obj) != OK) {
                fprintf(stderr, "keeper: scan hdr fail at %u off %llu remaining %llu\n",
                        i, (unsigned long long)(scan[0] - packbase),
                        (unsigned long long)$len(scan));
                break;
            }
            types_arr[i] = obj.type;
            ok64 zr = ZINFInflate(u8bIdle(k->buf1), scan);
            if (zr != OK) {
                fprintf(stderr, "keeper: scan inflate fail at %u: %s type=%u "
                        "size=%llu remaining=%llu\n",
                        i, ok64str(zr), obj.type, (unsigned long long)obj.size,
                        (unsigned long long)u8csLen(scan));
                break;
            }
            scanned++;
        }
        if (scanned < hdr.count)
            fprintf(stderr, "keeper: scan incomplete: %u/%u objects\n",
                    scanned, hdr.count);

        // DFS indexing: build delta dependency tree, walk depth-first.
        // Each object inflated exactly once, max chain_depth in RAM.

        pack_node *nodes = calloc(hdr.count + 1, sizeof(pack_node));  // 1-based
        wh128 *sorted_entries = malloc(hdr.count * sizeof(wh128));
        if (!nodes || !sorted_entries) {
            free(nodes); free(sorted_entries);
            free(offsets); free(types_arr);
            goto sync_fail;
        }

        // Build tree: map offset → index for OFS_DELTA parent lookup
        for (u32 i = 0; i < hdr.count; i++)
            nodes[i + 1].offset = offsets[i];

        // Phase 1: link OFS_DELTA by offset
        for (u32 i = 0; i < hdr.count; i++) {
            if (types_arr[i] != PACK_OBJ_OFS_DELTA) continue;
            pack_obj obj = {};
            u8cs from = {packbase + offsets[i], packbase + packlen};
            if (PACKDrainObjHdr(from, &obj) != OK) continue;
            u64 base_off = offsets[i] - obj.ofs_delta;
            u32 lo = 0, hi = hdr.count;
            while (lo < hi) {
                u32 mid = lo + (hi - lo) / 2;
                if (offsets[mid] < base_off) lo = mid + 1;
                else hi = mid;
            }
            if (lo < hdr.count && offsets[lo] == base_off) {
                u32 parent = lo + 1, me = i + 1;
                nodes[me].sibling = nodes[parent].child;
                nodes[parent].child = me;
            }
        }

        // Resolve base objects, then single-pass DFS with sorted
        // waiter buffer for REF_DELTA reverse index.
        u32 total_indexed = 0;
        u32 skipped = 0;
        b8 *resolved = calloc(hdr.count + 1, 1);

        // REF_DELTA waiters: sorted wh128 array {sha_prefix, 1-based idx}.
        // Binary search + scan replaces hash table (no collisions).
        Bwh128 waiters_buf = {};
        wh128bAllocate(waiters_buf, hdr.count);

        for (u32 i = 0; i < hdr.count; i++) {
            if (types_arr[i] != PACK_OBJ_REF_DELTA) continue;
            pack_obj obj = {};
            u8cs from = {packbase + offsets[i], packbase + packlen};
            if (PACKDrainObjHdr(from, &obj) != OK) {
                fprintf(stderr, "keeper: waiter: hdr fail at %u\n", i);
                skipped++; continue;
            }
            u64 sha_key = 0;
            memcpy(&sha_key, obj.ref_delta[0], 8);
            wh128 w = { .key = sha_key, .val = i + 1 };
            wh128bPush(waiters_buf, &w);
        }

        // Sort waiters for binary search
        a_dup(wh128, wsorted, wh128bData(waiters_buf));
        wh128sSort(wsorted);
        wh128cp wbuf = wsorted[0];
        size_t wlen = (size_t)(wsorted[1] - wsorted[0]);

        // Resolve base objects (types 1-4), drain waiters into DFS tree
        u8bReset(k->buf1);
        for (u32 i = 0; i < hdr.count; i++) {
            if (types_arr[i] < 1 || types_arr[i] > 4) continue;
            pack_obj obj = {};
            u8cs from = {packbase + offsets[i], packbase + packlen};
            if (PACKDrainObjHdr(from, &obj) != OK) { skipped++; continue; }
            if (obj.size > u8bIdleLen(k->buf1)) { skipped++; continue; }
            u8p cs = u8bIdleHead(k->buf1);
            u8s into = {cs, u8bTerm(k->buf1)};
            if (PACKInflate(from, into, obj.size) != OK) { skipped++; continue; }
            sha1 sha = {};
            { u8csc _c = {cs, cs + obj.size}; keep_git_sha1(&sha, obj.type, _c); }
            u64 sha_key = 0;
            memcpy(&sha_key, sha.data, 8);
            sorted_entries[total_indexed].key =
                keepKeyPack(types_arr[i], WHIFFHashlet60(&sha));
            sorted_entries[total_indexed].val =
                wh64Pack(KEEP_VAL_FLAGS, file_id, offsets[i]);
            total_indexed++;
            resolved[i + 1] = YES;
            keep_drain_waiters(wbuf, wlen, nodes, resolved,
                               sha_key, i + 1);
            // Don't advance buf1 — reuse space for next base
        }

        u32 base_count = total_indexed;
        fprintf(stderr, "keeper: %u base objects resolved\n", base_count);

        // Single-pass DFS: walk from each base with children.
        // After resolving each child, drain its waiters from the
        // sorted waiter buffer into the DFS tree.
        sha1 sha = {};
        #define MAX_CHAIN 64
        struct { u8p d_start; u8p d_end; u32 node; u8 base_type; } stk[MAX_CHAIN];

        for (u32 root_idx = 1; root_idx <= hdr.count; root_idx++) {
            if (!nodes[root_idx].child) continue;
            if (!resolved[root_idx]) continue;
            if (types_arr[root_idx - 1] < 1 || types_arr[root_idx - 1] > 4) continue;

            u8 root_type = types_arr[root_idx - 1];
            u8bReset(k->buf1);
            pack_obj obj = {};
            u8cs from = {packbase + offsets[root_idx - 1], packbase + packlen};
            if (PACKDrainObjHdr(from, &obj) != OK) { skipped++; continue; }
            if (obj.size > u8bIdleLen(k->buf1)) { skipped++; continue; }
            u8p cs = u8bIdleHead(k->buf1);
            u8s rinto = {cs, u8bTerm(k->buf1)};
            if (PACKInflate(from, rinto, obj.size) != OK) { skipped++; continue; }
            u8bFed(k->buf1, obj.size);

            int top = 0;
            stk[0].d_start = cs;
            stk[0].d_end = cs + obj.size;
            stk[0].node = root_idx;
            stk[0].base_type = root_type;

            while (top >= 0) {
                u32 cur = stk[top].node;
                u32 child = nodes[cur].child;
                if (!child) {
                    if (top > 0)
                        ((u8**)k->buf1)[2] = stk[top].d_start;
                    top--;
                    continue;
                }

                nodes[cur].child = nodes[child].sibling;

                if (top + 1 >= MAX_CHAIN) {
                    fprintf(stderr, "keeper: chain depth %d exceeded\n", MAX_CHAIN);
                    skipped++; continue;
                }

                u8p base_s = stk[top].d_start;
                u64 base_sz = (u64)(stk[top].d_end - stk[top].d_start);

                pack_obj dobj = {};
                u8cs dfrom = {packbase + offsets[child - 1], packbase + packlen};
                ok64 _ho = PACKDrainObjHdr(dfrom, &dobj);
                if (_ho != OK) {
                    fprintf(stderr, "keeper: DFS hdr fail at offset %llu: %s\n",
                            (unsigned long long)offsets[child - 1], ok64str(_ho));
                    skipped++; continue;
                }

                if (dobj.size > KEEP_BUFSZ / 2) {
                    fprintf(stderr, "keeper: object too large (%llu)\n",
                            (unsigned long long)dobj.size);
                    skipped++; continue;
                }
                u8s dinto = {objbuf, objbuf + KEEP_BUFSZ / 2};
                ok64 _io = PACKInflate(dfrom, dinto, dobj.size);
                if (_io != OK) {
                    fprintf(stderr, "keeper: inflate fail: %s\n", ok64str(_io));
                    skipped++; continue;
                }

                u8cs delta_sl = {objbuf, objbuf + dobj.size};
                u8cs base_sl = {base_s, base_s + base_sz};
                u8p rstart = u8bIdleHead(k->buf1);
                u8g aout = {rstart, rstart, u8bTerm(k->buf1)};
                ok64 _do = DELTApply(delta_sl, base_sl, aout);
                if (_do != OK) {
                    fprintf(stderr, "keeper: delta apply fail: %s\n", ok64str(_do));
                    skipped++; continue;
                }
                u64 rsz = u8gLeftLen(aout);
                u8bFed(k->buf1, rsz);

                { u8csc _c = {rstart, rstart + rsz}; keep_git_sha1(&sha, stk[0].base_type, _c); }
                sorted_entries[total_indexed].key =
                    keepKeyPack(stk[0].base_type, WHIFFHashlet60(&sha));
                sorted_entries[total_indexed].val =
                    wh64Pack(KEEP_VAL_FLAGS, file_id, offsets[child - 1]);
                total_indexed++;
                resolved[child] = YES;

                u64 sha_key = 0;
                memcpy(&sha_key, sha.data, 8);
                keep_drain_waiters(wbuf, wlen, nodes, resolved,
                                   sha_key, child);

                top++;
                stk[top].d_start = rstart;
                stk[top].d_end = rstart + rsz;
                stk[top].node = child;
                stk[top].base_type = stk[0].base_type;
            }
        }

        fprintf(stderr, "keeper: DFS indexed %u deltas from %u bases\n",
                total_indexed - base_count, base_count);

        wh128bFree(waiters_buf);

        // Cross-pack REF_DELTA resolution: unresolved deltas whose base
        // is in a previously-fetched pack (thin pack support).
        {
            u32 cross_resolved = 0;
            for (u32 i = 0; i < hdr.count; i++) {
                if (resolved[i + 1]) continue;
                if (types_arr[i] != PACK_OBJ_REF_DELTA) continue;

                pack_obj obj = {};
                u8cs from = {packbase + offsets[i], packbase + packlen};
                if (PACKDrainObjHdr(from, &obj) != OK) continue;

                // Look up base in full keeper index (previous packs)
                u64 base_hashlet = WHIFFHashlet60((sha1cp)obj.ref_delta[0]);
                u8 base_type = 0;
                u8bReset(k->buf3);
                ok64 go = KEEPGet(k, base_hashlet, 15, k->buf3, &base_type);
                if (go != OK) continue;

                // Inflate this delta
                u8bReset(k->buf4);
                if (obj.size > KEEP_BUFSZ / 2) continue;
                u64 idle_before = u8bIdleLen(k->buf4);
                if (ZINFInflate(u8bIdle(k->buf4), from) != OK) continue;
                u64 produced = idle_before - u8bIdleLen(k->buf4);
                if (produced == 0) continue;
                u8bFed(k->buf4, produced);

                // Apply delta
                a_dup(u8c, delta_sl, u8bDataC(k->buf4));
                a_dup(u8c, base_sl, u8bData(k->buf3));
                u8p rstart = u8bIdleHead(k->buf1);
                u8g aout = {rstart, rstart, u8bTerm(k->buf1)};
                if (DELTApply(delta_sl, base_sl, aout) != OK) continue;
                u64 rsz = u8gLeftLen(aout);

                // Compute SHA
                sha1 sha = {};
                { u8csc _c = {rstart, rstart + rsz}; keep_git_sha1(&sha, base_type, _c); }
                sorted_entries[total_indexed].key =
                    keepKeyPack(base_type, WHIFFHashlet60(&sha));
                sorted_entries[total_indexed].val =
                    wh64Pack(KEEP_VAL_FLAGS, file_id, offsets[i]);
                total_indexed++;
                cross_resolved++;
                resolved[i + 1] = YES;
            }
            if (cross_resolved > 0)
                fprintf(stderr, "keeper: cross-pack: resolved %u REF_DELTAs\n",
                        cross_resolved);
        }

        // Diagnostic: count unresolved by type
        {
            u32 unres_ofs = 0, unres_ref = 0, unres_base = 0, unres_noscan = 0;
            for (u32 i = 0; i < hdr.count; i++) {
                if (resolved[i + 1]) continue;
                if (types_arr[i] == 0) unres_noscan++;
                else if (types_arr[i] == PACK_OBJ_OFS_DELTA) unres_ofs++;
                else if (types_arr[i] == PACK_OBJ_REF_DELTA) unres_ref++;
                else unres_base++;
            }
            if (unres_ofs || unres_ref || unres_base || unres_noscan)
                fprintf(stderr, "keeper: unresolved: %u ofs, %u ref, %u base, %u unscannable\n",
                        unres_ofs, unres_ref, unres_base, unres_noscan);
        }

        free(resolved);
        fprintf(stderr, "keeper: indexed %u objects (%u skipped)\n",
                total_indexed, skipped);

        // Sort and dedup (wh128 dedup: same key+val only)
        if (total_indexed > 0) {
            wh128s sorted = {sorted_entries, sorted_entries + total_indexed};
            wh128sSort(sorted);
            wh128sDedup(sorted);
            u32 nfinal = (u32)(sorted[1] - sorted[0]);

            a_cstr(kdir, k->dir);
            a_pad(u8, idxpath, 1024);
            u8bFeed(idxpath, kdir);
            a_cstr(idxsep, "/" KEEP_IDX_DIR "/");
            u8bFeed(idxpath, idxsep);
            u32 idx_id = k->nruns + 1;
            RONu8sFeedPad(u8bIdle(idxpath), (u64)idx_id, KEEP_SEQNO_W);
            ((u8 **)idxpath)[2] += KEEP_SEQNO_W;
            a_cstr(ext, KEEP_IDX_EXT);
            u8bFeed(idxpath, ext);
            PATHu8gTerm(PATHu8gIn(idxpath));

            int fd = -1;
            FILECreate(&fd, PATHu8cgIn(idxpath));
            if (fd >= 0) {
                u8cs data = {(u8cp)sorted_entries, (u8cp)(sorted_entries + nfinal)};
                FILEFeedAll(fd, data);
                close(fd);
            }

            // Mmap into keeper's runs
            u8bp mapped = NULL;
            if (FILEMapRO(&mapped, PATHu8cgIn(idxpath)) == OK &&
                k->nruns < KEEP_MAX_LEVELS) {
                wh128cp base = (wh128cp)u8bDataHead(mapped);
                size_t n = (u8bIdleHead(mapped) - u8bDataHead(mapped)) / sizeof(wh128);
                k->runs[k->nruns][0] = base;
                k->runs[k->nruns][1] = base + n;
                k->run_maps[k->nruns] = mapped;
                k->nruns++;
            }

            // TODO: compact LSM if 1/8 invariant violated
        }

        free(sorted_entries);
        free(nodes);
        free(offsets);
        free(types_arr);
    }

sync_done:
    if (packbuf) { FILETrimBook(packbuf); FILEUnBook(packbuf); }
    u8bUnMap(rbuf_b); u8bUnMap(objbuf_b);
    { int status; waitpid(pid, &status, 0); }

    // Record refs in the reflog
    if (nrefs > 0) {
        a_cstr(kdir, k->dir);
        ref *refarr = calloc(nrefs, sizeof(ref));
        if (refarr) {
            time_t _t = time(NULL);
            struct tm *_tm = localtime(&_t);
            ron60 now = 0;
            RONOfTime(&now, _tm);
            // Build ?refname and ?sha strings into a shared pad
            // Each ref needs max ~260 bytes for key + ~44 for val
            Bu8 strbuf = {};
            u8bMap(strbuf, (u64)nrefs * 310);
            for (u32 i = 0; i < nrefs; i++) {
                refarr[i].time = now;
                refarr[i].type = REF_SHA;
                // key = "?refname"
                refarr[i].key[0] = u8bIdleHead(strbuf);
                u8bFeed1(strbuf, '?');
                a_cstr(rn, refnames[i]);
                u8bFeed(strbuf, rn);
                refarr[i].key[1] = u8bIdleHead(strbuf);
                // val = "?sha"
                refarr[i].val[0] = u8bIdleHead(strbuf);
                u8bFeed1(strbuf, '?');
                u8cs sha = {refrec[i].data, refrec[i].data + 40};
                u8bFeed(strbuf, sha);
                refarr[i].val[1] = u8bIdleHead(strbuf);
            }
            ok64 ro = REFSSyncRecord(kdir, refarr, nrefs);
            u8bUnMap(strbuf);
            if (ro != OK)
                fprintf(stderr, "keeper: warning: failed to record refs\n");
            else
                fprintf(stderr, "keeper: recorded %u ref(s)\n", nrefs);
            free(refarr);
        }
    }

    fprintf(stderr, "keeper: sync complete\n");
    done;

sync_fail:
    if (rbuf_b[0]) u8bUnMap(rbuf_b);
    if (objbuf_b[0]) u8bUnMap(objbuf_b);
    if (wfd >= 0) close(wfd);
    if (rfd >= 0) close(rfd);
    kill(pid, SIGTERM);
    { int status; waitpid(pid, &status, 0); }
    return KEEPFAIL;
}
