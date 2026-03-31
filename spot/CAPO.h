#ifndef LIBRDX_CAPO_H
#define LIBRDX_CAPO_H

#include <stdio.h>
#include "abc/INT.h"
#include "abc/MSET.h"
#include "abc/PATH.h"
#include "abc/RON.h"
#include "abc/RAP.h"

con ok64 CAPONOROOM = 0x30a6585d86d8616;

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

#define CAPO_DIR ".git/spot"
#define CAPO_IDX_EXT ".idx"
#define CAPO_SEQNO_WIDTH 10
#define CAPO_MAX_LEVELS MSET_MAX_LEVELS
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
fun u32 CAPOPathHash(u8cs path) { return (u32)RAPHash(path); }

// Pack trigram + path hash into u64
fun u64 CAPOEntry(u8cs tri, u8cs path) {
    return CAPOTriPack(tri) | (u64)CAPOPathHash(path);
}

// Index a single source file: parse, extract trigrams, append u64 entries
ok64 CAPOIndexFile(u64bp entries, u8csc source, u8csc ext, u8csc path);

// Write a sorted run to .git/spot/SEQNO.idx
ok64 CAPOIndexWrite(u8csc dir, u64cs run, u64 seqno);

// Load MSET stack from .git/spot/*.idx files (mmap each)
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

// Colorful cat: syntax-highlighted file output
ok64 CAPOCat(u8css files, u8csc reporoot);

// Token-level 3-way merge (git merge driver)
// spot --merge base ours theirs [-o output]
ok64 CAPOMerge(u8csc base, u8csc ours, u8csc theirs, u8csc outpath);

// Token-level diff with syntax-highlighted output
// spot --diff old new
// name: logical path for display (NULL = use old_path/new_path)
ok64 CAPODiff(u8csc old_path, u8csc new_path, u8csc name);

// Full reindex: all tracked files
ok64 CAPOReindex(u8csc reporoot);

// Parallel reindex: process K of N (indexes files where file# % N == K)
// Uses seqno = N*batch + K + 1 to avoid collisions between procs
ok64 CAPOReindexProc(u8csc reporoot, u32 nprocs, u32 proc);

// Incremental index: changed files only
ok64 CAPOHook(u8csc reporoot);

// Compact all .idx files into a single run
ok64 CAPOCompactAll(u8csc dir);

// Resolve spot index dir from reporoot (handles worktrees)
ok64 CAPOResolveDir(path8b out, u8csc reporoot);

// Write current HEAD sha to capodir/COMMIT
ok64 CAPOCommitWrite(u8csc reporoot, u8csc capodir);

// Read saved commit shas from capodir/COMMIT (one per line, oldest first).
// Returns count of valid SHAs in *count (0 if file missing/empty).
ok64 CAPOCommitRead(u32p count, u8csc capodir,
                    char shas[][44], u32 maxcount);

// Check if extension is known to tok/ tokenizers
b8 CAPOKnownExt(u8csc ext);

#endif
