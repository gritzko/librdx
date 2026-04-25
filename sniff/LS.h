#ifndef SNIFF_LS_H
#define SNIFF_LS_H

//  LS — `ls:` projector.  Emits a file listing for a worktree, a
//  subtree, a tree at a ref, or (TODO) the diff-stat between two
//  refs.  Two outputs: plain text (default, one path per line) or a
//  single HUNK TLV blob (--tlv) that `bro` can render as a pager
//  hunk.  See VERBS.md §"View projectors" and sniff/AT.md.
//
//  Modes, by first URI shape:
//    (none)                 — wt registry listing (like `sniff list`)
//    <path>                 — filter registry by subdir prefix
//    ?                      — tree at current branch tip
//    ?<ref|sha>             — tree at that ref/sha
//    <path>?<ref|sha>       — subtree at that ref/sha
//    ?<v1>..<v2>            — (TODO) diff-stat, +added -removed per file
//
//  Output contract:
//    tlv == NO  → plain ascii, `<path>\n` per entry, stdout.
//    tlv == YES → one HUNK TLV record wrapping the same text; URI
//                 field is a human-readable title (mode hint + ref);
//                 no toks, no hili.  Bro picks it up via BROPipeRun.

#include "abc/INT.h"
#include "abc/URI.h"

#include "SNIFF.h"

ok64 SNIFFLs(u8cs reporoot, uri const *u, b8 tlv);

#endif
