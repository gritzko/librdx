#ifndef SNIFF_AT_H
#define SNIFF_AT_H

//  AT: per-worktree branch + commit pointer.  See sniff/AT.md.
//
//  `.sniff/at.log` is an append-only log of checkout / commit
//  transitions.  Each line:
//
//      <ron60-time>\t?<branch>\t?<sha>\n
//
//  `<branch>` is the ref path minus leading `refs/` (e.g. `heads/main`)
//  or empty for a detached checkout.  The **tail** line is
//  authoritative — earlier lines are history.

#include "abc/INT.h"
#include "abc/RON.h"
#include "abc/BUF.h"

#define SNIFF_AT_FILE "at.log"

//  Tail entry.  `branch` and `sha` are copied into caller-owned buffers.
typedef struct {
    ron60 time;
    u8bp  branch;   // e.g. "heads/main"; empty = detached
    u8bp  sha;      // 40 hex chars
} sniff_at;

//  Read the tail line.  Returns:
//    OK         *out populated.
//    KEEPNONE   at.log missing or empty.
//    (other)    real error (malformed tail, I/O).
ok64 SNIFFAtRead(sniff_at *out);

//  Append one entry with RONNow() timestamp.  `branch` may be empty.
//  `sha` must be exactly 40 hex chars.
ok64 SNIFFAtAppend(u8cs branch, u8cs sha);

#endif
