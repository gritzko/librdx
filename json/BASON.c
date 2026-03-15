#include "BASON.h"

#include "abc/JSON.h"
#include "abc/LSM.h"
#include "abc/PRO.h"
#include "abc/RON.h"
#include "abc/UTF8.h"

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
        if (n > 1) {
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

// --- Array key helpers ---

ok64 BASONFeedInc(u8s into, u8cs orig) {
    sane(u8sOK(into) && $ok(orig));
    size_t w = $len(orig);
    test(w > 0, SBADARG);
    test($len(into) >= w, SNOROOM);
    memcpy(into[0], orig[0], w);
    for (size_t i = w; i > 0;) {
        i--;
        u8 v = RON64_REV[into[0][i]];
        test(v != 0xff, SBADARG);
        if (v < 63) {
            into[0][i] = RON64_CHARS[v + 1];
            into[0] += w;
            done;
        }
        into[0][i] = RON64_CHARS[0];
    }
    fail(SBADARG);
}

ok64 BASONFeedInfInc(u8s into, u8cs prev) {
    sane(u8sOK(into));
    size_t pl = $ok(prev) ? $len(prev) : 0;
    if (pl == 0) {
        test($len(into) >= 1, SNOROOM);
        into[0][0] = RON64_CHARS[1];
        into[0] += 1;
        done;
    }
    u8 d0 = RON64_REV[prev[0][0]];
    test(d0 != 0xff, SBADARG);
    u8 P;
    if (d0 < 32) P = 0;
    else if (d0 < 48) P = 1;
    else if (d0 < 56) P = 2;
    else if (d0 < 60) P = 3;
    else if (d0 < 62) P = 4;
    else P = 5;
    u8 W = P + 1;
    // Key shorter than needed width: pad with '1' (ban trailing zero)
    if (pl < W) {
        test($len(into) >= W, SNOROOM);
        memcpy(into[0], prev[0], pl);
        for (size_t i = pl; i < W; i++)
            into[0][i] = RON64_CHARS[1];
        into[0] += W;
        done;
    }
    // Copy first W chars, trim any trailing
    test($len(into) >= W, SNOROOM);
    memcpy(into[0], prev[0], W);
    u8 dP = RON64_REV[into[0][P]];
    test(dP != 0xff, SBADARG);
    if (dP < 63) {
        into[0][P] = RON64_CHARS[dP + 1];
        into[0] += W;
        done;
    }
    // Overflow: cascade backward, increment first non-63 digit, trim
    for (i8 i = (i8)P - 1; i >= 0; i--) {
        u8 di = RON64_REV[into[0][i]];
        test(di != 0xff, SBADARG);
        if (di < 63) {
            into[0][i] = RON64_CHARS[di + 1];
            into[0] += (size_t)(i + 1);
            done;
        }
    }
    fail(SBADARG);
}

#define BASON_MAX_KWIDTH 20

ok64 BASONFindMid(u8s into, u8cs left, u8cs right,
                  u64 len, u64 pcoll, u64 random) {
    sane(u8sOK(into));
    u64 need = len > 0 ? len * pcoll : pcoll;
    if (need == 0) need = 1;
    size_t wl = $ok(left) ? $len(left) : 0;
    size_t wr = $ok(right) ? $len(right) : 0;
    test(wl <= BASON_MAX_KWIDTH && wr <= BASON_MAX_KWIDTH, SBADARG);
    // Decode to digit arrays
    u8 ld[BASON_MAX_KWIDTH] = {};
    u8 rd[BASON_MAX_KWIDTH] = {};
    for (size_t i = 0; i < wl; i++) {
        u8 v = RON64_REV[left[0][i]];
        test(v != 0xff, SBADARG);
        ld[i] = v;
    }
    for (size_t i = 0; i < wr; i++) {
        u8 v = RON64_REV[right[0][i]];
        test(v != 0xff, SBADARG);
        rd[i] = v;
    }
    u8 w = (u8)(wl > wr ? wl : wr);
    if (w == 0) w = 1;
    for (; w <= BASON_MAX_KWIDTH; w++) {
        // Build left_w: first width-w key > left
        u8 lw[BASON_MAX_KWIDTH] = {};
        for (u8 i = 0; i < wl && i < w; i++) lw[i] = ld[i];
        if (wl >= w) {
            // Same width or longer: increment by 1
            i8 carry = 1;
            for (i8 i = (i8)w - 1; i >= 0 && carry; i--) {
                u8 sum = lw[i] + carry;
                if (sum >= 64) { lw[i] = 0; carry = 1; }
                else { lw[i] = sum; carry = 0; }
            }
            if (carry) continue;
        }
        // else wl < w: lw = ld padded with 0s, already > left in $cmp
        // Build right_w: last width-w key < right
        u8 rw[BASON_MAX_KWIDTH] = {};
        if (wr == 0) {
            for (u8 i = 0; i < w; i++) rw[i] = 63;
        } else {
            for (u8 i = 0; i < wr && i < w; i++) rw[i] = rd[i];
            // Subtract 1 (right padded with 0s, minus 1)
            i8 borrow = 1;
            for (i8 i = (i8)w - 1; i >= 0 && borrow; i--) {
                i8 d = (i8)rw[i] - borrow;
                if (d < 0) { rw[i] = 63; borrow = 1; }
                else { rw[i] = (u8)d; borrow = 0; }
            }
            if (borrow) continue;
        }
        // Check lw <= rw
        b8 valid = YES;
        for (u8 i = 0; i < w; i++) {
            if (lw[i] < rw[i]) break;
            if (lw[i] > rw[i]) { valid = NO; break; }
        }
        if (!valid) continue;
        // Compute gap = rw - lw as digit array
        u8 gap[BASON_MAX_KWIDTH] = {};
        i8 borrow2 = 0;
        for (i8 i = (i8)w - 1; i >= 0; i--) {
            i8 d = (i8)rw[i] - (i8)lw[i] - borrow2;
            if (d < 0) { gap[i] = (u8)(d + 64); borrow2 = 1; }
            else { gap[i] = (u8)d; borrow2 = 0; }
        }
        // Count significant digits to check if gap fits u64
        u8 sig = 0;
        for (u8 i = 0; i < w; i++) {
            if (gap[i] != 0) { sig = w - i; break; }
        }
        // 10 base-64 digits = 60 bits, fits u64
        if (sig > 10) {
            // Gap is enormous (>2^60), plenty of room
            // Add random offset (capped at 60 bits) to lw
            u64 offset = random & 0x0FFFFFFFFFFFFFFFull;
            u8 start[BASON_MAX_KWIDTH];
            memcpy(start, lw, w);
            u64 carry2 = offset;
            for (i8 i = (i8)w - 1; i >= 0 && carry2 > 0; i--) {
                u64 sum = (u64)start[i] + carry2;
                start[i] = (u8)(sum & 63);
                carry2 = sum >> 6;
            }
            test($len(into) >= w, SNOROOM);
            for (u8 i = 0; i < w; i++)
                into[0][i] = RON64_CHARS[start[i]];
            into[0] += w;
            done;
        }
        // Convert gap to u64
        u64 gap_val = 0;
        for (u8 i = 0; i < w; i++)
            gap_val = gap_val * 64 + gap[i];
        if (gap_val + 1 < need) continue;
        // Pick random offset within valid range
        u64 range = gap_val + 1 - need;
        u64 offset = (range > 0) ? random % (range + 1) : 0;
        // start = lw + offset
        u8 start[BASON_MAX_KWIDTH];
        memcpy(start, lw, w);
        u64 carry2 = offset;
        for (i8 i = (i8)w - 1; i >= 0 && carry2 > 0; i--) {
            u64 sum = (u64)start[i] + carry2;
            start[i] = (u8)(sum & 63);
            carry2 = sum >> 6;
        }
        test($len(into) >= w, SNOROOM);
        for (u8 i = 0; i < w; i++)
            into[0][i] = RON64_CHARS[start[i]];
        into[0] += w;
        done;
    }
    fail(SBADARG);
}

// --- Sort TLKV children by key (shared by parser and diff) ---

// TLKV raw-bytes slicer for LSMSort
static ok64 BASONSlice(u8csp rec, u8cs from) {
    sane(rec != NULL && $ok(from));
    u8cp start = from[0];
    u8 t; u8cs k, v;
    ok64 o = TLKVDrain(from, &t, k, v);
    if (o != OK) return o;
    rec[0] = start;
    rec[1] = from[0];
    return OK;
}

// TLKV key comparator for LSMSort
static b8 BASONKeyLess(u8cscp a, u8cscp b) {
    a_dup(u8c, fa, *a);
    a_dup(u8c, fb, *b);
    u8 ta, tb; u8cs ka, va, kb, vb;
    TLKVDrain(fa, &ta, ka, va);
    TLKVDrain(fb, &tb, kb, vb);
    return $cmp(ka, kb) < 0;
}

// TLKV right-wins merger for duplicate keys
static ok64 BASONRightWins(u8s into, u8css recs) {
    u8cscp last = $term(recs) - 1;
    return u8sFeed(into, *last);
}

ok64 BASONSortChildren(u8bp buf, u64bp idx) {
    sane(buf != NULL);
    // Past ends with 6-byte TLKVInto header; last byte is klen
    u8 klen = *(buf[1] - 1);
    u8p cstart = buf[1] + klen;
    size_t clen = (size_t)(buf[2] - cstart);
    if (clen > 0) {
        u8s children = {cstart, buf[2]};
        size_t ilen = u8bIdleLen(buf);
        test(ilen >= clen, BASONBAD);
        u8s tmp = {buf[2], buf[2] + clen};
        call(LSMSort, children, BASONSlice, BASONKeyLess,
             BASONRightWins, tmp);
        // Update write cursor (may shrink if dups merged)
        ((u8 **)buf)[2] = children[1];
        // Invalidate index entries (offsets stale after sort)
        if (idx != NULL && u64bDataLen(idx) > 1) {
            ((u64 **)idx)[2] = idx[1] + 1;
        }
    }
    done;
}

// --- JSON → BASON parser ---

#define BASON_PARSE_DEPTH 32

typedef struct {
    u8bp  buf;
    u64bp idx;
    u8    depth;
    struct {
        u8  ptype;      // 'O' or 'A'
        u8cs akey;      // previous array key ({NULL,NULL} if none)
        u8  akbuf[8];   // backing store for akey
        b8  have_key;   // object: key has been buffered
        b8  need_comma; // expect comma before next element
        b8  need_colon; // expect colon after key
    } frame[BASON_PARSE_DEPTH];
    u8  keybuf[256];
    u8  keylen;
} BASONparse;

// Resolve key for the current context into p->keybuf/keylen.
// Must be called before emitting any value.
static ok64 BASONpResolveKey(BASONparse *p) {
    sane(p != NULL);
    if (p->depth == 0) {
        p->keylen = 0;
        done;
    }
    if (p->frame[p->depth - 1].ptype == 'O') {
        test(p->frame[p->depth - 1].have_key, BASONBAD);
        test(!p->frame[p->depth - 1].need_colon, BASONBAD);
        p->frame[p->depth - 1].have_key = NO;
        p->frame[p->depth - 1].need_comma = YES;
        done;  // keybuf/keylen already set by on_string
    }
    // Array: require comma between elements (not before first)
    test(!p->frame[p->depth - 1].need_comma, BASONBAD);
    p->frame[p->depth - 1].need_comma = YES;
    // Array: generate lex-sortable RON64 index key
    u8 fi = p->depth - 1;
    u8s into = {p->keybuf, p->keybuf + 11};
    call(BASONFeedInfInc, into, p->frame[fi].akey);
    p->keylen = (u8)(into[0] - p->keybuf);
    memcpy(p->frame[fi].akbuf, p->keybuf, p->keylen);
    p->frame[fi].akey[0] = (u8cp)p->frame[fi].akbuf;
    p->frame[fi].akey[1] = (u8cp)p->frame[fi].akbuf + p->keylen;
    done;
}

// Set key slice from keybuf/keylen
#define BASONpKey(p, k) do { \
    (k)[0] = (u8cp)(p)->keybuf; \
    (k)[1] = (u8cp)(p)->keybuf + (p)->keylen; \
} while(0)

// Check if a JSON string body (without quotes) contains backslash
static b8 BASONpHasEscape(u8cs body) {
    for (u8cp q = body[0]; q < body[1]; ++q) {
        if (*q == '\\') return YES;
    }
    return NO;
}

static ok64 BASONpOnComma(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    test(p->depth > 0, BASONBAD);
    test(p->frame[p->depth - 1].need_comma, BASONBAD);
    p->frame[p->depth - 1].need_comma = NO;
    done;
}

static ok64 BASONpOnColon(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    test(p->depth > 0, BASONBAD);
    test(p->frame[p->depth - 1].ptype == 'O', BASONBAD);
    test(p->frame[p->depth - 1].need_colon, BASONBAD);
    p->frame[p->depth - 1].need_colon = NO;
    done;
}

static ok64 BASONpOnString(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    u8cs body = {tok[0] + 1, tok[1] - 1};
    u8cs utf8chk = {body[0], body[1]};
    test(utf8sValid(utf8chk) == OK, BASONBAD);
    // In object context, first string is a key
    if (p->depth > 0 && p->frame[p->depth - 1].ptype == 'O' &&
        !p->frame[p->depth - 1].have_key) {
        // Require comma between key:value pairs (not before first)
        test(!p->frame[p->depth - 1].need_comma, BASONBAD);
        if (BASONpHasEscape(body)) {
            u8s dst = {p->keybuf, p->keybuf + 255};
            call(JSONUnEscapeAll, dst, body);
            p->keylen = (u8)(dst[0] - p->keybuf);
        } else {
            size_t n = $len(body);
            if (n > 255) n = 255;
            memcpy(p->keybuf, body[0], n);
            p->keylen = (u8)n;
        }
        p->frame[p->depth - 1].have_key = YES;
        p->frame[p->depth - 1].need_colon = YES;
        done;
    }
    call(BASONpResolveKey, p);
    u8cs key; BASONpKey(p, key);
    if (BASONpHasEscape(body)) {
        a_pad(u8, tmp, 4096);
        call(JSONUnEscapeAll, tmp_idle, body);
        u8cs val = {(u8cp)_tmp, (u8cp)tmp_idle[0]};
        call(BASONFeed, p->idx, p->buf, 'S', key, val);
    } else {
        call(BASONFeed, p->idx, p->buf, 'S', key, body);
    }
    done;
}

static ok64 BASONpOnNumber(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    call(BASONpResolveKey, p);
    u8cs key; BASONpKey(p, key);
    call(BASONFeed, p->idx, p->buf, 'N', key, tok);
    done;
}

static ok64 BASONpOnLiteral(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    call(BASONpResolveKey, p);
    u8cs key; BASONpKey(p, key);
    if ($len(tok) == 4 && *tok[0] == 'n') {
        u8cs empty = {(u8cp)p->keybuf, (u8cp)p->keybuf};
        call(BASONFeed, p->idx, p->buf, 'B', key, empty);
    } else {
        call(BASONFeed, p->idx, p->buf, 'B', key, tok);
    }
    done;
}

static ok64 BASONpOnOpenObject(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    test(p->depth < BASON_PARSE_DEPTH, BASONBAD);
    call(BASONpResolveKey, p);
    u8cs key; BASONpKey(p, key);
    call(BASONFeedInto, p->idx, p->buf, 'O', key);
    p->frame[p->depth].ptype = 'O';
    p->frame[p->depth].akey[0] = NULL;
    p->frame[p->depth].akey[1] = NULL;
    p->frame[p->depth].have_key = NO;
    p->frame[p->depth].need_comma = NO;
    p->frame[p->depth].need_colon = NO;
    p->depth++;
    done;
}

static ok64 BASONpOnCloseObject(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    test(p->depth > 0, BASONBAD);
    test(p->frame[p->depth - 1].ptype == 'O', BASONBAD);
    test(!p->frame[p->depth - 1].have_key, BASONBAD);
    call(BASONSortChildren, p->buf, p->idx);
    p->depth--;
    call(BASONFeedOuto, p->idx, p->buf);
    done;
}

static ok64 BASONpOnOpenArray(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    test(p->depth < BASON_PARSE_DEPTH, BASONBAD);
    call(BASONpResolveKey, p);
    u8cs key; BASONpKey(p, key);
    call(BASONFeedInto, p->idx, p->buf, 'A', key);
    p->frame[p->depth].ptype = 'A';
    p->frame[p->depth].akey[0] = NULL;
    p->frame[p->depth].akey[1] = NULL;
    p->frame[p->depth].have_key = NO;
    p->frame[p->depth].need_comma = NO;
    p->frame[p->depth].need_colon = NO;
    p->depth++;
    done;
}

static ok64 BASONpOnCloseArray(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    test(p->depth > 0, BASONBAD);
    test(p->frame[p->depth - 1].ptype == 'A', BASONBAD);
    p->depth--;
    call(BASONFeedOuto, p->idx, p->buf);
    done;
}

// --- BASON → JSON export ---

// Emit a JSON-escaped string (with surrounding quotes) into out.
static ok64 BASONxString(u8s out, u8cs val) {
    sane(u8sOK(out));
    call(u8sFeed1, out, '"');
    call(JSONEscapeAll, out, val);
    call(u8sFeed1, out, '"');
    done;
}

// Recursive: export one BASON element (already drained) + its children.
static ok64 BASONxOne(u8s out, u64bp stack, u8csc data,
                      u8 type, u8cs val) {
    sane(u8sOK(out));
    if (type == 'O') {
        call(u8sFeed1, out, '{');
        call(BASONInto, stack, data, val);
        u8 ct; u8cs ck, cv;
        b8 first = YES;
        while (BASONDrain(stack, data, &ct, ck, cv) == OK) {
            if (!first) call(u8sFeed1, out, ',');
            first = NO;
            call(BASONxString, out, ck);
            call(u8sFeed1, out, ':');
            call(BASONxOne, out, stack, data, ct, cv);
        }
        call(BASONOuto, stack);
        call(u8sFeed1, out, '}');
    } else if (BASONPlex(type)) {
        // A, E, I, U — array containers
        call(u8sFeed1, out, '[');
        call(BASONInto, stack, data, val);
        u8 ct; u8cs ck, cv;
        b8 first = YES;
        while (BASONDrain(stack, data, &ct, ck, cv) == OK) {
            if (!first) call(u8sFeed1, out, ',');
            first = NO;
            call(BASONxOne, out, stack, data, ct, cv);
        }
        call(BASONOuto, stack);
        call(u8sFeed1, out, ']');
    } else if (type == 'N') {
        call(u8sFeed, out, val);
    } else if (type == 'B') {
        if ($len(val) == 0) {
            u8cs null_lit = {(u8cp)"null", (u8cp)"null" + 4};
            call(u8sFeed, out, null_lit);
        } else {
            call(u8sFeed, out, val);
        }
    } else {
        // S and all consonant leaf types: string
        call(BASONxString, out, val);
    }
    done;
}

ok64 BASONExportJSON(u8s out, u64bp stack, u8csc data) {
    sane(u8sOK(out) && stack != NULL && $ok(data));
    call(BASONOpen, stack, data);
    u8 type; u8cs key, val;
    while (BASONDrain(stack, data, &type, key, val) == OK) {
        call(BASONxOne, out, stack, data, type, val);
    }
    done;
}

// --- JSON → BASON parser ---

ok64 BASONParseJSON(u8bp buf, u64bp idx, u8cs json) {
    sane(buf != NULL && $ok(json));
    BASONparse p = {.buf = buf, .idx = idx, .depth = 0};
    JSONstate state = {
        .data = {json[0], json[1]},
        .ctx = &p,
        .on_string = BASONpOnString,
        .on_number = BASONpOnNumber,
        .on_literal = BASONpOnLiteral,
        .on_open_object = BASONpOnOpenObject,
        .on_close_object = BASONpOnCloseObject,
        .on_open_array = BASONpOnOpenArray,
        .on_close_array = BASONpOnCloseArray,
        .on_comma = BASONpOnComma,
        .on_colon = BASONpOnColon,
    };
    call(JSONLexer, &state);
    test(p.depth == 0, BASONBAD);
    done;
}
