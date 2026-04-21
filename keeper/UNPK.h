#ifndef KEEPER_UNPK_H
#define KEEPER_UNPK_H

//  UNPK: single-pass packfile indexer.
//
//  Given a pack mapped in memory, resolve every object's SHA-1 —
//  following OFS_DELTA / REF_DELTA chains — and emit one wh128
//  entry per object:  key = hashlet60 | type,  val = flags | file_id
//  | log_off.  Caller then sorts/dedups and writes the LSM run.
//
//  Algorithm:
//    1. Pre-scan: record (offset, type) per object, inflating into
//       scratch just to advance the cursor.
//    2. Build delta forest: link each OFS_DELTA to its parent by
//       offset; stash REF_DELTAs as waiters keyed on sha8.
//    3. Resolve bases; each base hash drains its REF_DELTA waiters
//       into the forest.
//    4. DFS-walk each base's subtree, applying deltas onto a stack
//       of inflated content kept in keeper's buf1.  Inflate-once;
//       arena depth bounded by chain depth.
//    5. Thin-pack fallback: REF_DELTAs whose base never appeared
//       in this pack look up the base via KEEPGet (previous packs
//       in the log / earlier runs in the LSM).
//
//  Offsets in emitted wh128.val are log-relative.  The caller must
//  pass a `pack` slice that already sits at its final log location
//  (i.e., a view into the mmapped log file), so scan-cursor offsets
//  off-by-log-base correspond to wh128 offsets directly.

#include "KEEP.h"
#include "dog/SHA1.h"
#include "dog/WHIFF.h"

con ok64 UNPKFAIL    = 0x7de3e23ca495;
con ok64 UNPKBADFMT  = 0x7de3e22ca34f59d;
con ok64 UNPKNOROOM  = 0x7de3e25d86d8616;

//  Per-object event delivered during UNPKIndex if `emit` is non-NULL.
//  `path` is derived from tree walks within the pack: for a blob, the
//  path at which its containing tree named it; for a tree, the path
//  of the tree itself (prefix used for its children); for a commit,
//  empty.  When a path cannot be derived (e.g., non-standard pack
//  ordering, blob whose tree is not in this pack), `path` is empty.
//  `content` points into keeper's scratch (k->buf1) and is valid only
//  for the duration of the callback.
typedef void (*unpk_emit_fn)(void *ctx, u8 type, sha1 const *sha,
                              u8csc path, u8cs content);

typedef struct {
    u8cs pack;        // log-mapped pack bytes ([pack[0]..pack[1]))
    u64  scan_start;  // byte offset in pack where first object starts
                      // (12 for a fresh PACK header, append-offset otherwise)
    u64  scan_end;    // byte offset where object data ends
                      // (packlen-20 for fresh with trailer, packlen otherwise)
    u32  count;       // number of objects to scan
    u32  file_id;     // keeper log file id for wh128.val
    unpk_emit_fn emit;    // optional: called once per resolved object
    void        *emit_ctx;
} unpk_in;

typedef struct {
    u32  indexed;     // successfully hashed + emitted
    u32  skipped;     // inflate / delta-apply / chain-depth failures
    u32  cross;       // resolved via thin-pack fallback
    u32  base_count;  // non-delta objects
    u32  paths_known; // objects for which the path map yielded a path
    u32  paths_empty; // objects emitted with an empty path fallback
} unpk_stats;

//  Index one pack.  Entries appended to `out` (unsorted, undeduped).
//  `k` supplies scratch buffers (buf1..buf4) and is used for thin-pack
//  REF_DELTA resolution via KEEPGet against previously-loaded packs.
//  If `in->emit` is non-NULL, every resolved object triggers a callback
//  with its type, sha, derived path, and content bytes.
ok64 UNPKIndex(keeper *k, unpk_in const *in,
               Bwh128 out, unpk_stats *stats);

//  Process-wide indexer hook.  When set, KEEPIngestFile threads these
//  into UNPKIndex's emit so a CLI (or embedder) can fan every resolved
//  object out to whatever indexes live in the same process — graf/spot
//  etc.  NULL by default; the keeper library never installs anything
//  itself, to avoid pulling graflib/spotlib into keeplib.
extern unpk_emit_fn keep_indexer_emit;
extern void        *keep_indexer_ctx;

#endif
