#ifndef SNIFF_PUT_H
#define SNIFF_PUT_H

//  PUT: record explicit stage intent in the ULOG.
//
//  Append-only in the new model: one `put <path>` row per input URI.
//  No tree work, no pack writes, no hashing — POST does all of that
//  at commit time by walking the baseline tree against the wt.
//
//  nuris==0 is a no-op (the "bare put" sweep is POST's job: with no
//  put/delete rows since the last post, POST auto-includes every
//  file whose mtime isn't in the ULOG stamp-set).
//
//  Each `uri` in `uris` is used for its path component.  Query /
//  fragment are ignored — `put` rows only carry a path.

#include "SNIFF.h"
#include "abc/URI.h"

ok64 PUTStage(u32 nuris, uri const *uris);

#endif
