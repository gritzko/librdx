#   Dog API

 1. Each dog provides a static library and an executable.
 2. CLI convention: `dog [verb] [--flags] URI*` (see dog/CLI.h).
    Common flags (each dog picks which to support):
      - `--index -i` rebuild the index
      - `--update -u` _update_ the index
      - `--status` report status (short)
      - `--tlv -t` provide output in hunk TLV mode (see HUNK)
      - `--install -n` install hook(s) in a git repo
    dog/CLI parses flags, verbs, and URIs into a `cli` struct.
    Flags are interleaved `[flag, val]` pairs in `cli.flags`.
 3. Each dog keeps its state in `$REPO_ROOT/.dogs/name`
 4. Dogs must understand the URI syntax (see below).
    Dog's CLI is callable as `name URI`.
 5. Dogs find their home and each other using dog/HOME
 6. If `.dogs/keeper` is present, keeper has the data; if
    not, `git` has the data
 7. Last-seen-commit tracking is in `.dogs/name/COMMIT`.
 8. The static lib must have `name` control struct and
      - `ok64 DOGOpen(name* state, path8s home, b8 rw)`
      - `ok64 DOGClose(name* state)`
 9. `.dogs/DOGS` lists the dogs `beagle` invokes by default
10. `be` dispatches HTTP-like command vocabulary to
    appropriate dogs.

##  URI convention

Dogs accept URIs of the form:

    [//authority] [path] [?ref] [#fragment]

  - `//authority` — remote host or alias (`//origin`, `//github.com/user/repo.git`)
  - `path` — repo-relative file or directory (always a real path)
  - `?ref` — git ref: branch, tag, SHA, range (`?main`, `?HEAD~3`, `?main..feat`)
  - `#fragment` — location or search within a file (parsed by dog/FRAG)

Short refs like `?main` are ambiguous — resolved by trying
refs/heads, refs/tags, then SHA prefix (same as git).
Use `?refs/heads/main` or `?refs/tags/v1.0` to disambiguate.

The `.git/` marker in the path splits the repo address from the
file path: `//github.com/user/repo.git/src/foo.c?main`.

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

##  `be` verb vocabulary

    be path                  view file (bro)
    be '#search.ext'         search working tree (spot)
    be get URI               repo → worktree (keeper/sniff)
    be post URI              worktree → repo (sniff/git)
    be put URI               repo → repo (keeper/git push)
    be diff URI              compare (graf)
    be patch URI             transform in place (spot replace)
    be merge URI             3-way merge (graf)
    be delete URI            remove branch/tag/file

No verb = read-only view or search.  A verb = action with direction.
