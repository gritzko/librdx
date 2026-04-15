//  SNIFF: file path registry + filesystem change log.
//
#include "SNIFF.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/KV.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/RAP.h"

// HASHkv64 instantiation
#define X(M, name) M##kv64##name
#include "abc/HASHx.h"
#undef X

// --- Rebuild offsets + names hash from the paths region ---

static ok64 sniff_index_paths(sniff *s) {
    sane(s && s->paths[0]);

    u32bReset(s->offsets);
    memset(kv64bHead(s->names), 0, kv64bLen(s->names) * sizeof(kv64));

    u8cs rest = {u8bDataHead(s->paths), u8bIdleHead(s->paths)};

    while (!$empty(rest)) {
        u8cp linestart = rest[0];
        u8cs scan = {rest[0], rest[1]};
        if (u8csFind(scan, '\n') != OK) break;
        rest[0] = scan[0];
        ++rest[0];

        u8csc ps = {linestart, scan[0]};
        if ($empty(ps)) continue;

        u8csc prefix = {u8bDataHead(s->paths), linestart};
        u32 off = (u32)$len(prefix);
        u32 idx = u32bDataLen(s->offsets);

        u32 **idle = u32bIdle(s->offsets);
        if (!$len(idle)) return SNIFFNOROOM;
        **idle = off;
        ++*idle;

        u64 h = RAPHash(ps);
        kv64 entry = {.key = h, .val = (u64)idx};
        kv64s tab = {kv64bHead(s->names), kv64bTerm(s->names)};
        HASHkv64Put(tab, &entry);
    }
    done;
}

// --- Aggregate changes log into state hash ---

static ok64 sniff_aggregate(sniff *s) {
    sane(s);
    memset(kv64bHead(s->state), 0, kv64bLen(s->state) * sizeof(kv64));

    u64cs elog = {(u64cp)u8bDataHead(s->changes),
                  (u64cp)u8bIdleHead(s->changes)};
    kv64s tab = {kv64bHead(s->state), kv64bTerm(s->state)};

    $for(u64c, e, elog) {
        if (*e == 0) continue;  // skip padding
        u64 key = SNIFF_KEY(wh64Type(*e), wh64Id(*e));
        kv64 entry = {.key = key, .val = wh64Off(*e)};
        HASHkv64Put(tab, &entry);
    }
    done;
}

// --- Bootstrap: seed paths + hashlets from git ls-files -s ---

static ok64 sniff_bootstrap(sniff *s, u8cs reporoot) {
    sane(s && $ok(reporoot));
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "git -C %.*s ls-files -s 2>/dev/null",
             (int)$len(reporoot), (char *)reporoot[0]);
    FILE *fp = popen(cmd, "r");
    if (!fp) done;

    char line[1024];
    u32 count = 0;
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = 0;
        if (len == 0) continue;

        // Format: "mode sha1 stage\tpath"
        // Find the tab separating stage from path
        char *tab = strchr(line, '\t');
        if (!tab) continue;
        char *pathstr = tab + 1;

        // Parse sha1 hex (between first space and tab)
        char *sha_start = strchr(line, ' ');
        if (!sha_start || sha_start >= tab) continue;
        sha_start++;

        // Convert first 16 hex chars → 8 bytes for hashlet
        a_pad(u8, shabin, 8);
        u8cs hex16 = {(u8cp)sha_start, (u8cp)sha_start};
        if (tab - sha_start >= 16)
            hex16[1] = (u8cp)sha_start + 16;
        else
            continue;
        HEXu8sDrainSome(shabin_idle, hex16);

        a$str(path, pathstr);
        u32 idx = SNIFFIntern(s, path);

        u64 hashlet = WHIFFHashlet40((sha1cp)u8bDataHead(shabin));
        SNIFFRecord(s, SNIFF_HASHLET, idx, hashlet);

        count++;
    }
    pclose(fp);

    if (count > 0)
        fprintf(stderr, "sniff: seeded %u path(s) from git\n", count);
    done;
}

// --- Read file into heap buffer (empty buf if file doesn't exist) ---

static ok64 sniff_slurp(Bu8 buf, char const *path) {
    sane(buf);
    u8bp mapped = NULL;
    a_cstr(ps, path);
    a_path(pp, ps);
    ok64 o = FILEMapRO(&mapped, PATHu8cgIn(pp));
    if (o != OK) done;  // file doesn't exist → empty buf
    a_dup(u8c, data, u8bDataC(mapped));
    call(u8bFeed, buf, data);
    FILEUnMap(mapped);
    done;
}

// --- Write buffer to file (create/truncate) ---

static ok64 sniff_flush(Bu8 buf, char const *path) {
    sane(buf);
    if (!u8bHasData(buf)) done;
    a_cstr(ps, path);
    a_path(pp, ps);
    int fd = -1;
    call(FILECreate, &fd, PATHu8cgIn(pp));
    a_dup(u8c, data, u8bDataC(buf));
    call(FILEFeedAll, fd, data);
    close(fd);
    done;
}

// --- Open ---

ok64 SNIFFOpen(sniff *s, u8cs reporoot, b8 rw) {
    sane(s && $ok(reporoot));
    memset(s, 0, sizeof(*s));

    // Ensure .dogs/sniff/ exists
    a_path(dir, reporoot);
    a_cstr(rel, "/" SNIFF_DIR);
    call(u8bFeed, dir, rel);
    call(PATHu8gTerm, PATHu8gIn(dir));
    if (rw) call(FILEMakeDirP, PATHu8cgIn(dir));

    // Build file paths for writeback
    snprintf(s->paths_path, sizeof(s->paths_path),
             "%.*s/" SNIFF_DIR "/paths.log",
             (int)$len(reporoot), (char *)reporoot[0]);
    snprintf(s->chg_path, sizeof(s->chg_path),
             "%.*s/" SNIFF_DIR "/state.log",
             (int)$len(reporoot), (char *)reporoot[0]);
    snprintf(s->head_path, sizeof(s->head_path),
             "%.*s/" SNIFF_DIR "/HEAD",
             (int)$len(reporoot), (char *)reporoot[0]);

    // Read HEAD
    {
        FILE *hf = fopen(s->head_path, "r");
        if (hf) {
            if (fgets(s->head, sizeof(s->head), hf)) {
                size_t len = strlen(s->head);
                while (len > 0 && (s->head[len - 1] == '\n' ||
                                   s->head[len - 1] == '\r'))
                    s->head[--len] = 0;
            }
            fclose(hf);
        }
    }

    // Read paths file into heap buffer
    call(u8bAllocate, s->paths, SNIFF_INIT_CAP);
    call(sniff_slurp, s->paths, s->paths_path);

    // Read changes file into heap buffer
    call(u8bAllocate, s->changes, SNIFF_INIT_CAP);
    call(sniff_slurp, s->changes, s->chg_path);

    // Allocate in-RAM tables
    call(u32bAllocate, s->offsets, SNIFF_HASH_SIZE);
    call(kv64bAllocate, s->names, SNIFF_HASH_SIZE);
    call(kv64bAllocate, s->state, SNIFF_HASH_SIZE * 4);

    // Build path index
    call(sniff_index_paths, s);

    // Bootstrap from git if empty
    if (rw && SNIFFCount(s) == 0) {
        call(sniff_bootstrap, s, reporoot);
    }

    // Aggregate changes into state hash
    call(sniff_aggregate, s);

    done;
}

// --- Update (re-scan if paths log grew) ---

ok64 SNIFFUpdate(sniff *s) {
    sane(s && s->paths[0]);
    call(sniff_index_paths, s);
    call(sniff_aggregate, s);
    done;
}

// --- Intern ---

u32 SNIFFIntern(sniff *s, u8cs path) {
    u64 h = RAPHash(path);
    kv64 probe = {.key = h, .val = 0};
    kv64s tab = {kv64bHead(s->names), kv64bTerm(s->names)};
    if (HASHkv64Get(&probe, tab) == OK) return (u32)probe.val;

    // New path: append "path\n" to paths buffer
    size_t plen = $len(path);
    if (u8bIdleLen(s->paths) < plen + 1)
        u8bReserve(s->paths, u8bDataLen(s->paths) + plen + 1);

    u32 off = (u32)u8bDataLen(s->paths);
    u32 idx = u32bDataLen(s->offsets);

    u8bFeed(s->paths, path);
    u8bFeed1(s->paths, '\n');

    u32 **oidle = u32bIdle(s->offsets);
    if ($len(oidle)) {
        **oidle = off;
        ++*oidle;
    }

    kv64 entry = {.key = h, .val = (u64)idx};
    HASHkv64Put(tab, &entry);

    return idx;
}

// --- Intern directory (trailing /) ---

u32 SNIFFInternDir(sniff *s, u8cs path) {
    if ($empty(path)) return SNIFFIntern(s, path);
    if (*$last(path) == '/') return SNIFFIntern(s, path);
    // Append /
    a_pad(u8, tmp, 2048);
    u8bFeed(tmp, path);
    u8bFeed1(tmp, '/');
    u8cs dp = {u8bDataHead(tmp), tmp[2]};
    return SNIFFIntern(s, dp);
}

// --- Sort path indices ---

static sniff const *g_sort_sniff;

static int sniff_cmp_idx(void const *a, void const *b) {
    u32 ia = *(u32 const *)a;
    u32 ib = *(u32 const *)b;
    u8cs pa = {}, pb = {};
    SNIFFPath(pa, g_sort_sniff, ia);
    SNIFFPath(pb, g_sort_sniff, ib);
    size_t la = $len(pa), lb = $len(pb);
    size_t minl = la < lb ? la : lb;
    int c = memcmp(pa[0], pb[0], minl);
    if (c != 0) return c;
    return (la > lb) - (la < lb);
}

ok64 SNIFFSort(sniff *s) {
    sane(s);
    u32 n = SNIFFCount(s);
    if (u32bDataLen(s->sorted) > 0) u32bReset(s->sorted);
    if (n == 0) done;

    // Ensure capacity
    if (u32bLen(s->sorted) < n) {
        u32bFree(s->sorted);
        call(u32bAllocate, s->sorted, n);
    }

    // Fill with 0..n-1
    for (u32 i = 0; i < n; i++) {
        u32 **idle = u32bIdle(s->sorted);
        **idle = i;
        ++*idle;
    }

    // Sort
    g_sort_sniff = s;
    qsort(u32bDataHead(s->sorted), n, sizeof(u32), sniff_cmp_idx);
    g_sort_sniff = NULL;

    done;
}

// --- Path lookup ---

ok64 SNIFFPath(u8csp out, sniff const *s, u32 index) {
    sane(out && s);
    if (index >= u32bDataLen(s->offsets)) fail(SNIFFFAIL);
    u32 off = *u32bDataAtP(s->offsets, index);
    u8cp start = u8bDataAtP(s->paths, off);
    u8cs scan = {start, u8bIdleHead(s->paths)};
    u8csFind(scan, '\n');
    u8cs range = {start, scan[0]};
    u8csMv(out, range);
    done;
}

// --- Record ---

ok64 SNIFFRecord(sniff *s, u8 type, u32 index, u64 off) {
    sane(s && s->changes[0]);
    wh64 entry = wh64Pack(type, index, off);
    if (u8bIdleLen(s->changes) < sizeof(wh64))
        u8bReserve(s->changes, u8bDataLen(s->changes) + sizeof(wh64));
    u8p *idle = (u8p *)&s->changes[2];
    memcpy(*idle, &entry, sizeof(wh64));
    *idle += sizeof(wh64);

    // Update state hash
    u64 key = SNIFF_KEY(type, index);
    kv64 kv = {.key = key, .val = off};
    kv64s tab = {kv64bHead(s->state), kv64bTerm(s->state)};
    HASHkv64Put(tab, &kv);

    done;
}

// --- Get ---

u64 SNIFFGet(sniff const *s, u8 type, u32 index) {
    u64 key = SNIFF_KEY(type, index);
    kv64 probe = {.key = key, .val = 0};
    kv64s tab = {kv64bHead(s->state), kv64bTerm(s->state)};
    if (HASHkv64Get(&probe, tab) == OK) return probe.val;
    return 0;
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
    if (s->paths[0]) {
        sniff_flush(s->paths, s->paths_path);
        u8bFree(s->paths);
    }
    if (s->changes[0]) {
        sniff_flush(s->changes, s->chg_path);
        u8bFree(s->changes);
    }
    u32bFree(s->offsets);
    kv64bFree(s->names);
    kv64bFree(s->state);
    u32bFree(s->sorted);
    memset(s, 0, sizeof(*s));
    done;
}
