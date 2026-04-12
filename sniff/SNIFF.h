#ifndef SNIFF_SNIFF_H
#define SNIFF_SNIFF_H

//  SNIFF: file path registry + filesystem change log.
//
//  On disk (.dogs/sniff/):
//    paths.log   newline-separated path strings, append-only, Book-mmap'd
//    state.log   flat append-only log of wh64 entries
//
//  Change entry types (wh64: type[4] | id[20] | off[40]):
//    SNIFF_HASHLET  (1)  id=path_index, off=sha hashlet (base version)
//    SNIFF_CHECKOUT (2)  id=path_index, off=mtime at checkout
//    SNIFF_CHANGED  (3)  id=path_index, off=mtime observed
//
//  Dirs also get path indices; type=0 carries tree object hashlets.
//
//  In RAM:
//    Bu32  offsets   offsets[i] = byte pos of path i in booked paths
//    Bkv64 names     RAPHash(path) → path_index
//    Bkv64 state     aggregated (type|id) → off from changes log

#include "abc/INT.h"
#include "abc/KV.h"
#include "abc/PATH.h"
#include "dog/WHIFF.h"

con ok64 SNIFFFAIL   = 0x7549f3ca495;
con ok64 SNIFFNOROOM = 0x7549f5d86d8616;

#define SNIFF_DIR       ".dogs/sniff"
#define SNIFF_PATH_BOOK (256UL << 20)  // 256 MB VA for paths
#define SNIFF_CHG_BOOK  (128UL << 20)  // 128 MB VA for changes
#define SNIFF_HASH_SIZE (1 << 20)      // 1M slots

// --- Entry types ---

#define SNIFF_HASHLET   1   // base object hashlet
#define SNIFF_CHECKOUT  2   // mtime at checkout (clean state)
#define SNIFF_CHANGED   3   // mtime observed (dirty)

// Key for state hash: low 24 bits of wh64 (type | id<<4)
#define SNIFF_KEY(type, id) \
    (((u64)(type) & WHIFF_TYPE_MASK) | \
     (((u64)(id) & WHIFF_ID_MASK) << WHIFF_ID_SHIFT))

// --- State ---

typedef struct {
    u8bp  paths;     // Book-mmap'd paths file
    u8bp  changes;   // Book-mmap'd changes file
    Bu32  offsets;   // path_index → byte offset in paths
    Bkv64 names;     // RAPHash(path) → path_index
    Bkv64 state;     // (type|id) → off, aggregated from changes
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

//  Build absolute path: reporoot/rel.
fun ok64 SNIFFFullpath(path8b out, u8cs reporoot, u8cs rel) {
    a_cstr(sep, "/");
    u8bFeed(out, reporoot);
    u8bFeed(out, sep);
    u8bFeed(out, rel);
    return PATHu8gTerm(PATHu8gIn(out));
}

//  Record a change entry to the log.
ok64 SNIFFRecord(sniff *s, u8 type, u32 index, u64 off);

//  Look up aggregated state for (type, index).
//  Returns the off value, or 0 if not found.
u64 SNIFFGet(sniff const *s, u8 type, u32 index);

#endif
