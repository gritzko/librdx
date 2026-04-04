# HITx.h — Generic Heap Iterator Template

A template for draining sorted data through a min-heap, supporting both
primitives (e.g. u64) and sorted slices (e.g. u64cs). Parameterized by
advancer `x`, merge policy `y`, and comparator `z`.

## Usage

Include `Sx.h` for type T first, then include `HITx.h`:

```c
#define X(M, name) M##u64##name
#include "HITx.h"
#undef X
```

For array types like u64cs that can't go through Sx.h, define `Swap`
manually and provide the x/y/z function pointer typedefs. For Start,
define `HIT_ENTRY_IS_SLICE` before inclusion:

```c
#define HIT_ENTRY_IS_SLICE
#define X(M, name) M##u64cs##name
#include "HITx.h"
#undef X
#undef HIT_ENTRY_IS_SLICE
```

## Functions

- `HITTDownYZ(heap, at, z)` — sift down at position
- `HITTUpYZ(heap, at, z)` — bubble up at position
- `HITTHeapYZ(heap, z)` — Floyd's heapify
- `HITTTopsYZ(heap, eqs, z)` — find equal-minimum group, move to front
- `HITTEject(heap, at)` — swap with last, shrink
- `HITTNextYZ(heap, out, x, y, z)` — drain one value into out slice
- `HITTStartZ(heap, z)` — filter empty entries, compact, heapify
  (requires `#define HIT_ENTRY_IS_SLICE` — only for slice-type entries)
- `HITTSeekXZ(heap, key, x, z)` — advance heap until top >= key;
  `x` seeks within one entry via binary search

## NextYZ flow

1. TopsYZ — find equal-minimum entries
2. `y(*out, tops)` — merge policy decides emit (OK) or skip (MISS)
3. For each top: `x(&entry, &min)` — advance past min value
   - OK → entry has more data, sift down
   - NODATA → entry exhausted, eject + sift replacement
4. If y returned OK, advance `out[0]`

## Separation of concerns

- `x` (advancer): type-specific entry advancement. Primitives: always
  return NODATA (consumed). Slices: advance past equal values, return
  OK if more data remains.
- `y` (merge policy): pure emit/skip decision. Union: always OK.
  Intersection: OK only if `$len(tops) >= nruns`.
- `z` (comparator): standard less-than.
