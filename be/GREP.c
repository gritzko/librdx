#include "BE.h"

#include <string.h>

#include "abc/PRO.h"
#include "ast/HILI.h"

// Check if char is a RON64 character (alphanumeric + _ ~)
#define BETriChar(c) (RON64_REV[(u8)(c)] != 0xff)

// Compute 2-char RON64 hashlet from path (12 bits = 4096 buckets)
ok64 BEHashlet(u8s into, u8cs path) {
    sane($ok(into) && $len(into) >= 2 && $ok(path));
    u64 h = RAPHash(path);
    u16 bucket = (u16)(h & 0xFFF);  // 12 bits
    into[0][0] = RON64_CHARS[bucket & 63];
    into[0][1] = RON64_CHARS[(bucket >> 6) & 63];
    into[0] += 2;
    done;
}

// Extract trigrams from BASON string leaves, call cb for each unique
ok64 BETriExtract(u8csc bason, BETriCBf cb, voidp arg) {
    sane($ok(bason) && cb != NULL);
    if ($empty(bason)) done;
    aBpad(u64, stk, 64);
    call(BASONOpen, stk, bason);
    int depth = 0;
    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk, bason, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            call(BASONOuto, stk);
            depth--;
            continue;
        }
        if (!BASONPlex(type) && $len(val) >= 3) {
            u8cp p = val[0];
            u8cp end = val[1] - 2;
            while (p <= end) {
                if (BETriChar(p[0]) && BETriChar(p[1]) && BETriChar(p[2])) {
                    u8cs tri = {p, p + 3};
                    call(cb, arg, tri);
                }
                p++;
            }
        } else if (BASONPlex(type)) {
            call(BASONInto, stk, bason, val);
            depth++;
        }
    }
    done;
}

// Extract symbol names from BASON 'F'-tagged leaves (definition names)
ok64 BESymExtract(u8csc bason, BESymCBf cb, voidp arg) {
    sane($ok(bason) && cb != NULL);
    if ($empty(bason)) done;
    aBpad(u64, stk, 64);
    call(BASONOpen, stk, bason);
    int depth = 0;
    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk, bason, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            call(BASONOuto, stk);
            depth--;
            continue;
        }
        if (type == BAST_TAG_NAME && !$empty(val)) {
            call(cb, arg, val);
        } else if (BASONPlex(type)) {
            call(BASONInto, stk, bason, val);
            depth++;
        }
    }
    done;
}

// ---- BASTGrepNodes: AST-aware line selection with context ----

// Binary search: find the line index for a byte offset.
// line_off[i] = byte offset of line i start. Returns line index (0-based).
static int BASTOffsetToLine(u32 *line_off, int nlines, u32 off) {
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

// Mark lines [l0..l1] in match bitset
static void BASTMarkLines(u8 *match, int nlines, u32 *line_off,
                          u32 start, u32 end) {
    if (end == 0) return;
    int l0 = BASTOffsetToLine(line_off, nlines, start);
    int l1 = BASTOffsetToLine(line_off, nlines, end - 1);
    if (l0 < 0) l0 = 0;
    if (l1 >= nlines) l1 = nlines - 1;
    for (int li = l0; li <= l1; li++) {
        match[li >> 3] |= (u8)(1 << (li & 7));
    }
}

ok64 BASTGrepNodes(u8s out, u8cs bason_data, int k,
                   BASTNodeCBf cb, void *ctx) {
    sane($ok(out) && $ok(bason_data) && cb != NULL);
    if ($empty(bason_data)) done;

    // === Pass 1: walk BASON, build line table + match bitset ===
    u32 _line_off[65536];
    int nlines = 1;
    _line_off[0] = 0;

    u8 _match[8192];
    memset(_match, 0, sizeof(_match));

    aBpad(u64, stk, 256);
    call(BASONOpen, stk, bason_data);

    u32 off_stk[256];
    int off_top = 0;
    u32 offset = 0;
    int depth = 0;

    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk, bason_data, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            if (off_top > 0) {
                off_top--;
                u32 entry = off_stk[off_top];
                if (entry & 0x80000000u) {
                    u32 start = entry & 0x7FFFFFFFu;
                    BASTMarkLines(_match, nlines, _line_off, start, offset);
                }
            }
            call(BASONOuto, stk);
            depth--;
            continue;
        }
        if (!BASONPlex(type)) {
            u32 len = (u32)$len(val);
            u32 node_start = offset;
            // Build line table from leaf content
            for (u32 i = 0; i < len && nlines < 65536; i++) {
                if (val[0][i] == '\n') {
                    _line_off[nlines] = offset + i + 1;
                    nlines++;
                }
            }
            offset += len;

            bason node = {
                .type = type, .ptype = 0,
                .data = NULL, .stack = NULL,
                .key = {key[0], key[1]},
                .val = {val[0], val[1]},
            };
            if (cb(&node, ctx)) {
                BASTMarkLines(_match, nlines, _line_off, node_start, offset);
            }
        } else {
            bason node = {
                .type = type, .ptype = 0,
                .data = NULL, .stack = NULL,
                .key = {key[0], key[1]},
                .val = {val[0], val[1]},
            };
            b8 matched = cb(&node, ctx);
            if (off_top < 256) {
                off_stk[off_top] = offset | (matched ? 0x80000000u : 0);
                off_top++;
            }
            call(BASONInto, stk, bason_data, val);
            depth++;
        }
    }

    // Trim trailing empty line (source ending with '\n')
    if (nlines > 1 && _line_off[nlines - 1] >= offset) nlines--;

    // === Expand match bitset by k context ===
    u8 _out[8192];
    memset(_out, 0, sizeof(_out));
    for (int i = 0; i < nlines; i++) {
        if (_match[i >> 3] & (1 << (i & 7))) {
            int lo = i - k;
            if (lo < 0) lo = 0;
            int hi = i + k;
            if (hi >= nlines) hi = nlines - 1;
            for (int j = lo; j <= hi; j++) {
                _out[j >> 3] |= (u8)(1 << (j & 7));
            }
        }
    }

    // === Pass 2: re-walk BASON, output selected lines ===
    aBpad(u64, stk2, 256);
    call(BASONOpen, stk2, bason_data);
    int depth2 = 0;
    int cur_line = 0;
    b8 prev_sel = NO;
    b8 any_out = NO;

    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk2, bason_data, &type, key, val);
        if (o != OK) {
            if (depth2 <= 0) break;
            call(BASONOuto, stk2);
            depth2--;
            continue;
        }
        if (!BASONPlex(type)) {
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
                    u8cs chunk = {p, chunk_end};
                    call(u8sFeed, out, chunk);
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

// ---- BASTDiffBuild: unified diff BASON ----

// Emit all nodes from a BASON subtree with given status prefix
static ok64 BASTDiffEmitAll(u8bp out, u64bp idx,
                             u64bp stk, u8csc data,
                             u8 type, u8cs key, u8cs val,
                             u8 status) {
    sane(1);
    u8 kbuf[512];
    size_t klen = $ok(key) && !$empty(key) ? (size_t)$len(key) : 0;
    kbuf[0] = status;
    if (klen > 0) memcpy(kbuf + 1, key[0], klen);
    u8cs pkey = {kbuf, kbuf + 1 + klen};

    if (BASONPlex(type)) {
        call(BASONFeedInto, idx, out, type, pkey);
        call(BASONInto, stk, data, val);
        u8 ct = 0;
        u8cs ck = {}, cv = {};
        while (BASONDrain(stk, data, &ct, ck, cv) == OK) {
            call(BASTDiffEmitAll, out, idx, stk, data, ct, ck, cv, status);
        }
        call(BASONOuto, stk);
        call(BASONFeedOuto, idx, out);
    } else {
        call(BASONFeed, idx, out, type, pkey, val);
    }
    done;
}

// Parallel walk of old + patch at one level, emit unified diff BASON
static ok64 BASTDiffLevel(u8bp out, u64bp idx,
                           u64bp ostk, u8csc odata,
                           u64bp pstk, u8csc pdata) {
    sane(1);
    u8 ot = 0, pt = 0;
    u8cs ok = {}, ov = {}, pk = {}, pv = {};
    ok64 oo = BASONDrain(ostk, odata, &ot, ok, ov);
    ok64 po = BASONDrain(pstk, pdata, &pt, pk, pv);

    while (oo == OK && po == OK) {
        int cmp = $cmp(ok, pk);
        if (cmp < 0) {
            // old only → equal
            call(BASTDiffEmitAll, out, idx, ostk, odata, ot, ok, ov, '=');
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
        } else if (cmp > 0) {
            // patch only → added
            call(BASTDiffEmitAll, out, idx, pstk, pdata, pt, pk, pv, '+');
            po = BASONDrain(pstk, pdata, &pt, pk, pv);
        } else {
            if (pt == 'B' && $len(pv) == 0) {
                // deleted (null tombstone)
                call(BASTDiffEmitAll, out, idx, ostk, odata, ot, ok, ov, '-');
            } else if (BASONPlex(ot) && BASONPlex(pt) && ot == pt) {
                // both containers, same type → recurse
                u8 kbuf[512];
                size_t klen =
                    $ok(ok) && !$empty(ok) ? (size_t)$len(ok) : 0;
                kbuf[0] = '~';
                if (klen > 0) memcpy(kbuf + 1, ok[0], klen);
                u8cs pkey = {kbuf, kbuf + 1 + klen};
                call(BASONFeedInto, idx, out, ot, pkey);
                call(BASONInto, ostk, odata, ov);
                call(BASONInto, pstk, pdata, pv);
                call(BASTDiffLevel, out, idx, ostk, odata, pstk, pdata);
                call(BASONOuto, ostk);
                call(BASONOuto, pstk);
                call(BASONFeedOuto, idx, out);
            } else {
                // replaced: old as removed, new as added
                call(BASTDiffEmitAll, out, idx, ostk, odata, ot, ok, ov, '-');
                call(BASTDiffEmitAll, out, idx, pstk, pdata, pt, pk, pv, '+');
            }
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
            po = BASONDrain(pstk, pdata, &pt, pk, pv);
        }
    }

    while (oo == OK) {
        call(BASTDiffEmitAll, out, idx, ostk, odata, ot, ok, ov, '=');
        oo = BASONDrain(ostk, odata, &ot, ok, ov);
    }
    while (po == OK) {
        call(BASTDiffEmitAll, out, idx, pstk, pdata, pt, pk, pv, '+');
        po = BASONDrain(pstk, pdata, &pt, pk, pv);
    }
    done;
}

ok64 BASTDiffBuild(u8bp out, u64bp idx,
                   u64bp ostk, u8csc odata,
                   u64bp pstk, u8csc pdata) {
    sane(out != NULL);
    call(BASONOpen, ostk, odata);
    if ($empty(pdata)) {
        // No changes — emit everything as equal
        u8 type;
        u8cs key, val;
        while (BASONDrain(ostk, odata, &type, key, val) == OK) {
            call(BASTDiffEmitAll, out, idx, ostk, odata, type, key, val, '=');
        }
        done;
    }
    call(BASONOpen, pstk, pdata);
    call(BASTDiffLevel, out, idx, ostk, odata, pstk, pdata);
    done;
}

// ---- BASTDiffRender: colored diff with k context lines ----

ok64 BASTDiffRender(u8s out, u8cs bason_data, int k) {
    sane($ok(out) && $ok(bason_data));
    if ($empty(bason_data)) done;

    a_pad(u8, _de, 16);
    escfeedBG256(_de_idle, HILI_DEL_BG);
    u8cs DEL = {_de[1], _de[2]};
    a_pad(u8, _ae, 16);
    escfeedBG256(_ae_idle, HILI_ADD_BG);
    u8cs ADD = {_ae[1], _ae[2]};
    u8cs RST = $u8str("\033[0m");

    // === Pass 1: build line table + mark changed lines ===
    u32 _line_off[65536];
    int nlines = 1;
    _line_off[0] = 0;

    u8 _match[8192];
    memset(_match, 0, sizeof(_match));

    aBpad(u64, stk, 256);
    call(BASONOpen, stk, bason_data);
    u32 offset = 0;
    int depth = 0;

    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk, bason_data, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            call(BASONOuto, stk);
            depth--;
            continue;
        }
        if (!BASONPlex(type)) {
            u32 len = (u32)$len(val);
            u32 node_start = offset;
            for (u32 i = 0; i < len && nlines < 65536; i++) {
                if (val[0][i] == '\n') {
                    _line_off[nlines] = offset + i + 1;
                    nlines++;
                }
            }
            offset += len;
            // Mark lines with '-' or '+' status
            if (!$empty(key) && (key[0][0] == '-' || key[0][0] == '+')) {
                BASTMarkLines(_match, nlines, _line_off, node_start, offset);
            }
        } else {
            call(BASONInto, stk, bason_data, val);
            depth++;
        }
    }

    // Trim trailing empty line
    if (nlines > 1 && _line_off[nlines - 1] >= offset) nlines--;

    // === Expand match bitset by k context ===
    u8 _out[8192];
    memset(_out, 0, sizeof(_out));
    for (int i = 0; i < nlines; i++) {
        if (_match[i >> 3] & (1 << (i & 7))) {
            int lo = i - k;
            if (lo < 0) lo = 0;
            int hi = i + k;
            if (hi >= nlines) hi = nlines - 1;
            for (int j = lo; j <= hi; j++) {
                _out[j >> 3] |= (u8)(1 << (j & 7));
            }
        }
    }

    // === Pass 2: output selected lines with ANSI colors ===
    aBpad(u64, stk2, 256);
    call(BASONOpen, stk2, bason_data);
    int depth2 = 0;
    int cur_line = 0;
    b8 prev_sel = NO;
    b8 any_out = NO;

    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk2, bason_data, &type, key, val);
        if (o != OK) {
            if (depth2 <= 0) break;
            call(BASONOuto, stk2);
            depth2--;
            continue;
        }
        if (!BASONPlex(type)) {
            u8 status = (!$empty(key)) ? key[0][0] : '=';
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
                    u8cs chunk = {p, chunk_end};
                    if (status == '-') {
                        call(u8sFeed, out, DEL);
                        call(u8sFeed, out, chunk);
                        call(u8sFeed, out, RST);
                    } else if (status == '+') {
                        call(u8sFeed, out, ADD);
                        call(u8sFeed, out, chunk);
                        call(u8sFeed, out, RST);
                    } else {
                        call(u8sFeed, out, chunk);
                    }
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

// ---- BASTTextDiff: line-level diff with k context ----

#define BASTDIFF_MAXLINES 4096

// Greedy line matching: for each old line, find first matching new line
// (in order). Produces an ordered matching (LCS-like).
static void BASTMatchLines(int *match, u8cs *old, int oldn,
                           u8cs *new_l, int newn) {
    int ni = 0;
    for (int oi = 0; oi < oldn; oi++) {
        match[oi] = -1;
        size_t olen = (size_t)$len(old[oi]);
        for (int j = ni; j < newn; j++) {
            if ((size_t)$len(new_l[j]) == olen &&
                memcmp(old[oi][0], new_l[j][0], olen) == 0) {
                match[oi] = j;
                ni = j + 1;
                break;
            }
        }
    }
}

ok64 BASTTextDiff(u8s out, u8cs old_text, u8cs new_text, int k) {
    sane($ok(out));

    a_pad(u8, _de, 16);
    escfeedBG256(_de_idle, HILI_DEL_BG);
    u8cs DEL = {_de[1], _de[2]};
    a_pad(u8, _ae, 16);
    escfeedBG256(_ae_idle, HILI_ADD_BG);
    u8cs ADD = {_ae[1], _ae[2]};
    u8cs RST = $u8str("\033[0m");
    u8cs SEP = $u8str("\033[34m--\033[0m\n");

    // Split into lines
    u8cs old_lines[BASTDIFF_MAXLINES];
    u8cs new_lines[BASTDIFF_MAXLINES];
    int oldn = 0, newn = 0;

    if ($ok(old_text) && !$empty(old_text)) {
        u8cp p = old_text[0];
        while (p < old_text[1] && oldn < BASTDIFF_MAXLINES) {
            u8cp nl = memchr(p, '\n', (size_t)(old_text[1] - p));
            u8cp end = nl ? nl + 1 : old_text[1];
            old_lines[oldn][0] = p;
            old_lines[oldn][1] = end;
            oldn++;
            p = end;
        }
    }

    if ($ok(new_text) && !$empty(new_text)) {
        u8cp p = new_text[0];
        while (p < new_text[1] && newn < BASTDIFF_MAXLINES) {
            u8cp nl = memchr(p, '\n', (size_t)(new_text[1] - p));
            u8cp end = nl ? nl + 1 : new_text[1];
            new_lines[newn][0] = p;
            new_lines[newn][1] = end;
            newn++;
            p = end;
        }
    }

    // Greedy matching
    int match[BASTDIFF_MAXLINES];
    BASTMatchLines(match, old_lines, oldn, new_lines, newn);

    // Build edit script: 'E' equal, 'D' delete, 'A' add
    u8 ops[BASTDIFF_MAXLINES * 2];
    u8cs elines[BASTDIFF_MAXLINES * 2];
    int nops = 0;
    int ni = 0;

    for (int oi = 0; oi < oldn; oi++) {
        if (match[oi] >= 0) {
            // New lines ni..match[oi]-1 are additions
            while (ni < match[oi] && nops < BASTDIFF_MAXLINES * 2) {
                ops[nops] = 'A';
                $mv(elines[nops], new_lines[ni]);
                nops++;
                ni++;
            }
            if (nops < BASTDIFF_MAXLINES * 2) {
                ops[nops] = 'E';
                $mv(elines[nops], old_lines[oi]);
                nops++;
            }
            ni = match[oi] + 1;
        } else {
            if (nops < BASTDIFF_MAXLINES * 2) {
                ops[nops] = 'D';
                $mv(elines[nops], old_lines[oi]);
                nops++;
            }
        }
    }
    // Remaining new lines
    while (ni < newn && nops < BASTDIFF_MAXLINES * 2) {
        ops[nops] = 'A';
        $mv(elines[nops], new_lines[ni]);
        nops++;
        ni++;
    }

    // Build change bitset and expand by k context
    u8 _changed[BASTDIFF_MAXLINES * 2 / 8];
    memset(_changed, 0, sizeof(_changed));
    for (int i = 0; i < nops; i++) {
        if (ops[i] != 'E')
            _changed[i >> 3] |= (u8)(1 << (i & 7));
    }

    u8 _show[BASTDIFF_MAXLINES * 2 / 8];
    memset(_show, 0, sizeof(_show));
    for (int i = 0; i < nops; i++) {
        if (_changed[i >> 3] & (1 << (i & 7))) {
            int lo = i - k;
            if (lo < 0) lo = 0;
            int hi = i + k;
            if (hi >= nops) hi = nops - 1;
            for (int j = lo; j <= hi; j++)
                _show[j >> 3] |= (u8)(1 << (j & 7));
        }
    }

    // Output selected lines with ANSI colors
    b8 prev_sel = NO;
    b8 any_out = NO;
    for (int i = 0; i < nops; i++) {
        b8 sel = (_show[i >> 3] >> (i & 7)) & 1;
        if (sel) {
            if (!prev_sel && any_out) {
                call(u8sFeed, out, SEP);
            }
            if (ops[i] == 'D') {
                call(u8sFeed, out, DEL);
                call(u8sFeed, out, elines[i]);
                call(u8sFeed, out, RST);
            } else if (ops[i] == 'A') {
                call(u8sFeed, out, ADD);
                call(u8sFeed, out, elines[i]);
                call(u8sFeed, out, RST);
            } else {
                call(u8sFeed, out, elines[i]);
            }
            any_out = YES;
        }
        prev_sel = sel;
    }

    done;
}

// ---- Grep: trigram-accelerated search ----

// Extract RON64 trigrams from query string, returns count
static int BEGrepTrigrams(u8cs query, u8cs trigrams[256]) {
    int tric = 0;
    if ($len(query) < 3) return 0;
    u8cp p = query[0];
    u8cp end = query[1] - 2;
    while (p <= end && tric < 256) {
        if (BETriChar(p[0]) && BETriChar(p[1]) && BETriChar(p[2])) {
            trigrams[tric][0] = p;
            trigrams[tric][1] = p + 3;
            tric++;
        }
        p++;
    }
    return tric;
}

// Lookup each trigram key, decode BASON hashlet objects, intersect into bitset
static ok64 BEGrepBitset(BEp be, u8 bitset[512], u8cs *trigrams, int tric) {
    sane(be != NULL && tric > 0);
    memset(bitset, 0xFF, 512);
    a_cstr(sch_tri, BE_SCHEME_TRI);
    for (int t = 0; t < tric; t++) {
        a_uri(tri_key, sch_tri, 0, be->loc.path, trigrams[t], 0);

        aBpad(u8, tbuf, 4096);
        ok64 go = ROCKGet(&be->db, tbuf, tri_key);
        if (go != OK) {
            done;  // trigram not in index -> no matches
        }

        u8 tri_bits[512];
        memset(tri_bits, 0, sizeof(tri_bits));
        u8cs tval = {tbuf[1], tbuf[2]};
        aBpad(u64, tstk, 32);
        call(BASONOpen, tstk, tval);
        u8 ttype = 0;
        u8cs tkey = {};
        u8cs tvv = {};
        // Enter the Object container
        if (BASONDrain(tstk, tval, &ttype, tkey, tvv) == OK &&
            ttype == 'O') {
            call(BASONInto, tstk, tval, tvv);
            while (BASONDrain(tstk, tval, &ttype, tkey, tvv) == OK) {
                if ($len(tkey) >= 2) {
                    u16 bucket = (u16)(RON64_REV[tkey[0][0]] |
                                       (RON64_REV[tkey[0][1]] << 6));
                    tri_bits[bucket >> 3] |= (1 << (bucket & 7));
                }
            }
            call(BASONOuto, tstk);
        }

        for (int i = 0; i < 512; i++) {
            bitset[i] &= tri_bits[i];
        }
    }
    done;
}

// BEScanStatMerged callback context for grep
typedef struct {
    BEp be;
    u8cs query;
    u8 *bitset;        // NULL = no filtering
    BEGrepCBf result_cb;
    voidp result_arg;
} BEGrepStatCtx;

// Stat-only callback: filter by trigram bitset, then point-get survivors
static ok64 BEGrepStatCB(voidp arg, u8cs relpath, BEmeta merged) {
    ok64 __ = OK;
    BEGrepStatCtx *ctx = (BEGrepStatCtx *)arg;
    BEp be = ctx->be;

    // Trigram bitset check: compute hashlet, skip if not in bitset
    if (ctx->bitset != NULL) {
        u8 fpbuf[256];
        u8s fps = {fpbuf, fpbuf + sizeof(fpbuf)};
        __ = u8sFeed(fps, be->loc.path);
        if (__ != OK) return __;
        u8sFeed1(fps, '/');
        __ = u8sFeed(fps, relpath);
        if (__ != OK) return __;
        u8cs full_path = {fpbuf, fps[0]};

        u8 hlbuf[4];
        u8s hls = {hlbuf, hlbuf + sizeof(hlbuf)};
        __ = BEHashlet(hls, full_path);
        if (__ != OK) return __;
        u16 bucket =
            (u16)(RON64_REV[hlbuf[0]] | (RON64_REV[hlbuf[1]] << 6));
        if (!(ctx->bitset[bucket >> 3] & (1 << (bucket & 7)))) {
            return OK;  // filtered out by trigram index
        }
    }

    // Point-get: merge be: content for this file only
    u8bp mbuf = be->scratch[BE_PATCH];
    u8bReset(mbuf);
    ok64 go = BEGetFileMerged(be, be->loc.path, relpath, mbuf, NULL);
    if (go != OK) return OK;  // skip on error
    u8cp m0 = mbuf[1], m1 = mbuf[2];
    u8cs bason = {m0, m1};
    if ($empty(bason)) return OK;

    // Export BASON to text
    u8bp rbuf = be->scratch[BE_RENDER];
    u8bReset(rbuf);
    aBpad(u64, stk, 256);
    if (BASTExport(u8bIdle(rbuf), stk, bason) != OK) return OK;
    u8cp t0 = rbuf[1], t1 = rbuf[2];
    u8cs txt = {t0, t1};
    size_t qlen = $len(ctx->query);
    if ($len(txt) < qlen) return OK;

    // Search for substring matches line by line
    u8cp ls = txt[0];
    int ln = 1;
    while (ls < txt[1]) {
        u8cp le = memchr(ls, '\n', (size_t)(txt[1] - ls));
        u8cp chunk_end = le ? le : txt[1];
        size_t ll = (size_t)(chunk_end - ls);
        if (ll >= qlen) {
            u8cp sp = ls;
            u8cp se = ls + ll - qlen + 1;
            while (sp <= se) {
                if (memcmp(sp, ctx->query[0], qlen) == 0) {
                    u8cs line = {ls, chunk_end};
                    ctx->result_cb(ctx->result_arg, relpath, ln, line);
                    break;
                }
                sp++;
            }
        }
        ls = le ? le + 1 : chunk_end;
        ln++;
    }
    return OK;
}

ok64 BEGrep(BEp be, uricp grep_uri, BEGrepCBf result_cb, voidp arg) {
    sane(be != NULL && grep_uri != NULL && result_cb != NULL);

    u8cs query = {grep_uri->fragment[0], grep_uri->fragment[1]};
    test($ok(query) && !$empty(query), BEBAD);

    // Extract trigrams from search pattern
    u8cs trigrams[256];
    int tric = BEGrepTrigrams(query, trigrams);

    if (tric == 0) {
        // No trigrams — full scan (stat + point-get every file)
        u8cs empty_pfx = {};
        BEGrepStatCtx ctx = {be, {query[0], query[1]},
                              NULL, result_cb, arg};
        call(BEScanStatMerged, be, empty_pfx, BEGrepStatCB, &ctx);
    } else {
        // Build bitset from trigram index, then filtered stat scan
        u8 bitset[512];
        call(BEGrepBitset, be, bitset, trigrams, tric);
        u8cs empty_pfx = {};
        BEGrepStatCtx ctx = {be, {query[0], query[1]},
                              bitset, result_cb, arg};
        call(BEScanStatMerged, be, empty_pfx, BEGrepStatCB, &ctx);
    }
    done;
}
