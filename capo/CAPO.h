#ifndef LIBRDX_CAPO_H
#define LIBRDX_CAPO_H

#include "abc/INT.h"
#include "abc/MSET.h"
#include "abc/RON.h"
#include "abc/RAP.h"

con ok64 CAPONOROOM = 0x30a6585d86d8616;

#define CAPO_DIR ".git/capo"
#define CAPO_IDX_EXT ".idx"
#define CAPO_SEQNO_WIDTH 10
#define CAPO_MAX_LEVELS MSET_MAX_LEVELS
#define CAPO_SCRATCH_LEN (1UL << 27)  // 128M u64 entries = 1GB
#define CAPO_FLUSH_AT    (1UL << 24)  // flush at 16M entries (~128MB)

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

// Write a sorted run to .git/capo/SEQNO.idx
ok64 CAPOIndexWrite(u8csc dir, u64cs run, u64 seqno);

// Load MSET stack from .git/capo/*.idx files (mmap each)
// stack: output array of runs; maps: output array of mapped buffers
// Returns number of files loaded in *nfiles
ok64 CAPOStackOpen(u64css stack, u8bp *maps, u32p nfiles, u8csc dir);

// Unmap all stack files
ok64 CAPOStackClose(u8bp *maps, u32 nfiles);

// Compact the LSM stack, unlink merged files
ok64 CAPOCompact(u8csc dir);

// Next available sequence number (max existing + 1)
ok64 CAPONextSeqno(u64p seqno, u8csc dir);

// Full query pipeline: selector -> trigrams -> intersect -> parse -> output
ok64 CAPOQuery(u8csc selector, u8csc reporoot);

// Full reindex: all tracked files
ok64 CAPOReindex(u8csc reporoot);

// Parallel reindex: process K of N (indexes files where file# % N == K)
// Uses seqno = N*batch + K + 1 to avoid collisions between procs
ok64 CAPOReindexProc(u8csc reporoot, u32 nprocs, u32 proc);

// Incremental index: changed files only
ok64 CAPOHook(u8csc reporoot);

// Compact all .idx files into a single run
ok64 CAPOCompactAll(u8csc dir);

#endif
