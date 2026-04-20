# graf/ — token-level diff & merge tool

`graf` is the user-facing diff and merge tool. It wraps the pure
diff function in [dog/DIFF.h](../dog/DIFF.h) and the 3-way merge in
[dog/JOIN.h](../dog/JOIN.h), pages results through [bro/](../bro/INDEX.md)
when stdout is a tty, and writes plain ASCII (git-diff style) otherwise.

## Modes

```
graf -d | --diff old new           token-level colored diff
graf --gitdiff <7 args from git>   git external diff driver
graf --merge base ours theirs      3-way merge to stdout
graf --merge base ours theirs -o f merge to file
graf -n | --install                install as git diff/merge driver
```

## Files

| File          | Purpose |
|---------------|---------|
| `GRAF.h`      | Producer staging (`graf_arena`, `graf_out_fd`, `graf_emit`), public diff/merge/install entries |
| `GRAF.c`      | Arena init/cleanup, `GRAFHunkEmit` (HUNKcb that serializes via `graf_emit` to `graf_out_fd`) |
| `DAG.{h,c}`   | Git object-graph indexer driven by `GRAFUpdate` (streaming DOG interface). Emits `wh128` records into LSM sorted runs under `.dogs/graf/`. Entry types: COMMIT_GEN, COMMIT_PARENT, COMMIT_TREE, PATH_VER. PATH_VER keys on 40-bit `RAPHash(path)`; collisions tolerated by query-side verification. No PATHS file. Keeper feeds commits, trees, blobs (in any order) per pack; graf caches trees in-memory, at `GRAFClose` walks each new commit's root tree top-down and emits PATH_VER for leaves whose blob was freshly delivered. |
| `JOIN.{h,c}`  | Token-level 3-way merge primitive: `JOINTokenize` (tokenize + RAPHash per token), `JOINMerge` (merge token streams via abc/DIFFx u64 LCS) |
| `TDIFF.{h,c}` | Token-level diff algorithm: `DIFFu8cs(arena, old, new, ext, name, cb, ctx)` runs LCS over u64 hashes (via JOIN), applies NEIL cleanup, yields hunks via `HUNKcb`. Pure library — no globals, no IO |
| `NEIL.{h,c}`  | Diff edit-list semantic cleanup: removes false short equalities, lossless boundary shifts |
| `DIFF.c`      | `GRAFDiff` — maps the two files, calls `DIFFu8cs` with `GRAFHunkEmit` as the callback; handles `--diff` and `--gitdiff` |
| `MERGE.c`     | `GRAFMerge` — 3-way merge using `JOIN`, writes resolved bytes to file or stdout |
| `INST.c`      | `GRAFInstall` — sets `diff.graf.command`, `merge.graf.driver`, writes `.git/info/attributes` |
| `GRAF.cli.c`  | `main()`: arg parsing, fork bro on tty, dispatch |

## Pager

When `--diff`/`--gitdiff` runs with a tty stdout, graf forks `bro`
(resolved via `HOMEResolveSibling`) and writes TLV hunks to the pipe.
With non-tty stdout, graf writes plain ASCII via `HUNKu8sFeedText`
directly.

`--merge` does not page — it produces a resolved file, not a hunk
stream. `--install` writes git config and exits.

## Git integration

```
git config diff.graf.command "graf --gitdiff"
git config merge.graf.name   "graf token merge"
git config merge.graf.driver "graf --merge %O %A %B -o %A"
```

`.git/info/attributes`:
```
* diff=graf
*.c merge=graf
*.h merge=graf
...
```

`graf --install` sets all of the above automatically.
