# sniff — worktree management

Checkout, status, staging, commit.  State lives in `.dogs/sniff/`.

## Headers

| Header | Description |
|--------|-------------|
| SNIFF.h | File path registry + filesystem change log. Heap-buffered `paths.log` and `state.log`, in-RAM hash tables for name lookup and aggregated state. Dirs stored with trailing `/`; root dir `/` reserved at open. `SNIFFSort()` builds depth-first sorted index. Entry types: `SNIFF_BLOB` (file blob hashlet), `SNIFF_TREE` (dir tree hashlet — root's slot is the **base tree** pointer), `SNIFF_CHECKOUT` (clean mtime), `SNIFF_CHANGED` (dirty mtime). |
| AT.h | Per-worktree branch + commit pointer. `.dogs/sniff/at.log` append-only, line format `<ron60-time>\t?<branch>\t?<sha>`. Tail is authoritative; keeper/beagle read it via `dog/AT.h`. See `sniff/AT.md`. |
| GET.h | Checkout (shared verb `get`): walk a commit tree from keeper, write files to worktree. Skips unchanged (hashlet match + file exists), protects dirty files, handles symlinks, skips submodules. Prunes stale tracked files. Appends an `at.log` entry and sets root `SNIFF_TREE` (base). |
| PUT.h | Stage (shared verb `put`): walk sorted path index depth-first, create blobs for dirty files (with old hashlet for delta compression), assemble tree objects. Writes back `SNIFF_BLOB` for touched files and `SNIFF_TREE` for every rebuilt subtree, including root — the new base. NULL file_set = stage everything dirty. No commit. |
| POST.h | Commit (shared verb `post`): wrap the current base tree (root `SNIFF_TREE`) into a commit with parent = `at.log` tail sha. If nothing has been staged (base == parent's tree), auto-runs `PUTStage(NULL)` first. Appends `at.log` with the new sha. |
| DEL.h | Delete-stage (shared verb `delete`): build new base tree excluding specified paths. Unchanged subtrees reused by hashlet. NULL del_set = stage every tracked file missing from disk. |

## Workflow

`get` seeds the base tree and appends to `at.log`.  `put` / `delete`
update the base (no commit, no `at.log` entry).  `post` wraps the
current base into a commit with parent = `at.log` tail sha and appends
a new `at.log` entry.  The base lives in sniff's `SNIFF_TREE` record at
the root-dir index — no separate pointer file.

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

## On-disk layout (`.dogs/sniff/`)

| File | Format |
|------|--------|
| `paths.log` | Newline-separated path strings (dirs end with `/`), append-only |
| `state.log` | Flat append-only log of `wh64` entries (type\|id\|off) |
| `HEAD` | Current commit: ref name (`refs/heads/main`) or 40-char hex SHA |
| `pid` | Watch daemon PID |

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
