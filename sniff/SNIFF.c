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

// Append a wh64 pair (HASHLET + CHECKOUT) to changes.
static ok64 SNIFFWritePair(sniff *s, u32 idx, u64 hashlet, u64 checkout) {
    sane(s && s->changes);
    wh64 h = wh64Pack(SNIFF_HASHLET, idx, hashlet);
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
        FILEBookEnsure(s->paths, $len(path) + 1);
        u8bFeed(s->paths, path);
        u8bFeed1(s->paths, '\n');

        u64 hashlet = WHIFFHashlet40((sha1cp)u8bDataHead(shabin));
        call(SNIFFWritePair, s, count, hashlet, 0);
        count++;
    }
    pclose(fp);

    if (count > 0) {
        FILEBookEnsure(s->paths, 1);
        u8bFeed1(s->paths, '\n');  // \n\n separator
        u8csbFree(s->past);
        u8cs region = {u8bDataHead(s->paths), u8bIdleHead(s->paths)};
        call(SNIFFScanAlloc, &s->past, region);
        fprintf(stderr, "sniff: seeded %u path(s) from git\n", count);
    }
    done;
}

// --- Open ---

ok64 SNIFFOpen(sniff *s, u8cs reporoot, b8 rw) {
    sane(s && $ok(reporoot));
    memset(s, 0, sizeof(*s));

    a_cstr(dogs, ".dogs");
    a_cstr(sniffdir, "sniff");

    a_path(dir, reporoot, dogs, sniffdir);
    if (rw) call(FILEMakeDirP, PATHu8cgIn(dir));

    {
        a_cstr(hf, "HEAD");
        a_path(hp, reporoot, dogs, sniffdir, hf);
        size_t len = u8bDataLen(hp);
        if (len >= sizeof(s->head_path)) len = sizeof(s->head_path) - 1;
        memcpy(s->head_path, u8bDataHead(hp), len);
        s->head_path[len] = 0;
    }

    {
        a_cstr(pf, "paths.log");
        a_path(pp, reporoot, dogs, sniffdir, pf);
        ok64 o = FILEBook(&s->paths, PATHu8cgIn(pp), SNIFF_PATH_BOOK);
        if (o == OK) {
            ((u8 **)s->paths)[2] = s->paths[3];
        } else if (rw) {
            call(FILEBookCreate, &s->paths, PATHu8cgIn(pp),
                 SNIFF_PATH_BOOK, 4096);
        } else {
            fail(o);
        }
    }

    {
        a_cstr(sf, "state.log");
        a_path(cp, reporoot, dogs, sniffdir, sf);
        ok64 o = FILEBook(&s->changes, PATHu8cgIn(cp), SNIFF_CHG_BOOK);
        if (o == OK) {
            ((u8 **)s->changes)[2] = s->changes[3];
        } else if (rw) {
            call(FILEBookCreate, &s->changes, PATHu8cgIn(cp),
                 SNIFF_CHG_BOOK, 4096);
        } else {
            fail(o);
        }
    }

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

    done;
}

ok64 SNIFFUpdate(sniff *s) {
    sane(s && s->paths);
    u32 sep = SNIFFFindSep(s->paths);
    u8cp mid = sep ? u8bDataHead(s->paths) + sep : u8bDataHead(s->paths);
    u8csbReset(s->data);
    u8cs data_r = {mid, u8bIdleHead(s->paths)};
    call(SNIFFScanPaths, s->data, data_r);
    done;
}

// --- Intern ---

u32 SNIFFIntern(sniff *s, u8cs path) {
    u32 past_count = (u32)u8csbDataLen(s->past);
    if (past_count > 0) {
        u8csc key = {path[0], path[1]};
        u8cscp base = u8csbDataHead(s->past);
        u8cscs pslice = {base, base + past_count};
        u8cs *found = (u8cs *)$bsearch(&key, pslice, u8cscmp);
        if (found) return (u32)(found - base);
    }

    u32 data_count = (u32)u8csbDataLen(s->data);
    u8cscp dp = u8csbDataHead(s->data);
    for (u32 i = 0; i < data_count; i++)
        if ($eq(dp[i], path)) return past_count + i;

    FILEBookEnsure(s->paths, $len(path) + 1);
    u8cp start = u8bIdleHead(s->paths);
    u8bFeed(s->paths, path);
    u8bFeed1(s->paths, '\n');
    u8cs newpath = {start, start + $len(path)};
    u8cssFeed1(u8csbIdle(s->data), newpath);
    return past_count + data_count;
}

u32 SNIFFInternDir(sniff *s, u8cs path) {
    if ($empty(path)) return SNIFFIntern(s, path);
    if (*$last(path) == '/') return SNIFFIntern(s, path);
    a_pad(u8, tmp, 2048);
    u8bFeed(tmp, path);
    u8bFeed1(tmp, '/');
    u8cs dp = {u8bDataHead(tmp), tmp[2]};
    return SNIFFIntern(s, dp);
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

    if (index < past_count && (type == SNIFF_HASHLET || type == SNIFF_CHECKOUT)) {
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

    if (index < past_count && (type == SNIFF_HASHLET || type == SNIFF_CHECKOUT)) {
        u32 slot = 2 * index + (type == SNIFF_CHECKOUT ? 1 : 0);
        size_t pos = (size_t)slot * sizeof(wh64);
        if (pos + sizeof(wh64) <= cpast) {
            wh64 entry = 0;
            memcpy(&entry, u8bDataHead(s->changes) + pos, sizeof(wh64));
            return wh64Off(entry);
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

    typedef struct { u64 hashlet; u64 checkout; u32 off; u32 len; } live_entry;
    Bu8 ent_mem = {};
    call(u8bAllocate, ent_mem, (u64)n * sizeof(live_entry));
    live_entry *entries = (live_entry *)u8bHead(ent_mem);

    Bu8 str_mem = {};
    call(u8bAllocate, str_mem, u8bDataLen(s->paths) + 256);
    u32 live = 0;

    for (u32 i = 0; i < n; i++) {
        u8cs p = {};
        if (SNIFFPath(p, s, i) != OK) continue;
        u64 h = SNIFFGet(s, SNIFF_HASHLET, i);
        u64 c = SNIFFGet(s, SNIFF_CHECKOUT, i);
        if (h == 0 && c == 0) continue;
        entries[live].hashlet = h;
        entries[live].checkout = c;
        entries[live].off = (u32)u8bDataLen(str_mem);
        entries[live].len = (u32)$len(p);
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
        FILEBookEnsure(s->paths, entries[i].len + 1);
        u8bFeed(s->paths, p);
        u8bFeed1(s->paths, '\n');
        call(SNIFFWritePair, s, i, entries[i].hashlet, entries[i].checkout);
    }

    FILEBookEnsure(s->paths, 1);
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
    memset(s, 0, sizeof(*s));
    done;
}
