#ifndef SPOT_RXLITS_H
#define SPOT_RXLITS_H

#include "abc/INT.h"

// Per-byte regex-literals callback.
//   ch    : a literal byte from the pattern
//   flush : YES when a meta char / class / shorthand ended the run
//   ctx   : opaque
// Return value reserved for future use; current implementation ignores it.
typedef ok64 (*rxlits_cb)(void *ctx, u8 ch, b8 flush);

// Walk a regex pattern, invoking cb for every literal byte (with flush=NO)
// and once for every meta-boundary (with flush=YES, ch=0).  A final flush
// is always emitted on completion.
void RXLITSu8sDrain(u8csc pattern, rxlits_cb cb, void *ctx);

#endif
