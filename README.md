# git dogs

**git dogs** is an experiment in refactoring git to make it suitable
for modern workflows. The data model and the syncing protocol are
left as-is to stay compatible with the existing mass of git repos.
The rest is reworked. The system is made syntax-aware, diffing and
merging is finer grained and draws heavily from CRDT ideas. Content
is indexed for efficient search.

The project dogfoods from day 1.

## The dogs

The repo is structured into *dogs*. Each dog has its purview, the
data and functions it is responsible for. Dogs coordinate to carry
out complex tasks.

  * **Bro**: interactive syntax-highlighted pager/viewer. 
  * **Spot**: structural code search, grep, regex, and replace 
    across a repo. Maintains a trigram index for instant lookups.
  * **Graf**: does token-level diffing, 3-way merges, history
    navigation. Maintains a history index. 
  * **Sniff**: serves the worktree, detects changes.
  * **Keeper**: keeps the data per se (git blobs, trees, commits).

New dogs may join, old dogs may learn new tricks.
If it works, it gets used. If it's used, it evolves.

## Quick start

Build (requires libsodium, lz4, zlib and cmake, also ninja is recommended):

    mkdir build && cd build
    CC=clang CXX=clang++ cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
    ninja

Index a repo (parallel on 8 cores):

    spot --fork 8

Search for a code pattern:

    spot -s 'ok64 o = OK;' .c

Rename a function call across all C files:

    spot -s 'OldFunc(x)' -r 'NewFunc(x)' .c

Grep with trigram-accelerated index:

    spot -g "TODO" .c

Regex grep:

    spot -p 'u\d+sFeed' .c

View a file with syntax highlighting:

    bro file.c

Token-level diff:

    graf --diff old.c new.c

Install as git diff/merge driver:

    graf --install

## Search patterns

**spot** matches structurally, not textually.  Whitespace and
formatting differences are ignored.

| Syntax | Meaning |
|--------|---------|
| `a`..`z` | Lowercase: match one token |
| `A`..`Z` | Uppercase: match any block of tokens (incl. nested brackets) |
| `(two spaces)` | Skip any token sequence (gap) |
| literals | Must match exactly |

Examples:

    spot -s 'T n = {a[0],a[1]};' .c        # find slice init patterns
    spot -s 'malloc(sizeof(a)*B);' .c       # find malloc calls
    spot -s 'TOK_VAL(A,B)' -r 'tok32Val(A,B)' .h  # rename macro

##  Is this VC funded?

Nope. The project is ran from a 10 yrs old ThinkCentre discarded by
a university using a very wide LG screen, also discarded. Heavy
things, including massive fuzzing, all run on a 32 core discounted
Hetzner server whose IP is banned by half the Internet. Coding is
mostly done by Claude Max 5x, in 2..5 parallel sessions.

##  Credits

Trigram indexing idea from [Russ Cox][c]. Tokenizers started with
[tree-sitter][t], later rewritten as [ragel][r] scanners for speed.
The Merkle scheme is by Linus Torvalds AFAIK.

[c]: https://swtch.com/~rsc/regexp/regexp4.html
[r]: https://www.colm.net/open-source/ragel/
[t]: https://tree-sitter.github.io/tree-sitter/
