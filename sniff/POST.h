#ifndef SNIFF_POST_H
#define SNIFF_POST_H

//  POST: build tree from worktree state, write blobs + tree objects.
//
//  Walks sniff sorted path index depth-first.  Unchanged subtrees
//  are reused by hashlet.  Dirty files get new blobs; old hashlet
//  is passed to keeper for delta compression.
//
//  commit_set: if non-NULL, only files with commit_set[idx]=1 are
//  considered changed.  If NULL, all files with changed mtime.
//  Files missing from disk are excluded (deletions).

#include "SNIFF.h"
#include "keeper/KEEP.h"

//  Build tree from worktree, write blobs + trees into pack.
//  Collects old SHAs from parent_hex (commit hex prefix).
//  Returns root tree SHA via tree_out.
ok64 POSTTree(sha1 *tree_out, sniff *s, keeper *k, keep_pack *p,
              u8cs reporoot, u8cs parent_hex, u8cp commit_set);

#endif
