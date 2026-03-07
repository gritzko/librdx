#  be-srv HTTP API

`be-srv [port]` serves a Beagle repo over HTTP (default port 8080).
Only `GET` is supported; other methods return `405`.

##  Request format

    GET /<path>[<suffix>][?<formula>] HTTP/1.1

The response mode is determined by the path suffix:

| suffix    | mode  | content-type          | description                    |
|-----------|-------|-----------------------|--------------------------------|
| `.bason`  | BASON | application/x-bason   | raw BASON TLKV records (be:)   |
| `.stat`   | STATE | application/x-bason   | metadata TLKV records (stat:)  |
| `/`       | DIR   | text/plain            | directory listing              |
| *(none)*  | RAW   | text/plain            | merged source text             |

##  Modes

### BASON  — `GET /path/to/file.bason`

Streams `be:` key-value records matching the path prefix.
Each record is a TLKV blob with the full DB key rewritten into
the root.  Both base and waypoint keys are filtered uniformly
by the formula (see below).  Fragment keys (commit messages)
are skipped.  Response is streamed (no Content-Length).

    GET /.bason          — all files in the project
    GET /src/.bason      — files under src/
    GET /main.c.bason    — single file

### STATE  — `GET /path/to/file.stat`

Same as BASON mode but scans the `stat:` scheme (file metadata:
mtime, mode, ftype).

    GET /main.c.stat     — metadata for main.c
    GET /.stat           — metadata for all files

### DIR  — `GET /path/`

Lists immediate children under the path prefix (from `be:` keys).
Directories have a trailing `/`.  One entry per line, deduplicated.

    GET /        — top-level listing
    GET /src/    — contents of src/

Response example:

    src/
    doc/
    main.c
    README.md

### RAW  — `GET /path/to/file`

Merges formula-matching records (base + waypoints) for the file,
exports the BASON tree back to source text, and returns it with
Content-Length.  Returns `404` if the file does not exist.

    GET /main.c          — rendered source of main.c

##  Formula (query string)

The optional `?query` selects which records to include.
Format: `branch1&stamp-branch2&stamp+branch3` (see VER.h).

The base key is the (0,0) waypoint.  It is included or excluded
by the same formula — no special casing.

Operators on each branch entry:

| op   | meaning            | includes base? |
|------|--------------------|----------------|
| none | all waypoints      | yes            |
| `-`  | time <= stamp      | yes            |
| `+`  | time >  stamp      | no             |
| `=`  | time == stamp      | no             |

A formula includes the base when at least one entry uses `none`
or `-`.  A formula with only `+` and `=` entries returns deltas
only (no base).

If no query is given, the server uses the branches from the `.be`
file (all-ANY formula, base included).

    GET /main.c?main             — main branch, base + all waypoints
    GET /.bason?main&feat        — main + feat, base + all waypoints
    GET /.bason?abc+main         — main waypoints after abc, no base

##  Error responses

| code | when                          |
|------|-------------------------------|
| 400  | unparseable request or URI    |
| 404  | file not found (RAW mode)     |
| 405  | method other than GET         |
| 500  | internal error (alloc, iter)  |

All error responses include `Connection: close`.

##  Running

    cd /path/to/worktree    # must contain .be file
    be-srv                  # listens on :8080
    be-srv 9090             # custom port

Handles SIGINT/SIGTERM for clean shutdown; ignores SIGPIPE.
