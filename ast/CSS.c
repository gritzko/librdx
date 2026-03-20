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

// ---- CSSMatch: single-pass recursive filter producing BASON output ----

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
        size_t pfxlen = plen - 1;
        if (nlen < pfxlen) return NO;
        return memcmp(name[0], pattern[0], pfxlen) == 0 ? YES : NO;
    }
    if (nlen != plen) return NO;
    return memcmp(name[0], pattern[0], nlen) == 0 ? YES : NO;
}

// Check if a single data node matches the query predicates.
static b8 CSSCheckNode(u64bp qstk, u8cs qdata,
                        u8 data_type, u8cs data_name,
                        u64bp dstk, u8cs ddata) {
    u8 qt = 0;
    u8cs qk = {}, qv = {};
    b8 matched = YES;

    while (BASONDrain(qstk, qdata, &qt, qk, qv) == OK) {
        if (qt == 'T') {
            if ($len(qv) != 1 || qv[0][0] != data_type)
                matched = NO;
        } else if (qt == 'F') {
            if (!CSSNameMatch(data_name, qv))
                matched = NO;
        } else if (qt == 'S') {
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
            (void)qv;
        } else if (qt == 'N') {
            (void)qv;
        } else if (BASONCollection(qt)) {
            b8 negated = ($ok(qk) && !$empty(qk) && qk[0][0] == '!');
            ok64 io = BASONInto(qstk, qdata, qv);
            if (io != OK) { matched = NO; continue; }
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
                        u8cs cname = {};
                        aBpad(u64, nstk, 64);
                        BASONInto(nstk, ddata, hv);
                        aBpad(u64, qstk2, 256);
                        BASONOpen(qstk2, qdata);
                        u8cs child_data = {hv[0], hv[1]};
                        (void)child_data;
                        if (CSSCheckNode(qstk2, qdata, ht, cname,
                                         hstk, ddata)) {
                            has_found = YES;
                        }
                        BASONInto(hstk, ddata, hv);
                        hd++;
                    } else {
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

// Copy one BASON element (leaf or container subtree) to output buffer
static ok64 CSSCopyElement(u8bp out, u8 type, u8cs key, u8cs val,
                            u64bp stk, u8cs data) {
    sane(out != NULL);
    if (BASONCollection(type)) {
        call(BASONFeedInto, NULL, out, type, key);
        call(BASONInto, stk, data, val);
        u8 ct = 0;
        u8cs ck = {}, cv = {};
        while (BASONDrain(stk, data, &ct, ck, cv) == OK) {
            call(CSSCopyElement, out, ct, ck, cv, stk, data);
        }
        call(BASONOuto, stk);
        call(BASONFeedOuto, NULL, out);
    } else {
        call(BASONFeed, NULL, out, type, key, val);
    }
    done;
}

// Count lines in a slice (for line-range matching)
static int CSSCountLines(u8cs data) {
    int n = 0;
    $for(u8c, p, data) {
        if (*p == '\n') n++;
    }
    return n;
}

// Recursive match: walk BASON, copy matching elements to out
static ok64 CSSMatchRec(u8bp out, u64bp dstk, u8cs ddata,
                         u8cs qdata, int npreds,
                         u8 *pred_types, u8cs *pred_vals, u8cs *pred_keys,
                         b8 has_line, u32 line_start, u32 line_end,
                         b8 has_name, int *cur_line) {
    sane(out != NULL);
    u8 type = 0;
    u8cs key = {}, val = {};
    while (BASONDrain(dstk, ddata, &type, key, val) == OK) {
        if (BASONCollection(type)) {
            // Check if this container matches predicates
            b8 type_ok = NO;
            b8 has_type_pred = NO;
            for (int pi = 0; pi < npreds; pi++) {
                if (pred_types[pi] == 'T' && $len(pred_vals[pi]) == 1) {
                    has_type_pred = YES;
                    if (pred_vals[pi][0][0] == type)
                        type_ok = YES;
                }
            }

            if (has_type_pred && type_ok) {
                // Container type matches. Check name if needed.
                // We need to peek into children for F-leaf name.
                // Use CSSCheckNode for full predicate check.
                aBpad(u64, qstk, 256);
                call(BASONOpen, qstk, qdata);
                u8 qt = 0;
                u8cs qk = {}, qv = {};
                ok64 qo = BASONDrain(qstk, qdata, &qt, qk, qv);
                if (qo == OK && qt == 'A') {
                    call(BASONInto, qstk, qdata, qv);

                    // Find container's name by peeking at children (recursive)
                    u8cs cname = {};
                    aBpad(u64, pstk, 256);
                    call(BASONOpen, pstk, ddata);
                    call(BASONInto, pstk, ddata, val);
                    int pd = 0;
                    u8 pt = 0;
                    u8cs pk = {}, pv = {};
                    while (1) {
                        ok64 po = BASONDrain(pstk, ddata, &pt, pk, pv);
                        if (po != OK) {
                            if (pd <= 0) break;
                            BASONOuto(pstk);
                            pd--;
                            continue;
                        }
                        if (pt == 'F') {
                            $mv(cname, pv);
                            break;
                        }
                        if (BASONCollection(pt)) {
                            BASONInto(pstk, ddata, pv);
                            pd++;
                        }
                    }
                    if (CSSCheckNode(qstk, qdata, type, cname,
                                     NULL, (u8cs){})) {
                        // Full match — check line range if present
                        b8 line_ok = YES;
                        if (has_line) {
                            // Count lines in container text to check overlap
                            // For now, accept if container starts in range
                            line_ok = NO;
                            int start_line = *cur_line + 1;  // 1-based
                            // Peek container text size for line counting
                            aBpad(u64, lstk, 256);
                            call(BASONOpen, lstk, ddata);
                            call(BASONInto, lstk, ddata, val);
                            int container_lines = 0;
                            u8 lt = 0;
                            u8cs lk = {}, lv = {};
                            while (BASONDrain(lstk, ddata, &lt, lk, lv) == OK) {
                                if (!BASONCollection(lt)) {
                                    container_lines += CSSCountLines(lv);
                                } else {
                                    // Just count via recursive walk
                                    int ld = 1;
                                    BASONInto(lstk, ddata, lv);
                                    while (ld > 0) {
                                        ok64 lo = BASONDrain(lstk, ddata, &lt, lk, lv);
                                        if (lo != OK) {
                                            BASONOuto(lstk);
                                            ld--;
                                        } else if (!BASONCollection(lt)) {
                                            container_lines += CSSCountLines(lv);
                                        } else {
                                            BASONInto(lstk, ddata, lv);
                                            ld++;
                                        }
                                    }
                                }
                            }
                            int end_line = start_line + container_lines;
                            if (end_line >= (int)line_start &&
                                start_line <= (int)line_end) {
                                line_ok = YES;
                            }
                        }
                        if (line_ok) {
                            // Copy entire container to output
                            call(CSSCopyElement, out, type, key, val,
                                 dstk, ddata);
                            // CSSCopyElement consumed children via BASONInto,
                            // dstk is back at this level, continue to next
                            // sibling. Update line count.
                            // (We don't need precise line tracking after copy)
                            continue;
                        }
                    }
                }
            }
            // Not matched at this level — recurse into children
            call(BASONInto, dstk, ddata, val);
            call(CSSMatchRec, out, dstk, ddata, qdata, npreds,
                 pred_types, pred_vals, pred_keys,
                 has_line, line_start, line_end, has_name, cur_line);
            call(BASONOuto, dstk);
        } else {
            // Leaf matching
            b8 leaf_match = YES;
            b8 has_type_pred = NO;
            for (int pi = 0; pi < npreds; pi++) {
                if (pred_types[pi] == 'T') {
                    has_type_pred = YES;
                    if ($len(pred_vals[pi]) != 1 ||
                        pred_vals[pi][0][0] != type) {
                        leaf_match = NO;
                    }
                } else if (pred_types[pi] == 'S') {
                    if (!CSSContains(val, pred_vals[pi]))
                        leaf_match = NO;
                } else if (pred_types[pi] == 'F') {
                    // Name predicates don't apply to leaves
                    leaf_match = NO;
                }
            }
            // If no type pred and has_name, leaves don't match
            if (!has_type_pred && has_name) leaf_match = NO;
            // Line range check
            if (leaf_match && has_line) {
                int leaf_line = *cur_line + 1;  // 1-based
                int leaf_lines = CSSCountLines(val);
                int leaf_end = leaf_line + leaf_lines;
                if (leaf_end < (int)line_start || leaf_line > (int)line_end)
                    leaf_match = NO;
            }
            if (leaf_match) {
                call(BASONFeed, NULL, out, type, key, val);
            }
            // Track line count
            *cur_line += CSSCountLines(val);
        }
    }
    done;
}

ok64 CSSMatch(u8bp out, u8cs bason_data, u8cs query) {
    sane(out != NULL && $ok(bason_data));
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
    b8 has_name = NO;
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

    // Walk data BASON, copy matching elements to output
    aBpad(u64, dstk, 256);
    call(BASONOpen, dstk, bason_data);
    int cur_line = 0;
    call(CSSMatchRec, out, dstk, bason_data, query, npreds,
         pred_types, pred_vals, pred_keys,
         has_line, line_start, line_end, has_name, &cur_line);

    done;
}

// ---- CSSExport: plain text rendering of filtered BASON ----

static ok64 CSSExportRec(u8s out, u64bp stk, u8cs data) {
    sane(1);
    u8 type = 0;
    u8cs key = {}, val = {};
    while (BASONDrain(stk, data, &type, key, val) == OK) {
        if (!BASONCollection(type)) {
            call(u8sFeed, out, val);
        } else {
            call(BASONInto, stk, data, val);
            call(CSSExportRec, out, stk, data);
            call(BASONOuto, stk);
        }
    }
    done;
}

ok64 CSSExport(u8s out, u8cs filtered) {
    sane($ok(out));
    if (!$ok(filtered) || $empty(filtered)) done;
    aBpad(u64, stk, 256);
    call(BASONOpen, stk, filtered);

    u8 type = 0;
    u8cs key = {}, val = {};
    b8 had_container = NO;
    while (BASONDrain(stk, filtered, &type, key, val) == OK) {
        if (!BASONCollection(type)) {
            call(u8sFeed, out, val);
        } else {
            if (had_container) {
                call(u8sFeed1, out, '\n');
            }
            had_container = YES;
            call(BASONInto, stk, filtered, val);
            call(CSSExportRec, out, stk, filtered);
            call(BASONOuto, stk);
        }
    }
    done;
}

// ---- CSSCat: syntax-highlighted rendering with separators ----

static ok64 CSSCatRec(u8s out, u64bp stk, u8cs data,
                       u8 *cstk, int depth) {
    sane(1);
    u8 type = 0;
    u8cs key = {}, val = {};
    while (BASONDrain(stk, data, &type, key, val) == OK) {
        if (!BASONCollection(type)) {
            b8 styled = HILILeaf(out, type);
            call(u8sFeed, out, val);
            if (styled) HILIRestore(out, cstk, depth);
        } else {
            int d = depth < HILI_MAXDEPTH ? depth : HILI_MAXDEPTH - 1;
            cstk[d] = type;
            HILIContainer(out, type);
            call(BASONInto, stk, data, val);
            call(CSSCatRec, out, stk, data, cstk, depth + 1);
            call(BASONOuto, stk);
            HILIRestore(out, cstk, depth);
        }
    }
    done;
}

ok64 CSSCat(u8s out, u8cs filtered, u8cs relpath) {
    sane($ok(out));
    if (!$ok(filtered) || $empty(filtered)) done;
    aBpad(u64, stk, 256);
    call(BASONOpen, stk, filtered);

    u8 cstack[HILI_MAXDEPTH];
    memset(cstack, 0, sizeof(cstack));

    u8 type = 0;
    u8cs key = {}, val = {};
    b8 had_container = NO;
    while (BASONDrain(stk, filtered, &type, key, val) == OK) {
        if (!BASONCollection(type)) {
            b8 styled = HILILeaf(out, type);
            call(u8sFeed, out, val);
            if (styled) escfeed(out, 0);
        } else {
            if (had_container) {
                // Gray separator: -- relpath --
                escfeed(out, GRAY);
                a_cstr(sep_pre, "-- ");
                call(u8sFeed, out, sep_pre);
                if ($ok(relpath) && !$empty(relpath))
                    call(u8sFeed, out, relpath);
                a_cstr(sep_post, " --\n");
                call(u8sFeed, out, sep_post);
                escfeed(out, 0);
            }
            had_container = YES;
            call(BASONInto, stk, filtered, val);
            call(CSSCatRec, out, stk, filtered, cstack, 0);
            call(BASONOuto, stk);
        }
    }
    done;
}
