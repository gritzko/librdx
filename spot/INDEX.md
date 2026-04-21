# spot/ — code search, replace, grep

Spot is a producer of hunks for search results. Display lives in
[bro/](../bro/INDEX.md). Diff and merge live in
[graf/](../graf/INDEX.md). When stdout is a tty, spot forks bro as
a pager and writes TLV hunks (see [dog/HUNK.h](../dog/HUNK.h)) to
the pipe. Otherwise it writes plain ASCII directly to stdout via
`HUNKu8sFeedText`.

## Headers

| Header | Purpose |
|--------|---------|
| CAPO.h | Main API: index, search, grep |
| CAPOi.h | Internal: shared helpers, CAPOFindExt macro, HIT u64cs template |
| LESS.h | Producer staging: scratch arena, `spot_out_fd`, `spot_emit`, `LESSHunkEmit` |
| SPOT.h | Structural pattern matching: tokenize, init, next, replace |

## Source files

| File | Purpose |
|------|---------|
| CAPO.c | Index management, SPOT search, hunk-building helpers |
| GREP.c | Substring grep (CAPOGrep), regex grep (CAPOPcreGrep) |
| CAPO.cli.c | CLI entry point: arg parsing, fork bro, pick `spot_emit` |
| LESS.c | Staging arena + `LESSHunkEmit` (serialize via `spot_emit` → `spot_out_fd`) |
| SPOT.c | SPOT pattern matching engine, needle flattening, replacement |

## Key functions (CAPO.h)

| Function | Purpose |
|----------|---------|
| `CAPOSpot` | Structural search (and replace) across repo |
| `CAPOGrep` | Substring grep with syntax-highlighted context (GREP.c) |
| `CAPOPcreGrep` | Regex grep via Thompson NFA + trigram filtering (GREP.c) |
| `CAPOCompact` / `CAPOCompactAll` | Compact LSM index runs |
| `CAPOResolveDir` | Resolve `<workspace>/.dogs/spot` dir |
| `CAPOIndexFile` | Tokenize one blob, emit trigram+symbol u64 postings |
| `CAPOCommitAppend` | Append a commit SHA to `.dogs/spot/COMMIT` |

Ingestion is driven by `spot get` (→ `SPOTIngestNewCommits`), which
`be` runs after `keeper get` / `sniff get`.  It walks every ref in
`.dogs/keeper/REFS`, skips commit SHAs already in `.dogs/spot/COMMIT`,
and for each new commit walks the tree via `KEEPLsFiles` + pulls
blobs through `KEEPGetExact`, feeding `CAPOIndexFile` per blob.
`spot get` opens spot rw (set by `spot` CLI when the verb is `get`).

Historic search (`spot … ?ref`) goes through `CAPOScanRef`: resolves
the ref via keeper, walks the tree, pulls each blob-of-matching-ext,
and runs the usual grep/pcre/snippet callbacks on the blob content.
`spot --replace` is refused when `?ref` is set (no on-disk file).

Worktree search uses the trigram filter for speed: paths whose
path-hash carries no needle-trigram entry are skipped.  Any file that
shares a path with a previously-indexed blob passes the filter, so
mutated-but-tracked files are covered (the index accumulates hashes
across history).  Strictly-untracked brand-new files with novel
trigrams would be skipped — sniff-changed-file enumeration could
bypass the filter explicitly; not currently wired.

## Key functions (SPOT.h)

| Function | Purpose |
|----------|---------|
| `SPOTTokenize` | Tokenize source into packed u32 buffer via tok/ |
| `SPOTInit` | Initialize pattern matcher (tokenizes needle) |
| `SPOTNext` | Find next structural match (OK or SPOTEND) |
| `SPOTReplace` | Apply replacement to all matches in one file |

## Internal helpers (CAPO.c, static)

| Function | Purpose |
|----------|---------|
| `CAPOFindFunc` | Walk backward to find enclosing function name |
| `CAPOGrepCtx` | Compute context line range around a byte position |

(`CAPOFormatTitle` was lifted into `dog/HUNK.c` as `HUNKu8sFormatTitle`.)

## Index format

Trigram index lives in `.dogs/spot/*.idx` (next to `.git`). Each entry is a u64:
upper 18 bits = trigram (3 RON64 chars), lower 32 bits = path hash.
Files are sorted MSET runs, compacted via LSM-style merging.
