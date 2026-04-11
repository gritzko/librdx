#ifndef SNIFF_SNIFF_H
#define SNIFF_SNIFF_H

//  SNIFF: file path registry + filesystem change log.
//
//  On disk (.dogs/sniff/):
//    paths     newline-separated path strings, append-only, Book-mmap'd
//    changes   flat append-only log of wh64 change entries
//
//  Change entry (wh64):
//    type[4]=flags | id[20]=path_index | off[40]=mtime_sec
//
//  In RAM (rebuilt from paths on init/update):
//    Bu32 offsets   offsets[i] = byte pos of path i in booked paths
//    Bkv64 hash     RAPHash(path) → path_index

#include "abc/INT.h"
#include "abc/KV.h"
#include "dog/WHIFF.h"

con ok64 SNIFFFAIL   = 0x7549f3ca495;
con ok64 SNIFFNOROOM = 0x7549f5d86d8616;

#define SNIFF_DIR       ".dogs/sniff"
#define SNIFF_PATH_BOOK (256UL << 20)  // 256 MB VA for paths
#define SNIFF_CHG_BOOK  (128UL << 20)  // 128 MB VA for changes
#define SNIFF_HASH_SIZE (1 << 20)      // 1M slots

// --- State ---

typedef struct {
    u8bp  paths;     // Book-mmap'd paths file
    u8bp  changes;   // Book-mmap'd changes file
    Bu32  offsets;   // path_index → byte offset in paths
    Bkv64 hash;      // RAPHash(path) → path_index
} sniff;

// --- Public API ---

//  Open .dogs/sniff/ state.  reporoot from HOMEFind.
ok64 SNIFFOpen(sniff *s, u8cs reporoot, b8 rw);

//  Close and unmap everything.
ok64 SNIFFClose(sniff *s);

//  Re-read paths log if it grew (another dog appended).
ok64 SNIFFUpdate(sniff *s);

//  path→index.  Appends to paths log if new.
u32  SNIFFIntern(sniff *s, u8cs path);

//  index→path string (pointer into booked mmap, stable until close).
ok64 SNIFFPath(u8csp out, sniff const *s, u32 index);

//  Number of known paths.
fun u32 SNIFFCount(sniff const *s) {
    return u32bDataLen(s->offsets);
}

//  Record a filesystem change to the changes log.
ok64 SNIFFRecord(sniff *s, u8 flags, u32 index, u64 mtime_sec);

#endif
