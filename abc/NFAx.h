//
// NFAx.h — Thompson NFA simulation template
// Include with X(M, name) defined, e.g. #define X(M, name) M##u8##name
//
// Provides:  X(NFA, MatchPlain), X(NFA, SearchPlain), X(NFA, Add), X(NFA, Step),
//            X(NFA, Emit), X(NFA, Patch), X(NFA, Frag1), X(NFA, PMerge),
//            X(NFA, Swap), X(NFA, IsMatch), X(NFA, WorkSize)
//
// State type X(nfa,) has: u8 op, T val, u16 out, u16 out1
// The T val field holds the literal value for NFA_LIT states.
//

#include "NFA.h"

#define T X(, )
#define Tc X(, c)
#define Ts X(, s)
#define Tcs X(, cs)
#define Tcp X(, cp)

// --- NFA state parameterized on T ---

typedef struct {
    u8 op;
    T val;
    u16 out, out1;
} X(nfa, );

typedef X(nfa, ) const X(nfa, c);
typedef X(nfa, ) *X(nfa, s)[2];
typedef X(nfa, ) const *X(nfa, cs)[2];
typedef X(nfa, ) *X(nfa, g)[3];

// --- Compile-time fragment ---

typedef struct {
    u16 start, poff, plen;
} X(nfa, f);

fun u64 X(NFA, WorkSize)(u64 nstates) { return 3 * nstates; }

// --- NFA builder primitives ---

fun u16 X(NFA, Emit)(X(nfa, g) prog, u32g patch,
                      u8 op, T val, u16 out, u16 out1) {
    if (prog[1] >= prog[2]) return NFA_NONE;
    u16 id = (u16)(prog[1] - prog[0]);
    prog[1]->op = op;
    prog[1]->val = val;
    prog[1]->out = out;
    prog[1]->out1 = out1;
    prog[1]++;
    return id;
}

fun void X(NFA, Patch)(X(nfa, ) *s, u32 *pb, X(nfa, f) f, u16 target) {
    for (u16 i = 0; i < f.plen; i++) {
        u32 e = pb[f.poff + i];
        u16 sid = e >> 1;
        if (e & 1)
            s[sid].out1 = target;
        else
            s[sid].out = target;
    }
}

fun ok64 X(NFA, Frag1)(u32g patch, X(nfa, f) *f,
                        u16 start, u16 sid, u8 which) {
    if (patch[1] >= patch[2]) return NFANOROOM;
    f->start = start;
    f->poff = (u16)(patch[1] - patch[0]);
    f->plen = 1;
    *patch[1]++ = ((u32)sid << 1) | which;
    return OK;
}

fun ok64 X(NFA, PMerge)(u32g patch, X(nfa, f) *r,
                         u16 start, X(nfa, f) a, X(nfa, f) b) {
    u16 total = a.plen + b.plen;
    if (patch[1] + total > patch[2]) return NFANOROOM;
    r->start = start;
    r->poff = (u16)(patch[1] - patch[0]);
    r->plen = total;
    for (u16 i = 0; i < a.plen; i++)
        *patch[1]++ = patch[0][a.poff + i];
    for (u16 i = 0; i < b.plen; i++)
        *patch[1]++ = patch[0][b.poff + i];
    return OK;
}

fun void X(NFA, Swap)(X(nfa, ) *s, u16 n, u16 si) {
    if (si == 0) return;
    X(nfa, ) tmp = s[0];
    s[0] = s[si];
    s[si] = tmp;
    for (u16 i = 0; i < n; i++) {
        u16 o = s[i].out, o1 = s[i].out1;
        if (o == si)
            s[i].out = 0;
        else if (o == 0)
            s[i].out = si;
        if (o1 == si)
            s[i].out1 = 0;
        else if (o1 == 0)
            s[i].out1 = si;
    }
}

// --- Thompson NFA simulation ---

fun void X(NFA, Add)(X(nfa, c) *s, u16 nstates, u16 *list, u16 *len,
                     u32 *gen, u32 gid, u16 sid, u64 pos, u64 tlen) {
    if (sid == NFA_NONE) return;
    if (gen[sid] == gid) return;
    gen[sid] = gid;
    u8 op = s[sid].op;
    if (op == NFA_SPLIT) {
        X(NFA, Add)(s, nstates, list, len, gen, gid, s[sid].out, pos, tlen);
        X(NFA, Add)(s, nstates, list, len, gen, gid, s[sid].out1, pos, tlen);
        return;
    }
    if (op == NFA_BOL) {
        if (pos == 0)
            X(NFA, Add)(s, nstates, list, len, gen, gid, s[sid].out, pos, tlen);
        return;
    }
    if (op == NFA_EOL) {
        if (pos == tlen)
            X(NFA, Add)(s, nstates, list, len, gen, gid, s[sid].out, pos, tlen);
        return;
    }
    list[(*len)++] = sid;
}

fun void X(NFA, Step)(X(nfa, c) *s, u16 nstates, u16 *cl, u16 clen,
                      u16 *nl, u16 *nlen,
                      u32 *gen, u32 *gid, Tc c, u64 pos, u64 tlen) {
    ++(*gid);
    *nlen = 0;
    for (u16 i = 0; i < clen; i++) {
        X(nfa, c) st = s[cl[i]];
        b8 match = NO;
        if (st.op == NFA_LIT)
            match = (st.val == c);
        else if (st.op == NFA_ANY)
            match = YES;
        if (match)
            X(NFA, Add)(s, nstates, nl, nlen, gen, *gid, st.out, pos, tlen);
    }
}

fun b8 X(NFA, IsMatch)(X(nfa, c) *s, u16 *list, u16 len) {
    for (u16 i = 0; i < len; i++)
        if (s[list[i]].op == NFA_MATCH) return YES;
    return NO;
}

// Anchored full match (generic, no character classes)
fun b8 X(NFA, MatchPlain)(X(nfa, cs) prog, u16 nstates, Tcs text, u32 *ws[2]) {
    u16 n = nstates;
    if (n == 0 || $len(ws) < 3 * (u64)n) return NO;
    X(nfa, c) *s = prog[0];
    u64 tlen = (u64)$len(text);

    u16 *la = (u16 *)ws[0];
    u16 *lb = (u16 *)(ws[0] + n);
    u32 *gen = ws[0] + 2 * n;
    memset(gen, 0, n * sizeof(u32));

    u16 *cl = la, *nl = lb;
    u16 clen = 0;
    u32 gid = 1;

    X(NFA, Add)(s, n, cl, &clen, gen, gid, 0, 0, tlen);

    u64 pos = 0;
    $for(Tc, cp, text) {
        X(NFA, Step)(s, n, cl, clen, nl, &clen, gen, &gid, *cp, pos + 1, tlen);
        u16 *tmp = cl;
        cl = nl;
        nl = tmp;
        pos++;
    }

    return X(NFA, IsMatch)(s, cl, clen);
}

// Unanchored search (generic, no character classes)
fun b8 X(NFA, SearchPlain)(X(nfa, cs) prog, u16 nstates, Tcs text, u32 *ws[2]) {
    u16 n = nstates;
    if (n == 0 || $len(ws) < 3 * (u64)n) return NO;
    X(nfa, c) *s = prog[0];
    u64 tlen = (u64)$len(text);

    u16 *la = (u16 *)ws[0];
    u16 *lb = (u16 *)(ws[0] + n);
    u32 *gen = ws[0] + 2 * n;
    memset(gen, 0, n * sizeof(u32));

    u16 *cl = la, *nl = lb;
    u16 clen = 0;
    u32 gid = 1;

    X(NFA, Add)(s, n, cl, &clen, gen, gid, 0, 0, tlen);
    if (X(NFA, IsMatch)(s, cl, clen)) return YES;

    u64 pos = 0;
    $for(Tc, cp, text) {
        X(NFA, Step)(s, n, cl, clen, nl, &clen, gen, &gid, *cp, pos + 1, tlen);
        X(NFA, Add)(s, n, nl, &clen, gen, gid, 0, pos + 1, tlen);
        u16 *tmp = cl;
        cl = nl;
        nl = tmp;
        pos++;
        if (X(NFA, IsMatch)(s, cl, clen)) return YES;
    }

    return NO;
}

#undef T
#undef Tc
#undef Ts
#undef Tcs
#undef Tcp
