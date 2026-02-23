//
// Created by gritzko on 11/19/25.
//
// TLV iterator using rec/bulk/opt pointer layout:
//   rec  - pointer to current record start (lit byte)
//   bulk - pointer to buffer [past, data, idle, end], PAGE-compatible
//   opt  - for readers: current read position (record end, next start)
//        - for SKIL writers: skiplist buffer pointer
//
// Read mode:
//   bulk points to buffer, read bounds are [bulk[1], bulk[2]] (data region)
//   opt tracks current position, starts at bulk[1], advances to record end
//   rec set to record start after reading
//   Multiple child readers can share buffer (read-only access)
//   For PAGE: call PAGEEnsure() before reading if bulk is paged
//
// Write mode:
//   bulk buffer shared by parent/child, write at bulk[2] (idle)
//   rec = start of TLV record being written (for compaction reference)
//   On closing PLEX: if body <= 255, compact header 5->2 bytes, shed 3
//
#include "RDX.h"
#include "abc/PRO.h"

// Read mode: iterate over TLV records
// next = current position (advances to next record after parse)
// opt = end of range (constant)
// bulk = underlying buffer (for PAGE)
ok64 rdxNextTLV(rdxp x) {
    sane(x && x->next && x->opt);
    u8cp pos = x->next;  // current position
    u8cp end = x->opt;   // range end (constant)
    if (pos >= end) {
        x->type = 0;
        x->loc = UINT32_MAX;
        return END;
    }

    // Drain TLV record using slice view
    u8cs data = {pos, end};
    u8cs idbody = {};
    u8cs value = {};
    u8 lit = 0;
    call(TLVDrainKeyVal, &lit, idbody, value, data);
    test(lit < 128, ok64sub(RDXBAD, RON_n));
    x->type = RDX_TYPE_LIT_REV[lit];
    test(x->type, ok64sub(RDXBAD, RON_o));

    // Advance next to next record (data[0] = record end)
    x->next = data[0];

    // Parse id
    test(u8csOK(idbody), ok64sub(RDXBAD, RON_p));
    if ($empty(idbody)) {
        zero(x->id);
    } else {
        call(ZINTu8sDrain128, idbody, &x->id.seq, &x->id.src);
    }

    // Parse value
    if (rdxTypePlex(x)) {
        u8csMv(x->plexc, value);
        x->flags = RDX_FMT_TLV;
    } else {
        call(rdxParseVal, x, value);
        if (x->type == RDX_TYPE_STRING) x->flags = RDX_UTF_ENC_UTF8;
    }
    done;
}

ok64 rdxIntoTLV(rdxp c, rdxp p) {
    sane(c && p && p->type && rdxTypePlex(p));
    c->format = p->flags;
    // Child reads from parent's plexc range
    c->next = p->plexc[0];       // start position
    c->opt = (u8p)p->plexc[1];   // end of range
    c->bulk = p->bulk;           // inherit buffer (for PAGE)
    c->ptype = p->type;
    c->flags = 0;
    if (!c->type) {
        zero(c->r);
        done;
    }
    rdx c2 = *c;
    ok64 o = OK;

    // For positional containers (root, tuple, linear), support INT index scan
    if (p->type == RDX_TYPE_ROOT || p->type == RDX_TYPE_TUPLE ||
        p->type == RDX_TYPE_LINEAR) {
        if (c2.type == RDX_TYPE_INT) {
            // Integer index: scan to position N
            // For LINEAR: skip tombstones (visible position)
            // For TUPLE/ROOT: literal position (tombstones count)
            i64 pos = c2.i;
            i64 count = -1;
            while (count < pos) {
                o = rdxNextTLV(c);
                if (o != OK) return NONE;
                // Only skip tombstones for LINEAR arrays
                if (p->type == RDX_TYPE_LINEAR && (c->id.seq & 1)) continue;
                count++;
            }
            done;
        } else if (p->type != RDX_TYPE_LINEAR && c2.type >= RDX_TYPE_PLEX_LEN) {
            // Non-INT non-plex: scan for matching element (not for LINEAR)
            while (OK == (o = rdxNextTLV(c))) {
                if (c->type != c2.type) continue;
                // Equal if neither is less than other
                if (!rdx1Z(c, &c2) && !rdx1Z(&c2, c)) done;
            }
            return NONE;
        } else if (p->type == RDX_TYPE_LINEAR) {
            // LINEAR with non-INT key: use comparison-based seek (fall through)
        } else {
            // Plex type as search key in OP container - not supported
            return NONE;
        }
    }

    // For keyed containers: use comparison-based seek
    do {
        o = rdxNextTLV(c);
    } while (o == OK && rdxZ(c, &c2));
    if (o == END || rdxZ(&c2, c)) o = NONE;
    return o;
}

ok64 rdxOutoTLV(rdxp c, rdxp p) { return OK; }

ok64 rdxWriteTLV1(rdxp x) { return NOTIMPLYET; }

// Write mode: bulk is buffer, use u8bIdle for writing
ok64 rdxWriteNextTLV(rdxp x) {
    sane(x && x->bulk);
    u8sp into = u8bIdle(x->bulk);  // mutable idle slice
    u8 lit = RDX_TYPE_LIT[x->type];
    a_pad(u8, id, 16);
    call(ZINTu8sFeed128, id_idle, x->id.seq, x->id.src);
    a_pad(u8, val, 16);

    switch (x->type) {
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX: {
            // PLEX: open with 5-byte header, plex[0] = record start for later compaction
            x->plex[0] = *into;  // current write position
            test(u8sLen(into) >= 5, TLVnoroom);
            call(u8sFeed1, into, lit);
            u32 z = 0;
            call(u8sFeed32, into, &z);  // placeholder length
            // Write id into body
            call(u8sFeed1, into, u8sLen(id_data));
            call(u8sFeed, into, id_datac);
            x->flags = RDX_FMT_TLV | RDX_FMT_WRITE;
            done;
        }
        case RDX_TYPE_FLOAT:
            call(ZINTu8sFeedFloat, val_idle, &x->f);
            call(TLVFeedKeyVal, into, lit, id_datac, val_datac);
            break;
        case RDX_TYPE_INT:
            call(ZINTu8sFeedInt, val_idle, &x->i);
            call(TLVFeedKeyVal, into, lit, id_datac, val_datac);
            break;
        case RDX_TYPE_REF:
            call(ZINTu8sFeed128, val_idle, x->r.seq, x->r.src);
            call(TLVFeedKeyVal, into, lit, id_datac, val_datac);
            break;
        case RDX_TYPE_STRING: {
            // Encode string to temp buffer first, then write as TLV record
            // (s and plex share union memory, so save string slice first)
            a_dup(u8c, str, x->s);
            a_pad(u8, enc, PAGESIZE);  // temp buffer for encoded string
            call(UTABLE[x->flags & RDX_UTF_ENC_BITS][UTF8_DECODER_ALL], enc_idle, str);
            call(TLVFeedKeyVal, into, lit, id_datac, enc_datac);
            break;
        }
        case RDX_TYPE_TERM:
            call(TLVFeedKeyVal, into, lit, id_datac, x->t);
            break;
        default:
            fail(ok64sub(RDXBAD, RON_q));
    }
    done;
}

// Write mode Into: child shares parent's bulk buffer
ok64 rdxWriteIntoTLV(rdxp c, rdxp p) {
    sane(c && p && p->type && p->bulk);
    c->format = p->flags;
    c->bulk = p->bulk;  // Share buffer with parent
    c->ptype = p->type;
    c->type = 0;
    c->flags = 0;
    c->opt = NULL;
    zero(c->r);
    done;
}

// Write mode Outo: close PLEX record, compact if short
ok64 rdxWriteOutoTLV(rdxp c, rdxp p) {
    sane(c && p && p->plex[0] && p->bulk);
    u8sp idle = u8bIdle(p->bulk);   // mutable idle slice
    u8p start = p->plex[0];         // Record start (lit byte), stored in plex[0]
    u8p pos = *idle;                // Current write position
    u64 tl = pos - start;           // Total length including header
    u64 bodylen = tl - 5;           // Body length (header is 5 bytes)

    // Fill in length and compact if short
    if (bodylen <= 0xff) {
        // Compact: shift body back 3 bytes, use short header
        *start |= TLVaA;
        *(start + 1) = (u8)bodylen;
        memmove(start + 2, start + 5, bodylen);
        *idle -= 3;  // Shed 3 bytes
    } else {
        // Long record: fill in u32 length
        *(u32*)(start + 1) = bodylen;
    }
    done;
}
