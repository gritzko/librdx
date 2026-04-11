# GURI — Git URI addressing

GURI maps git resources onto standard URIs.  The `guri` type IS `uri`
from abc/URI.h — no wrapper, no extra fields.  Git-specific meaning
is extracted by introspection functions.

## Grammar

```
[//authority] [path] [?ref] [#search]
```

| Part | URI field | Git meaning |
|------|-----------|-------------|
| `//authority` | authority | Remote host or alias |
| path | path | Repo-relative file, dir, or `.ext` filter |
| `?ref` | query | Branch, tag, SHA, range |
| `#search` | fragment | Line, grep, spot pattern, regex |

The `.git/` marker in the path splits the repo address from the
file path: `//github.com/user/repo.git/src/foo.c?main`.

## Examples

```
abc/MSET.h                          local file
abc/MSET.h?main                     file at ref
abc/MSET.h?main#42                  line 42 at ref
.c#TODO                             grep all C files
.c#'ok64 o = OK;'                   structural search
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
GURIRemote(remote, &g);   // → "origin"
GURIPath(path, &g);        // → "abc/MSET.h"
GURIRef(ref, &g);          // → "main"  (= g.query)
GURISearch(search, &g);    // → "42"    (= g.fragment)
GURIExt(ext, &g);          // → "h"

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
| `GURISearch` | Fragment = search spec |
| `GURIExt` | File extension (abc/PATH) |
| `GURIHasRemote` | `//authority` present? |
| `GURIHasRepo` | `.git/` boundary found? |
| `GURIHasPath` | File path present? |
| `GURIIsExtFilter` | Bare `.ext` like `.c`? |
| `GURIIsDir` | Trailing `/`? |
| `GURIIsRange` | Ref has `..` (not `...`)? |
| `GURISearchType` | LINE / GREP / SPOT / REGEX |

## Search types

| Fragment syntax | Type | Constant |
|----------------|------|----------|
| `42`, `L10-L20` | Line | `GURI_SEARCH_LINE` |
| bare text | Grep | `GURI_SEARCH_GREP` |
| `'quoted'` | Spot | `GURI_SEARCH_SPOT` |
| `/slashed/` | Regex | `GURI_SEARCH_REGEX` |
