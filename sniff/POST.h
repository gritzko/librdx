#ifndef SNIFF_POST_H
#define SNIFF_POST_H

//  POST: commit the current base tree.
//
//  Wraps the root-dir SNIFF_TREE hashlet into a commit object with
//  parent = current HEAD and updates HEAD to the new commit.
//
//  If the base tree is unset or equals the HEAD commit's tree (i.e.
//  no prior PUT/DELETE has staged anything), POSTCommit first calls
//  PUTStage(s, k, reporoot, NULL) to auto-stage everything dirty on
//  disk.  This matches `git commit -a` ergonomics.

#include "SNIFF.h"
#include "keeper/KEEP.h"

ok64 POSTCommit(sniff *s, keeper *k, u8cs reporoot,
                u8cs message, u8cs author, sha1 *sha_out);

#endif
