# capo — git repo trigram indexer with AST query

capo indexes source files in a git repo by trigrams, stores the index as
an LSM stack of MSET files in `.git/capo/`, and supports CSS-selector
queries against the AST of matching files.

## Usage

    capo                        full reindex (single process)
    capo --fork N               parallel reindex on N cores
    capo --hook                 incremental reindex (post-commit)
    capo '#fn.BASTParse'        find function by name
    capo 'fn:has(malloc)'       find functions containing "malloc"
    capo 'cmt:has(TODO)'        find comments containing "TODO"

## Examples

Index a repo on 4 cores:

    $ capo --fork 4
    capo: forking 4 workers
    OK   c    abc/MSET.h
    OK   c    ast/BAST.c
    ...
    capo: compacting all runs
    capo: done

Find a function by name:

    $ capo 'fn.CAPOTriCB'
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

    $ capo 'cmt:has(TODO)'
    --- js/io.cpp ---
    // TODO io.utf8()
    --- rdx/RDX.h ---
    // TODO: migrate RB.c to new self-contained format

Find all functions using a function:

    $ capo '#fn:has(u8sFeed)'

## Git hook

    echo '#!/bin/sh
    capo --hook' > .git/hooks/post-commit
    chmod +x .git/hooks/post-commit

## How it works

Source files are parsed with tree-sitter, trigrams extracted from leaf text
and packed with path hashes into u64 entries stored as sorted MSET runs in
`.git/capo/*.idx`. Queries extract trigrams from selector strings, seek each
in the MSET index, intersect path hash sets to narrow candidates, then parse
and CSS-match only the surviving files. `--fork N` stripes files across N
workers.

