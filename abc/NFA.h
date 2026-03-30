//
// NFA.h — Thompson NFA regex, constants + u8 regex compiler
//
// NFAx.h provides the generic simulation template.
// NFA.h instantiates it for u8 and adds regex string compilation
// with character classes, anchors, \d \w \s escapes, {n,m}.
//

#ifndef ABC_NFA_H
#define ABC_NFA_H

#include "OK.h"
#include "BUF.h"
#include "INT.h"

// --- NFA opcodes (shared by all instantiations) ---

enum {
    NFA_LIT = 0,
    NFA_ANY = 1,
    NFA_SPLIT = 2,
    NFA_MATCH = 3,
    NFA_BOL = 4,
    NFA_EOL = 5,
    NFA_CLASS = 6   // u8 only: val = index into class bitmap table
};

#define NFA_NONE 0xFFFF
#define NFA_CLASS_MAX 32
#define NFA_CLASS_BYTES 32

con ok64 NFANOROOM = 0x173ca5d86d8616;
con ok64 NFABADSYN = 0x173ca2ca35c897;

// --- Instantiate NFAx.h for u8 ---

#define X(M, name) M##u8##name
#include "NFAx.h"
#undef X

// ============================================================
//  u8 regex compiler: string -> NFA state array
// ============================================================

// Compile context (u8-specific: has character class bitmaps)
typedef struct {
    nfau8g prog;
    u8cs   data;
    u32g   patch;
    u8 cls[NFA_CLASS_MAX][NFA_CLASS_BYTES];
    u8 ncls;
} nfa8cc;

// Adapter: emit via compile context
fun u16 NFAu8CCEmit(nfa8cc *c, u8 op, u8 val, u16 out, u16 out1) {
    return NFAu8Emit(c->prog, c->patch, op, val, out, out1);
}

fun void NFAu8CCPatch(nfa8cc *c, nfau8f f, u16 target) {
    NFAu8Patch(c->prog[0], c->patch[0], f, target);
}

fun ok64 NFAu8CCFrag1(nfa8cc *c, nfau8f *f, u16 start, u16 sid, u8 which) {
    return NFAu8Frag1(c->patch, f, start, sid, which);
}

fun ok64 NFAu8CCPMerge(nfa8cc *c, nfau8f *r, u16 start, nfau8f a, nfau8f b) {
    return NFAu8PMerge(c->patch, r, start, a, b);
}

// --- Character class compilation ---

fun void NFAu8ClsSet(u8 *bmp, u8 ch) { bmp[ch >> 3] |= (1 << (ch & 7)); }

fun ok64 NFAu8Class(nfa8cc *c, nfau8f *f) {
    if (c->ncls >= NFA_CLASS_MAX) return NFANOROOM;
    u8 ci = c->ncls++;
    u8 *bmp = c->cls[ci];
    memset(bmp, 0, NFA_CLASS_BYTES);
    b8 neg = NO;

    u8csUsed1(c->data);
    if (!u8csEmpty(c->data) && *c->data[0] == '^') {
        neg = YES;
        u8csUsed1(c->data);
    }
    if (!u8csEmpty(c->data) && *c->data[0] == ']') {
        NFAu8ClsSet(bmp, ']');
        u8csUsed1(c->data);
    }
    while (!u8csEmpty(c->data) && *c->data[0] != ']') {
        u8 lo = *c->data[0]++;
        if (lo == '\\' && !u8csEmpty(c->data)) {
            u8 esc = *c->data[0]++;
            if (esc == 'd') {
                for (u8 ch = '0'; ch <= '9'; ch++) NFAu8ClsSet(bmp, ch);
                continue;
            } else if (esc == 'D') {
                for (u16 ch = 0; ch < 256; ch++)
                    if (ch < '0' || ch > '9') NFAu8ClsSet(bmp, (u8)ch);
                continue;
            } else if (esc == 'w') {
                for (u8 ch = '0'; ch <= '9'; ch++) NFAu8ClsSet(bmp, ch);
                for (u8 ch = 'A'; ch <= 'Z'; ch++) NFAu8ClsSet(bmp, ch);
                for (u8 ch = 'a'; ch <= 'z'; ch++) NFAu8ClsSet(bmp, ch);
                NFAu8ClsSet(bmp, '_');
                continue;
            } else if (esc == 'W') {
                for (u16 ch = 0; ch < 256; ch++)
                    if (!((ch >= '0' && ch <= '9') ||
                          (ch >= 'A' && ch <= 'Z') ||
                          (ch >= 'a' && ch <= 'z') || ch == '_'))
                        NFAu8ClsSet(bmp, (u8)ch);
                continue;
            } else if (esc == 's') {
                NFAu8ClsSet(bmp, ' ');
                NFAu8ClsSet(bmp, '\t');
                NFAu8ClsSet(bmp, '\n');
                NFAu8ClsSet(bmp, '\r');
                NFAu8ClsSet(bmp, '\f');
                NFAu8ClsSet(bmp, '\v');
                continue;
            } else if (esc == 'S') {
                for (u16 ch = 0; ch < 256; ch++)
                    if (ch != ' ' && ch != '\t' && ch != '\n' &&
                        ch != '\r' && ch != '\f' && ch != '\v')
                        NFAu8ClsSet(bmp, (u8)ch);
                continue;
            } else if (esc == 'n') {
                lo = '\n';
            } else if (esc == 't') {
                lo = '\t';
            } else if (esc == 'r') {
                lo = '\r';
            } else if (esc == 'f') {
                lo = '\f';
            } else if (esc == 'a') {
                lo = '\a';
            } else if (esc == 'e') {
                lo = 0x1b;
            } else if (esc == 'b') {
                lo = '\b';
            } else if (esc >= '0' && esc <= '7') {
                lo = esc - '0';
                if (!u8csEmpty(c->data) && *c->data[0] >= '0' &&
                    *c->data[0] <= '7')
                    lo = lo * 8 + (*c->data[0]++ - '0');
                if (!u8csEmpty(c->data) && *c->data[0] >= '0' &&
                    *c->data[0] <= '7')
                    lo = lo * 8 + (*c->data[0]++ - '0');
            } else {
                lo = esc;
            }
        }
        if (u8csLen(c->data) > 1 && c->data[0][0] == '-' &&
            c->data[0][1] != ']') {
            c->data[0]++;
            u8 hi = *c->data[0]++;
            if (hi == '\\' && !u8csEmpty(c->data)) {
                u8 esc = *c->data[0]++;
                if (esc == 'n')
                    hi = '\n';
                else if (esc == 't')
                    hi = '\t';
                else if (esc == 'r')
                    hi = '\r';
                else if (esc >= '0' && esc <= '7') {
                    hi = esc - '0';
                    if (!u8csEmpty(c->data) && *c->data[0] >= '0' &&
                        *c->data[0] <= '7')
                        hi = hi * 8 + (*c->data[0]++ - '0');
                    if (!u8csEmpty(c->data) && *c->data[0] >= '0' &&
                        *c->data[0] <= '7')
                        hi = hi * 8 + (*c->data[0]++ - '0');
                } else {
                    hi = esc;
                }
            }
            for (u16 ch = lo; ch <= hi; ch++) NFAu8ClsSet(bmp, (u8)ch);
        } else {
            NFAu8ClsSet(bmp, lo);
        }
    }
    if (u8csEmpty(c->data)) return NFABADSYN;
    u8csUsed1(c->data);

    if (neg)
        for (int i = 0; i < NFA_CLASS_BYTES; i++) bmp[i] = ~bmp[i];

    u16 sid = NFAu8CCEmit(c, NFA_CLASS, ci, NFA_NONE, NFA_NONE);
    if (sid == NFA_NONE) return NFANOROOM;
    return NFAu8CCFrag1(c, f, sid, sid, 0);
}

// --- Recursive descent parser ---

static ok64 NFAu8RAlt(nfa8cc *c, nfau8f *f);

fun ok64 NFAu8RAtom(nfa8cc *c, nfau8f *f) {
    if (u8csEmpty(c->data)) return NFABADSYN;
    u8 ch = *c->data[0];

    if (ch == '(') {
        u8csUsed1(c->data);
        ok64 o = NFAu8RAlt(c, f);
        if (o != OK) return o;
        if (u8csEmpty(c->data) || *c->data[0] != ')') return NFABADSYN;
        u8csUsed1(c->data);
        return OK;
    }
    if (ch == '[') return NFAu8Class(c, f);
    if (ch == '\\') {
        u8csUsed1(c->data);
        if (u8csEmpty(c->data)) return NFABADSYN;
        u8 esc = *c->data[0]++;
        if (esc == 'd' || esc == 'D' || esc == 'w' || esc == 'W' ||
            esc == 's' || esc == 'S') {
            if (c->ncls >= NFA_CLASS_MAX) return NFANOROOM;
            u8 ci = c->ncls++;
            u8 *bmp = c->cls[ci];
            memset(bmp, 0, NFA_CLASS_BYTES);
            if (esc == 'd' || esc == 'D') {
                for (u8 ch2 = '0'; ch2 <= '9'; ch2++) NFAu8ClsSet(bmp, ch2);
            } else if (esc == 'w' || esc == 'W') {
                for (u8 ch2 = '0'; ch2 <= '9'; ch2++) NFAu8ClsSet(bmp, ch2);
                for (u8 ch2 = 'A'; ch2 <= 'Z'; ch2++) NFAu8ClsSet(bmp, ch2);
                for (u8 ch2 = 'a'; ch2 <= 'z'; ch2++) NFAu8ClsSet(bmp, ch2);
                NFAu8ClsSet(bmp, '_');
            } else {
                NFAu8ClsSet(bmp, ' ');
                NFAu8ClsSet(bmp, '\t');
                NFAu8ClsSet(bmp, '\n');
                NFAu8ClsSet(bmp, '\r');
                NFAu8ClsSet(bmp, '\f');
                NFAu8ClsSet(bmp, '\v');
            }
            if (esc == 'D' || esc == 'W' || esc == 'S')
                for (int i = 0; i < NFA_CLASS_BYTES; i++) bmp[i] = ~bmp[i];
            u16 sid = NFAu8CCEmit(c, NFA_CLASS, ci, NFA_NONE, NFA_NONE);
            if (sid == NFA_NONE) return NFANOROOM;
            return NFAu8CCFrag1(c, f, sid, sid, 0);
        }
        u16 sid = NFAu8CCEmit(c, NFA_LIT, esc, NFA_NONE, NFA_NONE);
        if (sid == NFA_NONE) return NFANOROOM;
        return NFAu8CCFrag1(c, f, sid, sid, 0);
    }
    if (ch == '.') {
        u8csUsed1(c->data);
        u16 sid = NFAu8CCEmit(c, NFA_ANY, 0, NFA_NONE, NFA_NONE);
        if (sid == NFA_NONE) return NFANOROOM;
        return NFAu8CCFrag1(c, f, sid, sid, 0);
    }
    if (ch == '^') {
        u8csUsed1(c->data);
        u16 sid = NFAu8CCEmit(c, NFA_BOL, 0, NFA_NONE, NFA_NONE);
        if (sid == NFA_NONE) return NFANOROOM;
        return NFAu8CCFrag1(c, f, sid, sid, 0);
    }
    if (ch == '$') {
        u8csUsed1(c->data);
        u16 sid = NFAu8CCEmit(c, NFA_EOL, 0, NFA_NONE, NFA_NONE);
        if (sid == NFA_NONE) return NFANOROOM;
        return NFAu8CCFrag1(c, f, sid, sid, 0);
    }
    if (ch == '|' || ch == ')' || ch == '*' || ch == '+' || ch == '?')
        return NFABADSYN;

    u8csUsed1(c->data);
    u16 sid = NFAu8CCEmit(c, NFA_LIT, ch, NFA_NONE, NFA_NONE);
    if (sid == NFA_NONE) return NFANOROOM;
    return NFAu8CCFrag1(c, f, sid, sid, 0);
}

// Parse {n}, {n,}, {n,m}, {,m}
fun ok64 NFAu8RCounted(nfa8cc *c, u16 *nmin, u16 *nmax) {
    a_dup(u8c, save, c->data);
    u8csUsed1(c->data);
    while (!u8csEmpty(c->data) && *c->data[0] == ' ') u8csUsed1(c->data);
    *nmin = 0;
    *nmax = 0;
    b8 has_nmin = NO;
    while (!u8csEmpty(c->data) && *c->data[0] >= '0' && *c->data[0] <= '9') {
        *nmin = *nmin * 10 + (*c->data[0]++ - '0');
        has_nmin = YES;
    }
    while (!u8csEmpty(c->data) && *c->data[0] == ' ') u8csUsed1(c->data);
    if (u8csEmpty(c->data)) { u8csMv(c->data, save); return NFABADSYN; }
    if (*c->data[0] == '}') {
        if (!has_nmin) { u8csMv(c->data, save); return NFABADSYN; }
        u8csUsed1(c->data);
        *nmax = *nmin;
        return OK;
    }
    if (*c->data[0] != ',') { u8csMv(c->data, save); return NFABADSYN; }
    u8csUsed1(c->data);
    while (!u8csEmpty(c->data) && *c->data[0] == ' ') u8csUsed1(c->data);
    b8 has_nmax = NO;
    if (!u8csEmpty(c->data) && *c->data[0] == '}') {
        if (!has_nmin) { u8csMv(c->data, save); return NFABADSYN; }
        u8csUsed1(c->data);
        *nmax = 0xFFFF;
        return OK;
    }
    while (!u8csEmpty(c->data) && *c->data[0] >= '0' && *c->data[0] <= '9') {
        *nmax = *nmax * 10 + (*c->data[0]++ - '0');
        has_nmax = YES;
    }
    while (!u8csEmpty(c->data) && *c->data[0] == ' ') u8csUsed1(c->data);
    if (u8csEmpty(c->data) || *c->data[0] != '}') {
        u8csMv(c->data, save);
        return NFABADSYN;
    }
    u8csUsed1(c->data);
    if (!has_nmin && !has_nmax) { u8csMv(c->data, save); return NFABADSYN; }
    if (!has_nmin) *nmin = 0;
    if (!has_nmax) *nmax = 0xFFFF;
    return OK;
}

fun ok64 NFAu8RRep(nfa8cc *c, nfau8f *f) {
    u8c *atom_start = c->data[0];
    ok64 o = NFAu8RAtom(c, f);
    if (o != OK) return o;

    if (u8csEmpty(c->data)) return OK;

    u8 ch = *c->data[0];
    if (ch == '*') {
        u8csUsed1(c->data);
        u16 sid = NFAu8CCEmit(c, NFA_SPLIT, 0, f->start, NFA_NONE);
        if (sid == NFA_NONE) return NFANOROOM;
        NFAu8CCPatch(c, *f, sid);
        return NFAu8CCFrag1(c, f, sid, sid, 1);
    }
    if (ch == '+') {
        u8csUsed1(c->data);
        u16 sid = NFAu8CCEmit(c, NFA_SPLIT, 0, f->start, NFA_NONE);
        if (sid == NFA_NONE) return NFANOROOM;
        NFAu8CCPatch(c, *f, sid);
        f->poff = (u16)(c->patch[1] - c->patch[0]);
        f->plen = 0;
        return NFAu8CCFrag1(c, f, f->start, sid, 1);
    }
    if (ch == '?') {
        u8csUsed1(c->data);
        u16 sid = NFAu8CCEmit(c, NFA_SPLIT, 0, f->start, NFA_NONE);
        if (sid == NFA_NONE) return NFANOROOM;
        nfau8f skip;
        o = NFAu8CCFrag1(c, &skip, sid, sid, 1);
        if (o != OK) return o;
        return NFAu8CCPMerge(c, f, sid, *f, skip);
    }
    if (ch == '{') {
        u16 nmin, nmax;
        o = NFAu8RCounted(c, &nmin, &nmax);
        if (o != OK) return OK;
        if (nmax != 0xFFFF && nmax < nmin) return NFABADSYN;
        u8c *brace_end = c->data[0];

        if (nmin == 0 && nmax == 0) {
            u16 sid = NFAu8CCEmit(c, NFA_SPLIT, 0, NFA_NONE, NFA_NONE);
            if (sid == NFA_NONE) return NFANOROOM;
            o = NFAu8CCFrag1(c, f, sid, sid, 0);
            if (o != OK) return o;
            c->data[0] = brace_end;
            return OK;
        }

        if (nmin == 0) {
            nfau8f result, fi, skip;
            c->data[0] = atom_start;
            o = NFAu8RAtom(c, &fi);
            if (o != OK) return o;
            u16 sid = NFAu8CCEmit(c, NFA_SPLIT, 0, fi.start, NFA_NONE);
            if (sid == NFA_NONE) return NFANOROOM;
            o = NFAu8CCFrag1(c, &skip, sid, sid, 1);
            if (o != OK) return o;
            o = NFAu8CCPMerge(c, &result, sid, fi, skip);
            if (o != OK) return o;

            u16 limit = (nmax == 0xFFFF) ? 1 : nmax;
            for (u16 i = 1; i < limit; i++) {
                c->data[0] = atom_start;
                o = NFAu8RAtom(c, &fi);
                if (o != OK) return o;
                sid = NFAu8CCEmit(c, NFA_SPLIT, 0, fi.start, NFA_NONE);
                if (sid == NFA_NONE) return NFANOROOM;
                NFAu8CCPatch(c, result, sid);
                o = NFAu8CCFrag1(c, &skip, sid, sid, 1);
                if (o != OK) return o;
                o = NFAu8CCPMerge(c, &result, result.start, fi, skip);
                if (o != OK) return o;
            }

            if (nmax == 0xFFFF) {
                c->data[0] = atom_start;
                o = NFAu8RAtom(c, &fi);
                if (o != OK) return o;
                sid = NFAu8CCEmit(c, NFA_SPLIT, 0, fi.start, NFA_NONE);
                if (sid == NFA_NONE) return NFANOROOM;
                NFAu8CCPatch(c, fi, sid);
                NFAu8CCPatch(c, result, sid);
                result.poff = (u16)(c->patch[1] - c->patch[0]);
                result.plen = 0;
                o = NFAu8CCFrag1(c, &result, result.start, sid, 1);
                if (o != OK) return o;
            }

            *f = result;
            c->data[0] = brace_end;
            return OK;
        }

        nfau8f result = *f;
        for (u16 i = 1; i < nmin; i++) {
            nfau8f fi;
            c->data[0] = atom_start;
            o = NFAu8RAtom(c, &fi);
            if (o != OK) return o;
            NFAu8CCPatch(c, result, fi.start);
            result.poff = fi.poff;
            result.plen = fi.plen;
        }

        if (nmax == 0xFFFF) {
            nfau8f fi;
            c->data[0] = atom_start;
            o = NFAu8RAtom(c, &fi);
            if (o != OK) return o;
            u16 sid = NFAu8CCEmit(c, NFA_SPLIT, 0, fi.start, NFA_NONE);
            if (sid == NFA_NONE) return NFANOROOM;
            NFAu8CCPatch(c, fi, sid);
            NFAu8CCPatch(c, result, sid);
            result.poff = (u16)(c->patch[1] - c->patch[0]);
            result.plen = 0;
            o = NFAu8CCFrag1(c, &result, result.start, sid, 1);
            if (o != OK) return o;
        } else {
            for (u16 i = nmin; i < nmax; i++) {
                nfau8f fi, skip;
                c->data[0] = atom_start;
                o = NFAu8RAtom(c, &fi);
                if (o != OK) return o;
                u16 sid = NFAu8CCEmit(c, NFA_SPLIT, 0, fi.start, NFA_NONE);
                if (sid == NFA_NONE) return NFANOROOM;
                NFAu8CCPatch(c, result, sid);
                o = NFAu8CCFrag1(c, &skip, sid, sid, 1);
                if (o != OK) return o;
                o = NFAu8CCPMerge(c, &result, result.start, fi, skip);
                if (o != OK) return o;
            }
        }

        *f = result;
        c->data[0] = brace_end;
        return OK;
    }
    return OK;
}

fun b8 NFAu8RIsAtom(u8 ch) {
    return ch != '|' && ch != ')' && ch != '*' && ch != '+' && ch != '?';
}

fun ok64 NFAu8RSeq(nfa8cc *c, nfau8f *f) {
    if (u8csEmpty(c->data) || !NFAu8RIsAtom(*c->data[0]))
        return NFABADSYN;

    ok64 o = NFAu8RRep(c, f);
    if (o != OK) return o;

    while (!u8csEmpty(c->data) && NFAu8RIsAtom(*c->data[0])) {
        nfau8f f2;
        o = NFAu8RRep(c, &f2);
        if (o != OK) return o;
        NFAu8CCPatch(c, *f, f2.start);
        f->poff = f2.poff;
        f->plen = f2.plen;
    }
    return OK;
}

fun ok64 NFAu8RAlt(nfa8cc *c, nfau8f *f) {
    ok64 o = NFAu8RSeq(c, f);
    if (o != OK) return o;

    while (!u8csEmpty(c->data) && *c->data[0] == '|') {
        u8csUsed1(c->data);
        nfau8f f2;
        o = NFAu8RSeq(c, &f2);
        if (o != OK) return o;
        u16 sid = NFAu8CCEmit(c, NFA_SPLIT, 0, f->start, f2.start);
        if (sid == NFA_NONE) return NFANOROOM;
        o = NFAu8CCPMerge(c, f, sid, *f, f2);
        if (o != OK) return o;
    }
    return OK;
}

// ============================================================
//  Public u8 API: compile + u8-specific match wrappers
// ============================================================

fun ok64 NFAu8Compile(nfau8g prog, u8cs pat, u32 *ws[2]) {
    nfa8cc c = {
        .prog = {prog[2], prog[2], prog[1]},
        .data = {pat[0], pat[1]},
        .patch = {ws[0], ws[0], ws[1]},
        .ncls = 0};

    nfau8f f;
    ok64 o = NFAu8RAlt(&c, &f);
    if (o != OK) return o;
    if (!u8csEmpty(c.data)) return NFABADSYN;

    u16 n = (u16)(c.prog[1] - c.prog[0]);
    u16 mid = NFAu8Emit(c.prog, c.patch, NFA_MATCH, 0, NFA_NONE, NFA_NONE);
    if (mid == NFA_NONE) return NFANOROOM;
    NFAu8Patch(c.prog[0], c.patch[0], f, mid);
    n = (u16)(c.prog[1] - c.prog[0]);
    NFAu8Swap(c.prog[0], n, f.start);

    // class bitmaps stored after states; ncls in MATCH state's val
    u16 cls_slots =
        (c.ncls * NFA_CLASS_BYTES + sizeof(nfau8) - 1) / sizeof(nfau8);
    if (c.prog[1] + cls_slots > c.prog[2]) return NFANOROOM;
    memcpy(c.prog[1], c.cls, c.ncls * NFA_CLASS_BYTES);
    for (u16 i = 0; i < n; i++) {
        if (c.prog[0][i].op == NFA_MATCH) {
            c.prog[0][i].val = c.ncls;
            break;
        }
    }

    prog[0] = c.prog[1] + cls_slots;
    return OK;
}

// Recover nstates from compiled u8 NFA (accounts for class bitmaps)
fun u16 NFAu8States(nfau8cs prog) {
    u16 total = (u16)$len(prog);
    for (u16 i = 0; i < total; i++) {
        if (prog[0][i].op == NFA_MATCH) {
            u8 ncls = prog[0][i].val;
            u16 cls_slots =
                (ncls * NFA_CLASS_BYTES + sizeof(nfau8) - 1) / sizeof(nfau8);
            return total - cls_slots;
        }
    }
    return total;
}

fun u8c *NFAu8ClsBmp(nfau8c *s, u16 nstates, u8 ci) {
    return (u8c *)(s + nstates) + ci * NFA_CLASS_BYTES;
}

fun b8 NFAu8ClsMatch(u8c *bmp, u8 ch) {
    return (bmp[ch >> 3] >> (ch & 7)) & 1;
}

// u8-specific Step: handles NFA_CLASS and \n exclusion for NFA_ANY
fun void NFAu8StepCls(nfau8c *s, u16 nstates, u16 *cl, u16 clen,
                      u16 *nl, u16 *nlen,
                      u32 *gen, u32 *gid, u8 c, u64 pos, u64 tlen) {
    ++(*gid);
    *nlen = 0;
    for (u16 i = 0; i < clen; i++) {
        nfau8c st = s[cl[i]];
        b8 match = NO;
        switch (st.op) {
            case NFA_LIT:
                match = (st.val == c);
                break;
            case NFA_ANY:
                match = (c != '\n');
                break;
            case NFA_CLASS:
                match = NFAu8ClsMatch(NFAu8ClsBmp(s, nstates, st.val), c);
                break;
            default:
                break;
        }
        if (match)
            NFAu8Add(s, nstates, nl, nlen, gen, *gid, st.out, pos, tlen);
    }
}

// u8 anchored match (with class + \n support)
fun b8 NFAu8Match(nfau8cs prog, u8cs text, u32 *ws[2]) {
    u16 n = NFAu8States(prog);
    if (n == 0 || $len(ws) < 3 * (u64)n) return NO;
    nfau8c *s = prog[0];
    u64 tlen = (u64)$len(text);

    u16 *la = (u16 *)ws[0];
    u16 *lb = (u16 *)(ws[0] + n);
    u32 *gen = ws[0] + 2 * n;
    memset(gen, 0, n * sizeof(u32));

    u16 *cl = la, *nl = lb;
    u16 clen = 0;
    u32 gid = 1;

    NFAu8Add(s, n, cl, &clen, gen, gid, 0, 0, tlen);

    u64 pos = 0;
    $for(u8c, cp, text) {
        NFAu8StepCls(s, n, cl, clen, nl, &clen, gen, &gid, *cp, pos + 1, tlen);
        u16 *tmp = cl;
        cl = nl;
        nl = tmp;
        pos++;
    }

    return NFAu8IsMatch(s, cl, clen);
}

// u8 unanchored search (with class + \n support)
fun b8 NFAu8Search(nfau8cs prog, u8cs text, u32 *ws[2]) {
    u16 n = NFAu8States(prog);
    if (n == 0 || $len(ws) < 3 * (u64)n) return NO;
    nfau8c *s = prog[0];
    u64 tlen = (u64)$len(text);

    u16 *la = (u16 *)ws[0];
    u16 *lb = (u16 *)(ws[0] + n);
    u32 *gen = ws[0] + 2 * n;
    memset(gen, 0, n * sizeof(u32));

    u16 *cl = la, *nl = lb;
    u16 clen = 0;
    u32 gid = 1;

    NFAu8Add(s, n, cl, &clen, gen, gid, 0, 0, tlen);
    if (NFAu8IsMatch(s, cl, clen)) return YES;

    u64 pos = 0;
    $for(u8c, cp, text) {
        NFAu8StepCls(s, n, cl, clen, nl, &clen, gen, &gid, *cp, pos + 1, tlen);
        NFAu8Add(s, n, nl, &clen, gen, gid, 0, pos + 1, tlen);
        u16 *tmp = cl;
        cl = nl;
        nl = tmp;
        pos++;
        if (NFAu8IsMatch(s, cl, clen)) return YES;
    }

    return NO;
}

#endif
