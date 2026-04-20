#ifndef LIBRDX_CAPO_H
#define LIBRDX_CAPO_H

#include <stdio.h>
#include "abc/INT.h"
#include "abc/PATH.h"
#include "abc/RON.h"
#include "abc/RAP.h"

con ok64 CAPONOROOM = 0x30a6585d86d8616;
con ok64 CAPONODIFF = 0x30a6585d83523cf;  // no usable saved commit → full reindex
//  Singleton-open return codes, matching keeper/sniff/graf convention.
con ok64 SPOTOPEN   = 0x71961d619397;
con ok64 SPOTOPENRO = 0x71961d6193976d8;

extern b8 CAPO_COLOR;  // stdout is a terminal with color
extern b8 CAPO_TERM;   // stderr is a terminal

// Verbose call: prints step context on failure
#define vcall(step, f, ...)                                              \
    {                                                                    \
        __ = (f(__VA_ARGS__));                                           \
        if (__ != OK) {                                                  \
            fprintf(stderr, "spot: %s: %s (%s:%d)\n",                   \
                    step, ok64str(__), __func__, __LINE__);              \
            return __;                                                   \
        }                                                                \
    }

#define CAPO_DIR ".dogs/spot"
#define CAPO_IDX_EXT ".idx"
#define CAPO_SEQNO_WIDTH 10
#define CAPO_MAX_LEVELS 24
#define CAPO_SCRATCH_LEN (1UL << 27)  // 128M u64 entries = 1GB
#define CAPO_FLUSH_AT    (1UL << 24)  // flush at 16M entries (~128MB)
#define CAPO_MAX_SHAS 16

#define CAPOTriChar(c) (RON64_REV[(u8)(c)] != 0xff)

// Pack 3 RON64 chars into upper 32 bits of u64 (18-bit trigram, zero-padded)
fun u64 CAPOTriPack(u8cs tri) {
    u64 t = ((u64)RON64_REV[tri[0][0]] << 12) |
            ((u64)RON64_REV[tri[0][1]] << 6) |
            ((u64)RON64_REV[tri[0][2]]);
    return t << 32;
}

// Extract triplet from packed u64 entry
fun u64 CAPOTriOf(u64 entry) { return entry & 0xFFFFFFFF00000000ULL; }

// Path hash: lower 32 bits of RAPHash
fun u32 CAPOPathHash(u8csc path) { return (u32)RAPHash(path); }

// Pack trigram + path hash into u64
fun u64 CAPOEntry(u8cs tri, u8cs path) {
    return CAPOTriPack(tri) | (u64)CAPOPathHash(path);
}

// Index a single source file: parse, extract trigrams, append u64 entries
ok64 CAPOIndexFile(u64bp entries, u8csc source, u8csc ext, u8csc path);

// Write a sorted run to .dogs/spot/SEQNO.idx
ok64 CAPOIndexWrite(u8csc dir, u64cs run, u64 seqno);

// Load index stack from .dogs/spot/*.idx files (mmap each)
// stack: output array of runs; maps: output array of mapped buffers
// Returns number of files loaded in *nfiles
ok64 CAPOStackOpen(u64css stack, u8bp *maps, u32p nfiles, u8csc dir);

// Unmap all stack files
ok64 CAPOStackClose(u8bp *maps, u32 nfiles);

// Compact the LSM stack, unlink merged files
ok64 CAPOCompact(u8csc dir);

// Next available sequence number (max existing + 1)
ok64 CAPONextSeqno(u64p seqno, u8csc dir);

// Structural code search: needle is a code fragment, ext is file extension.
// When replace is non-empty, matched regions are replaced and files rewritten.
ok64 CAPOSpot(u8csc needle, u8csc replace, u8csc ext, u8csc reporoot,
              u8css files);

// Substring grep across all AST leaves (including comments).
// ext: optional language filter (empty = all parseable files).
// ctx_lines: max context lines above/below the match (like diff -C).
ok64 CAPOGrep(u8csc substring, u8csc ext, u8csc reporoot, u32 ctx_lines,
              u8css files);

// Regex grep using Thompson NFA (abc/NFA.h).
// pattern: regex string (supports . * + ? | () [] \d \w \s {n,m}).
// Extracts literal substrings for trigram index filtering, then NFA-matches
// candidate files line by line. Same output format as CAPOGrep.
ok64 CAPOPcreGrep(u8csc pattern, u8csc ext, u8csc reporoot, u32 ctx_lines,
                   u8css files);

// Compact all .idx files into a single run
ok64 CAPOCompactAll(u8csc dir);

// Resolve spot index dir from reporoot (<reporoot>/.dogs/spot)
ok64 CAPOResolveDir(path8b out, u8csc reporoot);

// Append a 40-char hex commit SHA to capodir/COMMIT (streaming ingest).
// No-op if sha40 already equals the tail entry; otherwise keeps the last
// CAPO_MAX_SHAS entries.
ok64 CAPOCommitAppend(u8csc capodir, u8csc sha40);

// Read saved commit shas from capodir/COMMIT (one per line, oldest first).
// Returns count of valid SHAs in *count (0 if file missing/empty).
ok64 CAPOCommitRead(u32p count, u8csc capodir,
                    char shas[][44], u32 maxcount);

// Check if extension is known to tok/ tokenizers
b8 CAPOKnownExt(u8csc ext);

// --- Symbol index entries ---

typedef u64 idx64;    // index entry

#define IDX64_TRI  0  // text trigram
#define IDX64_MEN  1  // S token — symbol mention
#define IDX64_DEF  2  // N token — symbol definition

fun u64 idx64Type(idx64 e)     { return e >> 62; }
fun u32 idx64Key(idx64 e)      { return (u32)(e >> 32) & 0x3FFFFFFF; }
fun u32 idx64PathHash(idx64 e) { return (u32)e; }

// Pack 30-bit symbol name hash into key position [61:32]
fun u64 CAPOSymKey(u8cs name) {
    return ((u64)((u32)(RAPHash(name)) & 0x3FFFFFFF)) << 32;
}

fun idx64 CAPOSymEntry(u64 type, u8cs name, u8cs path) {
    return (type << 62) | CAPOSymKey(name) | (u64)CAPOPathHash(path);
}

// --- DOG control struct (DOG.md rule 8) ---

#include "abc/FILE.h"
#include "dog/CLI.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"
#include "spot/LESS.h"

typedef struct {
    home    *h;                     // borrowed
    int      lock_fd;               // flock on .dogs/spot/.lock; -1 = none

    Bu8      arena;
    hunk     hunks[LESS_MAX_HUNKS];
    u8bp     maps[LESS_MAX_MAPS];
    Bu32     toks[LESS_MAX_MAPS];
    u32      nhunks;
    u32      nmaps;

    int          out_fd;
    spot_emit_fn emit;

    //  Ingestion scratch (rw only): postings accumulated by SPOTUpdate,
    //  flushed to a new .idx run when len >= CAPO_FLUSH_AT or on close.
    Bu64     entries;
    u64      seqno;                 // next run seqno

    b8 color;
    b8 term;
    b8 rw;
} spot;

typedef spot *spotp;
typedef spot const *spotcp;

//  Singleton.  Zero-initialised; populated by SPOTOpen.
extern spot SPOT;

// --- Public API (singleton, same contract as KEEP/SNIFF/GRAF) ---

//  Open spot state rooted at `home` (repo root).  Returns:
//    OK         I opened; pair with SPOTClose.
//    SPOTOPEN   already open compatible; use &SPOT, don't close.
//    SPOTOPENRO already ro and caller asked for rw.
//    (other)    real error — propagate.
ok64 SPOTOpen(home *h, b8 rw);

//  Run one CLI invocation.
ok64 SPOTExec(cli *c);

//  Feed a single git object into spot's trigram/symbol index.
//  obj_type uses DOG_OBJ_* (== git pack types).  For BLOB, `path`
//  picks the tokenizer via extension and provides the path_hash
//  used as the posting key.  TREE and TAG objects are no-ops
//  (keeper resolves tree → path on its side).  For COMMIT, the
//  object SHA is appended to .dogs/spot/COMMIT (rule 7).
ok64 SPOTUpdate(u8 obj_type, u8cs blob, u8csc path);

void SPOTClose(void);

//  Verb + value-flag tables for CLIParse.
extern char const *const SPOT_CLI_VERBS[];
extern char const SPOT_CLI_VAL_FLAGS[];

#endif
