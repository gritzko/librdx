#ifndef SNIFF_COM_H
#define SNIFF_COM_H

//  COM: commit worktree files to keeper.
//
//  Reads the current HEAD tree from keeper, diffs against sniff state,
//  creates blob objects for changed/new files, builds new tree objects
//  bottom-up, creates a commit object pointing to the new root tree.
//
//  commit_set: if non-NULL, only files with commit_set[idx]=1 are
//  included.  If NULL, all files with changed mtime are committed.
//  Files missing from disk are excluded from the new tree.

#include "SNIFF.h"
#include "keeper/KEEP.h"

ok64 COMCommit(sniff *s, keeper *k, u8cs reporoot,
               u8cs parent_hex, u8cs message, u8cs author,
               u8cp commit_set, sha1 *sha_out);

#endif
