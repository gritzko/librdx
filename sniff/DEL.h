#ifndef SNIFF_DEL_H
#define SNIFF_DEL_H

//  DEL: build a tree with specified files/dirs removed.
//
//  Walks sniff sorted path index, excludes entries in the delete
//  set, rebuilds only affected subtrees.  Unchanged subtrees are
//  reused by hashlet.

#include "SNIFF.h"
#include "keeper/KEEP.h"

//  Build tree excluding deleted paths.
//  del_set[idx]=1 for paths to remove.
//  parent_hex: commit to diff against.
//  Returns new root tree SHA via tree_out.
ok64 DELTree(sha1 *tree_out, sniff *s, keeper *k, keep_pack *p,
             u8cs reporoot, u8cs parent_hex, u8cp del_set);

#endif
