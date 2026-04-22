#ifndef DOG_AT_H
#define DOG_AT_H

//  Read the tail line of `<reporoot>/.sniff/at.log`.
//  Line format: `<ron60-time>\t?<branch>\t?<sha>\n`.
//
//  On OK:
//    - `branch_out` receives the branch path (may be empty = detached).
//    - `sha_out` receives exactly 40 hex chars.
//  Returns KEEPNONE when the file is missing/empty.
//
//  See sniff/AT.md for the spec.  This helper exists so keeper and
//  beagle can read the worktree pointer without linking snifflib.

#include "abc/INT.h"
#include "abc/BUF.h"

ok64 DOGAtTail(u8bp branch_out, u8bp sha_out, u8cs reporoot);

#endif
