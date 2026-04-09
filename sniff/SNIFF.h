#ifndef SNIFF_SNIFF_H
#define SNIFF_SNIFF_H

//  SNIFF: file path registry + filesystem change log.
//
//  On disk (.dogs/sniff/):
//    paths     newline-separated path strings, append-only, Book-mmap'd
//    changes   flat append-only log of u64 change entries
//
//  Change entry (u64):
//    flags[4] | path_index[20] | mtime_sec[32] | nsec_hi[8]
//
//  In RAM (rebuilt from paths on init/update):
//    Bu32 offsets   offsets[i] = byte pos of path i in booked paths
//    Bkv64 hash     RAPHash(path) → path_index

#include "abc/INT.h"
#include "abc/KV.h"

con ok64 SNIFFFAIL   = 0x7549f3ca495;
con ok64 SNIFFNOROOM = 0x7549f5d86d8616;

#define SNIFF_DIR       ".dogs/sniff"
#define SNIFF_PATH_BOOK (256UL << 20)  // 256 MB VA for paths
#define SNIFF_CHG_BOOK  (128UL << 20)  // 128 MB VA for changes
#define SNIFF_HASH_SIZE (1 << 20)      // 1M slots

// --- Change entry layout: flags[4] | index[20] | sec[32] | nsec_hi[8] ---

fun u64 SNIFFChange(u8 flags, u32 index, u64 sec, u32 nsec) {
    return ((u64)(flags & 0xf) << 60) |
           ((u64)(index & 0xfffff) << 40) |
           ((sec & 0xffffffffULL) << 8) |
           ((nsec >> 22) & 0xff);
}

fun u8  SNIFFChangeFlags(u64 e) { return (u8)(e >> 60); }
fun u32 SNIFFChangeIndex(u64 e) { return (u32)((e >> 40) & 0xfffff); }
fun u64 SNIFFChangeSec(u64 e)   { return (e >> 8) & 0xffffffffULL; }
fun u32 SNIFFChangeNsec(u64 e)  { return (u32)((e & 0xff) << 22); }

// --- State ---

typedef struct {
    u8bp  paths;     // Book-mmap'd paths file
    u8bp  changes;   // Book-mmap'd changes file
    Bu32  offsets;   // path_index → byte offset in paths
    Bkv64 hash;      // RAPHash(path) → path_index
} sniff;

// --- Public API ---

//  Load or create .dogs/sniff/ state.
//  dogsroot: where .dogs/ lives (from HOMEFindDogs)
//  worktree: checkout dir (from HOMEFind) — used for git bootstrap
ok64 SNIFFInit(sniff *s, u8cs dogsroot, u8cs worktree);

//  Re-read paths log if it grew (another dog appended).
ok64 SNIFFUpdate(sniff *s);

//  path→index.  Appends to paths log if new.
u32  SNIFFIntern(sniff *s, u8cs path);

//  index→path string (pointer into booked mmap, stable until FILEUnBook).
ok64 SNIFFPath(u8csp out, sniff const *s, u32 index);

//  Number of known paths.
fun u32 SNIFFCount(sniff const *s) {
    return u32bDataLen(s->offsets);
}

//  Record a filesystem change to the changes log.
ok64 SNIFFRecord(sniff *s, u32 index, u64 mtime_sec, u32 mtime_nsec);

//  Cleanup.
void SNIFFFree(sniff *s);

#endif
