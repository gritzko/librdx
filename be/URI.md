#   URI reference for Beagle SCM

All keys are URIs (`abc/URI.h`). All values are BASON.

##  Canonical URI form

    scheme://repo/project/path/file.ext?version-formula#fragment

| Part | Role | Example |
|------|------|---------|
| `scheme` | data plane | `be`, `stat`, `tri`, `sym` |
| `authority` | repo (FQDN) | `repo.team.org` |
| `path` | project + file path | `/proj/src/main.c` |
| `?query` | branch/version formula | `?main`, `?stamp-main&feat` |
| `#fragment` | in-file selection | `#commit`, `#substring` |

A milestone is essentially a repo. A branch is a chain of
waypoints (commits). An overlay is a branch that is kept
separate, never merged back.

##  Schemes (data planes)

| Scheme | Content | Example |
|--------|---------|---------|
| `be:`   | BASON AST tree or patch  | `be:proj/src/main.c` |
| `stat:` | metadata (mtime, mode)   | `stat:proj/src/main.c` |
| `tri:`  | trigram posting list     | `tri:proj?Ab` |
| `sym:`  | symbol name posting list | `sym:proj?funcName` |
| `std:`  | stdout redirect (CLI)    | `std:///file.c` |

A `stat:` prefix scan gives fast file listing without
touching BASON content. Index schemes (`tri:`, `sym:`) store
posting lists as BASON objects; merge = set-union.

##  RocksDB keys

DB keys use the scheme-only form (no authority):

    scheme:project/dir/file.c                      base
    scheme:project/dir/file.c?pad10stamp-branch    waypoint
    be:project/?pad10stamp-branch#commit           metadata

Built by `BEKeyBuild(into, scheme, path, query, fragment)`
which calls `URIMake(into, scheme, 0, path, query, frag)`.
Authority is always empty in DB keys.

##  `.be` file URI

    be://repo.fqdn/project?branch1&branch2

| Part | Meaning |
|------|---------|
| `be://` | scheme |
| `repo.fqdn` | authority: repo FQDN |
| `/project` | path: project within repo |
| `?main&feat` | query: active branch names joined by `&` |

First branch in the list is active (write target).
Updated on `BEPost`, `BEGet`, `BESetActive`.

##  Query: version formula

###  In DB keys (waypoint identifier)

    pad10_timestamp-branch

10-byte RON60-padded timestamp + `-` + RON60 branch name.
Built by `BEQueryBuild(into, stamp, branch)`.

No query = base version (repo/milestone snapshot).

###  In CLI and HTTP (version selector)

Entries separated by `&`. Each entry:

    [timestamp][-|+|=]origin

| Op | Meaning | Const |
|----|---------|-------|
| (none) | all waypoints on branch | VER_ANY |
| `-` | time <= stamp | VER_LE |
| `+` | time > stamp | VER_GT |
| `=` | time == stamp | VER_EQ |

Examples: `main`, `main&feat`, `stamp-main`,
`stamp+main&stamp=feat`.

Parsed by `VERFormParse`. A base entry (time=0, origin=0)
is auto-appended when any entry is ANY or LE.

##  Fragment

###  Current uses

| Fragment | Key pattern | Content |
|----------|-------------|---------|
| `#commit` | `be:proj/?stamp-branch#commit` | commit message |
| `#merge` | `be:proj/?stamp-branch#merge` | merge metadata |
| `#milestone` | `be:proj/?stamp-branch#milestone` | milestone metadata |
| (grep text) | grep URI fragment | search substring |

Fragment keys are skipped during content scans and streaming.

###  Grep

    be grep foo          # bare word -> treated as fragment
    be grep #foo         # explicit fragment
    be grep ?branch#foo  # branch-filtered search

`BEGrep` reads `grep_uri->fragment` as the search substring.
Trigram-accelerated via `tri:` posting lists.

###  CSS-like selector syntax (`ast/CSS`)

The fragment carries a CSS-like selector for in-file
node selection. Implementation in `ast/CSS.h`, `ast/CSS.c`,
`ast/CSS.c.rl` (ragel lexer).

**Pseudo-elements** (map to BASON type letters):

| Selector | BASON | Meaning |
|----------|-------|---------|
| `fn`     | E | function/method |
| `class`  | I | struct/enum/trait/class |
| `args`   | U | argument/parameter list |
| `obj`    | O | object |
| `block`  | A | generic container |
| `def`    | F | definition name |
| `cmt`    | D | comment |
| `str`    | G | string literal |
| `num`    | L | number literal |
| `type`   | T | type name |
| `kw`     | R | keyword |
| `pp`     | H | preprocessor |
| `punct`  | P | punctuation |

**Name match** (`.` = name, like CSS class):

    fn.main              function named "main"
    fn.BE*               functions starting with "BE"
    class.ROCKdb         struct named ROCKdb

**Combinators** (standard CSS):

    fn args              descendant (args anywhere inside fn)
    fn > args            child (direct)
    cmt + fn             adjacent sibling (fn right after cmt)
    cmt ~ fn             general sibling (fn after cmt)

**`:has()` / `:not()` — containment** (structural for
containers, textual for leaves — no nesting):

    fn:has(malloc)                  function using malloc
    cmt:has(TODO)                   TODO comments
    :not(fn)                        anything except functions

**Line ranges** (GitHub convention):

    L10                  line 10
    L10-20               lines 10..20

**Examples** in URIs:

    be cat file.c#fn.main
    be cat file.c#fn:has(malloc)
    be diff file.c#fn.main
    be grep file.c#cmt:has(TODO)

**Implementation**: `CSS.lex` → `CSS.c.rl` (via `lex`) →
`CSS.rl.c` (via ragel). Flat regular grammar, no nesting.
`CSSParse` builds a BASON query tree; `CSSMatch` walks the
data tree with a line bitset (same pattern as `BASTGrepNodes`).

##  CLI dispatch

`be verb [uri]` — arg parsed by `URIutf8Drain`, falls
back to plain path.

| Usage | Effect |
|-------|--------|
| `be post` | post all tracked files |
| `be post file.c` | post one file |
| `be post //repo/proj` | init new project |
| `be post //repo` | checkpoint (fork) |
| `be get` | export all files |
| `be get file.c` | export one file |
| `be get ?branch` | switch + export |
| `be get //repo/proj` | local checkout |
| `be get http://h/p` | clone from remote |
| `be get std:///f.c` | export to stdout |
| `be put ?branch` | merge branch into active |
| `be come branch` | switch active branch |
| `be delete file.c` | tombstone file |
| `be delete ?branch` | delete branch waypoints |
| `be grep text` | substring search |
| `be diff [file.c]` | colored diff |
| `be cat file.c` | syntax-highlighted view |
| `be mark name` | create milestone |
| `be fit branch` | merge into main |

##  HTTP server (be-srv)

    GET /path[.bason|.stat|/][?formula]
    POST /path[?branch]

| Suffix | Mode | Response |
|--------|------|----------|
| `.bason` | BASON | TLKV stream (be: keys) |
| `.stat` | STATE | TLKV stream (stat: keys) |
| `/` | DIR | text file listing |
| (none) | RAW | merged source text |

Sync: `/_files` returns SST listing for clone.

##  Remote URIs

    http://host:port[/project]

`BESyncClone` fetches `/_files`, downloads SSTs to
`$HOME/.be/<host.port>/`. Local shorthand: `//repo/project`.
