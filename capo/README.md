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

Find a function by name (`fn.NAME`):

    $ capo 'fn.CAPOTriCB'

<pre style="background:#1e1e1e;color:#ccc;padding:8px;border-radius:4px;font-size:13px">
<span style="color:#888">--- capo/CAPO.c ---</span>
<span style="color:#c44">static</span> <span style="color:#68f">ok64</span> <span style="font-weight:bold"><span style="color:#da3">CAPOTriCB</span></span><span style="color:#888">(</span><span style="color:#68f">voidp</span> arg<span style="color:#888">,</span> <span style="color:#68f">u8cs</span> tri<span style="color:#888">)</span> <span style="color:#888">{</span>
    <span style="color:#68f">CAPOTriCtx</span> <span style="color:#888">*</span>ctx <span style="color:#888">=</span> <span style="color:#888">(</span><span style="color:#68f">CAPOTriCtx</span> <span style="color:#888">*)</span>arg<span style="color:#888">;</span>
    <span style="color:#c44">if</span> <span style="color:#888">(*</span>ctx<span style="color:#888">-&gt;</span>idle <span style="color:#888">&gt;=</span> ctx<span style="color:#888">-&gt;</span>end<span style="color:#888">)</span> <span style="color:#c44">return</span> CAPONOROOM<span style="color:#888">;</span>
    <span style="color:#68f">u64</span> entry <span style="color:#888">=</span> CAPOTriPack<span style="color:#888">(</span>tri<span style="color:#888">)</span> <span style="color:#888">|</span> <span style="color:#888">(</span><span style="color:#68f">u64</span><span style="color:#888">)</span>ctx<span style="color:#888">-&gt;</span>path_hash<span style="color:#888">;</span>
    <span style="color:#888">**</span>ctx<span style="color:#888">-&gt;</span>idle <span style="color:#888">=</span> entry<span style="color:#888">;</span>
    <span style="color:#888">(*</span>ctx<span style="color:#888">-&gt;</span>idle<span style="color:#888">)++;</span>
    <span style="color:#c44">return</span> OK<span style="color:#888">;</span>
<span style="color:#888">}</span>
</pre>

Find comments containing a specific substring (`cmt:has(TEXT)`):

    $ capo 'cmt:has(TODO)'

<pre style="background:#1e1e1e;color:#ccc;padding:8px;border-radius:4px;font-size:13px">
<span style="color:#888">--- js/io.cpp ---</span>
<span style="color:#888">// TODO io.utf8()</span>
<span style="color:#888">--- rdx/RDX.h ---</span>
<span style="color:#888">// TODO: migrate RB.c to new self-contained format</span>
</pre>

## How it works

**Indexing.** Each source file is parsed with tree-sitter (via `BASTParse`).
All leaf text is scanned for RON64-filtered trigrams (3-char sequences from
`[0-9A-Z_a-z~]`). Each trigram + path hash pair is packed into a u64 and
appended to a scratch buffer. When the buffer fills, entries are sorted and
flushed as an `.idx` file in `.git/capo/`. After indexing, the LSM stack is
compacted into a single sorted run with MSET deduplication.

**Querying.** Specific strings are extracted from the CSS selector (function
names, `:has()` text). Their trigrams are looked up in the MSET index via
seek+scan. Path hash sets are intersected across trigrams to narrow
candidates. Only matching files are parsed and run through `CSSMatch`.
Results are output with ANSI syntax highlighting via `CSSCat`.

**Parallel mode.** `--fork N` spawns N worker processes. Worker K indexes
every file where `file_number % N == K`, using seqno `N*batch + K + 1` to
avoid collisions. The parent waits for all workers, then compacts everything
into one file.

## Git hook

    echo '#!/bin/sh
    capo --hook' > .git/hooks/post-commit
    chmod +x .git/hooks/post-commit

## Data format

Index files in `.git/capo/NNNNNNNNNN.idx` are flat arrays of little-endian
u64 values. Each entry packs an 18-bit RON64 trigram in the upper 32 bits
and a 32-bit path hash (from RAPHash) in the lower 32 bits. Files are named
with 10-char zero-padded RON64 sequence numbers.
