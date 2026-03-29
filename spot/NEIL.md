# NEIL — Semantic EDL Cleanup

NEIL post-processes token-level edit description lists (EDLs) produced by
Myers diff (`abc/DIFFx.h`). The diff operates on u64 token hashes, so it
often produces short false-match EQ entries — a semicolon or brace that
happens to match between two large changed regions. These false equalities
fragment the diff output and make it noisy. NEIL eliminates them.

## Interface

```c
#include "spot/NEIL.h"

ok64 NEILCleanup(e32g edl, u32cs old_toks, u32cs new_toks,
                 u8csc old_src, u8csc new_src);
```

Modifies `edl` in place. Token arrays and source slices are read-only
inputs used to measure byte lengths. Returns `OK` or `NEILBAD` on
allocation failure.

## Algorithm

Iterative false-equality elimination (up to 8 passes, stops when stable):

1. **Compute token offsets** — walk the EDL to map each entry to its
   position in the old/new token arrays.

2. **Scan EQ entries** — for each EQ, measure its byte span and the byte
   spans of edits on both sides (accumulated DEL+INS before the previous
   EQ, after the next EQ). Byte span is computed from packed u32 token
   offsets via `TOK_OFF`.

3. **Two-tier kill condition** — small EQs (< 16 bytes) are killed if
   `eq_bytes < before_bytes + after_bytes` (sum, aggressive). Larger EQs
   require both sides to individually exceed them:
   `eq_bytes < before_bytes && eq_bytes < after_bytes` (conservative).
   Killed EQs become DEL+INS pairs, merging into surrounding edits.

4. **Merge** — adjacent same-op entries are consolidated. If any EQ was
   killed, the next iteration may expose new candidates.

## Integration

Called in `CAPO.c` after `DIFFu64s()` and `CAPOJoinToks()`, before the
Phase 2 renderer emits colored output. The Phase 2 renderer separately
handles prefix/suffix extraction at change-region boundaries to recover
tokens that Myers misaligned.

## Naming

After Neil Fraser, author of the diff-match-patch semantic cleanup that
inspired this module.
