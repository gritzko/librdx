---
---
# `spot` — git repo code search, replace, diff and merge

`spot` makes a trigram index of a repo which allows for extra fast
token-aware search-and-replace in the entire repo. Also does
syntax-highlighted cat, token-level diff and 3-way merge.
git hooks keep the index updated.

## Examples

Index a repo on 8 cores:

    $ spot --fork 8
    spot: forking 8 workers
    OK   c    abc/MSET.h
    OK   c    spot/CAPO.c
    ...
    spot: compacting all runs
    spot: done

Scan for function invocations:

    $ spot -s 'u8csMv(A)' .c

<table><tr>
<td><img src="../blog/img/screen.png" alt="spot search" width="100%"></td>
<td><img src="../blog/img/diff.png" alt="spot diff" width="100%"></td>
</tr></table>

## Usage

    spot                              incremental update (or reindex)
    spot file.c                       colorful cat (syntax highlight)
    spot -i | spot --index            full reindex (single process)
    spot --fork N                     parallel reindex on N cores
    spot --hook                       same as bare `spot`
    spot -s "return 0;" .c            SPOT search: find pattern in C files
    spot -s "ok64 o = OK;" .c         SPOT search: find exact declaration
    spot -s "f(x,y)" -r "f(y,x)" .c   SPOT search + replace
    spot -g "TODO" .c                 grep: substring search (incl. comments)
    spot -g "TODO" -C 0 .c            grep: match line only, no context
    spot -p "u\d+sFeed" .c            regex grep (Thompson NFA)
    spot -p "regex" -C 0 .c           regex grep, match line only
    spot --diff old new               token-level colored diff
    spot --gitdiff                    git external diff driver
    spot --merge base ours theirs     token-level 3-way merge (stdout)
    spot --merge B O T -o out         merge to file

Flags: `-s`/`--spot` for SPOT search, `-r`/`--replace` for replacement
(requires `-s`), `-g`/`--grep` for substring search,
`-p`/`--pcre` for regex search (Thompson NFA),
`-C N`/`--context=N` for grep/regex context lines (default 3),
`-d`/`--diff` for token diff, `--gitdiff` for git diff driver,
`--merge` for 3-way merge.
Trailing args starting with `.` are extension filters (required for `-s`,
optional for `-g`). The extension selects the ragel tokenizer and the
file filter (`.c` matches `.c` and `.h`).

### SPOT pattern search

SPOT matches structurally, not textually — whitespace and formatting
differences are ignored. Lowercase placeholders (`a`–`z`) bind to a
single token, so `ok64 o = OK;` also matches `ok64 ret = OK;`.
Uppercase placeholders (`A`–`Z`) match any block of code, including
nested brackets — use these for multi-token arguments in function
calls. Two spaces (a gap) skip any token sequence.

Results are syntax-highlighted with context lines and
`--- file :: function() ---` headers:

    $ spot -s "ok64 o = OK;" .c
    --- abc/BIN.h :: ok64 BINu8csFeed(Bu8 buf, u8csc data) ---
        ok64 o = OK;
        ...
    --- abc/FILE.c :: ok64 FILEMapRO(u8bp *out, path8cg path) ---
        ok64 o = OK;
        ...

Rename a macro call to a function (uppercase placeholders match
multi-token arguments including nested brackets):

    $ spot -s "TOK_VAL(A,B,C,D)" -r "tok32Val(A,B,C,D)" .c .h

Rename a single-token function (lowercase matches one token):

    $ spot -s "OldFunc(x)" -r "NewFunc(x)" .c

Look for a typical `malloc` call pattern:

    $ spot -s 'malloc(a*B);' .c

Standardize `malloc()` argument order:

    $ spot -s 'malloc(sizeof(a)*B);' -r 'malloc(B*sizeof(a));' .c

### Grep

Grep does substring search across all token leaves including comments —
unlike SPOT, which skips comments and matches structurally. Results
are syntax-highlighted with context (default 3 lines) and
`--- file :: function() ---` headers.

Find TODOs in comments:

    $ spot -g "TODO" .c

Search all parseable files (omit the extension):

    $ spot -g "CAPOTriChar"

Control context lines with `-C N`:

    $ spot -g "TODO" -C 0 .c      # match line only
    $ spot -g "TODO" -C 1 .c      # 1 line above and below

### Regex grep

Regex grep (`-p`) uses a Thompson NFA engine (`abc/NFA.h`) for matching.
Supports `.` `*` `+` `?` `|` `()` `[]` `\d` `\w` `\s` `{n,m}`.
Literal substrings are extracted from the regex for trigram index
filtering (Russ Cox approach), so index speedup works when the pattern
contains literal runs of 3+ characters.

Find functions matching a naming pattern:

    $ spot -p 'u\d+sFeed' .c

Search for alternative names:

    $ spot -p 'tok32(Val|Tag|Pack)' .c

Regex grep with context control:

    $ spot -p 'CAPO\w+Grep' -C 1 .c

### Diff and merge

Token-level diff highlights changes at the token granularity, not lines:

    $ spot --diff old.c new.c

Three-way merge works as a git merge driver:

    $ spot --merge base.c ours.c theirs.c -o merged.c

## Git integration

`.gitattributes` (project root):

    *.c  diff=spot merge=spot
    *.h  diff=spot merge=spot

`.git/config` (or `git config`):

    git config diff.spot.command "spot --gitdiff"
    git config merge.spot.name "spot token merge"
    git config merge.spot.driver "spot --merge %O %A %B -o %A"

Post-commit hook (incremental reindex):

    echo '#!/bin/sh
    spot --hook' > .git/hooks/post-commit
    chmod +x .git/hooks/post-commit

## How it works

Source files are tokenized with ragel-generated tokenizers (`tok/`),
trigrams extracted from token text and packed with path hashes into u64
entries stored as sorted MSET runs in `.git/spot/*.idx`.

**SPOT mode:** extracts trigrams from the needle text, seeks each in the
MSET index, intersects path hash sets to narrow candidates, then
tokenizes each candidate file and runs flat token pattern matching
(SPOT) to find structural matches. Shows syntax-highlighted context
with function headers.

**Grep mode:** same trigram filtering, but walks all token leaves
including comments and does plain substring matching. Shows
syntax-highlighted context around each hit (default 3 lines,
adjustable with `-C`) with function headers.

**Diff/merge:** tokenizes both files, runs LCS-based diff on the token
streams, outputs syntax-highlighted results with function headers at
hunk boundaries. Merge extends this to three-way with conflict markers.

`--fork N` stripes files across N workers. Works with git worktrees.

##  Credits

The idea of repo trigram indexing is borrowed from [Russ Cox][c].
The initial version of the tool used [tree-sitter][t] grammars, 
later changed to [ragel-based][r] lexers for performance reasons.

[c]: https://swtch.com/~rsc/regexp/regexp4.html
[r]: https://www.colm.net/open-source/ragel/
[t]: https://tree-sitter.github.io/tree-sitter/
