#ifndef KEEPER_PATHS_H
#define KEEPER_PATHS_H

//  PATHS: keeper-side path registry.
//
//  Append-only table of repo-relative path strings, keyed by stable
//  u32 index.  Directory paths end with '/'.  Index 0 = empty path
//  (root tree).  Persisted at `.dogs/keeper/paths.log`.  Shared by
//  every dog that needs to name paths (checkout, indexers, etc.).
//
//  Backing state fields live on the `keeper` struct (paths_log,
//  paths_offs, paths_hash) — see keeper/KEEP.h.

#include "KEEP.h"

//  Idempotent: returns the existing index if `path` is already
//  interned, otherwise appends + records and returns the new index.
//  Returns 0 on failure (same value as the reserved empty-path idx,
//  so callers should check `KEEPPathCount` before/after if they need
//  to distinguish).
u32  KEEPIntern(keeper *k, u8cs path);

//  Reverse lookup: write a slice spanning the stored path at `idx`
//  (no trailing '\n') into `out`.  Slice stays valid while keeper is
//  open.  Returns KEEPNONE if `idx` is out of range.
ok64 KEEPPath(keeper const *k, u32 idx, u8csp out);

//  Number of interned paths (next index to be assigned).
fun u32 KEEPPathCount(keeper const *k) {
    return (u32)u32bDataLen(k->paths_offs);
}

//  Internal lifecycle — invoked by KEEPOpen / KEEPClose; not meant
//  to be called by dog callers.
ok64 KEEPPathsOpen(keeper *k, b8 rw);
void KEEPPathsClose(keeper *k);

#endif
