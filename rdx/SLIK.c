// SLIK: Skip List Interleaved Kilobytes
// Nested TLV with embedded SLOG skip lists per container
//
// See SLIK.md for format details and stack work description.
//
#include "RDX.h"

#include "abc/PRO.h"
#include "abc/SLOG.h"

con ok64 SLIKBAD = 0x1c55250b28d;
con ok64 SLIKNOROOM = 0x15543d86d8616;

// Close literal for SLIK containers
#define SLIK_C 'c'
#define SLIK_C_N 'C'  // normalized (after TLVaA mask)

// Check if TLV literal is a PLEX open (uppercase P/L/E/X)
fun b8 SLIKIsPlex(u8 lit) {
    return lit == 'P' || lit == 'L' || lit == 'E' || lit == 'X';
}

// Pop K record from stack if top is K, load its skip list onto stack
// Returns offset of first non-K entry (after-C or data record)
// stream = container range, stack = SLIK stack (c_off at bottom)
fun ok64 SLIKPopK(u64gp sg, u8csc stream) {
    sane(sg && $ok(stream));
    while (u64gLeftLen(sg) > 1) {
        u64 top = sg[0][u64gLeftLen(sg) - 1];
        if (top >= (u64)$len(stream)) break;

        u8 lit = *(stream[0] + top) & ~TLVaA;
        if (lit != 'K') break;  // Not K, done

        // Pop K offset
        call(u64gShed1, sg);

        // Parse K record
        u8cs krec = {stream[0] + top, stream[1]};
        u8 klit = 0;
        u8cs kval = {};
        call(TLVu8sDrain, krec, &klit, kval);

        // Decompress skip list (already reversed by SLOGu8bDrainSkips)
        a_pad(u64, tmp, 64);
        u64gp tmpg = u64bDataIdle(tmp);
        call(SLOGu8bDrainSkips, tmpg, kval);

        // Push onto stack (lower offsets on top)
        call(u64gFeed, sg, u64gLeftC(tmpg));
    }
    done;
}

// Forward declarations
fun b8 SLIKEulerZ(rdxcp a, rdxcp b);
fun ok64 SLIKLoadClose(u64gp stack, u8csc stream, u64 c_off, u64p c_end);

// Stack-safe comparator table for SLIKSeek (indexed by parent type)
static const rdxz SLIK_ZTABLE[RDX_TYPE_PLEX_LEN] = {
    rdxRootZ,     // ROOT - no ordering
    rdxTupleZ,    // TUPLE - no ordering
    rdxLinearZ,   // LINEAR - by id (reverse time)
    SLIKEulerZ,   // EULER - by value (stack-safe)
    rdxMultixZ,   // MULTIX - by source id
};

// Parse TLV at offset into rdx for comparison (sets plexc for TUPLE key extraction)
fun ok64 SLIKParseAt(rdxp x, u8csc stream, u64 off) {
    sane(x && $ok(stream) && off < (u64)$len(stream));
    u8cs rec = {stream[0] + off, stream[1]};
    call(rdxDrainTLV, x, rec);
    // Set plexc for TUPLE key extraction in SLIKEulerZ
    if (x->type == RDX_TYPE_TUPLE) {
        x->plexc[0] = rec[0];
        x->plexc[1] = stream[1];
    }
    done;
}

// SLIK seek: find element >= target in parent container
// c = target on input (c->type, c->i/s), found element on output
// p = parent container positioned AT the PLEX (after rdxNextSLIK returned it)
// Uses skip list for O(log n) search, then rdxNextSLIK for Phase 2
ok64 SLIKSeek(rdxp c, rdxp p) {
    sane(c && p && p->opt && rdxTypePlex(p));
    u64bp stack = (u64bp)p->opt;
    u64gp sg = u64bDataIdle(stack);

    u8csc stream = {p->plexc[0], p->plexc[1]};
    rdxz less = SLIK_ZTABLE[p->type];
    rdx target = *c;

    a_pad(u64, tmp, 64);
    u64gp tmpg = u64bDataIdle(tmp);
    u64 scan_from = 0, scan_to = (u64)$len(stream);

#ifdef ABC_TRACE
    fprintf(stderr, "SLIKSeek Phase1: stack_len=%zu, entries:", u64gLeftLen(sg));
    for (size_t k = 0; k < u64gLeftLen(sg); k++) {
        fprintf(stderr, " %lu", sg[0][k]);
    }
    fprintf(stderr, "\n");
#endif

    // Phase 1: narrow [scan_from, scan_to) using skip list
    // Stack has c_off at bottom (index 0), skip entries on top
    // Cases:
    // 1. FIRST less=true: scan_from = rec_end; continue
    // 2. FIRST less=false: scan_to = off; exit (scan_from unchanged)
    // 3. PLEX less=true: pop after-C; scan_from = after-C
    // 4. PLEX less=false: scan_to = off; exit (scan_from unchanged)
    // 5. K next>=target: expand K; scan_to = next; continue
    // 6. K next<target: scan_from = after next; discard K
    // 7. C own: scan_to = c_off; exit
    // 8. C child's: skip; continue
    while (u64gLeftLen(sg) > 1) {  // Keep c_off at bottom

        u64 off = 0;
        call(u64gPop, sg, &off);
        test(off < (u64)$len(stream), SLIKBAD);

        u8cs rec = {stream[0] + off, stream[1]};
        u8 lit = 0;
        u8cs val = {};
        call(TLVu8sDrain, rec, &lit, val);

        // Case 7, 8: C record
        if (lit == SLIK_C_N) {
            if (off == sg[0][0]) {
                // Case 7: own C - scan ends here
                scan_to = off;
                break;
            }
            // Case 8: child's C - skip
            continue;
        }

        // Cases 5, 6: K record - expand and check next element
        if (lit == SLOG_K_N) {
            u64bReset(tmp);
            call(SLOGu8bDrainSkips, tmpg, val);

            u64 nextoff = rec[0] - stream[0];
            u8 nextlit = *(stream[0] + nextoff) & ~TLVaA;

            if (nextlit == SLIK_C_N) {
                // Case 5 variant: K pointing to C
                scan_to = nextoff;
                call(u64gFeed, sg, u64gLeftC(tmpg));
            } else {
                rdx sample = {};
                call(SLIKParseAt, &sample, stream, nextoff);
                if (!less(&sample, &target)) {
                    // Case 5: next >= target
                    // Keep scan_from at previous position
                    scan_to = nextoff;
                    call(u64gFeed, sg, u64gLeftC(tmpg));
                } else {
                    // Case 6: next < target - skip past next record
                    u8cs nextrec = {stream[0] + nextoff, stream[1]};
                    u8 nlit = 0;
                    u8cs nval = {};
                    call(TLVu8sDrain, nextrec, &nlit, nval);
                    scan_from = nextrec[0] - stream[0];
                }
            }
            continue;
        }

        // Cases 1-4: Data record or PLEX
        rdx sample = {};
        call(SLIKParseAt, &sample, stream, off);
        if (!less(&sample, &target)) {
            // Cases 2, 4: element >= target
            // Push back - Phase 2 needs entries >= target as after_c refs
            call(u64gFeed1, sg, off);
            scan_to = off;
            break;
        }

        // Cases 1, 3: element < target
        if (SLIKIsPlex(lit)) {
            // Case 3: PLEX less=true - skip past it using after-C
            // after-C is next on stack (or in K record)
            if (u64gLeftLen(sg) > 1) {
                u64 next_entry = sg[0][u64gLeftLen(sg) - 1];
                // If next entry is a K record, load it to find after-C
                if (next_entry < (u64)$len(stream) &&
                    (*(stream[0] + next_entry) & ~TLVaA) == 'K') {
                    call(SLIKPopK, sg, stream);
                }
                if (u64gLeftLen(sg) > 1) {
                    u64 after_c = 0;
                    call(u64gPop, sg, &after_c);
                    scan_from = after_c;
                }
            }
        } else {
            // Case 1: FIRST less=true - continue from rec end
            scan_from = rec[0] - stream[0];
        }
    }

    if (scan_to == (u64)$len(stream) && u64gLeftLen(sg) > 0) {
        scan_to = sg[0][0];
    }

    // Phase 2: linear scan using rdxNextSLIK
    // Stack has entries >= target (pushed back in cases 2,4)
    // These serve as after_c references for PLEXes in [scan_from, scan_to)
    call(u64gFeed1, sg, scan_from);

#ifdef ABC_TRACE
    fprintf(stderr, "SLIKSeek Phase2: scan_from=%lu, scan_to=%lu, stream_len=%zu, target.type=%u, target.i=%ld, stack_len=%zu\n",
            scan_from, scan_to, (size_t)$len(stream), target.type, target.i, u64bDataLen(stack));
#endif

    rdx n = {
        .format = RDX_FMT_SLIK,
        .next = stream[0],
        .bulk = p->bulk,
        .opt = p->opt,
        .ptype = p->type,
    };

    while (rdxNextSLIK(&n) == OK) {
#ifdef ABC_TRACE
        fprintf(stderr, "SLIKSeek Phase2: got type=%u, id.seq=%lu\n", n.type, n.id.seq);
        if (n.type == RDX_TYPE_TUPLE) {
            fprintf(stderr, "  TUPLE plexc: %p-%p\n", (void*)n.plexc[0], (void*)n.plexc[1]);
        }
#endif
        if (!less(&n, &target)) {
            *c = n;
            done;
        }
    }

    fail(SLOGMISS);
}

// EULER comparator: stack-safe, never calls rdxInto
// For TUPLE elements, compares by key (first FIRST child)
fun b8 SLIKEulerZ(rdxcp a, rdxcp b) {
    if (!a || !b) return NO;

    // Extract key from a if it's a TUPLE
    rdx akey = *a;
    if (a->type == RDX_TYPE_TUPLE && a->plexc[0] && a->plexc[1]) {
        // Parse first child from TUPLE body (skip K records)
        u8cs rec = {a->plexc[0], a->plexc[1]};
        while (!$empty(rec) && (*rec[0] & ~TLVaA) == 'K') {
            u8 klit = 0;
            u8cs kval = {};
            TLVu8sDrain(rec, &klit, kval);
        }
        if (!$empty(rec)) {
            rdxDrainTLV(&akey, rec);
        }
    }

    // Extract key from b if it's a TUPLE
    rdx bkey = *b;
    if (b->type == RDX_TYPE_TUPLE && b->plexc[0] && b->plexc[1]) {
        u8cs rec = {b->plexc[0], b->plexc[1]};
        while (!$empty(rec) && (*rec[0] & ~TLVaA) == 'K') {
            u8 klit = 0;
            u8cs kval = {};
            TLVu8sDrain(rec, &klit, kval);
        }
        if (!$empty(rec)) {
            rdxDrainTLV(&bkey, rec);
        }
    }

    // Compare keys (both should be FIRST types now)
    if (akey.type != bkey.type) {
        return u8Z(&akey.type, &bkey.type);
    }
    if (akey.type < RDX_TYPE_PLEX_LEN) {
        // Still a PLEX (extraction failed), compare by id
        return id128RevZ(&akey.id, &bkey.id);
    }
    return rdx1Z(&akey, &bkey);
}

//
// Write path
//

void rdxWriteInitSLIK(rdxp x, u8bp buf, u64bp stack) {
    u64bReset(stack);
    u8bReset(buf);
    SLOGCreate(u64bDataIdle(stack), u8bDataIdle(buf));
    *x = (rdx){
        .format = RDX_FMT_SLIK | RDX_FMT_WRITE,
        .bulk = buf,
        .opt = (u8p)stack,
        .next = buf[1],  // container start = buffer data start
    };
}

ok64 rdxWriteNextSLIK(rdxp x) {
    sane(x && x->bulk && x->opt);
    u64bp stack = (u64bp)x->opt;
    u8sp into = u8bIdle(x->bulk);
    u8 lit = RDX_TYPE_LIT[x->type];

    call(u8bWantIdleLen, x->bulk, PAGESIZE);

    // Encode id
    a_pad(u8, id, 16);
    call(ZINTu8sFeed128, id_idle, x->id.seq, x->id.src);

    if (rdxTypePlex(x)) {
        // PLEX open: [LIT][len][idlen][id]
        u8cs empty = {};
        call(TLVFeedKeyVal, into, lit, id_datac, empty);
        // No SLOGSample for PLEX opens - child C offset pushed in Outo
    } else {
        // FIRST: [lit][len][idlen][id][value]
        a_pad(u8, val, PAGESIZE); // FIXME
        switch (x->type) {
            case RDX_TYPE_FLOAT:
                call(ZINTu8sFeedFloat, val_idle, &x->f);
                break;
            case RDX_TYPE_INT:
                call(ZINTu8sFeedInt, val_idle, &x->i);
                break;
            case RDX_TYPE_REF:
                call(ZINTu8sFeed128, val_idle, x->r.seq, x->r.src);
                break;
            case RDX_TYPE_STRING: {
                a_dup(u8c, str, x->s);
                call(u8bWantIdleLen, x->bulk, u8csLen(x->s)+8);
                a_pad(u8, enc, PAGESIZE); // FIXME
                UTABLE[x->flags & RDX_UTF_ENC_BITS][UTF8_ENCODER_ALL](enc_idle, str);
                call(u8sFeed, val_idle, enc_datac);
                break;
            }
            case RDX_TYPE_TERM:
                call(u8sFeed, val_idle, x->t);
                break;
            default:
                fail(SLIKBAD);
        }
        call(TLVFeedKeyVal, into, lit | TLVaA, id_datac, val_datac);
        // Sample after FIRST record for skip list
        // Pass "run" gauge: [container_start, write_pos, buf_end]
        u8p run[3] = {(u8p)x->next, x->bulk[2], x->bulk[3]};
        call(SLOGSample, u64bDataIdle(stack), run);
        // Sync back: if K record was written, run[1] advanced
        u8sFed(u8bIdle(x->bulk), run[1] - x->bulk[2]);
    }
    done;
}

ok64 rdxWriteIntoSLIK(rdxp c, rdxp p) {
    sane(c && p && p->bulk && p->opt);
    u64bp stack = (u64bp)p->opt;

    // Push parent stack length to PAST
    u64 marker = u64bDataLen(stack);
    call(u64bFeed1, stack, marker);
    u64sUsedAll(u64bData(stack));

    // Child container starts at current write position
    u8cp child_start = p->bulk[2];

    // Create fresh SLOG for child container
    // Pass "run" gauge starting at child_start
    u8p child_run[3] = {(u8p)child_start, p->bulk[2], p->bulk[3]};
    call(SLOGCreate, u64bDataIdle(stack), child_run);

    *c = (rdx){
        .format = RDX_FMT_SLIK | RDX_FMT_WRITE,
        .bulk = p->bulk,
        .opt = p->opt,
        .ptype = p->type,
        .next = child_start,  // container start for relative offsets
    };
    done;
}

// Write 'c' close record with SLOG-format body
// stack = SLOG stack with offsets, buf = output buffer
fun ok64 SLIKWriteClose(u64gp stack, u8bp buf) {
    sane(stack != NULL && u8bOK(buf));

    size_t count = u64gLeftLen(stack);
    call(u8bWantIdleLen, buf, count*8+8);
    if (count == 0) {
        // Empty container: write minimal close
        u8sp into = u8bIdle(buf);
        call(u8sFeed1, into, SLIK_C | TLVaA);
        call(u8sFeed1, into, 1);  // body = just reclen
        call(u8sFeed1, into, 3);  // reclen = 3
        done;
    }

    call(SLOGu8sFeedSkips, u8bIdle(buf), SLIK_C, u64gLeftC(stack), YES);

    u64gShedAll(stack);
    done;
}

ok64 rdxWriteOutoSLIK(rdxp c, rdxp p) {
    sane(c && p && p->bulk && p->opt && c->next);
    u64bp stack = (u64bp)p->opt;

    // Write 'c' close record
    call(SLIKWriteClose, u64bDataIdle(stack), p->bulk);

    // After-C offset relative to parent's container start
    u64 after_c = p->bulk[2] - p->next;

    // Pop parent stack from PAST
    u64 past_len = u64bPastLen(stack);
    if (past_len > 0) {
        u64 marker = stack[0][past_len - 1];
        u64sShedAll(u64bData(stack));
        call(u64gShed, u64bPastData(stack), marker + 1);
        u64sShed1(u64bData(stack));
    }

    // Push after-C offset to parent's stack (reader skips to here)
    call(u64bFeed1, stack, after_c);

    done;
}

ok64 rdxWriteFinishSLIK(rdxp x) {
    sane(x && x->bulk && x->opt && x->next);
    u64bp stack = (u64bp)x->opt;

    // Write root 'c' close
    call(SLIKWriteClose, u64bDataIdle(stack), x->bulk);
    done;
}

//
// Read path
//

// Load skip list from 'c' close record into stack
// stream = container content range, c_off = offset of 'c' within stream
// Returns c_end = offset after 'c' record
// Stack layout after: [c_off, ...skip offsets...] with c_off at bottom
fun ok64 SLIKLoadClose(u64gp stack, u8csc stream, u64 c_off, u64p c_end) {
    sane(stack != NULL && $ok(stream) && c_end != NULL);
    test(c_off < (u64)$len(stream), SLIKBAD);

    a_rest(u8c, rec, stream, c_off);
    u8 lit = 0;
    u8cs val = {};
    call(TLVu8sDrain, rec, &lit, val);
    test(lit == SLIK_C_N, SLIKBAD);

    *c_end = rec[0] - stream[0];

    // Push c_off as end marker (always at stack bottom)
    call(u64gFeed1, stack, c_off);

    // Body = compressed_data + reclen_byte
    size_t bodylen = $len(val);
    if (bodylen <= 1) {
        // Empty or minimal close - just c_off is enough
        done;
    }
    a_head(u8c, compressed, val, bodylen - 1);

    // Decompress skip list offsets on top of c_off
    // (SLOGu8bDrainSkips already reverses for lower-on-top order)
    call(SLOGu8bDrainSkips, stack, compressed);

    done;
}

void rdxInitSLIK(rdxp x, u8bp buf, u64bp stack) {
    u64bReset(stack);

    u64 end_pos = u8bDataLen(buf);
    if (end_pos < 3) {
        *x = (rdx){.format = RDX_FMT_SLIK, .next = buf[1], .bulk = buf, .opt = (u8p)stack};
        return;
    }

    // Find root 'c' close at end (read reclen from last byte)
    u8 reclen = buf[1][end_pos - 1];
    if (reclen < 3 || reclen > end_pos) {
        *x = (rdx){.format = RDX_FMT_SLIK, .next = buf[1], .bulk = buf, .opt = (u8p)stack};
        return;
    }

    u64 c_off = end_pos - reclen;
    u64 c_end = 0;
    u8csc stream = {buf[1], buf[1] + end_pos};

    ok64 o = SLIKLoadClose(u64bDataIdle(stack), stream, c_off, &c_end);
    if (o != OK) {
        u64bReset(stack);
        *x = (rdx){.format = RDX_FMT_SLIK, .next = buf[1], .bulk = buf, .opt = (u8p)stack};
        return;
    }

    // SLOG's position 0 (from SLOGCreate) serves as first child offset

    *x = (rdx){
        .format = RDX_FMT_SLIK,
        .next = buf[1],  // start of range
        .bulk = buf,
        .opt = (u8p)stack,
    };
}

ok64 rdxNextSLIK(rdxp x) {
    sane(x && x->next && x->opt);
    u64bp stack = (u64bp)x->opt;
    u8cp base = x->next;

    // Stack layout: [...skip offsets..., current_offset] with c_off at bottom
    if (u64bDataLen(stack) < 2) {
        x->type = 0;
        return END;
    }

    u64 cur_off = *u64bLast(stack);
    call(u64bPop, stack);

    // Check against c_offset (stack bottom)
    u64 c_off = stack[1][0];
    if (cur_off >= c_off) {
        x->type = 0;
        return END;
    }

    u8cp pos = base + cur_off;
    u8 firstlit = *pos & ~TLVaA;

    // Skip 'K' records during linear iteration (don't load, just move past)
    while (firstlit == 'K') {
        u8cs rec = {pos, base + c_off};
        u8 lit = 0;
        u8cs val = {};
        call(TLVu8sDrain, rec, &lit, val);

        // Move to next (skip K without loading)
        cur_off = rec[0] - base;
        if (cur_off >= c_off) {
            x->type = 0;
            return END;
        }
        pos = base + cur_off;
        firstlit = *pos & ~TLVaA;
    }

    // Parse TLV record
    u8cs data = {pos, base + c_off};
    u8cs idbody = {};
    u8cs value = {};
    u8 lit = 0;
    call(TLVDrainKeyVal, &lit, idbody, value, data);

    // Check for close record
    if (lit == SLIK_C_N) {
        x->type = 0;
        return END;
    }

    x->type = RDX_TYPE_LIT_REV[lit];
    test(x->type, SLIKBAD);

    // Parse id
    if ($empty(idbody)) {
        zero(x->id);
    } else {
        call(ZINTu8sDrain128, idbody, &x->id.seq, &x->id.src);
    }

    // Next record offset
    u64 next_off = data[0] - base;

    if (rdxTypePlex(x)) {
        // PLEX: next on stack is after_c or K leading to after_c
        test(u64bDataLen(stack) > 0, SLIKBAD);

        // Load K records and discard stale entries until we find after_c
        while (u64bDataLen(stack) > 1) {
            u64 top = *u64bLast(stack);

            // Discard stale entries <= current position
            if (top <= cur_off) {
                call(u64bPop, stack);
                continue;
            }

            // If top is K record, load it and continue
            if (top < c_off && (*(base + top) & ~TLVaA) == 'K') {
                call(u64bPop, stack);
                u8cs krec = {base + top, base + c_off};
                u8 klit = 0;
                u8cs kval = {};
                call(TLVu8sDrain, krec, &klit, kval);
                // Expand K (SLOGu8bDrainSkips already reverses)
                call(SLOGu8bDrainSkips, u64bDataIdle(stack), kval);
                continue;
            }

            // Top is after_c (not stale, not K)
            break;
        }

        u64 after_c = *u64bLast(stack);
        test(after_c > cur_off, SLIKBAD);  // after_c must be past current PLEX
        call(u64bPop, stack);

        // Push after-C offset for next iteration (to skip past this PLEX)
        call(u64bFeed1, stack, after_c);

        // plexc = [first_child, after_c)
        x->plexc[0] = data[0];
        x->plexc[1] = base + after_c;
        x->flags = RDX_FMT_SLIK;
    } else {
        // FIRST: push next offset if not already there
        if (u64bDataLen(stack) == 0 || *u64bLast(stack) != next_off) {
            call(u64bFeed1, stack, next_off);
        }

        // Parse value
        call(rdxParseVal, x, value);
        if (x->type == RDX_TYPE_STRING) x->flags = RDX_UTF_ENC_UTF8;
    }
    done;
}

ok64 rdxIntoSLIK(rdxp c, rdxp p) {
    sane(c && p && p->opt && rdxTypePlex(p));
    u64bp stack = (u64bp)p->opt;

    // Save seek key if provided
    u8 seek_type = c->type;
    rdx seek_key = *c;

    // Push parent stack length to PAST
    u64 marker = u64bDataLen(stack);
    call(u64bFeed1, stack, marker);
    u64sUsedAll(u64bData(stack));

    // plexc[1] points to end of C record
    u8cp base = p->plexc[0];
    u8cp c_end = p->plexc[1];
    u64 range_len = c_end - base;

    // Find C by reading reclen from last byte, load skip list
    if (range_len >= 3) {
        u8 reclen = *(c_end - 1);
        if (reclen >= 3 && reclen <= range_len) {
            u64 c_off = range_len - reclen;
            u64 c_end_off = 0;
            u8csc stream = {base, c_end};
            call(SLIKLoadClose, u64bDataIdle(stack), stream, c_off, &c_end_off);
        }
    }

    // SLOG's position 0 (from SLOGCreate) serves as first child offset

    *c = (rdx){
        .format = RDX_FMT_SLIK,
        .next = base,
        .bulk = p->bulk,
        .opt = p->opt,
        .ptype = p->type,
    };

    // No seek requested
    if (!seek_type) done;

    // Positional access for TUPLE/LINEAR
    if ((p->type == RDX_TYPE_ROOT || p->type == RDX_TYPE_TUPLE ||
         p->type == RDX_TYPE_LINEAR) &&
        seek_type == RDX_TYPE_INT) {
        i64 target = seek_key.i;
        i64 count = -1;
        while (count < target) {
            ok64 o = rdxNextSLIK(c);
            if (o != OK) return NONE;
            if (p->type == RDX_TYPE_LINEAR && (c->id.seq & 1)) continue;
            count++;
        }
        done;
    }

    // Use SLIKSeek for ordered containers (EULER, MULTIX, LINEAR)
    if (p->type >= RDX_TYPE_LINEAR && p->type <= RDX_TYPE_MULTIX) {
        // Set up c with seek target
        rdxMv(c, &seek_key);

        // Temporarily set p->plexc for SLIKSeek
        u8cs saved_plexc = {p->plexc[0], p->plexc[1]};
        p->plexc[0] = base;
        p->plexc[1] = c_end;

        ok64 o = SLIKSeek(c, p);

        p->plexc[0] = saved_plexc[0];
        p->plexc[1] = saved_plexc[1];

        if (o == OK) done;
        return NONE;
    }

    done;
}

ok64 rdxOutoSLIK(rdxp c, rdxp p) {
    (void)c;
    sane(p && p->opt);
    u64bp stack = (u64bp)p->opt;

    // Restore parent stack from PAST
    // PAST layout: [old_past..., old_data (M elements), marker=M]
    // We want DATA = old_data, PAST = old_past
    u64 past_len = u64bPastLen(stack);
    if (past_len > 0) {
        u64 marker = stack[0][past_len - 1];
        // buf[1] = start of old_data = buf[0] + past_len - marker - 1
        // buf[2] = end of old_data = buf[0] + past_len - 1 (exclude marker)
        ((u64**)stack)[1] = stack[0] + past_len - marker - 1;
        ((u64**)stack)[2] = stack[0] + past_len - 1;
    }
    done;
}
