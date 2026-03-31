# spot/ — code search, diff and merge

## Headers

| Header | Purpose |
|--------|---------|
| CAPO.h | Main API: index, search, grep, cat, diff, merge |
| CAPOi.h | Internal: shared helpers, CAPOFindExt macro, MSET/DIFF u64 templates |
| LESS.h | LESS pager API + arena (hunks, maps, alloc/write/defer) |
| SPOT.h | Structural pattern matching: tokenize, init, next, replace |
| NEIL.h | Diff semantic cleanup: remove false short equalities |

## Source files

| File | Purpose |
|------|---------|
| CAPO.c | Index management, SPOT search, shared display helpers |
| CAT.c | Syntax-highlighted file display (CAPOCat) |
| DIF2.c | Token-level diff (CAPODiff) and 3-way merge (CAPOMerge) |
| GREP.c | Substring grep (CAPOGrep), regex grep (CAPOPcreGrep) |
| CAPO.cli.c | CLI entry point, argument parsing |
| LESS.c | LESS pager + arena state/functions |
| SPOT.c | SPOT pattern matching engine, needle flattening, replacement |
| NEIL.c | Diff edit list cleanup (whitespace-only, short EQ removal) |

## Key functions (CAPO.h)

| Function | Purpose |
|----------|---------|
| `CAPOSpot` | Structural search (and replace) across repo |
| `CAPOGrep` | Substring grep with syntax-highlighted context (GREP.c) |
| `CAPOPcreGrep` | Regex grep via Thompson NFA + trigram filtering (GREP.c) |
| `CAPOCat` | Syntax-highlighted file display (CAT.c) |
| `CAPODiff` | Token-level diff with function headers at hunks (DIF2.c) |
| `CAPOMerge` | Token-level 3-way merge (DIF2.c) |
| `CAPOReindex` | Full reindex of all tracked files |
| `CAPOReindexProc` | Parallel reindex (worker K of N) |
| `CAPOHook` | Incremental index update (post-commit hook) |
| `CAPOCompact` | Compact LSM index runs |
| `CAPOResolveDir` | Resolve .git/spot dir (handles worktrees) |
| `CAPOIndexFile` | Index one file: extract trigrams into u64 entries |

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
| `CAPOFormatTitle` | Format `--- file :: func() ---` hunk title with truncation |
| `CAPOGrepCtx` | Compute context line range around a byte position |

## Index format

Trigram index lives in `.git/spot/*.idx`. Each entry is a u64:
upper 18 bits = trigram (3 RON64 chars), lower 32 bits = path hash.
Files are sorted MSET runs, compacted via LSM-style merging.
