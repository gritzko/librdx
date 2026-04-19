//  LS: append-only NUL-terminated path buffer.  See LS.h.
//
#include "LS.h"

#include "abc/PRO.h"

ok64 LSOpen(ls *s, u64 reserve) {
    sane(s && reserve > 0);
    memset(&s->buf, 0, sizeof(s->buf));
    return u8bMap(s->buf, reserve);
}

ok64 LSFeed(ls *s, u8csc path, u64 *off_out) {
    sane(s && off_out);
    u64 need = (u64)u8csLen(path) + 1;  // +1 for NUL
    if ((u64)u8bIdleLen(s->buf) < need) return LSNOROOM;

    *off_out = (u64)u8bDataLen(s->buf);
    call(u8bFeed, s->buf, path);
    call(u8bFeed1, s->buf, 0);
    done;
}

void LSGet(ls const *s, u64 off, u8csp out) {
    u8cp base = u8bDataHead(s->buf);
    u8cp end  = u8bIdleHead(s->buf);
    u8cp p    = base + off;
    if (p >= end) { out[0] = out[1] = NULL; return; }
    u8cp q = p;
    while (q < end && *q != 0) q++;
    out[0] = p;
    out[1] = q;
}

void LSClose(ls *s) {
    if (s && s->buf[0]) u8bUnMap(s->buf);
    memset(s, 0, sizeof(*s));
}
