#include "SLOG.h"

#include "PRO.h"
#include "RON.h"

u64 SLOG_STATS[SLOG_STAT_COUNT] = {0};

ok64 SLOGCreate(u64gp stack, u8gp data) {
    sane(stack != NULL && u8gOK(data));
    u64 zero = 0;
    call(u64gFeed1, stack, zero);
    done;
}

ok64 SLOGFeed(u64gp stack, u8gp data) {
    sane(stack != NULL && u8gOK(data));
    u64 off = u8gLeftLen(data);
    u8 r = SLOGRank(off);

    // Count entries from top that need flushing (rank < r)
    size_t count = 0;
    size_t len = u64gLeftLen(stack);
    while (count < len && SLOGRank(u64sAt(u64gLeft(stack), len - 1 - count)) < r)
        ++count;

    if (len>=128) 
        count = 64;
    if (count == 0) 
        done;

    // Pop and write compressed 'k' TLV record
    u64cs popped = {};
    call(u64gPopN, stack, count, popped);
    call(SLOGu8sFeedSkips, u8gRest(data), SLOG_K, popped, NO);
    ++SLOG_STATS[SLOG_STAT_KWRITE];

    done;
}

ok64 SLOGMark(u64gp stack, u8gp data) {
    sane(stack != NULL && u8gOK(data));

    // Record position before potential flush
    u64 off = u8gLeftLen(data);

    // Flush if needed (writes 'k' at position off)
    call(SLOGFeed, stack, data);

    // Push the mark position (either 'k' offset or data offset)
    call(u64gFeed1, stack, off);

    done;
}

ok64 SLOGSample(u64gp stack, u8gp data) {
    sane(stack != NULL && u8gOK(data));
    test(!u64gEmpty(stack), ok64sub(SLOGBAD, RON_a));

    u64 off = u8gLeftLen(data);
    u64 top = u64gTop(stack);

    // Only mark if we crossed into a new block
    u64 topblk = SLOGBlock(top);
    u64 offblk = SLOGBlock(off);

    if (offblk != topblk) {
        call(SLOGMark, stack, data);
    }

    done;
}

ok64 SLOGClose(u64gp stack, u8gp data) {
    sane(stack != NULL && u8gOK(data));

    size_t count = u64gLeftLen(stack);
    if (count == 0) done;

    call(SLOGu8sFeedSkips, u8gRest(data), SLOG_C, u64gLeftC(stack), YES);

    u64gShedAll(stack);

    done;
}

ok64 SLOGOpen(u64gp stack, u8csc stream) {
    sane(stack != NULL && $ok(stream));

    size_t len = $len(stream);
    test(len >= 3, ok64sub(SLOGBAD, RON_b));  // minimum: lit + len + reclen

    // Read last byte = record length, back up to 'c' record
    u8 reclen = $at(stream, len - 1);
    test(reclen >= 3 && reclen <= len, ok64sub(SLOGBAD, RON_c));

    // Parse 'c' TLV record
    a_tail(u8c, rec, stream, reclen);
    u8 lit = 0;
    u8cs val = {};
    call(TLVu8sDrain, rec, &lit, val);
    test(lit == SLOG_C_N, ok64sub(SLOGBAD, RON_d));

    // Body = compressed_data + reclen_byte
    size_t bodylen = $len(val);
    test(bodylen >= 1, ok64sub(SLOGBAD, RON_e));
    a_head(u8c, compressed, val, bodylen - 1);

    // Decompress into stack (already reversed by SLOGu8bDrainSkips)
    call(SLOGu8bDrainSkips, stack, compressed);

    done;
}

ok64 SLOGSeek(u64gp stack, u8csc stream, u8zs less, u8csc target) {
    sane(stack != NULL && $ok(stream) && less != NULL && $ok(target));

    a_pad(u64, tmp, 64);
    u64gp tmpg = u64bDataIdle(tmp);
    u64 scan_from = 0, scan_to = (u64)$len(stream);

    // Phase 1: narrow [scan_from, scan_to) using skip list
    while (!u64gEmpty(stack)) {
        u64 off = 0;
        call(u64gPop, stack, &off);
        ++SLOG_STATS[SLOG_STAT_POPS];
        test(off < (u64)$len(stream), ok64sub(SLOGBAD, RON_f));

        u8cs rec = {stream[0] + off, stream[1]};
        u8 lit = 0;
        u8cs val = {};
        call(TLVu8sDrain, rec, &lit, val);

        if (lit == SLOG_C_N) { call(u64gFeed1, stack, off); scan_to = off; break; }

        if (lit == SLOG_K_N) {
            // Expand K into tmp, check record after K
            u64bReset(tmp);
            call(SLOGu8bDrainSkips, tmpg, val);

            u64 nextoff = rec[0] - stream[0];
            u8 nextlit = 0;
            u8cs nextval = {};
            call(TLVu8sDrain, rec, &nextlit, nextval);

            ++SLOG_STATS[SLOG_STAT_CMP];
            if (nextlit == SLOG_C_N || !less((u8csc){stream[0] + nextoff, rec[0]}, target)) {
                ++SLOG_STATS[SLOG_STAT_KEXP];
                scan_to = rec[0] - stream[0];
                call(u64gFeed, stack, u64gLeftC(tmpg));
            } else {
                ++SLOG_STATS[SLOG_STAT_KSKIP];
                scan_from = rec[0] - stream[0];
            }
            continue;
        }

        // Data record
        ++SLOG_STATS[SLOG_STAT_CMP];
        u8csc fullrec = {stream[0] + off, rec[0]};
        if (!less(fullrec, target)) { scan_to = rec[0] - stream[0]; break; }
        scan_from = rec[0] - stream[0];
    }

    // Phase 2: linear scan [scan_from, scan_to)
    while (scan_from < scan_to) {
        u64 off = scan_from;
        u8cs rec = {stream[0] + off, stream[1]};
        u8 lit = 0;
        u8cs val = {};
        call(TLVu8sDrain, rec, &lit, val);
        scan_from = rec[0] - stream[0];

        if (lit == SLOG_C_N) break;
        if (lit == SLOG_K_N) continue;

        ++SLOG_STATS[SLOG_STAT_CMP];
        if (!less((u8csc){stream[0] + off, rec[0]}, target)) {
            call(u64gFeed1, stack, off);
            done;
        }
    }

    fail(SLOGMISS);
}

ok64 SLOGu8sFeedSkips(u8sp into, u8 lit, u64csc offs, b8 trailen) {
    sane($ok(into) && $ok(offs));
    if (u64csLen(offs) == 0) done;

    // Temp buffers for delta and ZINT encoding
    a_pad(u64, tmp, 256);
    a_pad(u8, body, 1024);

    // Copy offsets for delta encoding
    size_t count = u64csLen(offs);
    test(count <= 256, SLOGNOROOM);
    for (size_t i = 0; i < count; ++i) {
        tmp[0][i] = offs[0][i];
    }
    u64s tmps = {tmp[0], tmp[0] + count};

    // Delta encode
    call(ZINTu64sDelta, tmps, 0);

    // Pack as blocked varints into body's idle space
    u64cs deltas = {tmps[0], tmps[1]};
    call(ZINTu8sFeedBlocked, body_idle, deltas);

    u8cs fullbody = {};
    if (trailen) {
        // Append reclen byte for close records
        size_t complen = body_idle[0] - body[1];
        size_t bodylen = complen + 1;
        size_t reclen = TLVlen(bodylen);
        test(reclen <= 255, ok64sub(SLOGBAD, RON_g));
        *body_idle[0] = (u8)reclen;
        fullbody[0] = body[1];
        fullbody[1] = body_idle[0] + 1;
    } else {
        fullbody[0] = body[1];
        fullbody[1] = body_idle[0];
    }

    call(TLVu8sFeed, into, lit, fullbody);

    done;
}

ok64 SLOGu8bDrainSkips(u64gp into, u8csc body) {
    sane(into != NULL && $ok(body));
    if ($len(body) == 0) done;

    // Remember where new values will start
    u64p newstart = into[1];

    // Drain blocked varints into gauge's rest portion
    a_dup(u8c,cur,body);
    u64s rest = {into[1], into[2]};
    call(ZINTu8sDrainBlocked, cur, rest);

    // Update gauge: rest[0] now points past written values
    into[1] = rest[0];

    // Un-delta only newly-drained values (not pre-existing LEFT data)
    u64s newvals = {newstart, into[1]};
    call(ZINTu64sUndelta, newvals, 0);

    // Reverse for lower-on-top order (skip list is stored high-to-low)
    u64sReverse(newvals);

    done;
}
