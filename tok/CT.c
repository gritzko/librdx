#include "CT.h"

#include "abc/PRO.h"

static const char *CT_KEYWORDS[] = {
    "auto",     "break",    "case",     "char",     "const",
    "continue", "default",  "do",       "double",   "else",
    "enum",     "extern",   "float",    "for",      "goto",
    "if",       "inline",   "int",      "long",     "register",
    "return",   "restrict", "short",    "signed",   "sizeof",
    "static",   "struct",   "switch",   "typedef",  "union",
    "unsigned", "void",     "volatile", "while",
    "_Alignas", "_Alignof", "_Atomic",  "_Bool",
    "_Complex", "_Generic", "_Imaginary",
    "_Noreturn","_Static_assert", "_Thread_local",
    "alignas",  "alignof",  "bool",     "static_assert",
    "thread_local", "typeof", "typeof_unqual",
    "nullptr",  "true",     "false",
    "constexpr",
    "NULL",     "TRUE",     "FALSE",
    NULL,
};

static b8 CTIsKeyword(u8cs tok) {
    u64 len = u8csLen(tok);
    for (const char **kw = CT_KEYWORDS; *kw; ++kw) {
        u64 kwlen = 0;
        const char *k = *kw;
        while (k[kwlen]) ++kwlen;
        if (kwlen != len) continue;
        if (__builtin_memcmp(tok[0], k, len) == 0) return YES;
    }
    return NO;
}

ok64 CTonComment(u8cs tok, CTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('D', tok, state->ctx);
    done;
}

ok64 CTonString(u8cs tok, CTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('G', tok, state->ctx);
    done;
}

ok64 CTonNumber(u8cs tok, CTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('L', tok, state->ctx);
    done;
}

ok64 CTonPreproc(u8cs tok, CTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('H', tok, state->ctx);
    done;
}

ok64 CTonWord(u8cs tok, CTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) {
        u8 tag = CTIsKeyword(tok) ? 'R' : 'S';
        return state->cb(tag, tok, state->ctx);
    }
    done;
}

ok64 CTonPunct(u8cs tok, CTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('P', tok, state->ctx);
    done;
}

ok64 CTonSpace(u8cs tok, CTstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->cb) return state->cb('S', tok, state->ctx);
    done;
}

ok64 CTontok(u8cs tok, CTstate *state) { return OK; }
ok64 CTonRoot(u8cs tok, CTstate *state) { return OK; }
