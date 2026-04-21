# graf/ — token-level diff, merge, blame, history indexing

`graf` is a dog: it owns `.dogs/graf/` under a repo root and speaks
the four-function contract from [dog/DOG.md](../dog/DOG.md) §8
(`DOGOpen` / `DOGExec` / `DOGUpdate` / `DOGClose`).

Indexing is **streaming**: keeper feeds every commit/tree/blob it
ingests into `GRAFUpdate(obj_type, blob, path)`.  graf parses
commit headers, caches trees, and emits `wh128` DAG entries into
LSM-sorted runs.  No git CLI is ever invoked; no `.git/` is ever
read.  The only source of object data is keeper's pack store.

## Verbs

```
graf diff   old new                token-level colored diff (files on disk)
graf merge  base ours theirs       3-way merge; -o <file> to write out
graf blame  path                   token-level blame (reads keeper + DAG)
graf weave  path?from..to          weave diff across a ref range
graf index                         no-op; indexing is pushed by keeper
graf status                        index run/entry counts
```

## Files

| File          | Purpose |
|---------------|---------|
| `GRAF.h`      | Singleton state, arena, `graf_emit`, public API |
| `GRAF.c`      | `GRAFOpen` / `GRAFUpdate` / `GRAFClose`, arena init, `GRAFHunkEmit` |
| `GRAF.exe.c`  | `GRAFExec` — verb dispatch (diff / merge / blame / weave / status) |
| `GRAF.cli.c`  | `main()` — parse argv, open singleton, call `GRAFExec` |
| `DAG.{h,c}`   | LSM of `wh128` records under `.dogs/graf/` driven by `GRAFUpdate`. Types: COMMIT_GEN, COMMIT_PARENT, COMMIT_TREE, PATH_VER. PATH_VER keys on 40-bit `RAPHash(path)`; collisions verified by query-side keeper tree-walk. graf caches freshly-ingested blobs + trees per session; at `GRAFClose` it walks each new commit's root tree top-down and emits PATH_VER for leaves whose blob was freshly delivered. |
| `JOIN.{h,c}`  | `JOINTokenize` / `JOINMerge` — 3-way merge primitive over u64-hash token streams via abc/DIFFx LCS |
| `TDIFF.{h,c}` | Token-level diff: LCS on RAPHash, NEIL cleanup, emits hunks via `HUNKcb`.  Pure — no globals, no IO |
| `NEIL.{h,c}`  | Edit-list semantic cleanup: removes false short equalities, lossless boundary shifts |
| `DIFF.c`      | `GRAFDiff` — maps two files, calls `DIFFu8cs` with `GRAFHunkEmit` as callback |
| `MERGE.c`     | `GRAFMerge` — 3-way merge using `JOIN`, writes resolved bytes to file or stdout |
| `BLAME.c`     | `GRAFBlame` + `GRAFWeaveDiff` — walks DAG, pulls blobs via keeper, builds weave |
| `WEAVE.{h,c}` | Double-buffered weave of token versions with intro/del gens |

## Pager

When diff/weave/blame runs with a tty stdout, graf forks `bro`
(resolved via `HOMEResolveSibling`) and writes TLV hunks to the
pipe.  With non-tty stdout, graf writes plain ASCII via
`HUNKu8sFeedText` directly.  `merge` does not page.
