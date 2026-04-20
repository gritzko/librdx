#ifndef SNIFF_PUT_H
#define SNIFF_PUT_H

//  PUT: stage a set of files into a new base tree.
//
//  Walks the sniff sorted path index depth-first, rebuilding every
//  subtree that contains a staged file; unchanged subtrees are
//  reused by hashlet.  Dirty files get fresh blobs; old hashlets
//  feed delta compression in keeper.
//
//  Side-effects:
//    * writes blobs + tree objects into `p`;
//    * updates SNIFF_BLOB / SNIFF_CHECKOUT for every touched file;
//    * updates SNIFF_TREE for every rebuilt subtree, including the
//      root — so the next PUT/DELETE/POST sees the new base.
//
//  file_set: if non-NULL, only paths with file_set[idx]=1 are staged.
//  If NULL, stages every file with a changed mtime (dirty).  Files
//  missing from disk are silently excluded (use DELStage for those).

#include "SNIFF.h"
#include "keeper/KEEP.h"

ok64 PUTStage(sha1 *tree_out, keep_pack *p,
              u8cs reporoot, u8cp file_set);

#endif
