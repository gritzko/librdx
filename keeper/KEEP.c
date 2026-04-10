//  KEEP: local git object store.
//
//  Stores git packfiles under .dogs/keeper/, indexed by u64→w64
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
    size_t dlen = u8bDataLen(dir);
    if (dlen >= sizeof(k->dir)) dlen = sizeof(k->dir) - 1;
    memcpy(k->dir, u8bDataHead(dir), dlen);
    k->dir[dlen] = 0;
    u8cs keepdir = {(u8cp)k->dir, (u8cp)k->dir + dlen};

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

// --- Lookup: hashlet → wh64 val ---
// hexlen: number of significant hex chars in the hashlet (6-10).
// With 40-bit hashlets, max is 10.

ok64 KEEPLookup(keeper *k, u64 hashlet, size_t hexlen, u64p val) {
    sane(k && val);

    // Build range for prefix matching.
    // Hashlet is in MS 40 bits of wh64, so pack with type=0, id=0.
    // key_lo: hashlet with unspecified nibbles zeroed.
    // key_hi: hashlet with unspecified nibbles set to 0xf.
    if (hexlen > 10) hexlen = 10;
    u64 nbits = hexlen * 4;
    u64 shift = 40 - nbits;  // unspecified bits within the 40-bit field
    u64 hmask = shift < 40 ? (WHIFF_OFF_MASK >> shift) << shift : WHIFF_OFF_MASK;
    u64 hpre = hashlet & hmask;

    u64 key_lo = wh64Pack(0, 0, hpre);
    u64 key_hi = wh64Pack(0xf, WHIFF_ID_MASK, hpre | (~hmask & WHIFF_OFF_MASK));

    for (u32 r = 0; r < k->nruns; r++) {
        kv64cp base = k->runs[r][0];
        size_t len = (size_t)(k->runs[r][1] - base);
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

ok64 KEEPHas(keeper *k, u64 hashlet, size_t hexlen) {
    u64 val = 0;
    return KEEPLookup(k, hashlet, hexlen, &val);
}

// --- Get: inflate object from pack ---

#define KEEP_BUFSZ (1ULL << 26)  // 64 MB working buffer

ok64 KEEPGet(keeper *k, u64 hashlet, size_t hexlen, u8bp out) {
    sane(k && out);

    u64 val = 0;
    call(KEEPLookup, k, hashlet, hexlen, &val);

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
            u64 base_hashlet = wh64Hashlet(obj.ref_delta[0]);
            u64 base_val = 0;
            rc = KEEPLookup(k, base_hashlet, 10, &base_val);
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
                u8 sha[20];
                SHA1Sum(sha, NULL, 0);  // placeholder — need proper hash
                // FIXME: compute proper git object SHA-1 (header + content)

                u64 hashlet = wh64Hashlet(sha);
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

// --- Import: read git .idx v2 file alongside .pack, build kv64 index ---
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
    u8cs kdir = {(u8cp)k->dir, (u8cp)k->dir + strlen(k->dir)};
    {
        a_pad(u8, dst, 1024);
        call(u8bFeed, dst, kdir);
        call(u8bFeed1, dst, '/');
        call(RONu8sFeedPad, u8bIdle(dst), (u64)file_id, KEEP_SEQNO_W);
        ((u8 **)dst)[2] += KEEP_SEQNO_W;
        a_cstr(ext, KEEP_PACK_EXT);
        call(u8bFeed, dst, ext);
        call(PATHu8gTerm, PATHu8gIn(dst));

        int fd = -1;
        call(FILECreate, &fd, PATHu8cgIn(dst));
        u8cs data = {u8bDataHead(pack_map),
                     u8bDataHead(pack_map) + (u8bIdleHead(pack_map) - u8bDataHead(pack_map))};
        call(FILEFeedall, fd, data);
        close(fd);
    }

    // Build kv64 entries from the idx tables
    kv64 *entries = (kv64 *)malloc((u64)nobjects * sizeof(kv64));
    if (!entries) { FILEUnMap(pack_map); FILEUnMap(idx_map); failc(KEEPNOROOM); }

    for (u32 i = 0; i < nobjects; i++) {
        u8cp sha = sha_table + (u64)i * 20;
        u64 hashlet = wh64Hashlet(sha);

        // 4-byte offset (BE), high bit = large offset flag
        u8cp offp = off_table + (u64)i * 4;
        u64 off = ((u64)offp[0] << 24) | ((u64)offp[1] << 16) |
                  ((u64)offp[2] << 8) | offp[3];

        if (off & 0x80000000ULL) {
            // Large offset: index into 8-byte offset table
            // FIXME: handle >2GB packs
            off &= 0x7FFFFFFF;
        }

        entries[i].key = wh64Pack(HASH_SHA1, 0, hashlet);
        entries[i].val = wh64Pack(KEEP_PACK, file_id, off);
    }

    // Sort and dedup
    kv64s sorted = {entries, entries + nobjects};
    kv64sSort(sorted);
    kv64sDedup(sorted);
    u32 nentries = (u32)(sorted[1] - sorted[0]);

    // Write .idx file
    {
        u64 seqno = (u64)file_id;
        a_pad(u8, idxpath, 1024);
        call(u8bFeed, idxpath, kdir);
        call(u8bFeed1, idxpath, '/');
        call(RONu8sFeedPad, u8bIdle(idxpath), seqno, KEEP_SEQNO_W);
        ((u8 **)idxpath)[2] += KEEP_SEQNO_W;
        a_cstr(ext2, KEEP_IDX_EXT);
        call(u8bFeed, idxpath, ext2);
        call(PATHu8gTerm, PATHu8gIn(idxpath));

        int fd = -1;
        call(FILECreate, &fd, PATHu8cgIn(idxpath));
        u8cs data = {(u8cp)entries, (u8cp)(entries + nentries)};
        call(FILEFeedall, fd, data);
        close(fd);
    }

    free(entries);
    FILEUnMap(pack_map);
    FILEUnMap(idx_map);

    fprintf(stderr, "keeper: indexed %u objects\n", nentries);
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
