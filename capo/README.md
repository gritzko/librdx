# capo — git repo code search (AST query + pattern match)

capo indexes source files in a git repo by trigrams, stores the index as
an LSM stack of MSET files in `.git/capo/`, and supports two search modes:
CSS-selector queries and SPOT structural pattern matching.

## Usage

    capo                              full reindex (single process)
    capo --fork N                     parallel reindex on N cores
    capo --hook                       incremental reindex (post-commit)
    capo -c "fn.BASTParse"            CSS query: find function by name
    capo -c "fn:has(malloc)"          CSS query: functions containing "malloc"
    capo -c "fn:has(malloc)" .c       CSS query: filter to C files only
    capo -s "return 0;" .c            SPOT search: find pattern in C files
    capo -s "ok64 o = OK;" .c         SPOT search: find exact declaration

Flags: `-c`/`--css` for CSS queries, `-s`/`--spot` for SPOT search,
`-r`/`--replace` for replacement (future, requires `-s`).
Trailing args starting with `.` are extension filters.
The extension determines both the parser and the file filter (`.c` matches
`.c` and `.h` since both use tree-sitter-c).

## Examples

Index a repo on 4 cores:

    $ capo --fork 4
    capo: forking 4 workers
    OK   c    abc/MSET.h
    OK   c    ast/BAST.c
    ...
    capo: compacting all runs
    capo: done

### CSS queries

Find a function by name:

    $ capo -c "fn.CAPOTriCB"
    --- capo/CAPO.c ---
    static ok64 CAPOTriCB(voidp arg, u8cs tri) {
        CAPOTriCtx *ctx = (CAPOTriCtx *)arg;
        if (*ctx->idle >= ctx->end) return CAPONOROOM;
        u64 entry = CAPOTriPack(tri) | (u64)ctx->path_hash;
        **ctx->idle = entry;
        (*ctx->idle)++;
        return OK;
    }

Find TODOs:

    $ capo -c "cmt:has(TODO)"
    --- js/io.cpp ---
    // TODO io.utf8()
    --- rdx/RDX.h ---
    // TODO: migrate RB.c to new self-contained format

Find all functions using a function:

    $ capo -c "fn:has(u8sFeed)"

### SPOT pattern search

Find return statements:

    $ capo -s "return 0;" .c
    --- abc/01.h ---
    return 0;
    return 0;
    --- abc/CURL.c ---
    return 0;
    ...

Find a specific declaration pattern:

    $ capo -s "ok64 o = OK;" .c
    --- abc/BIN.h ---
    ok64 o = OK;
    --- abc/FILE.c ---
    ok64 o = OK;
    ok64 o = OK;
    ...

SPOT matches structurally, not textually — whitespace and formatting
differences are ignored. Placeholders (single lowercase letters) bind
to any matching subtree, so `ok64 o = OK;` also matches `ok64 ret = OK;`.

## Git hook

    echo '#!/bin/sh
    capo --hook' > .git/hooks/post-commit
    chmod +x .git/hooks/post-commit

## How it works

Source files are parsed with tree-sitter, trigrams extracted from leaf text
and packed with path hashes into u64 entries stored as sorted MSET runs in
`.git/capo/*.idx`.

**CSS mode:** extracts trigrams from selector strings, seeks each in the
MSET index, intersects path hash sets to narrow candidates, then parses
and CSS-matches only the surviving files.

**SPOT mode:** extracts trigrams from the needle text, uses the same index
intersection to narrow candidates, then parses each candidate file and
runs tree-pattern matching (SPOT) to find structural matches.

`--fork N` stripes files across N workers. Works with git worktrees.

