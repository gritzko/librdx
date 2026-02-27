#include "BASON.h"

#include "abc/PRO.h"

// --- Read path ---

ok64 BASONOpen(u64bp stack, u8csc data) {
    sane(stack != NULL && $ok(data));
    call(u64bFeed1, stack, (u64)$len(data));  // end
    call(u64bFeed1, stack, (u64)0);            // cursor
    done;
}

ok64 BASONDrain(u64bp stack, u8csc data, u8p type, u8cs key, u8cs val) {
    sane(stack != NULL && $ok(data) && type != NULL);
    test(u64bDataLen(stack) >= 2, BASONBAD);
    u64 *top = u64bLast(stack);
    for (;;) {
        u64 cursor = *top;
        u64 end = *(top - 1);
        if (cursor >= end) return BASONEND;
        u8 tag = *(data[0] + cursor);
        u8 raw = (tag & TLVaA) ? (tag & ~TLVaA) : tag;
        if (raw == 'X' || raw == 'K' || raw == 'C') {
            // Index or SLOG metadata — skip with TLV drain
            u8cs from = {data[0] + cursor, data[0] + end};
            u8 lit; u8cs body;
            call(TLVu8sDrain, from, &lit, body);
            *top = (u64)(from[0] - data[0]);
            continue;
        }
        // BASON data record — drain as TLKV
        u8cs from = {data[0] + cursor, data[0] + end};
        call(TLKVDrain, from, type, key, val);
        *top = (u64)(from[0] - data[0]);
        done;
    }
}

ok64 BASONInto(u64bp stack, u8csc data, u8csc val) {
    sane(stack != NULL && $ok(data));
    u64 cs = (u64)(val[0] - data[0]);
    u64 ce = (u64)(val[1] - data[0]);
    *u64bLast(stack) = ce;      // overwrite cursor with children_end
    call(u64bFeed1, stack, cs);  // push children_start as new cursor
    done;
}

ok64 BASONOuto(u64bp stack) {
    sane(stack != NULL);
    test(u64bDataLen(stack) >= 3, BASONBAD);
    call(u64bPop, stack);  // pop cursor; end below becomes new cursor
    done;
}

ok64 BASONSeek(u64bp stack, u8csc data, u8csc target) {
    sane(stack != NULL && $ok(data) && $ok(target));
    test(u64bDataLen(stack) >= 2, BASONBAD);
    u64 *top = u64bLast(stack);
    u64 base = *top;       // children_start (call before any Next)
    u64 end = *(top - 1);  // children_end
    if (base >= end) return BASONEND;
    // Need at least 4 bytes for minimal index record
    if (end - base < 4) { done; }
    // Read trailing u16 = total index record length
    u16 total_len = 0;
    memcpy(&total_len, data[0] + end - 2, 2);
    if (total_len < 4 || (u64)total_len > end - base) { done; }
    // Verify index tag
    u8cp idx_start = data[0] + end - total_len;
    u8 tag = *idx_start;
    u8 raw = (tag & TLVaA) ? (tag & ~TLVaA) : tag;
    if (raw != BASON_IDX) { done; }
    // Drain TLV header
    u8cs from = {idx_start, data[0] + end};
    u8 ttype; u8cs body;
    ok64 o = TLVu8sDrain(from, &ttype, body);
    if (o != OK) { done; }
    // Body = [entries (8B each)][total_len (2B)]
    size_t blen = $len(body);
    if (blen < 10) { done; }  // need at least 1 entry + u16
    size_t entries_bytes = blen - 2;
    if (entries_bytes % 8 != 0) { done; }
    size_t n = entries_bytes / 8;
    u64c *entries = (u64c *)body[0];
    u64 target_pfx = BASONKeyPrefix(target);
    // Binary search: first entry where prefix >= target
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        u64 entry_pfx = entries[mid] >> 16;
        if (entry_pfx < target_pfx)
            lo = mid + 1;
        else
            hi = mid;
    }
    // Seek to page before target for linear scan
    u64 found;
    if (lo > 0) {
        u64 prev = entries[lo - 1];
        u16 page_off = (u16)(prev & 0xFFFF);
        found = base + (u64)(lo - 1) * BASON_PAGE + page_off;
    } else {
        found = base;
    }
    *top = found;
    done;
}

// --- Write helpers ---

// Sample record position for the current index level.
fun ok64 BASONSample(u64bp idx, u8bp buf, u8csc key) {
    sane(idx != NULL && buf != NULL);
    u64 write_base = idx[1][0];  // first entry = children start offset
    u64 rec_off = (u64)u8bDataLen(buf) - write_base;
    u64 page = rec_off / BASON_PAGE;
    size_t n_entries = u64bDataLen(idx) - 1;  // subtract base entry
    while (n_entries <= page) {
        u16 page_off = (u16)(rec_off - n_entries * BASON_PAGE);
        u64 pfx = BASONKeyPrefix(key);
        u64 entry = (u64)page_off | (pfx << 16);
        call(u64bFeed1, idx, entry);
        n_entries++;
    }
    done;
}

ok64 BASONFeedInto(u64bp idx, u8bp buf, u8 type, u8csc key) {
    sane(buf != NULL);
    if (idx != NULL && u64bDataLen(idx) > 0) {
        // Sample this container for parent's index (before bury)
        call(BASONSample, idx, buf, key);
    }
    call(TLKVInto, buf, type, key);
    if (idx != NULL) {
        call(u64bBury, idx);
        call(u64bFeed1, idx, (u64)u8bDataLen(buf));  // write_base
    }
    done;
}

ok64 BASONFeed(u64bp idx, u8bp buf, u8 type, u8csc key, u8csc val) {
    sane(buf != NULL);
    if (idx != NULL) {
        call(BASONSample, idx, buf, key);
    }
    call(TLKVFeed, u8bIdle(buf), type, key, val);
    done;
}

ok64 BASONFeedOuto(u64bp idx, u8bp buf) {
    sane(buf != NULL);
    if (idx != NULL) {
        size_t n = u64bDataLen(idx) - 1;  // entries (exclude base)
        if (n > 0) {
            u64 *entries = idx[1] + 1;
            size_t entries_bytes = n * 8;
            size_t body_len = entries_bytes + 2;
            size_t hdr_len = (body_len <= 0xff) ? 2 : 5;
            size_t total = hdr_len + body_len;
            if (u8bIdleLen(buf) < total) {
                u64bDigup(idx);
                return TLVnoroom;
            }
            // TLV header
            if (body_len <= 0xff) {
                u8bFeed1(buf, BASON_IDX | TLVaA);
                u8bFeed1(buf, (u8)body_len);
            } else {
                u8bFeed1(buf, BASON_IDX);
                u32 bl = (u32)body_len;
                u8sFeed32(u8bIdle(buf), &bl);
            }
            // Entry data
            memcpy(buf[2], entries, entries_bytes);
            ((u8 **)buf)[2] += entries_bytes;
            // Trailing total record length
            u16 tl = (u16)total;
            memcpy(buf[2], &tl, 2);
            ((u8 **)buf)[2] += 2;
        }
        call(u64bDigup, idx);
    }
    call(TLKVOuto, buf);
    done;
}
