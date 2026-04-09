//  SNIFF: file path registry + filesystem change log.
//
#include "SNIFF.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/KV.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/RAP.h"

// HASHkv64 instantiation
#define X(M, name) M##kv64##name
#include "abc/HASHx.h"
#undef X

// --- Rebuild offsets + hash from the paths region ---

static ok64 sniff_index_paths(sniff *s) {
    sane(s && s->paths);

    // Reset offsets and hash
    u32bReset(s->offsets);
    memset(kv64bHead(s->hash), 0, kv64bLen(s->hash) * sizeof(kv64));

    u8cs rest = {u8bDataHead(s->paths), u8bIdleHead(s->paths)};

    while (!$empty(rest)) {
        u8cp linestart = rest[0];
        u8cs scan = {rest[0], rest[1]};
        if (u8csFind(scan, '\n') != OK) break;
        // scan[0] now at '\n'; advance rest past it
        rest[0] = scan[0];
        ++rest[0];

        u8csc ps = {linestart, scan[0]};
        if ($empty(ps)) continue;

        u8csc prefix = {u8bDataHead(s->paths), linestart};
        u32 off = (u32)$len(prefix);
        u32 idx = u32bDataLen(s->offsets);

        // Append offset
        u32 **idle = u32bIdle(s->offsets);
        if (!$len(idle)) return SNIFFNOROOM;
        **idle = off;
        ++*idle;

        // Insert into hash
        u64 h = RAPHash(ps);
        kv64 entry = {.key = h, .val = (u64)idx};
        kv64s tab = {kv64bHead(s->hash), kv64bTerm(s->hash)};
        HASHkv64Put(tab, &entry);
    }
    done;
}

// --- Bootstrap: seed path list from git ls-tree --all ---

static ok64 sniff_bootstrap(sniff *s, u8cs reporoot) {
    sane(s && $ok(reporoot));
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "git -C %.*s ls-tree -r --name-only --all 2>/dev/null",
             (int)$len(reporoot), (char *)reporoot[0]);
    FILE *fp = popen(cmd, "r");
    if (!fp) done;  // not a git repo, nothing to seed

    char line[1024];
    u32 count = 0;
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = 0;
        if (len == 0) continue;
        a$str(path, line);
        SNIFFIntern(s, path);
        count++;
    }
    pclose(fp);

    if (count > 0)
        fprintf(stderr, "sniff: seeded %u path(s) from git\n", count);
    done;
}

// --- Init ---

ok64 SNIFFInit(sniff *s, u8cs reporoot) {
    sane(s && $ok(reporoot));
    memset(s, 0, sizeof(*s));

    // Ensure .dogs/sniff/ exists
    a_path(dir, reporoot);
    a_cstr(rel, "/" SNIFF_DIR);
    call(u8bFeed, dir, rel);
    call(PATHu8gTerm, PATHu8gIn(dir));
    call(FILEMakeDirP, PATHu8cgIn(dir));

    // Book paths file
    {
        a_path(pp, reporoot);
        a_cstr(pr, "/" SNIFF_DIR "/paths");
        call(u8bFeed, pp, pr);
        call(PATHu8gTerm, PATHu8gIn(pp));

        ok64 o = FILEBook(&s->paths, PATHu8cgIn(pp), SNIFF_PATH_BOOK);
        if (o != OK)
            call(FILEBookCreate, &s->paths, PATHu8cgIn(pp),
                 SNIFF_PATH_BOOK, 4096);
    }

    // Book changes file
    {
        a_path(cp, reporoot);
        a_cstr(cr, "/" SNIFF_DIR "/changes");
        call(u8bFeed, cp, cr);
        call(PATHu8gTerm, PATHu8gIn(cp));

        ok64 o = FILEBook(&s->changes, PATHu8cgIn(cp), SNIFF_CHG_BOOK);
        if (o != OK)
            call(FILEBookCreate, &s->changes, PATHu8cgIn(cp),
                 SNIFF_CHG_BOOK, 4096);
    }

    // Allocate offsets array (up to 1M paths)
    call(u32bAllocate, s->offsets, SNIFF_HASH_SIZE);

    // Allocate hash table
    call(kv64bAllocate, s->hash, SNIFF_HASH_SIZE);

    // Build index from existing paths
    call(sniff_index_paths, s);

    // Bootstrap: if paths log is empty, seed from git ls-tree
    if (SNIFFCount(s) == 0 && $ok(reporoot)) {
        call(sniff_bootstrap, s, reporoot);
    }
    done;
}

// --- Update (re-scan if paths log grew) ---

ok64 SNIFFUpdate(sniff *s) {
    sane(s && s->paths);
    // Re-index from scratch.  FILEBook keeps the mapping current
    // if another process extended the file (the kernel updates the
    // mapped region for shared mappings).  For private Book mappings
    // we may need FILEBookExtend — but for now, a full rebuild is
    // safe and simple.
    call(sniff_index_paths, s);
    done;
}

// --- Intern ---

u32 SNIFFIntern(sniff *s, u8cs path) {
    u64 h = RAPHash(path);
    kv64 probe = {.key = h, .val = 0};
    kv64s tab = {kv64bHead(s->hash), kv64bTerm(s->hash)};
    if (HASHkv64Get(&probe, tab) == OK) return (u32)probe.val;

    // New path: append "path\n" to booked paths file
    size_t plen = $len(path);
    FILEBookEnsure(s->paths, plen + 1);

    u32 off = (u32)u8bDataLen(s->paths);
    u32 idx = u32bDataLen(s->offsets);

    u8bFeed(s->paths, path);
    u8bFeed1(s->paths, '\n');

    // Record offset
    u32 **oidle = u32bIdle(s->offsets);
    if ($len(oidle)) {
        **oidle = off;
        ++*oidle;
    }

    // Insert into hash
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

// --- Record change ---

ok64 SNIFFRecord(sniff *s, u32 index, u64 mtime_sec, u32 mtime_nsec) {
    sane(s && s->changes);
    u64 entry = SNIFFChange(0, index, mtime_sec, mtime_nsec);
    call(FILEBookEnsure, s->changes, sizeof(u64));
    u8 **cidle = u8bIdle(s->changes);
    memcpy(*cidle, &entry, sizeof(u64));
    *cidle += sizeof(u64);
    done;
}

// --- Free ---

void SNIFFFree(sniff *s) {
    if (!s) return;
    if (s->paths) FILEUnBook(s->paths);
    if (s->changes) FILEUnBook(s->changes);
    u32bFree(s->offsets);
    kv64bFree(s->hash);
    memset(s, 0, sizeof(*s));
}
