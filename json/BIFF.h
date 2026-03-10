#ifndef LIBRDX_BIFF_H
#define LIBRDX_BIFF_H

#include "BASON.h"
#include "PASS.h"

// Merge two BASONs (right-wins). Result written to out buf+idx.
// Objects: sorted-key parallel walk, same container type recurses.
// Arrays: positional walk by key, tail of longer appended.
// Scalars: right-side value replaces left-side value.
ok64 BASONMerge(u8bp out, u64bp idx,
                u64bp lstk, u8csc ldata,
                u64bp rstk, u8csc rdata);

// N-way merge using iterator heap. inputs ordered oldest-to-newest.
// Objects: sorted-key heap walk, same-type containers recurse.
// Arrays: positional walk by key, tail of longest appended.
// Scalars: last input wins. Null tombstone (type B, empty val) deletes.
ok64 BASONMergeN(u8bp out, u64bp idx, u8css inputs);

// u8ys-compatible wrapper for rocksdb merge operator.
ok64 BASONMergeY(u8s into, u8css inputs);

// Diff: produce patch such that merge(old, patch) == new.
// Deletions represented as null (type B, empty val).
// Added/changed keys emitted as-is.
// Identical subtrees omitted.
// hbuf: scratch buffer for hashes/work; if NULL, malloc'd internally.
ok64 BASONDiff(u8bp out, u64bp idx,
               u64bp ostk, u8csc odata,
               u64bp nstk, u8csc ndata,
               u64bp hbuf);

// Render colored diff: walk old BASON + patch in parallel,
// emit leaf values with ANSI colors (red+strike=del, green=add).
ok64 BASONDiffRender(u8s out,
                     u64bp ostk, u8csc odata,
                     u64bp pstk, u8csc pdata);

// PASS-based colored diff: walk two BASON states in parallel,
// emit colored output with context-line trimming.
// ctx=0 means no trimming (show all lines).
ok64 BASONDiffPrint(u8s out, u8csc odata, u8csc ndata, u32 ctx,
                     u8cs name);

#endif
