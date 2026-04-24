#ifndef SNIFF_DEL_H
#define SNIFF_DEL_H

//  DELETE: record explicit removal intent in the ULOG.
//
//  Append-only mirror of PUT: one `delete <path>` row per input URI.
//  No tree work, no pack writes — POST resolves the effective tree
//  at commit time.
//
//  nuris==0 is a no-op (bare `delete` → POST includes every tracked
//  file that's missing from disk, same sweep rule as PUT).
//
//  Each `uri` in `uris` is used for its path component only.

#include "SNIFF.h"
#include "abc/URI.h"

ok64 DELStage(u32 nuris, uri const *uris);

#endif
