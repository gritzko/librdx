//  SNIFF: file path registry + filesystem change log.
//
//  Sorted-array design: checkout paths are sorted (PAST), post-checkout
//  paths append unsorted (DATA).  PAST and DATA separated by \n\n in
//  paths.log.  State entries: 2 per PAST path (HASHLET, CHECKOUT) at
//  fixed positions, post-checkout events appended to tail.
//
#include "SNIFF.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "keeper/GIT.h"
#include "keeper/WALK.h"

fun u32 SNIFFChangesPast(sniff const *s) {
    return (u32)(u8csbDataLen(s->past) * 2 * sizeof(wh64));
}

// Scan newline-delimited paths into slice buffer.  Stops at \n\n or end.
static ok64 SNIFFScanPaths(Bu8cs buf, u8cs region) {
    sane(Bok(buf) && $ok(region));
    while (!$empty(region)) {
        u8cp linestart = region[0];
        u8cs scan = {region[0], region[1]};
        if (u8csFind(scan, '\n') != OK) break;
        u8cs path = {linestart, scan[0]};
        region[0] = scan[0];
        ++region[0];
        if ($empty(path)) continue;
        call(u8cssFeed1, u8csbIdle(buf), path);
    }
    done;
}

// Find \n\n separator.  Returns offset after it, or 0 if absent.
static u32 SNIFFFindSep(u8bp paths) {
    u8cs r = {u8bDataHead(paths), u8bIdleHead(paths)};
    while (!$empty(r)) {
        u8cs scan = {r[0], r[1]};
        if (u8csFind(scan, '\n') != OK) break;
        r[0] = scan[0];
        ++r[0];
        if (!$empty(r) && *r[0] == '\n') {
            ++r[0];
            return (u32)(r[0] - u8bDataHead(paths));
        }
    }
    return 0;
}

// Allocate Bu8cs, scan region into it.
static ok64 SNIFFScanAlloc(Bu8cs *buf, u8cs region) {
    sane(buf);
    size_t est = $len(region) / 8 + 1024;
    call(u8csbAllocate, *buf, est);
    call(SNIFFScanPaths, *buf, region);
    done;
}

// Append a wh64 pair (BLOB + CHECKOUT) to changes.  Bootstrap path:
// ls-files lists only regular files, so the base type is always BLOB.
static ok64 SNIFFWritePair(sniff *s, u32 idx, u64 hashlet, u64 checkout) {
    sane(s && s->changes);
    wh64 h = wh64Pack(SNIFF_BLOB, idx, hashlet);
    wh64 c = wh64Pack(SNIFF_CHECKOUT, idx, checkout);
    call(FILEBookEnsure, s->changes, 2 * sizeof(wh64));
    u8p *idle = (u8p *)&s->changes[2];
    memcpy(*idle, &h, sizeof(wh64));
    *idle += sizeof(wh64);
    memcpy(*idle, &c, sizeof(wh64));
    *idle += sizeof(wh64);
    done;
}

// --- Bootstrap ---

static ok64 SNIFFBootstrap(sniff *s, u8cs reporoot) {
    sane(s && $ok(reporoot));
    a_pad(u8, cmd, 2 * FILE_PATH_MAX_LEN);
    a_cstr(pre, "git -C ");
    a_cstr(suf, " ls-files -s 2>/dev/null");
    call(u8bFeed, cmd, pre);
    call(u8bFeed, cmd, reporoot);
    call(u8bFeed, cmd, suf);
    call(u8bFeed1, cmd, 0);
    FILE *fp = popen((char *)u8bDataHead(cmd), "r");
    if (!fp) done;

    char line[1024];
    u32 count = 0;
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = 0;
        if (len == 0) continue;

        char *tab = strchr(line, '\t');
        if (!tab) continue;
        char *sha_start = strchr(line, ' ');
        if (!sha_start || sha_start >= tab) continue;
        sha_start++;

        a_pad(u8, shabin, 8);
        u8cs hex16 = {(u8cp)sha_start, (u8cp)sha_start};
        if (tab - sha_start >= 16)
            hex16[1] = (u8cp)sha_start + 16;
        else
            continue;
        HEXu8sDrainSome(shabin_idle, hex16);

        a$str(path, tab + 1);
        call(FILEBookEnsure, s->paths, $len(path) + 1);
        u8bFeed(s->paths, path);
        u8bFeed1(s->paths, '\n');

        u64 hashlet = WHIFFHashlet40((sha1cp)u8bDataHead(shabin));
        call(SNIFFWritePair, s, count, hashlet, 0);
        count++;
    }
    pclose(fp);

    if (count > 0) {
        call(FILEBookEnsure, s->paths, 1);
        u8bFeed1(s->paths, '\n');  // \n\n separator
        u8csbFree(s->past);
        u8cs region = {u8bDataHead(s->paths), u8bIdleHead(s->paths)};
        call(SNIFFScanAlloc, &s->past, region);
        fprintf(stderr, "sniff: seeded %u path(s) from git\n", count);
    }
    done;
}

// --- Open ---

ok64 SNIFFOpen(sniff *s, home *h, b8 rw) {
    sane(s && h);
    zerop(s);
    s->h = h;
    a_dup(u8c, reporoot, u8bDataC(h->root));

    a_cstr(dogs, ".dogs");
    a_cstr(sniffdir, "sniff");

    a_path(dir, reporoot, dogs, sniffdir);
    if (rw) call(FILEMakeDirP, $path(dir));

    {
        a_cstr(hf, "HEAD");
        a_path(hp, reporoot, dogs, sniffdir, hf);
        size_t len = u8bDataLen(hp);
        if (len >= sizeof(s->head_path)) len = sizeof(s->head_path) - 1;
        memcpy(s->head_path, u8bDataHead(hp), len);
        s->head_path[len] = 0;

        //  Load HEAD contents if the file exists.  Without this,
        //  every new session starts with an empty s->head and
        //  POSTCommit can't find its parent commit.
        FILE *hfp = fopen(s->head_path, "r");
        if (hfp) {
            size_t n = fread(s->head, 1, sizeof(s->head) - 1, hfp);
            fclose(hfp);
            // Strip trailing newline(s).
            while (n > 0 && (s->head[n - 1] == '\n' ||
                              s->head[n - 1] == '\r' ||
                              s->head[n - 1] == ' '))
                n--;
            s->head[n] = 0;
        }
    }

    //  Locate actual data end in a mmap-booked file.  FILEBook maps
    //  up to the next page boundary and zero-fills the tail past EOF;
    //  we must skip that padding so the next session's writes don't
    //  leave a gap.  paths.log is text ending in '\n'; state.log is
    //  a stream of 8-byte-aligned wh64 entries — scan by quad to
    //  survive legal entries whose high byte happens to be zero.
    #define SCAN_BYTE_END(buf) do {                                \
        u8p e = (u8p)(buf)[3];                                     \
        u8p b = (u8p)(buf)[0];                                     \
        while (e > b && e[-1] == 0) e--;                           \
        ((u8 **)(buf))[2] = e;                                     \
    } while (0)
    #define SCAN_WH64_END(buf) do {                                \
        u8p e = (u8p)(buf)[3];                                     \
        u8p b = (u8p)(buf)[0];                                     \
        size_t n = (size_t)(e - b) / sizeof(wh64);                 \
        u64 *arr = (u64 *)b;                                       \
        while (n > 0 && arr[n - 1] == 0) n--;                      \
        ((u8 **)(buf))[2] = b + n * sizeof(wh64);                  \
    } while (0)

    {
        a_cstr(pf, "paths.log");
        a_path(pp, reporoot, dogs, sniffdir, pf);
        ok64 o = FILEBook(&s->paths, $path(pp), SNIFF_PATH_BOOK);
        if (o == OK) {
            SCAN_BYTE_END(s->paths);
        } else if (rw) {
            call(FILEBookCreate, &s->paths, $path(pp),
                 SNIFF_PATH_BOOK, 4096);
        } else {
            fail(o);
        }
    }

    {
        a_cstr(sf, "state.log");
        a_path(cp, reporoot, dogs, sniffdir, sf);
        ok64 o = FILEBook(&s->changes, $path(cp), SNIFF_CHG_BOOK);
        if (o == OK) {
            SCAN_WH64_END(s->changes);
        } else if (rw) {
            call(FILEBookCreate, &s->changes, $path(cp),
                 SNIFF_CHG_BOOK, 4096);
        } else {
            fail(o);
        }
    }
    #undef SCAN_BYTE_END
    #undef SCAN_WH64_END

    u32 sep = SNIFFFindSep(s->paths);
    u8cp base = u8bDataHead(s->paths);
    u8cp end = u8bIdleHead(s->paths);
    u8cp mid = sep ? base + sep : base;

    u8cs past_r = {base, mid};
    u8cs data_r = {mid, end};
    call(SNIFFScanAlloc, &s->past, past_r);
    call(SNIFFScanAlloc, &s->data, data_r);

    if (rw && SNIFFCount(s) == 0)
        call(SNIFFBootstrap, s, reporoot);

    //  Ensure the root-dir path "/" is interned.  Its SNIFF_TREE
    //  hashlet is the base tree (staged by PUT/DELETE, committed by
    //  POST).  Harmless no-op on reopen of an existing state.
    if (rw) (void)SNIFFRootIdx(s);

    done;
}

//  Feed a git object into sniff's index.  Tree objects contribute
//  their entries' path names so `sniff` learns repo paths without
//  having to walk the worktree.  Other types are currently ignored
//  (commits/tags don't carry paths; blobs are content).
//
//  TODO: parse tree object (u8 mode SP name NUL sha[20]) and intern
//  each name.  For now this is a stub that accepts and drops the
//  blob — keeper can call it during fetch without error.
ok64 SNIFFUpdate(sniff *s, u8 obj_type, u8cs blob, u8csc path) {
    sane(s);
    (void)obj_type; (void)blob; (void)path;
    done;
}

// --- Intern ---

ok64 SNIFFInternIdx(sniff *s, u8cs path, u32 *idx) {
    sane(s && idx);
    u32 past_count = (u32)u8csbDataLen(s->past);
    if (past_count > 0) {
        u8csc key = {path[0], path[1]};
        u8cscp base = u8csbDataHead(s->past);
        u8cscs pslice = {base, base + past_count};
        u8cs *found = (u8cs *)$bsearch(&key, pslice, u8cscmp);
        if (found) { *idx = (u32)(found - base); done; }
    }

    u32 data_count = (u32)u8csbDataLen(s->data);
    u8cscp dp = u8csbDataHead(s->data);
    for (u32 i = 0; i < data_count; i++)
        if ($eq(dp[i], path)) { *idx = past_count + i; done; }

    call(FILEBookEnsure, s->paths, $len(path) + 1);
    u8cp start = u8bIdleHead(s->paths);
    call(u8bFeed, s->paths, path);
    call(u8bFeed1, s->paths, '\n');
    u8cs newpath = {start, start + $len(path)};
    call(u8csbReserve, s->data, 1);
    call(u8cssFeed1, u8csbIdle(s->data), newpath);
    *idx = past_count + data_count;
    done;
}

u32 SNIFFIntern(sniff *s, u8cs path) {
    u32 idx = 0;
    SNIFFInternIdx(s, path, &idx);
    return idx;
}

u32 SNIFFInternDir(sniff *s, u8cs path) {
    if ($empty(path)) return SNIFFRootIdx(s);
    if (*$last(path) == '/') return SNIFFIntern(s, path);
    a_pad(u8, tmp, 2048);
    u8bFeed(tmp, path);
    u8bFeed1(tmp, '/');
    u8cs dp = {u8bDataHead(tmp), tmp[2]};
    return SNIFFIntern(s, dp);
}

u32 SNIFFRootIdx(sniff *s) {
    a_cstr(root, "/");
    return SNIFFIntern(s, root);
}

u64 SNIFFBaseTree(sniff *s) {
    return SNIFFGet(s, SNIFF_TREE, SNIFFRootIdx(s));
}

// --- Sort ---

static sniff const *g_sort_sniff;

static int SNIFFCmpIdx(void const *a, void const *b) {
    u8cs pa = {}, pb = {};
    SNIFFPath(pa, g_sort_sniff, *(u32 const *)a);
    SNIFFPath(pb, g_sort_sniff, *(u32 const *)b);
    return u8cscmp(&pa, &pb);
}

ok64 SNIFFSort(sniff *s) {
    sane(s);
    u32 n = SNIFFCount(s);
    if (u32bDataLen(s->sorted) > 0) u32bReset(s->sorted);
    if (n == 0) done;
    if (u32bLen(s->sorted) < n) {
        u32bFree(s->sorted);
        call(u32bAllocate, s->sorted, n);
    }

    // Fill 0..n-1, qsort.  PAST is already sorted so this is fast.
    for (u32 i = 0; i < n; i++) {
        u32 **idle = u32bIdle(s->sorted);
        **idle = i;
        ++*idle;
    }
    g_sort_sniff = s;
    qsort(u32bDataHead(s->sorted), n, sizeof(u32), SNIFFCmpIdx);
    g_sort_sniff = NULL;
    done;
}

// --- Path lookup ---

ok64 SNIFFPath(u8csp out, sniff const *s, u32 index) {
    sane(out && s);
    u32 past_count = (u32)u8csbDataLen(s->past);
    u8cscp p;
    if (index < past_count)
        p = u8csbDataHead(s->past) + index;
    else {
        u32 di = index - past_count;
        if (di >= (u32)u8csbDataLen(s->data)) fail(SNIFFFAIL);
        p = u8csbDataHead(s->data) + di;
    }
    out[0] = (*p)[0];
    out[1] = (*p)[1];
    done;
}

// --- Record / Get ---

ok64 SNIFFRecord(sniff *s, u8 type, u32 index, u64 off) {
    sane(s && s->changes);
    u32 past_count = (u32)u8csbDataLen(s->past);
    u32 cpast = SNIFFChangesPast(s);

    if (index < past_count &&
        (type == SNIFF_BLOB || type == SNIFF_TREE || type == SNIFF_CHECKOUT)) {
        u32 slot = 2 * index + (type == SNIFF_CHECKOUT ? 1 : 0);
        size_t pos = (size_t)slot * sizeof(wh64);
        if (pos + sizeof(wh64) <= cpast) {
            wh64 entry = wh64Pack(type, index, off);
            memcpy(u8bDataHead(s->changes) + pos, &entry, sizeof(wh64));
            done;
        }
    }

    wh64 entry = wh64Pack(type, index, off);
    call(FILEBookEnsure, s->changes, sizeof(wh64));
    u8p *idle = (u8p *)&s->changes[2];
    memcpy(*idle, &entry, sizeof(wh64));
    *idle += sizeof(wh64);
    done;
}

u64 SNIFFGet(sniff const *s, u8 type, u32 index) {
    u32 past_count = (u32)u8csbDataLen(s->past);
    u32 cpast = SNIFFChangesPast(s);

    if (index < past_count &&
        (type == SNIFF_BLOB || type == SNIFF_TREE || type == SNIFF_CHECKOUT)) {
        u32 slot = 2 * index + (type == SNIFF_CHECKOUT ? 1 : 0);
        size_t pos = (size_t)slot * sizeof(wh64);
        if (pos + sizeof(wh64) <= cpast) {
            wh64 entry = 0;
            memcpy(&entry, u8bDataHead(s->changes) + pos, sizeof(wh64));
            // Slot 0 holds either BLOB or TREE; require exact match.
            if (type == SNIFF_CHECKOUT || wh64Type(entry) == type)
                return wh64Off(entry);
            return 0;
        }
    }

    u64cp tail = (u64cp)(u8bDataHead(s->changes) + cpast);
    u64cp end = (u64cp)u8bIdleHead(s->changes);
    while (end > tail) {
        --end;
        wh64 e = *end;
        if (e != 0 && wh64Type(e) == type && wh64Id(e) == index)
            return wh64Off(e);
    }
    return 0;
}

// --- Compact ---

ok64 SNIFFCompact(sniff *s) {
    sane(s);
    u32 n = SNIFFCount(s);
    if (n == 0) done;

    typedef struct {
        u64 hashlet; u64 checkout; u32 off; u32 len; u8 type;
    } live_entry;
    Bu8 ent_mem = {};
    call(u8bAllocate, ent_mem, (u64)n * sizeof(live_entry));
    live_entry *entries = (live_entry *)u8bHead(ent_mem);

    Bu8 str_mem = {};
    call(u8bAllocate, str_mem, u8bDataLen(s->paths) + 256);
    u32 live = 0;

    for (u32 i = 0; i < n; i++) {
        u8cs p = {};
        if (SNIFFPath(p, s, i) != OK) continue;
        u8 t = SNIFFIsDir(s, i) ? SNIFF_TREE : SNIFF_BLOB;
        u64 h = SNIFFGet(s, t, i);
        u64 c = SNIFFGet(s, SNIFF_CHECKOUT, i);
        if (h == 0 && c == 0) continue;
        entries[live].hashlet = h;
        entries[live].checkout = c;
        entries[live].off = (u32)u8bDataLen(str_mem);
        entries[live].len = (u32)$len(p);
        entries[live].type = t;
        u8bFeed(str_mem, p);
        live++;
    }

    // Sort entries by path (insertion sort on copied strings)
    u8cp str_base = u8bHead(str_mem);
    for (u32 i = 1; i < live; i++) {
        live_entry tmp = entries[i];
        u8cs tp = {str_base + tmp.off, str_base + tmp.off + tmp.len};
        u32 j = i;
        while (j > 0) {
            u8cs jp = {str_base + entries[j - 1].off,
                       str_base + entries[j - 1].off + entries[j - 1].len};
            if (u8cscmp(&jp, &tp) <= 0) break;
            entries[j] = entries[j - 1];
            j--;
        }
        entries[j] = tmp;
    }

    u8bReset(s->paths);
    u8bReset(s->changes);

    for (u32 i = 0; i < live; i++) {
        u8cs p = {str_base + entries[i].off,
                  str_base + entries[i].off + entries[i].len};
        call(FILEBookEnsure, s->paths, entries[i].len + 1);
        u8bFeed(s->paths, p);
        u8bFeed1(s->paths, '\n');
        wh64 h = wh64Pack(entries[i].type, i, entries[i].hashlet);
        wh64 c = wh64Pack(SNIFF_CHECKOUT, i, entries[i].checkout);
        call(FILEBookEnsure, s->changes, 2 * sizeof(wh64));
        u8p *idle = (u8p *)&s->changes[2];
        memcpy(*idle, &h, sizeof(wh64)); *idle += sizeof(wh64);
        memcpy(*idle, &c, sizeof(wh64)); *idle += sizeof(wh64);
    }

    call(FILEBookEnsure, s->paths, 1);
    u8bFeed1(s->paths, '\n');  // \n\n separator

    u8csbFree(s->past);
    u8cs past_r = {u8bDataHead(s->paths), u8bIdleHead(s->paths)};
    call(SNIFFScanAlloc, &s->past, past_r);

    u8csbFree(s->data);
    call(u8csbAllocate, s->data, 1024);

    u8bFree(ent_mem);
    u8bFree(str_mem);
    done;
}

// --- SetHead ---

ok64 SNIFFSetHead(sniff *s, u8cs val) {
    sane(s);
    size_t len = $len(val);
    if (len >= sizeof(s->head)) len = sizeof(s->head) - 1;
    memcpy(s->head, val[0], len);
    s->head[len] = 0;
    FILE *hf = fopen(s->head_path, "w");
    if (!hf) fail(SNIFFFAIL);
    fprintf(hf, "%s\n", s->head);
    fclose(hf);
    done;
}

// --- Close ---

ok64 SNIFFClose(sniff *s) {
    sane(s);
    if (s->paths) { FILETrimBook(s->paths); FILEUnBook(s->paths); }
    if (s->changes) { FILETrimBook(s->changes); FILEUnBook(s->changes); }
    u8csbFree(s->past);
    u8csbFree(s->data);
    u32bFree(s->sorted);
    zerop(s);
    done;
}

// --- Parent-commit helpers ---

ok64 SNIFFParentTreeSha(sha1 *tree_out, keeper *k, u8cs parent_hex) {
    sane(tree_out && k && $ok(parent_hex));

    size_t hexlen = $len(parent_hex);
    if (hexlen > 15) hexlen = 15;
    u64 hashlet = WHIFFHexHashlet60(parent_hex);

    Bu8 cbuf = {};
    call(u8bAllocate, cbuf, 1UL << 24);
    u8 ctype = 0;
    ok64 o = KEEPGet(k, hashlet, hexlen, cbuf, &ctype);
    if (o != OK) { u8bFree(cbuf); return o; }

    // Dereference annotated tag.
    if (ctype == DOG_OBJ_TAG) {
        u8cs body = {u8bDataHead(cbuf), u8bIdleHead(cbuf)};
        u8cs field = {}, value = {};
        sha1 tag_sha = {};
        a_raw(tag_bin, tag_sha);
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if ($empty(field)) break;
            if ($len(field) == 6 && memcmp(field[0], "object", 6) == 0 &&
                $len(value) >= 40) {
                u8cs hex40 = {value[0], $atp(value, 40)};
                HEXu8sDrainSome(tag_bin, hex40);
                break;
            }
        }
        u64 ch = WHIFFHashlet60(&tag_sha);
        u8bReset(cbuf);
        o = KEEPGet(k, ch, 15, cbuf, &ctype);
        if (o != OK) { u8bFree(cbuf); return o; }
    }
    if (ctype != DOG_OBJ_COMMIT) { u8bFree(cbuf); fail(SNIFFFAIL); }

    u8cs commit_body = {u8bDataHead(cbuf), u8bIdleHead(cbuf)};
    o = GITu8sCommitTree(commit_body, tree_out->data);
    u8bFree(cbuf);
    return o;
}

typedef struct {
    sniff *s;
    sha1  *shas;
    u32    capacity;
} collect_ctx;

static ok64 collect_visit(u8cs path, u8 kind, u8cp esha, u8cs blob,
                           void0p vctx) {
    (void)blob;
    collect_ctx *c = (collect_ctx *)vctx;
    if (kind == WALK_KIND_SUB) return WALKSKIP;

    u32 idx;
    if (kind == WALK_KIND_DIR && $empty(path))
        idx = SNIFFRootIdx(c->s);
    else if (kind == WALK_KIND_DIR)
        idx = SNIFFInternDir(c->s, path);
    else
        idx = SNIFFIntern(c->s, path);

    if (idx < c->capacity)
        memcpy(c->shas[idx].data, esha, 20);
    return OK;
}

ok64 SNIFFCollectParentTree(sniff *s, keeper *k, u8cs parent_hex,
                             sha1 *sha_tab, u32 capacity) {
    sane(s && k && sha_tab);
    if (!$ok(parent_hex) || $empty(parent_hex)) done;

    sha1 tree_sha = {};
    call(SNIFFParentTreeSha, &tree_sha, k, parent_hex);

    collect_ctx ctx = {.s = s, .shas = sha_tab, .capacity = capacity};
    return WALKTreeLazy(k, tree_sha.data, collect_visit, &ctx);
}

ok64 SNIFFCollectBaseTree(sniff *s, keeper *k,
                           sha1 *sha_tab, u32 capacity) {
    sane(s && k && sha_tab);
    u64 base = SNIFFBaseTree(s);
    if (base == 0) done;  // no base yet — empty sha_tab.

    // 40-bit hashlet → align as top 40 bits of keeper's 60-bit slot.
    Bu8 tbuf = {};
    call(u8bAllocate, tbuf, 1UL << 24);
    u8 otype = 0;
    ok64 o = KEEPGet(k, base << 20, 10, tbuf, &otype);
    if (o != OK || otype != DOG_OBJ_TREE) {
        u8bFree(tbuf);
        return o == OK ? SNIFFFAIL : o;
    }
    sha1 tree_sha = {};
    KEEPObjSha(&tree_sha, DOG_OBJ_TREE, u8bDataC(tbuf));
    u8bFree(tbuf);

    collect_ctx ctx = {.s = s, .shas = sha_tab, .capacity = capacity};
    return WALKTreeLazy(k, tree_sha.data, collect_visit, &ctx);
}
