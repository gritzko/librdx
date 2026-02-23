#include "RDX.h"
#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/ZINT.h"

fun u64 SKILBlock(u64 pos) { return (pos + 0x7f) >> 7; }
u64 SKILRank(u64 pos) {
    u64 b = SKILBlock(pos);
    return b ^ (b - 1);
}
#define SKIL_MIN_RANK 7

con ok64 SKILBAD = 0x1c51254b28d;

// Accessor for reader's bounds slice (same as TLV)
#define SKILdata2(x) ((u8csp)(x)->bulk)

ok64 rdxNextSKIL(rdxp x) {
    sane(x && x->next && x->opt);
    u8cp pos = x->next;
    u8cp end = x->opt;
    // Skip SKIL_LIT records
    while (pos < end && SKIL_LIT == (*pos & ~TLVaA)) {
        u8cs data = {pos, end};
        u8 lit;
        u8cs val;
        call(TLVu8sDrain, data, &lit, val);
        pos = data[0];
    }
    x->next = pos;  // Update position after skipping
    return rdxNextTLV(x);
}

ok64 rdxIntoSKIL(rdxp c, rdxp p) {
    sane(c && p);
    a_dup(u8c, data, p->plexc);
    p->flags = RDX_FMT_SKIL;
    // For OP containers (root, tuple), small data, no search key, or
    // LINEAR with INT index: delegate to TLV (skip list can't help with
    // positional)
    if ($len(data) <= 0xff || c->type == 0 || p->type == RDX_TYPE_ROOT ||
        p->type == RDX_TYPE_TUPLE ||
        (p->type == RDX_TYPE_LINEAR && c->type == RDX_TYPE_INT))
        return rdxIntoTLV(c, p);
    a_pad(u64, skipb, 248);  // fixme :)
    u8 flen = *u8csLast(data);
    test(flen + 3 < u8csLen(data), SKILBAD);
    a_tail(u8c, flush, data, flen + 3);
    u8 lit = 0;
    u8cs blocks = {};
    call(TLVu8sDrain, flush, &lit, blocks);
    test(lit == SKIL_LIT, SKILBAD);
    call(u8csShed1, blocks);
    call(ZINTu8sDrainBlocked, blocks, skipb_idle);
    call(ZINTu64sUndelta, skipb_data, 0);
    u64 from = 0;
    rdxz Z = ZTABLE[p->type];

    // Iterate backwards through skip pointers
    $for(u64c, sp, skipb_datac) {
        u64 pos = *sp;
        a_rest(u8c, back, data, pos);
        if (!u8csLen(back)) continue;
        u8 lit;
        u8cs blocks2 = {};
        // Skip over any consecutive SKIL_LIT entries
        while (u8csLen(back) && (**back & ~TLVaA) == SKIL_LIT) {
            pos += u8csLen(back);
            call(TLVu8sDrain, back, &lit, blocks2);
            pos -= u8csLen(back);
        }
        if (!u8csLen(back)) continue;
        // Create temp reader at this position
        rdx rec = {.format = RDX_FMT_SKIL,
                   .next = back[0],      // position
                   .opt = (u8p)back[1],  // range end
                   .bulk = p->bulk};
        call(rdxNextTLV, &rec);
        if (Z(&rec, c)) {
            // rec < c: target is after this position, keep searching
            from = pos;
            continue;
        } else if (Z(c, &rec)) {
            // c < rec: target is before this position
            // Load sub-skip-list and restart search in narrower range
            if (*blocks2) {
                u64bReset(skipb);
                call(ZINTu8sDrainBlocked, blocks2, skipb_idle);
                call(ZINTu64sUndelta, skipb_data, 0);
                if (u64csLen(skipb_datac) == 0) break;  // mishap?
                sp = u64csHead(skipb_datac) - 1;
                continue;
            } else {
                // No sub-skip-list available, use current position
                break;
            }
        } else {
            // Equal: found exact match
            from = pos;
            break;
        }
    }

    // Set up child iterator for TLV reading: next=position, opt=end
    c->format = p->flags;
    c->bulk = p->bulk;             // Share parent's buffer
    c->next = p->plexc[0] + from;  // Start position
    c->opt = (u8p)p->plexc[1];     // End of plex content
    c->ptype = p->type;
    c->flags = 0;
    rdx c2 = *c;
    ok64 o = OK;
    do {
        o = rdxNextSKIL(c);
    } while (o == OK && rdxZ(c, &c2));
    if (o == END || rdxZ(&c2, c)) o = NONE;
    return o;
}

ok64 rdxOutoSKIL(rdxp c, rdxp p) { return rdxOutoTLV(c, p); }

ok64 rdxWriteSKIL(rdxp x, u64 len) {
    sane(x && len && x->opt && x->bulk);
    u64bp skipb =
        (u64bp)x->opt;  // skiplist in opt (TLV uses plex[0] for record_start)
    u64sp skips = u64bData(skipb);
    if (!u64sLen(skips)) {
        call(u64bFeed1, skipb, 0);
    }
    u64 prev = **skips;
    u64 pos = prev + len;
    **skips = pos;
    if (SKILBlock(prev) == SKILBlock(pos)) done;
    u64 rank = SKILRank(pos);
    a_tailf(u64c, drain, skips, (SKILRank(**drain) < rank));
    size_t dl = u64csLen(drain);
    if (rank >= SKIL_MIN_RANK && dl > 0) {
        a_pad(u8, rec, 4096);
        a_pad(u64, tmp, 64);
        call(u64bFeed, tmp, drain);
        call(ZINTu64sDelta, tmp_data, 0);
        call(ZINTu8sFeedBlocked, rec_idle, tmp_datac);
        u8sp idle = u8bIdle(x->bulk);
        u64 l = u8sLen(idle);
        call(TLVu8sFeed, idle, SKIL_LIT, rec_datac);
        l -= u8sLen(idle);
        **skips += l;
        u64sShed(skips, dl);
    }
    call(u64bFeed1, skipb, pos);
    done;
}

ok64 rdxWriteNextSKIL(rdxp x) {
    sane(x && x->bulk);
    u8sp idle = u8bIdle(x->bulk);
    u8p posBefore = *idle;  // Position before write
    call(rdxWriteNextTLV, x);
    if (!rdxTypePlex(x)) {
        u8p posAfter = *idle;
        call(rdxWriteSKIL, x, posAfter - posBefore);
    }
    done;
}

ok64 rdxWriteIntoSKIL(rdxp c, rdxp p) {
    sane(c && p && p->opt && p->bulk);
    u64bp skipb = (u64bp)p->opt;
    call(u64bFeed1, skipb,
         u64bDataLen(skipb));      // push marker (parent's DataLen)
    u64sUsedAll(u64bData(skipb));  // marker moves to PAST with parent's list
    call(u64bFeed1, skipb, 0);
    call(rdxWriteIntoTLV, c, p);
    c->opt = p->opt;  // Share skiplist buffer (AFTER TLV which clears opt)
    c->format = RDX_FMT_SKIL | RDX_FMT_WRITE;
    done;
}

ok64 rdxWriteOutoSKIL(rdxp c, rdxp p) {
    sane(c && p && c->opt && p->bulk && p->plex[0]);

    u64bp skips = (u64bp)c->opt;  // skiplist in opt, record_start in plex[0]
    u8sp idle = u8bIdle(p->bulk);
    u64 container_size = **u64bData(skips);
    if (container_size > 0xff) {
        a_pad(u8, rec, 4096);
        a_pad(u64, tmp, 64);
        a_dup(u64c, sk, u64bDataC(skips));
        u64csFed1(sk);
        call(u64bFeed, tmp, sk);
        call(ZINTu64sDelta, tmp_data, 0);
        call(ZINTu8sFeedBlocked, rec_idle, tmp_datac);
        call(u8sFeed1, rec_idle, u8sLen(rec_data));
        call(TLVu8sFeed, idle, SKIL_LIT, rec_datac);
    }

    u64 marker =
        *u64csLast(u64bPastC(skips));    // marker = parent's original DataLen
    call(u64sShedAll, u64bData(skips));  // shed child DATA
    call(u64gShed, u64bPastData(skips),
         marker + 1);                  // shed marker + parent's list from PAST
    call(u64sShed1, u64bData(skips));  // remove recovered marker slot from DATA

    call(rdxWriteOutoTLV, c, p);
    // After Outo, *idle points to end of PLEX record (possibly compacted)
    // p->plex[0] points to start of PLEX record
    u64 l = *idle - p->plex[0];  // Total PLEX record length
    call(rdxWriteSKIL, p, l);

    done;
}

// Internal recursive copy: any input -> SKIL output with data pumping
static ok64 rdxCopyToSKIL_(rdxp out, rdxp from, int fd, u8bp buf,
                           size_t threshold) {
    sane(out && from && !rdxWritable(from));

    scan(rdxNext, from) {
        rdxMv(out, from);
        call(rdxWriteNextSKIL, out);

        if (rdxTypePlex(from)) {
            rdx cout = {};
            rdx cfrom = {};
            call(rdxWriteIntoSKIL, &cout, out);
            call(rdxInto, &cfrom, from);
            call(rdxCopyToSKIL_, &cout, &cfrom, fd, buf, threshold);
            call(rdxWriteOutoSKIL, &cout, out);
            call(rdxOuto, &cfrom, from);

            // Check threshold and flush if needed
            // Sync gauge with buffer: bulk gauge tracks DataIdle boundary
            size_t datalen = u8bDataLen(buf);
            if (datalen >= threshold) {
                ssize_t written = write(fd, buf[1], datalen);
                if (written < 0) fail(FILEerror);
                // Reset buffer after flush (data written to file)
                u8bReset(buf);
                // Reinit bulk to the buffer
                out->bulk = buf;
            }
        }
    }
    seen(END);
    out->type = 0;
    done;
}

// Copy from any format to SKIL, streaming to file with data pumping
// - from: input iterator (any format)
// - fd: output file descriptor
// - buf: caller-provided output buffer
// - threshold: flush when DATA >= threshold (minimum 4KB)
ok64 rdxCopyToSKIL(rdxp from, int fd, u8bp buf, size_t threshold) {
    sane(from && !rdxWritable(from) && fd >= 0 && u8bOK(buf));
    test(threshold >= 4 * KB, BADARG);

    // Create output iterator with skip buffer
    a_pad0(u64, skipbuf, 256 * 1024);
    rdx out = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE,
               .opt = (u8p)skipbuf,  // skiplist in opt
               .bulk = buf};

    // Copy recursively
    call(rdxCopyToSKIL_, &out, from, fd, buf, threshold);

    // Final flush: write remaining data
    size_t datalen = u8bDataLen(buf);
    if (datalen > 0) {
        ssize_t written = write(fd, buf[1], datalen);
        if (written < 0) fail(FILEerrno(errno));
        if ((size_t)written != datalen) fail(RDXBADFILE);
    }

    done;
}

// Internal: Y-style merge of SKIL inputs -> SKIL output, no vtables
// Uses rdxgRest pattern: child iterators go into the idle portion of the gauge
static ok64 rdxCopySKILs_(rdxp out, rdxg ins, int fd, u8bp buf,
                          size_t threshold) {
    sane(out && rdxgOK(ins));

    rdxz Z = ZTABLE[out->ptype];
    size_t len = rdxgLeftLen(ins);
    if (len == 0) return OK;

    // Dup left before it contracts - keeps original gauge intact
    a_dup(rdx, left, rdxgLeft(ins));

    while (rdxsLen(left)) {
        a_head(rdx, eqs, left, len);

        // Advance all equal-key iterators
        $rof(rdx, r, eqs) {
            ok64 o = rdxNextSKIL(r);
            if (o == OK) {
                rdxsDownAtZ(left, r - *left, Z);
            } else if (o == END) {
                rdxSwap(r, rdxsLast(left));
                rdxsShed1(left);
                if (r < rdxsTerm(left)) rdxsDownAtZ(left, r - *left, Z);
            } else {
                fail(o);
            }
        }

        if (rdxsEmpty(left)) break;

        // Find elements with smallest key
        rdxsTopsZ(left, eqs, Z);
        len = rdxsLen(eqs);

        // Pick winner among equals
        if (len > 1) call(rdxsHeapZ, eqs, rdxWinZ);

        // Write winning element
        rdxMv(out, *eqs);
        call(rdxWriteNextSKIL, out);

        if (rdxTypePlex(*eqs)) {
            // Collect child iterators into gauge rest (no stack allocation)
            rdxg children = {};
            rdxsGauge(rdxgRest(ins), children);
            rdxs wins = {};
            rdxsTopsZ(eqs, wins, rdxWinZ);
            $for(rdx, w, wins) {
                rdxp cc = 0;
                call(rdxgFedP, children, &cc);
                call(rdxIntoSKIL, cc, w);
            }

            // Recurse into children
            rdx cout = {};
            call(rdxWriteIntoSKIL, &cout, out);
            call(rdxCopySKILs_, &cout, children, fd, buf, threshold);
            call(rdxWriteOutoSKIL, &cout, out);

            // Outo for all input iterators
            rdxp child_iters = *rdxgLeft(children);
            size_t i = 0;
            $for(rdx, w, wins) { rdxOutoSKIL(child_iters + i++, w); }

            // Check threshold and flush if needed
            // Sync gauge with buffer: bulk gauge tracks DataIdle boundary
            size_t datalen = u8bDataLen(buf);
            if (datalen >= threshold) {
                ssize_t written = write(fd, buf[1], datalen);
                if (written < 0) fail(FILEerror);
                u8bReset(buf);
                // Reinit bulk to the buffer
                out->bulk = buf;
            }
        }
    }

    out->type = 0;
    done;
}

// Merge multiple SKIL inputs to SKIL output, no vtable lookups
// - inputs: gauge of SKIL iterators
// - fd: output file descriptor
// - buf: caller-provided output buffer (mmap'd)
// - threshold: flush when DATA >= threshold
ok64 rdxCopySKILs(rdxg inputs, int fd, u8bp buf, size_t threshold) {
    sane(rdxgOK(inputs) && !rdxgEmpty(inputs) && fd >= 0 && u8bOK(buf));
    test(threshold >= 4 * KB, BADARG);

    // Create output iterator with skip buffer
    a_pad0(u64, skipbuf, 256 * 1024);
    rdx out = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE,
               .opt = (u8p)skipbuf,  // skiplist in opt
               .bulk = buf};
    out.ptype = (**inputs).ptype;

    // Merge recursively
    call(rdxCopySKILs_, &out, inputs, fd, buf, threshold);

    // Final flush
    size_t datalen = u8bDataLen(buf);
    if (datalen > 0) {
        ssize_t written = write(fd, buf[1], datalen);
        if (written < 0) fail(FILEerrno(errno));
        if ((size_t)written != datalen) fail(RDXBADFILE);
    }

    done;
}
