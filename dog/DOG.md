#   Dog API

 1. Each dog provides a static library and an executable.
 2. CLI convention: `dog [verb] [--flags] URI*` (see dog/CLI.h).
 3. Each dog keeps its state in `$REPO_ROOT/.dogs/name`
 4. Dogs must understand the URI syntax (see below).
    Dog's CLI is callable as `name URI`.
 5. Dogs find their home and each other using dog/HOME
 6. If `.dogs/keeper` is present, keeper has the data; if
    not, `git` has the data
 7. Last-seen-commit tracking is in `.dogs/name/COMMIT`.
 8. The static lib has a `name` control struct and four uniform
    entry points:
      - `ok64 DOGOpen(u8cs branch, b8 rw)`
           open the dog's shards for `branch` in the process-wide
           home singleton.  May be called repeatedly with different
           branches to compose a multi-branch view; the FIRST call
           (across *all* dogs, process-wide) decides whether the
           session is writable.  Later calls are downgraded to ro
           regardless of `rw` — see "Branch sharding" below.
      - `ok64 DOGExec(name* state, cli* c)`
           execute a parsed CLI (verb + flags + URIs) against
           the open state; equivalent to invoking `name ...` as
           a process
      - `ok64 DOGUpdate(name* state, u8 obj_type, u8cs blob, u8csc path)`
           feed a single git object (type + raw blob) into the
           dog's index; payload-ingestion path used by fetch and
           checkout.  Updates always land in `HOMEWriteBranch()` —
           the branch that won slot 0 at the first rw open.
      - `ok64 DOGClose(name* state)`
           close every shard opened by this dog.
 9. `be` links every dog's static lib directly — no subprocess
    fork/exec. It parses one CLI, opens the dogs it needs, calls
    `NAMEExec` on each in order, and closes them.
10. `be` dispatches the HTTP-like verb vocabulary below to other dogs.

##  URI convention

Dogs accept URIs of the form:

    [scheme:] [//authority] [path] [?ref] [#fragment]

  - `//authority` — remote host or alias (`//origin`, `//host/path/repo`)
  - `path` — repo-relative file or directory (always a real path)
  - `?ref` — branch, tag, SHA, range (`?main`, `?HEAD~3`, `?main..feat`)
  - `#fragment` — location or search within a file (parsed by dog/FRAG)

Short refs like `?main` are ambiguous — resolved by trying
`heads/`, then `tags/`, then SHA prefix.  Use `?heads/main` or
`?tags/v1.0` to disambiguate.

##  Query mini-language (dog/QURY)

The `?query` slot names a ref, a sha, or a set/range of either.
Grammar is a strict subset of `gitrevisions(7)`.

    query   = spec ('&' spec)*            -- set (e.g. merge parents)
            | spec '..' spec              -- range (for diffs)
    spec    = path ancestry?              -- path-like ref or sha
    path    = seg ('/' seg)*              -- branch/tag path or hex sha
    ancestry = ('~' | '^') digit*         -- first/Nth parent walk

### Canonical form

Every query that enters a ULOG row (keeper REFS, sniff attribution
log) is canonicalised via `dog/DOG:DOGCanonURI` before it is
written.  The canonicaliser is an input filter, not a defensive
check in writers; writers that construct queries internally are
expected to produce canonical form directly.

Rules applied to the query:

  - Strip leading `refs/`: `refs/heads/X` → `heads/X`, `refs/tags/X` → `tags/X`.
  - Collapse the trunk aliases to the empty query (the trunk):
    `heads/master`, `heads/main`, `heads/trunk` — and bare
    `master`, `main`, `trunk` — all become `?` (present-but-empty).
  - Leave everything else untouched: `heads/feat`, `tags/v1.0`,
    40-hex SHAs, ranges, sets.

Rules applied to the fragment (the "value" slot in K/V usage):

  - Strip a single leading `?` — values carried as `?<sha>` are
    equivalent to bare `<sha>`.
  - The fragment is either a bare 40-hex SHA or empty.

### Trunk

The default branch is called **trunk** and its canonical query is
the empty string.  We do not use `HEAD` — that is a git term.  A
trunk-move row is written as `?#<sha>` (present-but-empty query,
non-empty fragment); a deletion row for some named branch is
`?feature/fix1#` (present-but-empty fragment).

For a remote git repo that uses `main` or `master`, keeper observes
peer advertisements under `<peer>?heads/main#<sha>`; canonicalisation
collapses that to `<peer>?#<sha>`.

### What the mini-language does *not* cover

Deliberately excluded from the subset:

  - `@{upstream}` / `@{push}` / `@{N}` — reflog & remote-tracking
    markers are not part of the dog model.
  - `^{tree}` / `^{commit}` / `^{tag}` — peel operators; if you want
    a tree, follow the commit yourself.
  - `:/text` — commit-message search belongs in dog/FRAG, not QURY.

##  Fragment syntax (dog/FRAG)

The `#fragment` is a mini-language with first-char dispatch:

  - `#symbol` — identifier (function name or grep text)
  - `#symbol:42` — identifier + line number
  - `#symbol:10-20` — identifier + line range
  - `#42` — line number
  - `#10-20` — line range
  - `#'snippet'` — structural search
  - `#/regex/` — pcre search

Trailing `.ext` filters by file type (one or more):

  - `#TODO.c` — grep "TODO" in .c files
  - `#FILEFeedAll.c.cpp` — grep in .c and .cpp
  - `#'ok64 o'.c` — structural search in .c
  - `#/u8sFeed/.c.h` — regex in .c and .h

##  HTTP verb vocabulary

The verb vocabulary shared by all dogs is more or less HTTP-like:

    be get URI               repo → worktree retrieval
    be post URI              worktree → repo filing
    be delete URI            remove branch/tag/file
    be put URI               repo → repo, intra-repo ops
    be patch URI             transform in place (spot replace)

No verb = read-only view or search.  A verb = action with direction.

    be /path                 view file
    be '#search.ext'         search working tree (spot)
