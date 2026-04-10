#ifndef XX_WALK_H
#define XX_WALK_H

//  WALK: git object graph traversal
//
//  Provides a persistent walker over a BELT repository that
//  caches mmapped log + index for efficient repeated lookups.
//  Graph walks (commit history, tree enumeration, common ancestor)
//  use generation numbers stored in belt128 for pruning.

#include "BELT.h"

con ok64 WALKFAIL	= 0x80a5543ca495;
con ok64 WALKNONE	= 0x80a5545d85ce;
con ok64 WALKNOROOM	= 0x80a5545d86d8616;
con ok64 WALKBADFMT	= 0x80a5542ca34f59d;

#define WALK_BUFSZ (1 << 26)  // 64 MB resolve scratch

//  Walk state — opened on a belt dir, reused across lookups.
typedef struct {
    u8bp      logmap;
    u8cp      pack;
    u64       packlen;
    belt128cs runs[BELT_MAX_LEVELS];
    u8bp      maps[BELT_MAX_LEVELS];
    u32       nmaps;
    belt128css stack;
    u8p       buf1, buf2;
} walk;

//  Visitor callback: called with (hashlet, type, content, ctx).
//  Return OK to continue walking, anything else to stop.
typedef ok64 (*walk_fn)(u64 hashlet, u8 type, u8cs content, voidp ctx);

//  Open a walker on a belt directory. Mmaps log + all index runs.
ok64 WALKOpen(walk *w, u8cs belt_dir);

//  Close walker, unmap everything, free buffers.
ok64 WALKClose(walk *w);

//  Get object by hashlet. Writes content into `out` gauge,
//  sets `out_type` to BELT_COMMIT/TREE/BLOB/TAG.
ok64 WALKGet(walk *w, u64 hashlet, u8g out, u8p out_type);

//  Get object by raw 20-byte SHA-1.
ok64 WALKGetSha(walk *w, u8cp sha, u8g out, u8p out_type);

//  Parse tree SHA-1 from decompressed commit content.
//  Writes 20-byte binary SHA-1 into tree_sha.
ok64 WALKCommitTree(u8cs commit, u8 tree_sha[20]);

//  Walk commits reachable from head_sha (20-byte), newest first.
//  Stops at stop_sha (exclusive); pass NULL for full history.
//  Uses gen numbers for early BFS termination.
ok64 WALKCommits(walk *w, u8cp head_sha, u8cp stop_sha,
                 walk_fn visit, voidp ctx);

//  Walk tree at tree_sha (20-byte) recursively, depth-first.
//  Visits every blob and subtree.
ok64 WALKTree(walk *w, u8cp tree_sha, walk_fn visit, voidp ctx);

//  Find common ancestor of two commits (20-byte SHA-1s).
//  Uses gen-based pruning for simultaneous BFS.
//  Writes result into out (20 bytes). Returns WALKNONE if unrelated.
ok64 WALKAncestor(walk *w, u8cp sha_a, u8cp sha_b, u8 out[20]);

//  Enumerate objects reachable from head but not from base.
//  Visits commits, trees, and blobs in the difference set.
ok64 WALKMissing(walk *w, u8cp head_sha, u8cp base_sha,
                 walk_fn visit, voidp ctx);

//  Materialize a tree into a directory on the filesystem.
ok64 WALKCheckout(walk *w, u8cp tree_sha, u8cs dest);

#endif
