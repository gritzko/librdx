#ifndef SNIFF_CHE_H
#define SNIFF_CHE_H

//  CHE: checkout a commit tree from keeper into the worktree.
//
//  Skips unchanged files (hashlet match), protects dirty files
//  (worktree modified), creates symlinks for mode 120000,
//  skips submodules (mode 160000).  Records SNIFF_HASHLET +
//  SNIFF_CHECKOUT for every written file.

#include "SNIFF.h"
#include "keeper/KEEP.h"

//  Checkout a commit from keeper into the worktree.
//  hex: commit SHA hex prefix (6–10 chars).
ok64 CHECheckout(sniff *s, keeper *k, u8cs reporoot, u8cs hex);

#endif
