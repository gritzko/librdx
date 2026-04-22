#ifndef SNIFF_STAGE_H
#define SNIFF_STAGE_H

//  STAGE: per-branch staging pack.  See sniff/STAGE.md.
//
//  The staging log is a git-style pack store at
//  `.sniff/<branch>/` that holds tree+blob objects produced by
//  `be put` / `be delete`.  On `be post`, reachable staged objects
//  are re-packed canonically (commit→trees→blobs) and appended to
//  keeper's main log; then STAGEDrop removes the staging dir.
//
//  This module owns ONLY the file/dir plumbing and the staging-
//  local `.idx`.  All pack byte encoding uses keeper's
//  KEEPPackFeed / PACKDrainObjHdr / PACKInflate.  Sniff implements
//  zero pack logic.
//
//  Layout mirrors keeper's main log:
//      .sniff/<branch>/log/0000000001.pack
//      .sniff/<branch>/idx/0000000001.idx

#include "keeper/KEEP.h"

//  Resolve `<reporoot>/.sniff/<branch>/` into `out`.
ok64 STAGEDir(path8b out, u8cs reporoot, u8cs branch);

//  Resolve the current staging branch: read from at.log tail, or
//  default to "heads/master" when at.log is missing/empty.  Fails if
//  at.log's tail is detached (empty branch field).  Fills `out`.
ok64 STAGEBranch(u8bp out);

//  Open the branch staging pack for append.  Sets up p->log, p->entries,
//  p->file_id=1, p->pack_offset.  Writes the 12-byte PACK magic on
//  a fresh file.  Creates log/ and idx/ subdirs if missing.
ok64 STAGEOpen(keep_pack *p, u8cs branch);

//  Finalize: push a pack bookmark into p->entries, sort, merge with
//  any pre-existing idx, rewrite the single .idx file.  Unmaps the
//  FILEBook.  (No PACK count patch — git-only concern.)
ok64 STAGEClose(keep_pack *p, u8cs branch);

//  Lookup an object hashlet in the staging idx.  Returns OK + val
//  (wh64Pack(flags, file_id, offset)) or KEEPNONE.
ok64 STAGELookup(u8cs branch, u64 hashlet60, size_t hexlen, u64p val_out);

//  Retrieve an object by hashlet: lookup + inflate.
ok64 STAGEGet(u8cs branch, u64 hashlet60, size_t hexlen,
              u8bp out, u8p out_type);

//  Walk every object entry in the staging idx in key-sorted order.
//  Each callback receives (type, body, hashlet, ctx).  Bookmarks are
//  skipped.
ok64 STAGEEach(u8cs branch, keep_cb cb, void *ctx);

//  rm -rf .sniff/<branch>/.  Idempotent.
ok64 STAGEDrop(u8cs branch);

#endif
