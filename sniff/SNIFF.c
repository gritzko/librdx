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
    sane(s && s->paths);

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

        u64 hashlet = wh64Hashlet(u8bDataHead(shabin));
        SNIFFRecord(s, SNIFF_HASHLET, idx, hashlet);

        count++;
    }
    pclose(fp);

    if (count > 0)
        fprintf(stderr, "sniff: seeded %u path(s) from git\n", count);
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

    // Book paths file
    {
        a_path(pp, reporoot);
        a_cstr(pr, "/" SNIFF_DIR "/paths.log");
        call(u8bFeed, pp, pr);
        call(PATHu8gTerm, PATHu8gIn(pp));

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

    // Book changes file
    {
        a_path(cp, reporoot);
        a_cstr(cr, "/" SNIFF_DIR "/state.log");
        call(u8bFeed, cp, cr);
        call(PATHu8gTerm, PATHu8gIn(cp));

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
    sane(s && s->paths);
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

    // New path: append "path\n" to booked paths file
    size_t plen = $len(path);
    FILEBookEnsure(s->paths, plen + 1);

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
    sane(s && s->changes);
    wh64 entry = wh64Pack(type, index, off);
    call(FILEBookEnsure, s->changes, sizeof(wh64));
    u8 **cidle = u8bIdle(s->changes);
    memcpy(*cidle, &entry, sizeof(wh64));
    *cidle += sizeof(wh64);

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

// --- Close ---

ok64 SNIFFClose(sniff *s) {
    sane(s);
    if (s->paths) {
        FILETrimBook(s->paths);
        FILEUnBook(s->paths);
    }
    if (s->changes) {
        FILETrimBook(s->changes);
        FILEUnBook(s->changes);
    }
    u32bFree(s->offsets);
    kv64bFree(s->names);
    kv64bFree(s->state);
    memset(s, 0, sizeof(*s));
    done;
}
