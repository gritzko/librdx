#ifndef LIBRDX_BIFF_H
#define LIBRDX_BIFF_H

#include "BASON.h"

// Merge two BASONs (right-wins). Result written to out buf+idx.
// Objects: sorted-key parallel walk, same container type recurses.
// Arrays: positional walk by key, tail of longer appended.
// Scalars: right-side value replaces left-side value.
ok64 BASONMerge(u8bp out, u64bp idx,
                u64bp lstk, u8csc ldata,
                u64bp rstk, u8csc rdata);

// Diff: produce patch such that merge(old, patch) == new.
// Deletions represented as null (type B, empty val).
// Added/changed keys emitted as-is.
// Identical subtrees omitted.
ok64 BASONDiff(u8bp out, u64bp idx,
               u64bp ostk, u8csc odata,
               u64bp nstk, u8csc ndata);

#endif
