# GURI — Git URI addressing

GURI extends standard URI syntax for git repo navigation.
Every resource — local file, remote branch, line range, search
result — has one address.

## Grammar

```
[//authority] [path] [?ref] [#search]
```

| Part | Meaning | Examples |
|------|---------|---------|
| `//authority` | Remote host or alias | `//github.com`, `//origin` |
| `path` | Repo-relative file or dir | `abc/MSET.h`, `src/`, `.c` |
| `?ref` | Git ref: branch, tag, SHA, range | `?main`, `?HEAD~3`, `?main..feat` |
| `#search` | Line, grep, structural, regex | `#42`, `#TODO`, `#'f(x)'`, `#/\d+/` |

## Path conventions

| Path form | Meaning |
|-----------|---------|
| `abc/MSET.h` | File (working tree) |
| `abc/` | Directory |
| `.c` | Extension filter (all `.c` files) |
| `.` | Working tree root |
| `gritzko/dogs.git/abc/MSET.h` | `.git/` splits repo address from file path |

## Search types (#fragment)

| Syntax | Type | Example |
|--------|------|---------|
| `42`, `L10-L20` | Line number / range | `abc/MSET.h#42` |
| bare text | Substring grep | `.c#TODO` |
| `'quoted'` | Structural search (spot) | `.c#'f(x,y)'` |
| `/slashed/` | Regex grep | `.c#/u\d+sFeed/` |

## Examples

```
abc/MSET.h                          local working tree file
abc/MSET.h?main                     file at branch main
abc/MSET.h?HEAD~3#42               line 42, three commits back
.c#TODO                             grep all C files for TODO
.c#'ok64 o = OK;'                   structural search
?main..feat                         commit range (diff/log)
//origin/abc/MSET.h?feat            file on remote at branch
//github.com/gritzko/dogs.git/abc/MSET.h?main
```

## Ref syntax (?query)

| Form | Meaning |
|------|---------|
| `?main` | Branch |
| `?v1.0` | Tag |
| `?abc123` | SHA prefix |
| `?HEAD~3` | Relative ref |
| `?main..feat` | Two-dot range (log/diff) |
| `?main...feat` | Three-dot range (merge-base) |

## API

```c
#include "beagle/GURI.h"

guri g = {};
u8cs input = $u8str("//origin/abc/MSET.h?main#42");
GURIu8sDrain(input, &g);

g.remote   → "origin"
g.path     → "abc/MSET.h"
g.ref      → "main"
g.search   → "42"
g.ext      → "h"
g.has_remote  = YES
g.has_path    = YES
g.has_ref     = YES
g.has_search  = YES

GURISearchType(&g) → GURI_SEARCH_LINE
```

## Introspection flags

| Flag | Meaning |
|------|---------|
| `has_remote` | `//authority` present |
| `has_repo` | `.git/` boundary found |
| `has_path` | File path present |
| `has_ref` | `?ref` present |
| `has_search` | `#search` present |
| `is_ext_filter` | Path is bare `.ext` |
| `is_dir` | Path ends with `/` |
| `is_range` | Ref contains `..` |
