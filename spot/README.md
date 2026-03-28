# `spot` — git repo code search, replace, diff and merge

`spot` makes a trigram index of a repo which allows for extra fast
token-aware search-and-replace in the entire repo. Also does
syntax-highlighted cat, token-level diff and 3-way merge.
git hooks keep the index updated.

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
    spot --diff old new               token-level colored diff
    spot --gitdiff                    git external diff driver
    spot --merge base ours theirs     token-level 3-way merge (stdout)
    spot --merge B O T -o out         merge to file

Flags: `-s`/`--spot` for SPOT search, `-r`/`--replace` for replacement
(requires `-s`), `-g`/`--grep` for substring search,
`-C N`/`--context=N` for grep context lines (default 3),
`-d`/`--diff` for token diff, `--gitdiff` for git diff driver,
`--merge` for 3-way merge.
Trailing args starting with `.` are extension filters (required for `-s`,
optional for `-g`). The extension selects the ragel tokenizer and the
file filter (`.c` matches `.c` and `.h`).

## Examples

Index a repo on 8 cores:

    $ spot --fork 8
    spot: forking 8 workers
    OK   c    abc/MSET.h
    OK   c    spot/CAPO.c
    ...
    spot: compacting all runs
    spot: done

### SPOT pattern search

SPOT matches structurally, not textually — whitespace and formatting
differences are ignored. Placeholders (single lowercase letters) bind
to any matching token, so `ok64 o = OK;` also matches `ok64 ret = OK;`.
Uppercase placeholders match any block of code. Multiple spaces (gaps)
match any token sequence.

Find a specific declaration pattern:

    $ spot -s "ok64 o = OK;" .c
    --- abc/BIN.h ---
    ok64 o = OK;
    --- abc/FILE.c ---
    ok64 o = OK;
    ok64 o = OK;
    ...

Rename a function:

    $ spot -s "OldFunction(X)" -r "NewFunction(X)" .c

Look for a typical `malloc` call pattern:

    $ spot -s 'malloc(a*B);' .c

Standardize `malloc()` argument order:

    $ spot -s 'malloc(sizeof(a)*B);' -r 'malloc(B*sizeof(a));' .c

### Grep

Grep does substring search across all token leaves including comments —
unlike SPOT, which skips comments and matches structurally. Results
show the matching line with a few lines of context (default 3, like
`diff -C`).

Find TODOs in comments:

    $ spot -g "TODO" .c

Search all parseable files (omit the extension):

    $ spot -g "CAPOTriChar"

Control context lines with `-C N`:

    $ spot -g "TODO" -C 0 .c      # match line only
    $ spot -g "TODO" -C 1 .c      # 1 line above and below

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
(SPOT) to find structural matches.

**Grep mode:** same trigram filtering, but walks all token leaves
including comments and does plain substring matching. Shows a
line-context window around each hit (default 3 lines, adjustable
with `-C`).

**Diff/merge:** tokenizes both files, runs LCS-based diff on the token
streams, outputs syntax-highlighted results. Merge extends this to
three-way with conflict markers.

`--fork N` stripes files across N workers. Works with git worktrees.
