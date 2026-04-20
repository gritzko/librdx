#ifndef SNIFF_DEL_H
#define SNIFF_DEL_H

//  DELETE: stage a tree with the specified paths removed.
//
//  Walks the sniff sorted path index, excluding every entry marked
//  in `del_set`, and rebuilds only the subtrees that were affected.
//  Unchanged subtrees are reused by hashlet.
//
//  Side-effects mirror PUT:
//    * writes tree objects into `p`;
//    * updates SNIFF_TREE for every rebuilt subtree (including root);
//    * clears SNIFF_BLOB / SNIFF_CHECKOUT for deleted files.
//
//  del_set: if NULL, auto-stage every tracked file that is missing
//  from disk (the "DELETE no-args" CLI shape).

#include "SNIFF.h"

//  Writes tree objects to the current branch's staging pack (opened
//  internally via STAGE).  No commit.
ok64 DELStage(sha1 *tree_out, u8cs reporoot, u8cp del_set);

#endif
