#include "BASON.h"

#include "abc/JSON.h"
#include "abc/PRO.h"
#include "abc/RON.h"

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

// --- JSON → BASON parser ---

#define BASON_PARSE_DEPTH 32

typedef struct {
    u8bp  buf;
    u64bp idx;
    u8    depth;
    struct {
        u8  ptype;      // 'O' or 'A'
        u32 aidx;       // array element counter
        b8  have_key;   // object: key has been buffered
        b8  need_comma; // expect comma before next element
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
        p->frame[p->depth - 1].have_key = NO;
        p->frame[p->depth - 1].need_comma = YES;
        done;  // keybuf/keylen already set by on_string
    }
    // Array: require comma between elements (not before first)
    test(!p->frame[p->depth - 1].need_comma, BASONBAD);
    p->frame[p->depth - 1].need_comma = YES;
    // Array: generate lex-sortable RON64 index key
    u8s into = {p->keybuf, p->keybuf + 11};
    u8p start = into[0];
    call(RONu8sFeedInc, into, p->frame[p->depth - 1].aidx++);
    p->keylen = (u8)(into[0] - start);
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

static ok64 BASONpOnString(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    u8cs body = {tok[0] + 1, tok[1] - 1};
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
        done;
    }
    call(BASONpResolveKey, p);
    u8cs key; BASONpKey(p, key);
    if (BASONpHasEscape(body)) {
        u8 tmp[4096];
        u8s dst = {tmp, tmp + sizeof(tmp)};
        call(JSONUnEscapeAll, dst, body);
        u8cs val = {(u8cp)tmp, (u8cp)dst[0]};
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
    p->frame[p->depth].aidx = 0;
    p->frame[p->depth].have_key = NO;
    p->frame[p->depth].need_comma = NO;
    p->depth++;
    done;
}

static ok64 BASONpOnCloseObject(u8cs tok, void *ctx) {
    sane($ok(tok) && ctx != NULL);
    BASONparse *p = (BASONparse *)ctx;
    test(p->depth > 0, BASONBAD);
    test(p->frame[p->depth - 1].ptype == 'O', BASONBAD);
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
    p->frame[p->depth].aidx = 0;
    p->frame[p->depth].have_key = NO;
    p->frame[p->depth].need_comma = NO;
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
    } else if (type == 'A') {
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
    } else if (type == 'S') {
        call(BASONxString, out, val);
    } else if (type == 'N') {
        call(u8sFeed, out, val);
    } else if (type == 'B') {
        if ($len(val) == 0) {
            u8cs null_lit = {(u8cp)"null", (u8cp)"null" + 4};
            call(u8sFeed, out, null_lit);
        } else {
            call(u8sFeed, out, val);
        }
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
    };
    call(JSONLexer, &state);
    test(p.depth == 0, BASONBAD);
    done;
}
