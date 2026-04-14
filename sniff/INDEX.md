# sniff — worktree management

Checkout, status, staging, commit.  State lives in `.dogs/sniff/`.

## Headers

| Header | Description |
|--------|-------------|
| SNIFF.h | File path registry + filesystem change log. Book-mmap'd `paths.log` and `state.log`, in-RAM hash tables for name lookup and aggregated state. Dirs stored with trailing `/`. `SNIFFSort()` builds depth-first sorted index. Entry types: `SNIFF_HASHLET` (base SHA hashlet), `SNIFF_CHECKOUT` (clean mtime), `SNIFF_CHANGED` (dirty mtime). |
| GET.h | Checkout (shared verb `get`): walk a commit tree from keeper, write files to worktree. Skips unchanged (hashlet match), protects dirty files, handles symlinks, skips submodules. Prunes stale tracked files. |
| POST.h | Tree building (shared verb `post`): walk sorted path index depth-first, create blobs for dirty files (with old hashlet for delta compression), assemble tree objects. Unchanged subtrees reused by hashlet. |
| PUT.h | Commit (shared verb `put`): resolve parent, call `POSTTree`, create commit object. Thin wrapper around POST. |
| DEL.h | Delete from tree (shared verb `delete`): build new tree excluding specified files/dirs. Unchanged subtrees reused by hashlet. |

## CLI (`sniff`)

| Command | What |
|---------|------|
| `sniff index` | Rebuild index (record `SNIFF_CHECKOUT` mtimes) |
| `sniff update` | Update mtimes (`SNIFF_CHANGED`) |
| `sniff status` | Show dirty/deleted files (M/D) |
| `sniff checkout <hex>` | Checkout commit from keeper |
| `sniff commit -m <msg> --parent <hex>` | Commit changed files |
| `sniff commit -a -m <msg> --parent <hex>` | Scan + commit all |
| `sniff commit -m <msg> --parent <hex> f1 f2` | Commit specific files |
| `sniff watch` | Start inotify daemon (fork, pidfile) |
| `sniff stop` | Stop watch daemon |
| `sniff list` | List all known paths |

### Shared verbs (dispatched by `be`)

| Verb | What |
|------|------|
| `sniff get <hex>` | Checkout (repo -> worktree) |
| `sniff post --parent <hex> [files]` | Build tree (worktree -> keeper) |
| `sniff put -m <msg> --parent <hex> [files]` | Commit |
| `sniff delete --parent <hex> f1 f2` | Remove files from tree |

Flags: `-m` message, `--parent` parent SHA, `--author` author string, `-a` commit-all.

## On-disk layout (`.dogs/sniff/`)

| File | Format |
|------|--------|
| `paths.log` | Newline-separated path strings (dirs end with `/`), append-only, Book-mmap'd |
| `state.log` | Flat append-only log of `wh64` entries (type\|id\|off) |
| `pid` | Watch daemon PID |

## Tests (`test/SNIFF.c`)

| Test | What |
|------|------|
| `SNIFFInternPath` | Intern + path round-trip, dedup |
| `SNIFFRecordGet` | Record + get round-trip, overwrite |
| `SNIFFPersist` | State survives close/reopen |
| `SNIFFCheckoutCommit` | Full checkout + modify + commit round-trip |

## Dependencies

Links: `keeplib`, `gitcompat`, `dog`, `abc-core`.  Uses `abc/FSW` for inotify in CLI.
