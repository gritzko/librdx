#ifndef KEEPER_SYNC_H
#define KEEPER_SYNC_H

//  SYNC: plain dog sync — TLV over stdin/stdout.  See keeper/SYNC.md.
//
//  Record tags (long form on the wire; short form for small bodies):
//    H  Hello     u8 version, u8 verb ('G'=get, 'P'=post)
//    W  Watermark u32le pack_seq, u64le pack_len, u64le reflog_len
//                 (empty body = unknown peer)
//    L  List     16-byte wh128 bookmark, OR 8-byte u64le final_length
//                 (tail sentinel, one per log file at end of list)
//    E  End      empty; terminates a list or the session
//    Q  pack     raw pack bytes (stripped or framed — ingest end decides)
//    R  reflog   raw reflog tail bytes
//    X  error    u64le ok64 code + optional utf-8 message

#include "abc/INT.h"
#include "abc/OK.h"
#include "abc/S.h"
#include "abc/B.h"
#include "dog/WHIFF.h"
#include "keeper/KEEP.h"

con ok64 SYNCFAIL    = 0x7225cc3ca495;
con ok64 SYNCBADREC  = 0x7225cc2ca35b38c;
con ok64 SYNCUNRELAT = 0xc89731e5db39529d;
con ok64 SYNCPROTO   = 0x1c8973196d8758;

//  Record tag letters.  These are the "logical" (long-form) tags;
//  TLV framing picks short vs long on the wire by body length.
#define SYNC_TAG_HELLO 'H'
#define SYNC_TAG_WATER 'W'
#define SYNC_TAG_LIST  'L'
#define SYNC_TAG_END   'E'
#define SYNC_TAG_PACK  'Q'
#define SYNC_TAG_REFS  'R'
#define SYNC_TAG_ERROR 'X'

#define SYNC_VERSION   1

//  Verb codes (H body, second byte).
#define SYNC_VERB_GET  'G'  // receiver asks peer for packs
#define SYNC_VERB_POST 'P'  // sender pushes packs to peer

//  Watermark: receiver's view of the sender's tail.  Zero-valued
//  struct means "unknown peer" — transmitted as an empty W body.
typedef struct {
    u32 pack_seq;     // last seen pack log file_id
    u64 pack_len;     // bytes at pack_seq observed
    u64 reflog_len;   // bytes of REFS observed
} sync_wm;

//  Feed/Drain: append/remove one SYNC record from a buffer.
//  Buffers use abc/B semantics — feeders append to IDLE, drainers
//  consume from DATA.  All integers little-endian.

//  Feeders — append one record to `out->IDLE`, growing DATA.
ok64 SYNCFeedHello(u8bp out, u8 version, u8 verb);
ok64 SYNCFeedWater(u8bp out, sync_wm const *wm);  // wm==NULL → empty
ok64 SYNCFeedList (u8bp out, wh128 const *bookmark);
ok64 SYNCFeedSentinel(u8bp out, u64 final_length);
ok64 SYNCFeedEnd  (u8bp out);
ok64 SYNCFeedPack (u8bp out, u8csc body);
ok64 SYNCFeedRefs (u8bp out, u8csc body);
ok64 SYNCFeedError(u8bp out, ok64 code, u8csc msg);

//  Drainers — read one record's tag + body.  `value` receives the
//  payload slice (pointing into `from`'s underlying bytes).  The
//  caller then parses the body with the appropriate SYNCParse*.
//  Returns OK on success, TLVNODATA if more bytes are needed.
ok64 SYNCDrain(u8cs from, u8p tag, u8csp value);

//  -- session entry points (POSIX fd transport) --

//  Server: read H from `in_fd`, dispatch to the Get / Post handler,
//  serve until a final E is emitted.  Stateless across sessions.
//  `peer_uri` is informational (for logs) and may be empty.
ok64 SYNCServe(keeper *k, int in_fd, int out_fd);

//  Client: spawn `keeper --sync <path>` at the other end of `uri`
//  and execute a Get (pull) session.  URI accepts `be://host/path`
//  (ssh transport) and `file:///absolute/path` (local spawn).
ok64 SYNCGet(keeper *k, u8csc uri);

//  Body parsers (cheap — the record body is already in memory).
ok64 SYNCParseHello(u8cs body, u8 *version, u8 *verb);
ok64 SYNCParseWater(u8cs body, sync_wm *wm, b8 *present);
ok64 SYNCParseList (u8cs body, wh128 *bookmark, u64 *sentinel_len,
                    b8 *is_sentinel);
ok64 SYNCParseError(u8cs body, ok64 *code, u8csp msg);

#endif
