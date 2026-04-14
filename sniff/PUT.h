#ifndef SNIFF_PUT_H
#define SNIFF_PUT_H

//  PUT: create a commit from worktree state.
//
//  Resolves parent, calls POSTTree to build blobs + trees,
//  creates commit object.  Thin wrapper around POST.

#include "SNIFF.h"
#include "keeper/KEEP.h"

//  Commit worktree changes to keeper.
//  commit_set: NULL=all changed, else bitmap.
//  Returns commit SHA via sha_out.
ok64 PUTCommit(sniff *s, keeper *k, u8cs reporoot,
               u8cs parent_hex, u8cs message, u8cs author,
               u8cp commit_set, sha1 *sha_out);

#endif
