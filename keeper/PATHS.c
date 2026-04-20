//  PATHS: keeper-side path registry.
//
//  `.dogs/keeper/paths.log` is a newline-separated append-only list of
//  repo-relative paths.  Each path's identity is its u32 index in the
//  sequence.  Directory paths carry a trailing '/'.  Index 0 = empty
//  path (repo root).
//
//  In-memory:
//    paths_log   FILEBook'd mmap of paths.log (stable address, grows).
//    paths_offs  Bu32, offsets_offs[i] = byte position of path i in the log.
//    paths_hash  Bkv64, hash64(path) -> u32 index, for dedup on intern.
//
#include "PATHS.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/HASH.h"
#include "abc/PATH.h"
#include "abc/PRO.h"

// kv64 hashtable templates
#define X(M, name) M##kv64##name
#include "abc/HASHx.h"
#undef X

#define KEEP_PATHS_BOOK (256UL << 20)  // 256 MB virtual reservation
#define KEEP_PATHS_HASH_SIZE (1u << 20) // 1M slots

//  Simple 64-bit string hash — FNV-1a.  Cheap; good enough for dedup.
static u64 path_hash(u8cs p) {
    u64 h = 0xcbf29ce484222325ULL;
    u8cp s = p[0], e = p[1];
    for (; s < e; s++) {
        h ^= *s;
        h *= 0x100000001b3ULL;
    }
    //  Avoid 0 (kv64 treats {key=0,val=0} as empty slot).
    return h ? h : 1;
}

//  Rescan the log file, rebuild offsets + hash map.  Called once at open.
static ok64 paths_rebuild_index(keeper *k) {
    sane(k);
    u32bReset(k->paths_offs);

    //  Clear the kv64 table.
    kv64 *base = k->paths_hash[0];
    size_t n = (size_t)(k->paths_hash[3] - k->paths_hash[0]);
    memset(base, 0, n * sizeof(kv64));
    k->paths_hash[1] = k->paths_hash[0];
    k->paths_hash[2] = k->paths_hash[3];

    u8cp cur = u8bDataHead(k->paths_log);
    u8cp end = u8bIdleHead(k->paths_log);
    while (cur < end) {
        u8cp nl = cur;
        while (nl < end && *nl != '\n') nl++;
        u32 off = (u32)(cur - u8bDataHead(k->paths_log));
        call(u32bPush, k->paths_offs, &off);
        u8cs path = {cur, nl};
        u64 h = path_hash(path);
        kv64 e = { .key = h, .val = (u64)KEEPPathCount(k) - 1 };
        kv64s tbl = {k->paths_hash[0], k->paths_hash[3]};
        HASHkv64Put(tbl, &e);
        cur = (nl < end) ? nl + 1 : end;
    }
    done;
}

ok64 KEEPPathsOpen(keeper *k, b8 rw) {
    sane(k && k->h);
    a_dup(u8c, reporoot, u8bDataC(k->h->root));
    a_cstr(dogs, ".dogs");
    a_cstr(keep, "keeper");
    a_cstr(fn, "paths.log");
    a_path(pp, reporoot, dogs, keep, fn);

    ok64 o = FILEBook(&k->paths_log, $path(pp), KEEP_PATHS_BOOK);
    if (o != OK) {
        if (!rw) return o;
        call(FILEBookCreate, &k->paths_log, $path(pp),
             KEEP_PATHS_BOOK, 4096);
    } else {
        //  Trim tail of zero bytes left by FILEBook's page alignment.
        u8p e = k->paths_log[3];
        u8p b = k->paths_log[0];
        while (e > b && e[-1] == 0) e--;
        ((u8 **)k->paths_log)[2] = e;
    }

    //  Allocate offsets + hash.
    call(u32bAllocate, k->paths_offs, 1u << 16);
    call(kv64bAllocate, k->paths_hash, KEEP_PATHS_HASH_SIZE);
    memset(k->paths_hash[0], 0,
           (size_t)(k->paths_hash[3] - k->paths_hash[0]) * sizeof(kv64));
    k->paths_hash[1] = k->paths_hash[0];
    k->paths_hash[2] = k->paths_hash[3];

    call(paths_rebuild_index, k);

    done;
}

void KEEPPathsClose(keeper *k) {
    if (!k) return;
    if (k->paths_log) { FILETrimBook(k->paths_log); FILEUnBook(k->paths_log); }
    k->paths_log = NULL;
    u32bFree(k->paths_offs);
    if (k->paths_hash[0]) kv64bFree(k->paths_hash);
    memset(k->paths_offs, 0, sizeof(k->paths_offs));
    memset(k->paths_hash, 0, sizeof(k->paths_hash));
}

u32 KEEPIntern(keeper *k, u8cs path) {
    if (!k) return 0;
    u64 h = path_hash(path);

    kv64s tbl = {k->paths_hash[0], k->paths_hash[3]};
    kv64 probe = { .key = h, .val = 0 };
    if (HASHkv64Get(&probe, tbl) == OK) {
        //  Verify match (hash collisions are possible).
        u32 idx = (u32)probe.val;
        u8cs stored = {};
        if (KEEPPath(k, idx, stored) == OK &&
            u8csLen(stored) == u8csLen(path) &&
            memcmp(stored[0], path[0], u8csLen(path)) == 0) {
            return idx;
        }
        //  Collision on hash but different string.  Fall through and
        //  append a new entry; the hashtable slot will be replaced for
        //  future lookups of the new string (old entry still findable
        //  via linear re-scan, but current schema doesn't surface that).
    }

    //  Append to paths.log.
    if (FILEBookEnsure(k->paths_log, u8csLen(path) + 1) != OK) return 0;
    u32 off = (u32)u8bDataLen(k->paths_log);
    if (u8bFeed(k->paths_log, path) != OK) return 0;
    if (u8bFeed1(k->paths_log, '\n') != OK) return 0;
    u32 idx = KEEPPathCount(k);
    if (u32bPush(k->paths_offs, &off) != OK) return 0;

    kv64 e = { .key = h, .val = (u64)idx };
    HASHkv64Put(tbl, &e);
    return idx;
}

ok64 KEEPPath(keeper const *k, u32 idx, u8csp out) {
    sane(k && out);
    if (idx >= KEEPPathCount(k)) return KEEPNONE;
    u32 const *base = (u32 const *)k->paths_offs[0];
    u32 off  = base[idx];
    u32 next = (idx + 1 < KEEPPathCount(k))
             ? base[idx + 1]
             : (u32)u8bDataLen(k->paths_log);
    u8cp start = u8bDataHead(k->paths_log) + off;
    u8cp end   = u8bDataHead(k->paths_log) + next;
    //  Drop trailing '\n' if present.
    if (end > start && end[-1] == '\n') end--;
    out[0] = start;
    out[1] = end;
    done;
}
