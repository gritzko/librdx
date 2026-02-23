#include "JDR.h"

#include "RDX.h"
#include "abc/01.h"
#include "abc/PRO.h"
#include "abc/UTF8.h"

// JDR writer: bulk is the output buffer, formatting flags in opt
#define JDR_X64(x) ((u64)(uintptr_t)(x)->opt)
#define JDR_X64_SET(x, v) ((x)->opt = (u8p)(uintptr_t)(v))

ok64 rdxIntoJDR(rdxp c, rdxp p) {
    sane(c && p && p->type);
    u8 seek_type = c->type;
    rdx seek_key = *c;
    c->format = p->flags;
    c->next = (u8p)p->plexc[0];
    c->opt = (u8p)p->plexc[1];
    c->ptype = p->type;
    c->loc = 0;
    c->flags = 0;
    if (!seek_type) {
        zero(c->r);
    } else if (p->type == RDX_TYPE_ROOT || p->type == RDX_TYPE_TUPLE ||
               (p->type == RDX_TYPE_LINEAR && seek_key.type == RDX_TYPE_INT)) {
        // For positional containers (root, tuple) or LINEAR with INT index
        ok64 o;
        if (seek_key.type == RDX_TYPE_INT) {
            // Integer index: scan to position N
            // For LINEAR: skip tombstones (visible position)
            // For TUPLE/ROOT: literal position (tombstones count)
            i64 pos = seek_key.i;
            i64 count = -1;
            while (count < pos) {
                o = rdxNextJDR(c);
                if (o != OK) return NONE;
                // Only skip tombstones for LINEAR arrays
                if (p->type == RDX_TYPE_LINEAR && (c->id.seq & 1)) continue;
                count++;
            }
        } else if (seek_key.type >= RDX_TYPE_PLEX_LEN) {
            // Non-INT non-plex: scan for matching element
            while (OK == (o = rdxNextJDR(c))) {
                if (c->type != seek_key.type) continue;
                // Equal if neither is less than other
                if (!rdx1Z(c, &seek_key) && !rdx1Z(&seek_key, c)) break;
            }
            if (o != OK) return NONE;
        } else {
            // Plex type as search key in OP container - not supported
            return NONE;
        }
    } else {
        // Seek to matching element using ptype comparator
        // Iterate while c < key, stop when c >= key
        rdxz Z = ZTABLE[p->type];
        ok64 o;
        while (OK == (o = rdxNextJDR(c))) {
            if (!Z(c, &seek_key)) break;
        }
        if (o != OK && o != END) fail(o);
        // Not found: either END (exhausted) or key < c (overshot)
        if (o == END || Z(&seek_key, c)) return NONE;
    }
    done;
}

ok64 rdxOutoJDR(rdxp c, rdxp p) {
    sane(p);
    if (c) p->next = c->next;
    if (p->loc & 1) {
        p->loc += 2;
    } else {
        p->loc += 1;
    }
    done;
}

ok64 rdxSkipJDR(rdxp x) {
    sane(x && rdxTypePlex(x));
    rdx c = {};
    call(rdxIntoJDR, &c, x);
    ok64 o;
    while (NEXT == (o = JDRLexer(&c))) {
        if (rdxTypePlex(&c)) call(rdxSkipJDR, &c);
    }
    if (o != END && o != OK && o != BACK) {
        fail(o);
    }
    call(rdxOutoJDR, &c, x);
    done;
}

ok64 RDXutf8sFeedID(utf8s into, id128cp ref);
ok64 rdxFeedStamp(u8s into, id128cp id) {
    sane(into && id);
    if (id128Empty(id)) return OK;
    call(u8sFeed1, into, '@');
    if (id->src) {
        call(RONutf8sFeed, into, ron60Max & id->src);
        utf8sFeed1(into, '-');
    }
    call(RONutf8sFeed, into, ron60Max & id->seq);
    done;
}

ok64 UTF8EscapeAll(u8s into, u8cs from);

ok64 rdxWriteNextJDR(rdxp x) {
    sane(x);
    u8sp into = u8bIdle(x->bulk);
    u8 level = ok64Lit(JDR_X64(x), 0);
    u8 ws = ok64Lit(JDR_X64(x), 1);
    u8 inl = ok64Lit(JDR_X64(x), 2);
    b8 pin = (x->format & ~RDX_FMT_WRITE) == RDX_FMT_JDR_PIN;
    if (x->loc != 0) {
        u8 sep = pin ? ':' : ',';
        call(u8sFeed1, into, sep);
    }
    if (!pin) {
        switch (ws) {
            case 0:
                break;
            case 1:
                if (x->loc != 0) call(u8sFeed1, into, ' ');
                break;
            case 2:
            case 4:
                if (level > 0 || x->loc > 0) {
                    call(u8sFeed1, into, '\n');
                    call(u8sFeed1xN, into, ' ', level * ws);
                }
                break;
        }
    }
    switch (x->type) {
        case 0:
            fail(ok64sub(RDXBAD,RON_j));
        case RDX_TYPE_TUPLE:
            // Inline mode: use colon notation, but not for nested tuples
            if ((inl == RON_I || inl == RON_i) && x->ptype != RDX_TYPE_TUPLE) {
                $mv(x->plex, into);
                x->flags = RDX_FMT_JDR_PIN | RDX_FMT_WRITE;
                break;
            }
            // fallthrough
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX:
            call(u8sFeed1, into, RDX_TYPE_BRACKET_OPEN[x->type]);
            call(rdxFeedStamp, into, &x->id);
            if (!id128Empty(&x->id)) call(u8sFeed1, into, ' ');
            $mv(x->plex, into);
            x->flags = RDX_FMT_JDR | RDX_FMT_WRITE;
            break;
        case RDX_TYPE_FLOAT:
            call(utf8sFeedFloat, into, &x->f);
            call(rdxFeedStamp, into, &x->id);
            break;
        case RDX_TYPE_INT:
            call(utf8sFeedInt, into, &x->i);
            call(rdxFeedStamp, into, &x->id);
            break;
        case RDX_TYPE_REF:
            call(RDXutf8sFeedID, into, &x->r);
            call(rdxFeedStamp, into, &x->id);
            break;
        case RDX_TYPE_STRING:
            call(utf8sFeed1, into, '"');
            if ((x->flags & RDX_UTF_ENC_BITS) == RDX_UTF_ENC_UTF8_ESC) {
                // Already escaped, just copy
                call(u8sFeed, into, x->s);
            } else if ((x->flags & RDX_UTF_ENC_BITS) == RDX_UTF_ENC_UTF8) {
                // Escape for JDR output
                call(UTF8EscapeAll, into, x->s);
            } else {
                fail(NOTIMPLYET);
            }
            call(utf8sFeed1, into, '"');
            call(rdxFeedStamp, into, &x->id);
            break;
        case RDX_TYPE_TERM:
            call(u8sFeed, into, x->t);
            call(rdxFeedStamp, into, &x->id);
            break;
    }
    ++x->loc;
    done;
}

ok64 rdxWriteIntoJDR(rdxp c, rdxp p) {
    sane(c && p && p->type);
    c->bulk = p->bulk;
    c->ptype = p->type;
    c->type = 0;
    c->flags = 0;
    c->loc = 0;
    JDR_X64_SET(c, JDR_X64(p) + 1);
    c->format = p->flags;
    u8 inl = ok64Lit(JDR_X64(p), 2);
    if (inl == RON_I)
        JDR_X64_SET(c, JDR_X64(c) & ~(63UL << 6));  // 'I': zero ws mode
    zero(c->r);
    done;
}

ok64 rdxWriteOutoJDR(rdxp c, rdxp p) {
    sane(c && p && c->bulk == p->bulk);
    u8sp into = u8bIdle(c->bulk);
    if (p->type) {
        b8 pin = (c->format & ~RDX_FMT_WRITE) == RDX_FMT_JDR_PIN;
        if (pin) {
            if (p->type == RDX_TYPE_TUPLE) {
                if (c->loc > 1) {
                } else if (c->loc == 1) {
                    call(u8sFeed1, into, ':');
                } else if (c->loc == 0) {
                    call(u8sFeed2, into, '(', ')');
                }
            }
        } else {
            u8 level = ok64Lit(JDR_X64(p), 0);
            u8 ws = ok64Lit(JDR_X64(c), 1);
            switch (ws) {
                case 0:
                case 1:
                    break;
                case 2:
                case 4:
                    call(u8sFeed1, into, '\n');
                    call(u8sFeed1xN, into, ' ', level * ws);
                    break;
            }
            call(u8sFeed1, into, RDX_TYPE_BRACKET_CLOSE[p->type]);
        }
    }
    done;
}

ok64 RDXutf8sFeedID(utf8s into, id128cp ref) {
    if (unlikely($len(into) < 24)) return NOROOM;
    u8* start = *into;
    RONutf8sFeed(into, ron60Max & ref->src);
    u8* after_src = *into;
    utf8sFeed1(into, '-');
    RONutf8sFeed(into, ron60Max & ref->seq);
    u8* end = *into;
    // Check if result looks like float (e.g., "1e-0", "E-5")
    // Pattern: src ends with 'e'/'E', seq starts with digit
    u8 src_last = *(after_src - 1);
    u8 seq_first = *(after_src + 1);  // skip '-'
    if ((src_last == 'e' || src_last == 'E') &&
        (seq_first >= '0' && seq_first <= '9')) {
        // Prepend "00" - floats can't have leading zeros
        size_t len = end - start;
        memmove(start + 2, start, len);
        start[0] = '0';
        start[1] = '0';
        *into += 2;
    }
    return OK;
}

ok64 RDXutf8sDrainID(utf8cs from, id128p ref) {
    a_dup(u8c, t, from);
    // Skip leading zeros (added to disambiguate from floats)
    while ($len(t) > 0 && **t == '0' && $len(t) > 1 && (*t)[1] != '-') {
        t[0]++;
    }
    u8 DELIM = '-';
    u8c* p = $u8find(t, &DELIM);
    ok64 o = OK;
    if (p == NULL) {
        test($len(t) <= 10, RONbad);
        ref->src = 0;
        o = RONutf8sDrain(&ref->seq, t);
        if (o == OK) from[0] = t[0];
    } else {
        u8cs src = {t[0], p};
        u8cs time = {p + 1, t[1]};
        test($len(src) <= 10 && $len(time) <= 10, RONbad);
        o = RONutf8sDrain(&ref->src, src);
        if (o == OK) o = RONutf8sDrain(&ref->seq, time);
        if (o == OK) from[0] = t[1];
    }
    return o;
}

const u8 RDX_TYPE_LIT_REV[128] = {
    ['P'] = RDX_TYPE_TUPLE,  ['L'] = RDX_TYPE_LINEAR, ['E'] = RDX_TYPE_EULER,
    ['X'] = RDX_TYPE_MULTIX, ['F'] = RDX_TYPE_FLOAT,  ['I'] = RDX_TYPE_INT,
    ['R'] = RDX_TYPE_REF,    ['S'] = RDX_TYPE_STRING, ['T'] = RDX_TYPE_TERM,
    ['f'] = RDX_TYPE_FLOAT,  ['i'] = RDX_TYPE_INT,    ['r'] = RDX_TYPE_REF,
    ['s'] = RDX_TYPE_STRING, ['t'] = RDX_TYPE_TERM,
};

const u8 RDX_TYPE_BRACKET_REV[] = {
    ['('] = RDX_TYPE_TUPLE,  ['['] = RDX_TYPE_LINEAR, ['{'] = RDX_TYPE_EULER,
    ['<'] = RDX_TYPE_MULTIX, [')'] = RDX_TYPE_TUPLE,  [']'] = RDX_TYPE_LINEAR,
    ['}'] = RDX_TYPE_EULER,  ['>'] = RDX_TYPE_MULTIX,
};
