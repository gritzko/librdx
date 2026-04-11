#ifndef KEEPER_REFS_H
#define KEEPER_REFS_H

//  REFS: URI→URI append-only reflog for keeper.
//
//  Stored in .dogs/keeper/REFS as plain text, one mapping per line:
//    <ron60-timestamp>\t<from-uri>\t<to-uri>\n
//
//  Examples:
//    Cb2q1~00	//github	https://github.com/torvalds/linux.git
//    Cb2q1~00	?refs/heads/master	?6d707f8f9e42072b84c6a00ac959af59356affea
//    Cb2q2A00	?refs/heads/master	?abc123def456789...
//
//  Aliases map authority→full URL:
//    //github → https://github.com/torvalds/linux.git
//
//  Ref→SHA maps use query syntax:
//    ?refs/heads/master → ?<hex-sha>
//
//  Resolution: chase from→to iteratively (max REFS_MAX_CHAIN).
//  Compaction: collapse entries with same from-key, keep latest.

#include "abc/INT.h"
#include "abc/URI.h"
#include "abc/RON.h"
#include "abc/FILE.h"

con ok64 REFSFAIL  = 0x11c53ca495;
con ok64 REFSNONE  = 0x11c55d85ce;
con ok64 REFSBAD   = 0x11c538b28d;

#define REFS_FILE     "REFS"
#define REFS_MAX_CHAIN 8
#define REFS_MAX_REFS  1024

//  Append one from→to mapping with current timestamp.
ok64 REFSAppend(u8csc dir, u8csc from_uri, u8csc to_uri);

//  Resolve a URI by chasing from→to chain.
//  Resolves authority (alias) and query (ref) independently.
//  Result parts written into `resolved` uri struct.
//  `arena` provides scratch space for resolved strings.
ok64 REFSResolve(urip resolved, u8bp arena, u8csc dir, u8csc uri);

//  Record refs from a sync: parallel arrays of ref name slices
//  and hex SHA slices.  Appends ?refname→?sha lines.
ok64 REFSSyncRecord(u8csc dir, u8css refs, u8css shas, u32 nrefs);

//  List current (latest) value for each known from-URI.
typedef ok64 (*refs_cb)(u8cs from, u8cs to, ron60 timestamp, void *ctx);
ok64 REFSEach(u8csc dir, refs_cb cb, void *ctx);

//  Compact: rewrite REFS keeping only latest entry per from-key.
ok64 REFSCompact(u8csc dir);

#endif
