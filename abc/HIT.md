# HITx.h — Heap of ITerators

A HIT is a min-heap of sorted slices (iterators). Instantiate with the
**element** type; all slice/heap types are derived automatically.

```c
fun void u64csSwap(u64cs *a, u64cs *b) { /* swap two slices */ }

#define X(M, name) M##u64##name
#include "HITx.h"
#undef X
```

The only prerequisite is `X(,csSwap)` (e.g. `u64csSwap`) since array
types can't go through Sx.h.

## Types

| Derived type | Example   | Meaning                          |
|-------------|-----------|----------------------------------|
| X(,cs)      | u64cs     | Entry = const slice (iterator)   |
| X(,css)     | u64css    | Heap = slice of entries          |
| X(,csss)    | u64csss   | Slice of heaps (for IntersectMerge) |

## Functions

### Core

| Function            | Description                            |
|--------------------|----------------------------------------|
| `HITTStart(heap)`  | Filter empties, compact, heapify       |
| `HITTStep(heap)`   | Advance top, eject if done, sift down  |
| `HITTMerge(heap, &out)` | Sorted deduplicated drain         |
| `HITTIntersect(heap, &out, n)` | Emit only values in all N entries |
| `HITTSeek(heap, &key)` | Binary-search advance to key       |

### IntersectMerge (HIT-of-HITs)

| Function                        | Description                       |
|--------------------------------|-----------------------------------|
| `HITTsIntersectMerge(oh, &out)` | Intersect merged outputs of N inner HITs |
| `HITTSkipValue(inner)`          | Advance inner HIT past current top value |

## Merge

Drains the heap producing a sorted, deduplicated sequence. Each step
pops the minimum, advances all entries that had that value, skips
remaining duplicates.

```c
u64cs runs[3] = {{a, a+4}, {b, b+3}, {c, c+5}};
u64css heap = {runs, runs + 3};
HITu64Start(heap);
u64 buf[64]; u64p out = buf;
HITu64Merge(heap, &out);
// buf[0..out-buf) contains sorted deduplicated merge
```

## Intersect

Like Merge, but only emits values present in ALL N iterators.

```c
HITu64Start(heap);
u64 buf[64]; u64p out = buf;
HITu64Intersect(heap, &out, 3);  // 3 = number of runs
```

## IntersectMerge (HIT-of-HITs)

Intersects the merged outputs of N inner HITs without intermediate
buffers. Each inner HIT is a heap of sorted runs (e.g. mmapped index
files). The outer heap walks them in lockstep: emit a value only when
all inner HITs produce it.

```c
u64cs *outers[N][2];   // N inner HITs
outers[i][0] = runs_i; outers[i][1] = runs_i + nruns_i;
HITu64Start(outers[i]);  // start each inner HIT

u64csss oh = {outers, outers + N};
u64 buf[...]; u64p out = buf;
HITu64sIntersectMerge(oh, &out);
```

## Seek

Advances all entries until the heap top is >= key, using binary search
within each entry.

```c
u64 key = 42;
ok64 o = HITu64Seek(heap, &key);  // NODATA if all exhausted
```
