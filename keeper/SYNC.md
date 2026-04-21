# dog plain sync

`SYNC` transfers keeper packs and reflog tails between two dog
repos over a single byte-stream (stdin/stdout of an ssh'd
`keeper --sync` on the peer).  It is **plain**: one direction of
pack flow per session, whole packs only, no negotiation of object
subsets, no thin packs, no delta resolution on the wire.

Contract assumed by plain sync:

    both peers forked from the same upstream (i.e. share a common
    first pack).  If the receiver has no prior watermark for the
    peer and the peer's first advertised pack is unknown locally,
    the receiver aborts.  Reconciling unrelated histories (graf
    comparison, repack) is a future iteration.

## Transport

    be://host/path

`be`/`keeper` dials `ssh host keeper --sync path` and talks TLV
over the resulting pipe.  `path` is the remote repo root (the dir
holding `.dogs/keeper/`).  Local `file://` transport skips ssh
and spawns `keeper --sync path` directly.

## Watermarks

Watermarks are **client-side only**.  The server keeps no memory
of which client synced when; reflog gossip makes per-client state
unnecessary.

**MVP status:** watermarks are sent as zero (`W()` empty body) and
the server always full-syncs.  Composite watermark persistence to
REFS, delta-style incremental sync, and pack-level filtering are
deferred — see "Out of scope" below.  The format below describes
the eventual target:

    <time>  be://host/path  #(SSS,LLLL;SSS,LLLL;RRRR,RRRR)

The to-uri is a fragment-only URI.  All six numbers are written as
lowercase hex (no `0x` prefix, no padding), produced by `sprintf`
with `%llx` / `%x` as appropriate.  The three semicolon-separated
pairs are:

    self_seq,self_len    client's own pack log tail at end of session
    peer_seq,peer_len    server's pack log tail the client observed
    self_rlen,peer_rlen  reflog byte lengths at end of session

Absent entry ⇒ unknown peer, full sync.

## TLV frame

Uses `abc/TLV`.  Short form (`tag|0x40`, u8 len, body) for
records up to 255 bytes; long form (tag, u32 len LE, body) for
larger.  Tag letters below are upper-case; the framer picks
short/long by body size.

All multi-byte integers are little-endian.

## Record types

| Tag | Name     | Direction          | Body                                         |
|-----|----------|--------------------|----------------------------------------------|
| `H` | Hello    | initiator → peer   | u8 version, u8 verb (`G`=get, `P`=post)      |
| `W` | Watermark| receiver → sender  | u32 pack_seq, u64 pack_len, u64 reflog_len; empty = unknown |
| `Q` | pack file| sender → receiver  | **whole log file bytes** (PACK header + stripped objects, no git trailer) |
| `R` | reflog   | sender → receiver  | raw reflog tail bytes (append-only stream)   |
| `E` | End      | either             | empty; terminates a list or the session      |
| `X` | Error    | either             | ron60 ok64 code + optional utf8 message      |

One Q carries one keeper log file (`.dogs/keeper/log/NNNNNNNNNN.pack`)
verbatim — so the receiver can mmap it as a fresh log file and
UNPK-index it with `scan_start=12, scan_end=file_len` (object count
comes from the embedded PACK header).  Per-pack `L` bookmarks
remain reserved for future delta-style incremental sync and are
not emitted in MVP full-sync mode.

## Session flow

### Get (`verb = 'G'`, client receives packs)

    C → S : H(1, 'G')
    C → S : W()                                 ; MVP: always empty → full sync
    S → C : Q, Q, …, E                          ; one Q per log file
    S → C : R, R, …, E                          ; whole reflog (dedup on ingest)
    C     : for each Q, write new log/NNN.pack,
            UNPK-index into a new idx/NNN.idx run
    C     : dedup & append reflog entries to own REFS

### Post (`verb = 'P'`, client sends packs)

    C → S : H(1, 'P')
    C → S : W()                                 ; MVP: always empty → full sync
    C → S : Q, Q, …, E
    C → S : R, R, …, E
    S     : ingest each Q as a new log/NNN.pack + idx run
    S     : dedup & append reflog entries to own REFS
    S → C : E                                   ; ack: post completed

In both directions the **client** sends the watermark (always its
own remembered view of the server).  The server is stateless
across sessions.

## Reflog gossip

Reflog sync is not authoritative per peer — entries spread
gossip-style.  Both sides must:

  * **Dedup** on receive: skip entries already present locally
    (same `<time, key, val>` triple).
  * **Filter** entries of no local interest (policy TBD —
    candidates: watermarks for unrelated peers, worktree markers
    from other repos).

The filter policy and its configuration are TODO.

## Error handling

`X(code, msg?)` aborts the session.  Codes propagate via ron60.
Partial packs in the log are harmless: no bookmark was written,
so they are invisible to lookup and will be overwritten on the
next append (FILEBook reserves the gap).  The watermark is only
advanced after a clean final `E`.

## Server entry point

`keeper --sync <path>` reads `H` from stdin to learn the verb,
then drives the corresponding flow.  Runs without a tty, no
prompts, exit code = ok64.  Server keeps no per-peer state.

## Out of scope (future iterations)

- Pack trailer SHA-1 verification on ingest (objects are verified
  per-hash today; pack-level check deferred).
- Graf-assisted reconciliation when histories have diverged.
- Reflog filter policy.
- Concurrent sessions against one server.
- Authentication beyond ssh.
