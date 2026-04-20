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
unnecessary.  On a successful session end, the client appends one
composite watermark entry to its REFS:

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
| `L` | List     | sender → receiver  | wh128 pack bookmark, OR 8-byte tail sentinel |
| `E` | End      | either             | empty; terminates a list or the session      |
| `Q` | pack     | sender → receiver  | raw pack bytes (`PACK`…trailer, 20 B SHA1)   |
| `R` | reflog   | sender → receiver  | raw reflog tail bytes (append-only stream)   |
| `X` | Error    | either             | ron60 ok64 code + optional utf8 message      |

### `L` body

Two forms, distinguished by body size:

  * **bookmark (16 B)** — wh128 entry copied verbatim from the
    sender's index:

        key = file_id20 | offset40 | type4(PACK)
        val = hashlet60 | flags4

  * **tail sentinel (8 B)** — the last `L` for a given file_id
    carries only a u64 `final_length`.  Receiver uses it to close
    the previous bookmark's byte range (length = final_length −
    previous.offset).

## Session flow

### Get (`verb = 'G'`, client receives packs)

    C → S : H(1, 'G')
    C → S : W(pack_seq, pack_len, reflog_len)   ; or W() if unknown
    S → C : L, L, …, E                          ; bookmarks past W
    [receiver sanity check: first L's hashlet must be known
     locally, else C → S : X(UNRELATED), abort]
    S → C : Q, Q, …, E                          ; one Q per bookmark L
    S → C : R, R, …, E                          ; reflog tail past W.reflog_len
    C     : append packs, index per-pack, verify objects
    C     : dedup & filter reflog entries, append to own REFS
    C     : on final E, write composite watermark to REFS

### Post (`verb = 'P'`, client sends packs)

    C → S : H(1, 'P')
    C → S : W(pack_seq, pack_len, reflog_len)   ; client's view of
                                                  server, from its REFS
    C → S : L, L, …, E                          ; C's bookmarks past W
    [S sanity check: first L known locally or W() accepted]
    C → S : Q, Q, …, E
    C → S : R, R, …, E                          ; C's reflog tail past
                                                  W.reflog_len
    S     : append packs, index, verify
    S     : dedup & filter reflog entries, append to own REFS
    S     : send final E
    C     : on final E, write composite watermark to REFS

In both directions the **client** sends the watermark (always its
own remembered view of the server) and always updates its own
REFS watermark at session end.  The server is stateless across
sessions.

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
