# GURI — Git URI addressing

GURI maps git resources onto standard URIs.  The `guri` type IS `uri`
from abc/URI.h — no wrapper, no extra fields.  Git-specific meaning
is extracted by introspection functions.

## Grammar

```
[//authority] [path] [?ref] [#search[.ext]]
```

| Part | URI field | Git meaning |
|------|-----------|-------------|
| `//authority` | authority | Remote host or alias |
| path | path | Repo-relative file or dir (always a real path) |
| `?ref` | query | Branch, tag, SHA, range |
| `#search` | fragment | Line, grep, spot pattern, regex; trailing `.ext` filters by file type |

The `.git/` marker in the path splits the repo address from the
file path: `//github.com/user/repo.git/src/foo.c?main`.

The path always names a filesystem object (repo, directory, file).
Extension filters (`.c`, `.h`) belong in the fragment, not the path.

## Examples

```
abc/MSET.h                          local file
abc/MSET.h?main                     file at ref
abc/MSET.h?main#42                  line 42 at ref
#TODO.c                             grep "TODO" in .c files
#'ok64 o = OK'.c                    structural search in .c
#/u8s.*Feed/.h                      regex search in .h
#BROHunkNext                        grep all files
?main..feat                         commit range
//origin/abc/MSET.h?feat            remote file at ref
//github.com/gritzko/dogs.git/abc/MSET.h?main
```

## Introspection API

`guri` is `uri`.  Parse with `GURIu8sDrain`, then query:

```c
guri g = {};
u8cs input = ...; // "//origin/abc/MSET.h?main#42"
GURIu8sDrain(input, &g);

u8cs remote = {}, path = {}, ref = {}, ext = {};
GURIRemote(remote, &g);   // -> "origin"
GURIPath(path, &g);        // -> "abc/MSET.h"
GURIRef(ref, &g);          // -> "main"  (= g.query)
GURISearch(search, &g);    // -> "42"    (= g.fragment)
GURISearchExt(ext, &g);    // -> ""      (no ext filter)

GURIHasRemote(&g)   // YES
GURIHasPath(&g)      // YES
GURIIsRange(&g)      // NO
GURISearchType(&g)   // GURI_SEARCH_LINE
```

| Function | Returns |
|----------|---------|
| `GURIRemote` | Authority without `//` |
| `GURIRepo` | Path portion before `.git/` |
| `GURIPath` | File path after `.git/` (or whole path) |
| `GURIRef` | Query = git ref |
| `GURISearch` | Fragment search pattern (without trailing `.ext`) |
| `GURISearchExt` | Extension filter from fragment (e.g. `c` from `#TODO.c`) |
| `GURIExt` | File extension from path (abc/PATH) |
| `GURIHasRemote` | `//authority` present? |
| `GURIHasRepo` | `.git/` boundary found? |
| `GURIHasPath` | File path present? |
| `GURIIsDir` | Trailing `/`? |
| `GURIIsRange` | Ref has `..` (not `...`)? |
| `GURISearchType` | LINE / GREP / SPOT / REGEX |

## Search types (fragment syntax)

| Fragment | Type | Constant | Example |
|----------|------|----------|---------|
| `42`, `L10-L20` | Line | `GURI_SEARCH_LINE` | `#42` |
| bare text | Grep | `GURI_SEARCH_GREP` | `#TODO.c` |
| `'quoted'` | Spot | `GURI_SEARCH_SPOT` | `#'ok64 o'.c` |
| `/slashed/` | Regex | `GURI_SEARCH_REGEX` | `#/Feed/.h` |

A trailing `.ext` in the fragment filters results by file extension.
When omitted, all file types are searched. The `.ext` is not part of
the search pattern itself.
