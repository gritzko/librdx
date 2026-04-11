#ifndef SNIFF_COM_H
#define SNIFF_COM_H

//  COM: commit changed worktree files to keeper.
//
//  Reads the current HEAD tree from keeper, diffs against sniff state,
//  creates blob objects for changed files, builds new tree objects
//  bottom-up, creates a commit object pointing to the new root tree.

#include "SNIFF.h"
#include "keeper/KEEP.h"

//  Commit changed files to keeper.
//  parent_hex: parent commit SHA prefix (current HEAD).
//  message: commit message.
//  author: "Name <email>" string.
//  sha_out: receives the 20-byte SHA of the new commit.
ok64 COMCommit(sniff *s, keeper *k, u8cs reporoot,
               u8cs parent_hex, u8cs message, u8cs author,
               u8 sha_out[20]);

#endif
