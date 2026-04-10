//  KEEP: local git object store.
//
//  Stores git packfiles under .dogs/keeper/, indexed by hash64→w64
//  in LSM sorted runs of kv64 entries.
//
#include "KEEP.h"

#include "DELT.h"
#include "PACK.h"
#include "SHA1.h"
#include "ZINF.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/RON.h"
#include "dog/HOME.h"

// kv64 templates for LSM
#define X(M, name) M##kv64##name
#include "abc/QSORTx.h"
#undef X

fun void kv64csSwap(kv64cs *a, kv64cs *b) {
    kv64c *t0 = (*a)[0], *t1 = (*a)[1];
    (*a)[0] = (*b)[0]; (*a)[1] = (*b)[1];
    (*b)[0] = t0; (*b)[1] = t1;
}

#define X(M, name) M##kv64##name
#include "abc/MSETx.h"
#undef X

// --- Helpers ---

static ok64 keep_resolve_dir(path8b out, u8cs reporoot) {
    sane(out);
    if ($empty(reporoot)) {
        call(HOMEFindDogs, out);
    } else {
        call(PATHu8bFeed, out, reporoot);
    }
    a_cstr(rel, "/" KEEP_DIR);
    call(u8bFeed, out, rel);
    call(PATHu8gTerm, PATHu8gIn(out));
    done;
}

// --- Open: mmap pack files + load index runs ---

ok64 KEEPOpen(keeper *k, u8cs reporoot) {
    sane(k);
    memset(k, 0, sizeof(*k));

    a_path(dir);
    call(keep_resolve_dir, dir, reporoot);
    a_dup(u8c, keepdir, u8bDataC(dir));
    $mv(k->root, keepdir);

    // Ensure directory exists
    call(FILEMakeDirP, PATHu8cgIn(dir));

    // Scan .packs files
    {
        a_path(dpat);
        call(PATHu8bFeed, dpat, keepdir);
        DIR *d = opendir((char *)u8bDataHead(dpat));
        if (d) {
            char names[KEEP_MAX_FILES][64];
            u32 count = 0;
            struct dirent *e;
            while ((e = readdir(d)) != NULL && count < KEEP_MAX_FILES) {
                size_t nlen = strlen(e->d_name);
                if (nlen < 7) continue;  // SEQNO + .packs
                if (strcmp(e->d_name + nlen - 6, KEEP_PACK_EXT) != 0) continue;
                if (nlen > 63) continue;
                memcpy(names[count], e->d_name, nlen + 1);
                count++;
            }
            closedir(d);

            // Sort by name
            for (u32 i = 0; i + 1 < count; i++)
                for (u32 j = i + 1; j < count; j++)
                    if (strcmp(names[i], names[j]) > 0) {
                        char tmp[64];
                        memcpy(tmp, names[i], 64);
                        memcpy(names[i], names[j], 64);
                        memcpy(names[j], tmp, 64);
                    }

            for (u32 i = 0; i < count; i++) {
                u8cs fn = {(u8cp)names[i], (u8cp)names[i] + strlen(names[i])};
                a_path(fpath, keepdir, fn);
                u8bp mapped = NULL;
                if (FILEMapRO(&mapped, PATHu8cgIn(fpath)) == OK) {
                    k->packs[k->npacks] = mapped;
                    k->npacks++;
                }
            }
        }
    }

    // Scan .idx files
    {
        a_path(dpat);
        call(PATHu8bFeed, dpat, keepdir);
        DIR *d = opendir((char *)u8bDataHead(dpat));
        if (d) {
            char names[KEEP_MAX_LEVELS][64];
            u32 count = 0;
            struct dirent *e;
            while ((e = readdir(d)) != NULL && count < KEEP_MAX_LEVELS) {
                size_t nlen = strlen(e->d_name);
                if (nlen < 5) continue;
                if (strcmp(e->d_name + nlen - 4, KEEP_IDX_EXT) != 0) continue;
                if (nlen > 63) continue;
                memcpy(names[count], e->d_name, nlen + 1);
                count++;
            }
            closedir(d);

            for (u32 i = 0; i + 1 < count; i++)
                for (u32 j = i + 1; j < count; j++)
                    if (strcmp(names[i], names[j]) > 0) {
                        char tmp[64];
                        memcpy(tmp, names[i], 64);
                        memcpy(names[i], names[j], 64);
                        memcpy(names[j], tmp, 64);
                    }

            for (u32 i = 0; i < count; i++) {
                u8cs fn = {(u8cp)names[i], (u8cp)names[i] + strlen(names[i])};
                a_path(fpath, keepdir, fn);
                u8bp mapped = NULL;
                if (FILEMapRO(&mapped, PATHu8cgIn(fpath)) == OK) {
                    kv64cp base = (kv64cp)u8bDataHead(mapped);
                    size_t n = (u8bIdleHead(mapped) - u8bDataHead(mapped)) / sizeof(kv64);
                    k->runs[k->nruns][0] = base;
                    k->runs[k->nruns][1] = base + n;
                    k->run_maps[k->nruns] = mapped;
                    k->nruns++;
                }
            }
        }
    }

    done;
}

// --- Close ---

ok64 KEEPClose(keeper *k) {
    sane(k);
    for (u32 i = 0; i < k->npacks; i++)
        if (k->packs[i]) FILEUnMap(k->packs[i]);
    for (u32 i = 0; i < k->nruns; i++)
        if (k->run_maps[i]) FILEUnMap(k->run_maps[i]);
    memset(k, 0, sizeof(*k));
    done;
}

// --- Lookup: hash64 → w64 val ---

ok64 KEEPLookup(keeper *k, hash64 h, u64p val) {
    sane(k && val);
    // Construct search key: match on the prefix bits, ignoring type
    u64 prefix = hash64Prefix(h);
    kv64 probe = {.key = h, .val = 0};

    // Search each run via binary search
    for (u32 r = 0; r < k->nruns; r++) {
        kv64cp base = k->runs[r][0];
        size_t len = (size_t)(k->runs[r][1] - base);
        size_t lo = 0, hi = len;
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            // Compare on prefix only (mask out type bits in key)
            u64 mk = base[mid].key & HASH_PREFIX_MASK;
            if (mk < prefix) lo = mid + 1;
            else hi = mid;
        }
        if (lo < len && (base[lo].key & HASH_PREFIX_MASK) == prefix) {
            *val = base[lo].val;
            done;
        }
    }
    return KEEPNONE;
}

// --- Has ---

ok64 KEEPHas(keeper *k, hash64 h) {
    u64 val = 0;
    return KEEPLookup(k, h, &val);
}

// --- Get: inflate object from pack ---

#define KEEP_BUFSZ (1ULL << 26)  // 64 MB working buffer

ok64 KEEPGet(keeper *k, hash64 h, u8bp out) {
    sane(k && out);

    u64 val = 0;
    call(KEEPLookup, k, h, &val);

    u32 file_id = wh64Id(val);
    u64 offset  = wh64Off(val);

    if (file_id >= k->npacks) return KEEPNONE;
    u8bp pack_map = k->packs[file_id];
    u8cp pack = u8bDataHead(pack_map);
    u64 packlen = (u64)(u8bIdleHead(pack_map) - pack);

    if (offset >= packlen) return KEEPFAIL;

    // Chase delta chain, resolve to base object
    u64 chain[256];
    int depth = 0;
    u64 cur = offset;
    u8 obj_type = 0;

    // Allocate working buffers
    u8p buf1 = (u8p)malloc(KEEP_BUFSZ);
    u8p buf2 = (u8p)malloc(KEEP_BUFSZ);
    if (!buf1 || !buf2) { free(buf1); free(buf2); return KEEPNOROOM; }

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
            hash64 base_h = hash64FromSha1(obj.ref_delta[0], HASH_SHA1);
            u64 base_val = 0;
            rc = KEEPLookup(k, base_h, &base_val);
            if (rc != OK) goto cleanup;
            // Base might be in a different pack file
            u32 bfile = wh64Id(base_val);
            if (bfile != file_id) {
                // Cross-file delta — need recursive get
                // For now, fail; proper impl would recurse
                rc = KEEPFAIL;
                goto cleanup;
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

    // Copy result into caller's output buffer
    {
        u8cs content = {result, result + outsz};
        rc = u8bFeed(out, content);
    }

cleanup:
    free(buf1);
    free(buf2);
    return rc;
}

// --- Scan ---

ok64 KEEPScan(keeper *k, u64 from_val, keep_cb cb, void *ctx) {
    sane(k && cb);

    u32 file_id = wh64Id(from_val);
    u64 offset  = wh64Off(from_val);

    if (file_id >= k->npacks) return KEEPNONE;
    u8cp pack = u8bDataHead(k->packs[file_id]);
    u64 packlen = (u64)(u8bIdleHead(k->packs[file_id]) - pack);

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
                // Compute SHA-1 for hash64
                // git object = "<type> <size>\0<content>"
                char hdr[64];
                int hlen = snprintf(hdr, sizeof(hdr), "%s %llu",
                    obj.type == 1 ? "commit" :
                    obj.type == 2 ? "tree" :
                    obj.type == 3 ? "blob" : "tag",
                    (unsigned long long)obj.size);
                u8 sha[20];
                SHA1Sum(sha, NULL, 0);  // placeholder — need proper hash
                // FIXME: compute proper git object SHA-1 (header + content)

                hash64 h = hash64FromSha1(sha, HASH_SHA1);
                u8cs content = {buf, buf + obj.size};
                o = cb(obj.type, content, h, ctx);
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

// --- Sync (stub) ---

ok64 KEEPSync(keeper *k, u8cs remote) {
    sane(k);
    (void)remote;
    // FIXME: implement upload-pack negotiation using graf's DAG
    fprintf(stderr, "keeper: sync not yet implemented\n");
    return KEEPFAIL;
}
