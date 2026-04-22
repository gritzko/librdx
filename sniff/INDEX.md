# sniff — worktree management

Checkout, status, staging, commit.  Per-wt sniff state lives at
`<wt>/.sniff/` (sibling of `.dogs/`).  The store itself is always
`<repo>/.dogs/`; secondary worktrees reach it through a `.dogs`
symlink → primary's `.dogs/`.  Separating `.sniff/` out of the store
keeps it per-wt and lets the entire store symlink through cleanly.

Phase 2b (target) will replace `.sniff/at.log` with the pointer-pair
`<wt>/.dogs` FILE + `<store>/<branch>/WT` back-pointer described in
`sniff/AT.md`; staging will move into the branch dir alongside
canonical packs.  Phase 2a is what's live today.

## Headers

| Header | Description |
|--------|-------------|
| SNIFF.h | File path registry + filesystem change log. Heap-buffered `watch.sniff` paths/state logs, in-RAM hash tables for name lookup and aggregated state. Dirs stored with trailing `/`; root dir `/` reserved at open. `SNIFFSort()` builds depth-first sorted index. Entry types: `SNIFF_BLOB` (file blob hashlet), `SNIFF_TREE` (dir tree hashlet — root's slot is the **base tree** pointer), `SNIFF_CHECKOUT` (clean mtime), `SNIFF_CHANGED` (dirty mtime). Lives in the branch dir (one wt per branch). |
| AT.h | Worktree ↔ branch pointer pair: `<wt>/.dogs` (URI → store + branch) and `<branch-dir>/WT` (abspath of the wt). Replaces the old `at.log`. See `sniff/AT.md`. |
| GET.h | Checkout (shared verb `get`): walk a commit tree from keeper, write files to worktree. Skips unchanged (hashlet match + file exists), protects dirty files, handles symlinks, skips submodules. Prunes stale tracked files. Rewrites `<wt>/.dogs` and `<branch-dir>/WT`, sets root `SNIFF_TREE` (base). |
| PUT.h | Stage (shared verb `put`): walk sorted path index depth-first, create blobs for dirty files (with old hashlet for delta compression), assemble tree objects into the branch's `stage.sniff`. Writes back `SNIFF_BLOB` for touched files and `SNIFF_TREE` for every rebuilt subtree, including root — the new base. NULL file_set = stage everything dirty. No commit. |
| POST.h | Commit (shared verb `post`): wrap the current base tree into a commit with parent = branch `REFS` tail sha. If nothing has been staged (base == parent's tree), auto-runs `PUTStage(NULL)` first. Promotes the repacked canonical pack into `<branch-dir>/NNNNN.keeper` and appends the new sha to `<branch-dir>/REFS`. |
| DEL.h | Delete-stage (shared verb `delete`): build new base tree excluding specified paths. Unchanged subtrees reused by hashlet. NULL del_set = stage every tracked file missing from disk. |

## Workflow

`get` seeds the base tree and rewrites the `.dogs` + `WT` pointer
pair.  `put` / `delete` update the base in the branch's `stage.sniff`
(no commit, no REFS entry).  `post` wraps the current base into a
commit with parent = branch `REFS` tail sha, promotes the staged
objects into the branch's canonical pack, and appends the new sha
to the branch `REFS`.  The base lives in sniff's `SNIFF_TREE` record
at the root-dir index — no separate pointer file.

## CLI (`sniff`)

| Command | What |
|---------|------|
| `sniff index` | Rebuild index (record `SNIFF_CHECKOUT` mtimes) |
| `sniff update` | Update mtimes (`SNIFF_CHANGED`) |
| `sniff status` | Show dirty/deleted files (M/D) |
| `sniff get <hex>` | Checkout commit from keeper (alias: `checkout`) |
| `sniff put [files]` | Stage files into a new base tree (no commit) |
| `sniff delete [files]` | Stage deletion into a new base tree |
| `sniff post -m <msg>` | Commit the current base (alias: `commit`) |
| `sniff watch` | Start inotify daemon (fork, pidfile) |
| `sniff stop` | Stop watch daemon |
| `sniff list` | List all known paths |

Flags: `-m` message, `--author` author string.

### Empty-set shortcuts
- `sniff put` (no args) stages every file with a changed mtime.
- `sniff delete` (no args) stages every tracked file missing from disk.
- `sniff post` auto-stages dirty when no explicit `put`/`delete` ran.

## On-disk layout

Per worktree (`<wt>/`):

| File | Format |
|------|--------|
| `.dogs` | One-line URI: `/abs/store?heads/<branch>` (or `?<sha>` for detached). See `AT.md`. |
| `pid` | Watch daemon PID (if `sniff watch` is running) |

Per branch (`<store>/<branch-dir>/`, sharing the dir with keeper's
canonical packs):

| File | Format |
|------|--------|
| `WT` | Abspath of the wt on this branch (absent = none). |
| `watch.sniff` paths | Newline-separated path strings (dirs end with `/`), append-only |
| `watch.sniff` state | Flat append-only log of `wh64` entries (type\|id\|off) |
| `stage.sniff` | Staging pack (tree + blob), stripped-framed. See `STAGE.md`. |
| `stage.idx` | Staging wh128 bookmarks + per-object entries |
| `stage.lock` | flock for single-writer staging |

## Tests

C tests (`test/SNIFF.c`):

| Test | What |
|------|------|
| `SNIFFInternPath` | Intern + path round-trip, dedup (root `/` reserved at idx 0) |
| `SNIFFRecordGet` | Record + get round-trip, overwrite |
| `SNIFFPersist` | State survives close/reopen |
| `SNIFFCheckoutCommit` | Full checkout + modify + commit via `POSTCommit` |
| `SNIFFRoundTrip` | Full get + modify + post + delete + get, HEAD tracking |

Script test (`test/workflow.sh`, wired as `SNIFFworkflow`):
drives the `sniff` CLI against a toy worktree through the six
standalone-workflow scenarios — initial `post`, `get`, accumulating
`put a` + `put b` + `post`, bare `put`, named `delete foo`, bare
`delete`.

## Dependencies

Links: `keeplib`, `gitcompat`, `dog`, `abc-core`.  Uses `abc/FSW` for inotify in CLI.
