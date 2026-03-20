#include "CSS.h"

#include <string.h>

#include "abc/PRO.h"
#include "ast/HILI.h"
#include "json/BASON.h"

// Kind-name to BASON type mapping
typedef struct {
    const char *name;
    u8 type;
} CSSKindEntry;

static const CSSKindEntry CSS_KINDS[] = {
    {"fn", 'E'},     {"class", 'I'},  {"args", 'U'},
    {"obj", 'O'},    {"block", 'A'},  {"def", 'F'},
    {"cmt", 'D'},    {"str", 'G'},    {"num", 'L'},
    {"type", 'T'},   {"kw", 'R'},     {"pp", 'H'},
    {"punct", 'P'},
};

#define CSS_NKINDS (sizeof(CSS_KINDS) / sizeof(CSS_KINDS[0]))

static u8 CSSLookupKind(u8cs tok) {
    size_t tlen = (size_t)$len(tok);
    for (size_t i = 0; i < CSS_NKINDS; i++) {
        size_t nlen = strlen(CSS_KINDS[i].name);
        if (nlen == tlen && memcmp(tok[0], CSS_KINDS[i].name, nlen) == 0)
            return CSS_KINDS[i].type;
    }
    return 0;
}

// Flush a pending combinator as a 'P' leaf before emitting the next predicate
static ok64 CSSFlushComb(CSSstate *st) {
    sane(st != NULL);
    if (st->pending_comb == 0) done;
    u8 cbuf[1] = {st->pending_comb};
    u8cs cval = {cbuf, cbuf + 1};
    u8cs nokey = {};
    call(BASONFeed, st->idx, st->buf, 'P', nokey, cval);
    st->pending_comb = 0;
    done;
}

// ---- Lexer callbacks ----

ok64 CSSonIdent(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    call(CSSFlushComb, state);
    u8cs nokey = {};
    if (state->after_dot) {
        // Name match: .name → F leaf with val=name
        state->after_dot = 0;
        call(BASONFeed, state->idx, state->buf, 'F', nokey, tok);
    } else {
        // Kind match: look up type table
        u8 kind = CSSLookupKind(tok);
        if (kind != 0) {
            u8 tbuf[1] = {kind};
            u8cs tval = {tbuf, tbuf + 1};
            call(BASONFeed, state->idx, state->buf, 'T', nokey, tval);
        } else {
            // Unknown kind: treat as text match
            call(BASONFeed, state->idx, state->buf, 'S', nokey, tok);
        }
    }
    done;
}

ok64 CSSonDot(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    (void)tok;
    state->after_dot = 1;
    done;
}

ok64 CSSonStar(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    (void)tok;
    // Star: wildcard, no predicate emitted (match any node)
    done;
}

ok64 CSSonChild(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    (void)tok;
    state->pending_comb = '>';
    done;
}

ok64 CSSonAdjacent(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    (void)tok;
    state->pending_comb = '+';
    done;
}

ok64 CSSonSibling(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    (void)tok;
    state->pending_comb = '~';
    done;
}

ok64 CSSonHas(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    (void)tok;
    u8cs nokey = {};
    call(BASONFeedInto, state->idx, state->buf, 'E', nokey);
    state->paren_depth++;
    done;
}

ok64 CSSonNot(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    (void)tok;
    // :not() — emit 'E' container with a '!' key to signal negation
    u8 nbuf[1] = {'!'};
    u8cs nkey = {nbuf, nbuf + 1};
    call(BASONFeedInto, state->idx, state->buf, 'E', nkey);
    state->paren_depth++;
    done;
}

ok64 CSSonClose(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    (void)tok;
    if (state->paren_depth <= 0) fail(CSSBAD);
    call(BASONFeedOuto, state->idx, state->buf);
    state->paren_depth--;
    done;
}

ok64 CSSonLine(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    call(CSSFlushComb, state);
    // Parse "L<n>" or "L<n>-<m>"
    u8cp p = tok[0] + 1;  // skip 'L'
    u32 start = 0;
    while (p < tok[1] && *p >= '0' && *p <= '9') {
        start = start * 10 + (u32)(*p - '0');
        p++;
    }
    u32 end = start;
    if (p < tok[1] && *p == '-') {
        p++;
        end = 0;
        while (p < tok[1] && *p >= '0' && *p <= '9') {
            end = end * 10 + (u32)(*p - '0');
            p++;
        }
    }
    // Store line range as 8 bytes: 4 bytes start + 4 bytes end (big-endian)
    u8 lbuf[8];
    lbuf[0] = (u8)(start >> 24);
    lbuf[1] = (u8)(start >> 16);
    lbuf[2] = (u8)(start >> 8);
    lbuf[3] = (u8)(start);
    lbuf[4] = (u8)(end >> 24);
    lbuf[5] = (u8)(end >> 16);
    lbuf[6] = (u8)(end >> 8);
    lbuf[7] = (u8)(end);
    u8cs lval = {lbuf, lbuf + 8};
    u8cs nokey = {};
    call(BASONFeed, state->idx, state->buf, 'N', nokey, lval);
    done;
}

ok64 CSSonRoot(u8cs tok, CSSstate *state) {
    sane(state != NULL);
    (void)tok;
    done;
}

// ---- CSSParse ----

ok64 CSSParse(u8bp qbuf, u64bp qidx, u8cs selector) {
    sane(qbuf != NULL && qidx != NULL && $ok(selector));
    CSSstate st = {};
    st.data[0] = selector[0];
    st.data[1] = selector[1];
    st.buf = qbuf;
    st.idx = qidx;
    u8cs nokey = {};
    call(BASONFeedInto, qidx, qbuf, 'A', nokey);
    ok64 o = CSSLexer(&st);
    if (o != OK) return o;
    if (st.paren_depth != 0) return CSSBAD;
    call(BASONFeedOuto, qidx, qbuf);
    done;
}

// ---- CSSMatch ----

// Check if a BASON node matches a query predicate tree.
// query is the parsed query BASON data; we walk it in parallel with the data.

// Helper: check if text contains substring
static b8 CSSContains(u8cs haystack, u8cs needle) {
    size_t hlen = (size_t)$len(haystack);
    size_t nlen = (size_t)$len(needle);
    if (nlen == 0) return YES;
    if (nlen > hlen) return NO;
    u8cp end = haystack[0] + hlen - nlen + 1;
    u8cp p = haystack[0];
    while (p < end) {
        if (memcmp(p, needle[0], nlen) == 0) return YES;
        p++;
    }
    return NO;
}

// Helper: check if name matches (exact or glob with trailing *)
static b8 CSSNameMatch(u8cs name, u8cs pattern) {
    size_t nlen = (size_t)$len(name);
    size_t plen = (size_t)$len(pattern);
    if (plen == 0) return YES;
    if (plen > 0 && pattern[0][plen - 1] == '*') {
        // Prefix match
        size_t pfxlen = plen - 1;
        if (nlen < pfxlen) return NO;
        return memcmp(name[0], pattern[0], pfxlen) == 0 ? YES : NO;
    }
    if (nlen != plen) return NO;
    return memcmp(name[0], pattern[0], nlen) == 0 ? YES : NO;
}

// Match context for recursive walk
typedef struct {
    u8cs query;      // query BASON data
    u8 *match_bits;  // per-line match bitset
    u32 *line_off;   // line offset table
    int nlines;      // number of lines
} CSSMatchCtx;

// Check if a single data node (with its subtree) matches the query predicates.
// qstk/qdata walk the query; dstk/ddata walk the data.
// Returns YES if all query predicates at this level are satisfied.
static b8 CSSCheckNode(u64bp qstk, u8cs qdata,
                        u8 data_type, u8cs data_name,
                        u64bp dstk, u8cs ddata) {
    u8 qt = 0;
    u8cs qk = {}, qv = {};
    b8 matched = YES;

    while (BASONDrain(qstk, qdata, &qt, qk, qv) == OK) {
        if (qt == 'T') {
            // Type match: qv[0] is the expected BASON type letter
            if ($len(qv) != 1 || qv[0][0] != data_type)
                matched = NO;
        } else if (qt == 'F') {
            // Name match: qv is the expected name
            if (!CSSNameMatch(data_name, qv))
                matched = NO;
        } else if (qt == 'S') {
            // Text match: search for qv substring in data subtree leaves
            b8 found = NO;
            aBpad(u64, sstk, 256);
            ok64 so = BASONOpen(sstk, ddata);
            if (so == OK) {
                int sd = 0;
                u8 st = 0;
                u8cs sk = {}, sv = {};
                while (1) {
                    ok64 soo = BASONDrain(sstk, ddata, &st, sk, sv);
                    if (soo != OK) {
                        if (sd <= 0) break;
                        BASONOuto(sstk);
                        sd--;
                        continue;
                    }
                    if (!BASONCollection(st)) {
                        if (CSSContains(sv, qv)) {
                            found = YES;
                            break;
                        }
                    } else {
                        BASONInto(sstk, ddata, sv);
                        sd++;
                    }
                }
            }
            if (!found) matched = NO;
        } else if (qt == 'P') {
            // Combinator: skip for now (handled at higher level)
            (void)qv;
        } else if (qt == 'N') {
            // Line range: handled separately
            (void)qv;
        } else if (BASONCollection(qt)) {
            // :has() or :not() container
            b8 negated = ($ok(qk) && !$empty(qk) && qk[0][0] == '!');
            ok64 io = BASONInto(qstk, qdata, qv);
            if (io != OK) { matched = NO; continue; }

            // Walk data subtree looking for match
            b8 has_found = NO;
            aBpad(u64, hstk, 256);
            ok64 ho = BASONOpen(hstk, ddata);
            if (ho == OK) {
                int hd = 0;
                u8 ht = 0;
                u8cs hk = {}, hv = {};
                while (1) {
                    ok64 hoo = BASONDrain(hstk, ddata, &ht, hk, hv);
                    if (hoo != OK) {
                        if (hd <= 0) break;
                        BASONOuto(hstk);
                        hd--;
                        continue;
                    }
                    if (BASONCollection(ht)) {
                        // Check container: find its name from first F child
                        u8cs cname = {};
                        aBpad(u64, nstk, 64);
                        BASONInto(nstk, ddata, hv);
                        // Peek for F child (name)
                        // Actually we need to build the full sub-data
                        // For simplicity, check type match
                        aBpad(u64, qstk2, 256);
                        BASONOpen(qstk2, qdata);
                        // Re-walk query predicates against this child
                        u8cs child_data = {hv[0], hv[1]};
                        if (CSSCheckNode(qstk2, qdata, ht, cname,
                                         hstk, ddata)) {
                            has_found = YES;
                        }
                        BASONInto(hstk, ddata, hv);
                        hd++;
                    } else {
                        // Leaf: check text match against has-query predicates
                        aBpad(u64, qstk2, 256);
                        ok64 qo2 = BASONOpen(qstk2, qdata);
                        if (qo2 == OK) {
                            u8cs lname = {};
                            if (CSSCheckNode(qstk2, qdata, ht, lname,
                                             NULL, (u8cs){})) {
                                has_found = YES;
                            }
                        }
                    }
                    if (has_found) break;
                }
            }
            BASONOuto(qstk);

            if (negated) has_found = !has_found;
            if (!has_found) matched = NO;
        }
    }
    return matched;
}

// Binary search: find line index for byte offset
static int CSSOffsetToLine(u32 *line_off, int nlines, u32 off) {
    int lo = 0, hi = nlines;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (line_off[mid] <= off)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo - 1;
}

// Mark lines in bitset
static void CSSMarkLines(u8 *match, int nlines, u32 *line_off,
                          u32 start, u32 end) {
    if (end == 0) return;
    int l0 = CSSOffsetToLine(line_off, nlines, start);
    int l1 = CSSOffsetToLine(line_off, nlines, end - 1);
    if (l0 < 0) l0 = 0;
    if (l1 >= nlines) l1 = nlines - 1;
    for (int li = l0; li <= l1; li++)
        match[li >> 3] |= (u8)(1 << (li & 7));
}

ok64 CSSMatch(u8s out, u8cs bason_data, u8cs query,
              int context_lines, b8 use_color) {
    sane($ok(out) && $ok(bason_data));
    if ($empty(bason_data)) done;
    if (!$ok(query) || $empty(query)) done;

    // Decode query: extract predicates from the root 'A' container
    aBpad(u64, qstk, 256);
    call(BASONOpen, qstk, query);
    u8 qt = 0;
    u8cs qk = {}, qv = {};
    ok64 qo = BASONDrain(qstk, query, &qt, qk, qv);
    if (qo != OK || qt != 'A') done;
    call(BASONInto, qstk, query, qv);

    // Read query predicates into arrays
    u8 pred_types[32];
    u8cs pred_vals[32];
    u8cs pred_keys[32];
    int npreds = 0;
    b8 has_line = NO;
    b8 has_name = NO;  // whether an F (name) predicate exists
    u32 line_start = 0, line_end = 0;

    {
        aBpad(u64, qs2, 256);
        call(BASONOpen, qs2, query);
        u8 q2t = 0;
        u8cs q2k = {}, q2v = {};
        ok64 q2o = BASONDrain(qs2, query, &q2t, q2k, q2v);
        if (q2o == OK && q2t == 'A') {
            call(BASONInto, qs2, query, q2v);
            while (npreds < 32 &&
                   BASONDrain(qs2, query, &q2t, q2k, q2v) == OK) {
                if (q2t == 'N' && $len(q2v) == 8) {
                    // Line range predicate
                    has_line = YES;
                    line_start = ((u32)q2v[0][0] << 24) |
                                 ((u32)q2v[0][1] << 16) |
                                 ((u32)q2v[0][2] << 8) |
                                 ((u32)q2v[0][3]);
                    line_end = ((u32)q2v[0][4] << 24) |
                               ((u32)q2v[0][5] << 16) |
                               ((u32)q2v[0][6] << 8) |
                               ((u32)q2v[0][7]);
                } else {
                    pred_types[npreds] = q2t;
                    pred_vals[npreds][0] = q2v[0];
                    pred_vals[npreds][1] = q2v[1];
                    pred_keys[npreds][0] = q2k[0];
                    pred_keys[npreds][1] = q2k[1];
                    if (q2t == 'F') has_name = YES;
                    npreds++;
                }
            }
        }
    }

    // Pass 1: walk data BASON, build line table + match bitset
    u32 _line_off[65536];
    int nlines = 1;
    _line_off[0] = 0;

    u8 _match[8192];
    memset(_match, 0, sizeof(_match));

    aBpad(u64, dstk, 256);
    call(BASONOpen, dstk, bason_data);

    u32 off_stk[256];
    int off_top = 0;
    u32 offset = 0;
    int depth = 0;

    // Track container type match (bit 31 = full match, bit 30 = type matched awaiting name)
    // off_stk stores: offset | 0x80000000 (matched) | 0x40000000 (type ok, name pending)

    for (;;) {
        u8 type = 0;
        u8cs key = {}, val = {};
        ok64 o = BASONDrain(dstk, bason_data, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            // Pop container: check if it was marked to match
            if (off_top > 0) {
                off_top--;
                u32 entry = off_stk[off_top];
                if (entry & 0x80000000u) {
                    u32 start = entry & 0x3FFFFFFFu;
                    CSSMarkLines(_match, nlines, _line_off, start, offset);
                }
            }
            call(BASONOuto, dstk);
            depth--;
            continue;
        }

        if (!BASONCollection(type)) {
            u32 len = (u32)$len(val);
            u32 node_start = offset;
            // Build line table
            for (u32 i = 0; i < len && nlines < 65536; i++) {
                if (val[0][i] == '\n') {
                    _line_off[nlines] = offset + i + 1;
                    nlines++;
                }
            }
            offset += len;

            // Check leaf against predicates
            for (int pi = 0; pi < npreds; pi++) {
                if (pred_types[pi] == 'S') {
                    // Text match: search for substring in leaf value
                    if (CSSContains(val, pred_vals[pi])) {
                        CSSMarkLines(_match, nlines, _line_off,
                                     node_start, offset);
                    }
                } else if (pred_types[pi] == 'T' &&
                           $len(pred_vals[pi]) == 1) {
                    // Type match: check leaf type
                    if (pred_vals[pi][0][0] == type) {
                        CSSMarkLines(_match, nlines, _line_off,
                                     node_start, offset);
                    }
                }
            }
            // Name match: if this is an F leaf, find ancestor with name pending
            if (type == 'F') {
                for (int si = off_top - 1; si >= 0; si--) {
                    if (off_stk[si] & 0x40000000u) {
                        for (int pi = 0; pi < npreds; pi++) {
                            if (pred_types[pi] == 'F' &&
                                CSSNameMatch(val, pred_vals[pi])) {
                                off_stk[si] |= 0x80000000u;
                                off_stk[si] &= ~0x40000000u;
                            }
                        }
                        break;
                    }
                }
            }
        } else {
            // Container: check type/name predicates
            b8 type_ok = NO;

            // Check type predicates
            for (int pi = 0; pi < npreds; pi++) {
                if (pred_types[pi] == 'T' && $len(pred_vals[pi]) == 1) {
                    if (pred_vals[pi][0][0] == type) {
                        type_ok = YES;
                    }
                }
            }

            u32 flags = 0;
            if (type_ok && has_name) {
                // Type matched, but name check pending → bit 30
                flags = 0x40000000u;
            } else if (type_ok) {
                // Type matched, no name needed → fully matched
                flags = 0x80000000u;
            }

            if (off_top < 256) {
                off_stk[off_top] = offset | flags;
                off_top++;
            }
            call(BASONInto, dstk, bason_data, val);
            depth++;
        }
    }

    // Handle line range predicate
    if (has_line && nlines > 0) {
        // Line numbers are 1-based in the selector
        int ls = (int)line_start - 1;
        int le = (int)line_end - 1;
        if (ls < 0) ls = 0;
        if (le >= nlines) le = nlines - 1;
        for (int i = ls; i <= le; i++)
            _match[i >> 3] |= (u8)(1 << (i & 7));
    }

    // Trim trailing empty line
    if (nlines > 1 && _line_off[nlines - 1] >= offset) nlines--;

    // Expand match bitset by context_lines
    u8 _out[8192];
    memset(_out, 0, sizeof(_out));
    for (int i = 0; i < nlines; i++) {
        if (_match[i >> 3] & (1 << (i & 7))) {
            int lo = i - context_lines;
            if (lo < 0) lo = 0;
            int hi = i + context_lines;
            if (hi >= nlines) hi = nlines - 1;
            for (int j = lo; j <= hi; j++)
                _out[j >> 3] |= (u8)(1 << (j & 7));
        }
    }

    // Pass 2: re-walk BASON, output selected lines
    aBpad(u64, stk2, 256);
    call(BASONOpen, stk2, bason_data);
    int depth2 = 0;
    int cur_line = 0;
    b8 prev_sel = NO;
    b8 any_out = NO;

    for (;;) {
        u8 type = 0;
        u8cs key = {}, val = {};
        ok64 o = BASONDrain(stk2, bason_data, &type, key, val);
        if (o != OK) {
            if (depth2 <= 0) break;
            call(BASONOuto, stk2);
            depth2--;
            continue;
        }
        if (!BASONCollection(type)) {
            u8cp p = val[0];
            while (p < val[1]) {
                u8cp nl = memchr(p, '\n', (size_t)(val[1] - p));
                u8cp chunk_end = nl ? nl + 1 : val[1];
                b8 sel = (_out[cur_line >> 3] >> (cur_line & 7)) & 1;
                if (sel) {
                    if (!prev_sel && any_out) {
                        a_cstr(sep, "--\n");
                        call(u8sFeed, out, sep);
                    }
                    b8 styled = use_color ? HILILeaf(out, type) : NO;
                    u8cs chunk = {p, chunk_end};
                    call(u8sFeed, out, chunk);
                    if (styled) escfeed(out, 0);
                    prev_sel = YES;
                    any_out = YES;
                }
                if (nl) {
                    prev_sel = sel;
                    cur_line++;
                }
                p = chunk_end;
            }
        } else {
            call(BASONInto, stk2, bason_data, val);
            depth2++;
        }
    }

    done;
}
