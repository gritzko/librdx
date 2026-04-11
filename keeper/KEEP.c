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
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/RON.h"
#include "dog/HOME.h"

// kv64 templates for LSM
#define X(M, name) M##kv64##name
#include "abc/QSORTx.h"
#include "abc/HASHx.h"
#undef X

fun void kv64csSwap(kv64cs *a, kv64cs *b) {
    kv64c *t0 = (*a)[0], *t1 = (*a)[1];
    (*a)[0] = (*b)[0]; (*a)[1] = (*b)[1];
    (*b)[0] = t0; (*b)[1] = t1;
}

#define X(M, name) M##kv64##name
#include "abc/MSETx.h"
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
            char cwd[1024];
            if (!getcwd(cwd, sizeof(cwd))) fail(KEEPFAIL);
            char *p = cwd + strlen(cwd);
            while (p > cwd) {
                char probe[1100];
                snprintf(probe, sizeof(probe), "%.*s/.dogs",
                         (int)(p - cwd), cwd);
                struct stat st;
                if (stat(probe, &st) == 0 && S_ISDIR(st.st_mode)) {
                    u8cs found = {(u8cp)cwd, (u8cp)p};
                    call(PATHu8bFeed, out, found);
                    goto found_root;
                }
                while (p > cwd && *(p-1) != '/') p--;
                if (p > cwd) p--;
            }
            fail(KEEPFAIL);
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

    // Pre-allocate working buffers for KEEPGet (mmap, reset per call)
    call(u8bMap, k->buf1, KEEP_BUFSZ);
    call(u8bMap, k->buf2, KEEP_BUFSZ);

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

ok64 KEEPGet(keeper *k, u64 hashlet, size_t hexlen, u8bp out, u8p out_type) {
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
    if (out_type) *out_type = obj_type;

    // Copy result into caller's output buffer
    {
        u8cs content = {result, result + outsz};
        rc = u8bFeed(out, content);
    }

cleanup:
    return rc;
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

static ok64 keep_verify_sha(keeper *k, u8 expected_sha[20],
                             u32 *checked, u32 *failed) {
    u64 hashlet = wh64Hashlet(expected_sha);
    if (verify_seen(hashlet)) return OK;  // already verified
    verify_mark(hashlet);

    #define VERIFY_BUFSZ (1ULL << 24)  // 16 MB
    u8p objmem = malloc(VERIFY_BUFSZ);
    if (!objmem) return KEEPNOROOM;
    Bu8 obj = {};
    obj[0] = obj[1] = obj[2] = objmem;
    obj[3] = objmem + VERIFY_BUFSZ;
    u8 obj_type = 0;

    ok64 rc = KEEPGet(k, hashlet, 10, obj, &obj_type);
    if (rc != OK) {
        char hex[12];
        wh64HashletHex(hex, hashlet, 10);
        fprintf(stderr, "  MISS: %s\n", hex);
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

    u8 actual_sha[20];
    SHA1Sum(actual_sha, tmp, hlen + 1 + content_sz);
    free(tmp);

    if (memcmp(actual_sha, expected_sha, 20) != 0) {
        char hex_exp[12], hex_got[12];
        wh64HashletHex(hex_exp, wh64Hashlet(expected_sha), 10);
        wh64HashletHex(hex_got, wh64Hashlet(actual_sha), 10);
        fprintf(stderr, "  HASH MISMATCH: expected %s got %s\n", hex_exp, hex_got);
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
                u8 tree_sha[20];
                u8s sb = {tree_sha, tree_sha + 20};
                u8cs hx = {value[0], value[0] + 40};
                HEXu8sDrainSome(sb, hx);
                ok64 o = keep_verify_sha(k, tree_sha, checked, failed);
                if (o != OK) {
                    char hex[12];
                    wh64HashletHex(hex, wh64Hashlet(tree_sha), 10);
                    fprintf(stderr, "  tree %s verify failed\n", hex);
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
            u8 child_sha[20];
            memcpy(child_sha, entry_sha[0], 20);
            o = keep_verify_sha(k, child_sha, checked, failed);
            if (o != OK) {
                char hex[12];
                wh64HashletHex(hex, wh64Hashlet(child_sha), 10);
                fprintf(stderr, "  child %s verify failed\n", hex);
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
                u8 target_sha[20];
                u8s sb = {target_sha, target_sha + 20};
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

    u8 sha[20];
    u8s sb = {sha, sha + 20};
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

// --- Sync: clone or update from remote via git-upload-pack ---

#include "PKT.h"
#include "ZINF.h"
#include <sys/wait.h>

// Compute git object SHA-1: hash("<type> <size>\0<content>")
static void keep_git_sha1(u8 sha[20], u8 type, u8cp content, u64 sz, u8p tmp) {
    static char const *type_str[] = {
        [1] = "commit", [2] = "tree", [3] = "blob", [4] = "tag"};
    char hbuf[64];
    int hlen = snprintf(hbuf, sizeof(hbuf), "%s %lu",
                        type_str[type], (unsigned long)sz);
    memcpy(tmp, hbuf, hlen);
    tmp[hlen] = 0;
    memcpy(tmp + hlen + 1, content, sz);
    SHA1Sum(sha, tmp, hlen + 1 + sz);
}

// Resolve object at offset, chasing OFS/REF deltas via hash table idx.
static ok64 keep_resolve(u8cp pack, u64 packlen, kv64s idx,
                          u64 off, u8 *out_type,
                          u8p buf1, u8p buf2, u64 bufsz,
                          u8p *result, u64 *outsz) {
    sane(pack && buf1 && buf2 && result && outsz);
    u64 chain[256];
    int depth = 0;
    u64 cur = off;

    for (;;) {
        pack_obj obj = {};
        u8cs from = {pack + cur, pack + packlen};
        call(PACKDrainObjHdr, from, &obj);

        if (obj.type >= 1 && obj.type <= 4) {
            *out_type = obj.type;
            if (obj.size > bufsz) return KEEPNOROOM;
            u8s into = {buf1, buf1 + bufsz};
            call(PACKInflate, from, into, obj.size);
            *result = buf1;
            *outsz = obj.size;
            break;
        }

        if (depth >= 256) return KEEPFAIL;
        chain[depth++] = cur;

        if (obj.type == PACK_OBJ_OFS_DELTA) {
            cur = cur - obj.ofs_delta;
        } else if (obj.type == PACK_OBJ_REF_DELTA) {
            u64 sha_key = 0;
            memcpy(&sha_key, obj.ref_delta[0], 8);
            kv64 lookup = {.key = sha_key, .val = 0};
            ok64 o = HASHkv64Get(&lookup, idx);
            if (o != OK) return o;
            cur = lookup.val;
        } else {
            return KEEPFAIL;
        }
    }

    u8p src = buf1;
    u8p dst = buf2;
    for (int i = depth - 1; i >= 0; i--) {
        pack_obj dobj = {};
        u8cs from = {pack + chain[i], pack + packlen};
        call(PACKDrainObjHdr, from, &dobj);
        u8p dinst = dst + bufsz / 2;
        if (dobj.size > bufsz / 2) return KEEPNOROOM;
        u8s dinto = {dinst, dinst + bufsz / 2};
        call(PACKInflate, from, dinto, dobj.size);
        u8cs delta = {dinst, dinst + dobj.size};
        u8cs base = {src, src + *outsz};
        u8g out = {dst, dst, dst + bufsz / 2};
        call(DELTApply, delta, base, out);
        *outsz = u8gLeftLen(out);
        u8p tmp = src; src = dst; dst = tmp;
    }
    *result = src;
    done;
}

ok64 KEEPSync(keeper *k, u8cs remote,
              char const *const *wants, char const *const *haves) {
    sane(k);

    if ($empty(remote)) {
        fprintf(stderr, "keeper: sync requires a remote\n");
        return KEEPFAIL;
    }

    // Parse remote: "ssh host path" or just "path" for local
    char remote_str[1024];
    snprintf(remote_str, sizeof(remote_str), "%.*s",
             (int)$len(remote), (char *)remote[0]);

    // Build command: ssh host git-upload-pack 'path'
    // or local: git-upload-pack 'path'
    char cmd[2048];
    char *space = strchr(remote_str, ' ');
    if (space) {
        *space = 0;
        char *host = remote_str;
        char *path = space + 1;
        snprintf(cmd, sizeof(cmd), "ssh %s git-upload-pack '%s'", host, path);
    } else {
        snprintf(cmd, sizeof(cmd), "git-upload-pack '%s'", remote_str);
    }

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
    char head_hex[44];
    memcpy(head_hex, line[0], 40);
    head_hex[40] = 0;

    // Drain remaining refs until flush
    #define MAX_REFS 1024
    char refs[MAX_REFS][44];
    u32 nrefs = 0;
    memcpy(refs[nrefs++], head_hex, 41);

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
        if ($len(line) >= 40 && nrefs < MAX_REFS) {
            // Skip peeled tag lines (contain ^{})
            u8cp hat = line[0];
            b8 peeled = NO;
            while (hat < line[1] - 2)
                if (*hat == '^' && *(hat+1) == '{') { peeled = YES; break; }
                else hat++;
            if (peeled) continue;
            // Skip duplicate SHAs
            b8 dup = NO;
            for (u32 j = 0; j < nrefs; j++)
                if (memcmp(refs[j], line[0], 40) == 0) { dup = YES; break; }
            if (dup) continue;
            memcpy(refs[nrefs], line[0], 40);
            refs[nrefs][40] = 0;
            nrefs++;
        }
    }

    fprintf(stderr, "keeper: %u ref(s), HEAD=%.12s\n", nrefs, head_hex);

    // Build want/have negotiation
    {
        u8 wbuf[65536];
        u8s ws = {wbuf, wbuf + sizeof(wbuf)};
        b8 first_want = YES;

        if (wants) {
            // Specific wants: match against advertised refs
            for (int wi = 0; wants[wi]; wi++) {
                // Verify this SHA was advertised
                b8 advertised = NO;
                for (u32 j = 0; j < nrefs; j++)
                    if (memcmp(refs[j], wants[wi], 40) == 0)
                        { advertised = YES; break; }
                if (!advertised) {
                    fprintf(stderr, "keeper: want %.12s not advertised, skipping\n",
                            wants[wi]);
                    continue;
                }
                char pay[256];
                int plen;
                if (first_want) {
                    plen = snprintf(pay, sizeof(pay),
                        "want %.40s no-progress\n", wants[wi]);
                    first_want = NO;
                } else {
                    plen = snprintf(pay, sizeof(pay),
                        "want %.40s\n", wants[wi]);
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
                        "want %.40s no-progress\n", refs[i]);
                    first_want = NO;
                } else {
                    plen = snprintf(pay, sizeof(pay),
                        "want %.40s\n", refs[i]);
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
        if (haves) {
            for (int hi = 0; haves[hi]; hi++) {
                char pay[256];
                int plen = snprintf(pay, sizeof(pay),
                    "have %.40s\n", haves[hi]);
                u8cs payload = {(u8cp)pay, (u8cp)pay + plen};
                PKTu8sFeed(ws, payload);
            }
        }

        u8 donepay[] = "done\n";
        u8cs donecs = {donepay, donepay + 5};
        PKTu8sFeed(ws, donecs);

        u64 wlen = ws[0] - wbuf;
        if (write(wfd, wbuf, wlen) != (ssize_t)wlen) goto sync_fail;
    }
    close(wfd);
    wfd = -1;

    // Read packfile response
    rlen = 0;
    for (;;) {
        ssize_t n = read(rfd, rbuf + rlen, SYNC_BUFSZ - rlen);
        if (n <= 0) break;
        rlen += (u64)n;
        if (rlen >= SYNC_BUFSZ) break;
    }
    close(rfd);
    rfd = -1;

    // Parse response: NAK (full clone) or ACK <sha> (incremental)
    u8cs resp = {rbuf, rbuf + rlen};
    po = PKTu8sDrain(resp, line);
    if (po != OK) goto sync_fail;
    if ($len(line) >= 3 && memcmp(line[0], "NAK", 3) == 0) {
        // Full clone response
    } else if ($len(line) >= 3 && memcmp(line[0], "ACK", 3) == 0) {
        // Incremental: may have multiple ACK lines before pack
        for (;;) {
            ok64 ao = PKTu8sDrain(resp, line);
            if (ao == PKTFLUSH) break;
            if (ao != OK) break;
            if ($len(line) >= 3 && memcmp(line[0], "NAK", 3) == 0) break;
            // More ACK lines — skip
        }
    } else {
        fprintf(stderr, "keeper: unexpected response: %.*s\n",
                (int)$len(line), (char *)line[0]);
        goto sync_fail;
    }

    u8cp packbase = resp[0];
    pack_hdr hdr = {};
    po = PACKDrainHdr(resp, &hdr);
    if (po != OK || hdr.version != 2) goto sync_fail;
    u64 packlen = rlen - (packbase - rbuf);

    fprintf(stderr, "keeper: received %u objects, %llu bytes\n",
            hdr.count, (unsigned long long)packlen);

    // Save packfile
    u32 file_id = k->npacks + 1;
    {
        a_pad(u8, dst, 1024);
        u8cs kdir = {(u8cp)k->dir, (u8cp)k->dir + strlen(k->dir)};
        u8bFeed(dst, kdir);
        u8bFeed1(dst, '/');
        RONu8sFeedPad(u8bIdle(dst), (u64)file_id, KEEP_SEQNO_W);
        ((u8 **)dst)[2] += KEEP_SEQNO_W;
        a_cstr(ext, KEEP_PACK_EXT);
        u8bFeed(dst, ext);
        PATHu8gTerm(PATHu8gIn(dst));

        int fd = -1;
        ok64 o = FILECreate(&fd, PATHu8cgIn(dst));
        if (o != OK) goto sync_fail;
        u8cs data = {packbase, packbase + packlen};
        FILEFeedall(fd, data);
        close(fd);
    }

    // Scan pack: record offsets + types
    {
        u64 *offsets = calloc(hdr.count, sizeof(u64));
        u8 *types_arr = calloc(hdr.count, 1);
        if (!offsets || !types_arr) { free(offsets); free(types_arr); goto sync_fail; }

        u8cs scan = {resp[0], rbuf + rlen};
        for (u32 i = 0; i < hdr.count; i++) {
            offsets[i] = scan[0] - packbase;
            pack_obj obj = {};
            if (PACKDrainObjHdr(scan, &obj) != OK) break;
            types_arr[i] = obj.type;
            u64 consumed = 0, produced = 0;
            u64 outsz = obj.size < SYNC_BUFSZ ? obj.size : SYNC_BUFSZ;
            ZINFInflate(scan[0], $len(scan), buf1, outsz, &consumed, &produced);
            scan[0] += consumed;
        }

        // Multi-pass delta resolution + SHA-1 → kv64 index
        u64 tblsz = ((u64)hdr.count * 4 + 15) & ~15ULL;
        if (tblsz < 256) tblsz = 256;
        kv64 *idxmem = calloc(tblsz, sizeof(kv64));
        if (!idxmem) { free(offsets); free(types_arr); goto sync_fail; }
        kv64s idx = {idxmem, idxmem + tblsz};

        b8 *done_flag = calloc(hdr.count, 1);
        if (!done_flag) { free(idxmem); free(offsets); free(types_arr); goto sync_fail; }

        u32 total_indexed = 0;
        kv64 *sorted_entries = malloc(hdr.count * sizeof(kv64));
        u32 nsorted = 0;

        for (int pass = 0; pass < 32; pass++) {
            u32 progress = 0;
            for (u32 i = 0; i < hdr.count; i++) {
                if (done_flag[i]) continue;
                u8 obj_type = 0;
                u8p content = NULL;
                u64 content_sz = 0;

                if (types_arr[i] >= 1 && types_arr[i] <= 4) {
                    obj_type = types_arr[i];
                    pack_obj obj = {};
                    u8cs from = {packbase + offsets[i], packbase + packlen};
                    if (PACKDrainObjHdr(from, &obj) != OK) { done_flag[i] = 1; continue; }
                    if (obj.size > SYNC_BUFSZ) { done_flag[i] = 1; continue; }
                    u8s into = {buf1, buf1 + SYNC_BUFSZ};
                    if (PACKInflate(from, into, obj.size) != OK) { done_flag[i] = 1; continue; }
                    content = buf1;
                    content_sz = obj.size;
                } else {
                    if (keep_resolve(packbase, packlen, idx,
                                      offsets[i], &obj_type,
                                      buf1, buf2, SYNC_BUFSZ,
                                      &content, &content_sz) != OK) continue;
                }

                if (obj_type < 1 || obj_type > 4) { done_flag[i] = 1; continue; }

                u8 sha[20];
                keep_git_sha1(sha, obj_type, content, content_sz, objbuf);

                // Insert into hash table for REF_DELTA resolution
                u64 sha_key = 0;
                memcpy(&sha_key, sha, 8);
                kv64 entry = {.key = sha_key, .val = offsets[i]};
                HASHkv64Put(idx, &entry);

                // Also build sorted kv64 for LSM index
                if (sorted_entries && nsorted < hdr.count) {
                    u64 hashlet = wh64Hashlet(sha);
                    sorted_entries[nsorted].key = wh64Pack(HASH_SHA1, 0, hashlet);
                    sorted_entries[nsorted].val = wh64Pack(KEEP_PACK, file_id, offsets[i]);
                    nsorted++;
                }

                done_flag[i] = 1;
                total_indexed++;
                progress++;
            }
            fprintf(stderr, "keeper: pass %d: +%u (%u/%u)\n",
                    pass, progress, total_indexed, hdr.count);
            if (progress == 0) break;
        }

        // Sort and write LSM index
        if (sorted_entries && nsorted > 0) {
            kv64s sorted = {sorted_entries, sorted_entries + nsorted};
            kv64sSort(sorted);
            kv64sDedup(sorted);
            nsorted = (u32)(sorted[1] - sorted[0]);

            a_pad(u8, idxpath, 1024);
            u8cs kdir = {(u8cp)k->dir, (u8cp)k->dir + strlen(k->dir)};
            u8bFeed(idxpath, kdir);
            u8bFeed1(idxpath, '/');
            RONu8sFeedPad(u8bIdle(idxpath), (u64)file_id, KEEP_SEQNO_W);
            ((u8 **)idxpath)[2] += KEEP_SEQNO_W;
            a_cstr(ext, KEEP_IDX_EXT);
            u8bFeed(idxpath, ext);
            PATHu8gTerm(PATHu8gIn(idxpath));

            int fd = -1;
            FILECreate(&fd, PATHu8cgIn(idxpath));
            if (fd >= 0) {
                u8cs data = {(u8cp)sorted_entries, (u8cp)(sorted_entries + nsorted)};
                FILEFeedall(fd, data);
                close(fd);
            }
        }

        free(sorted_entries);
        free(done_flag);
        free(idxmem);
        free(offsets);
        free(types_arr);
    }

sync_done:
    u8bUnMap(rbuf_b); u8bUnMap(objbuf_b);
    { int status; waitpid(pid, &status, 0); }

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
