//
// DEF.c — Mark symbol definitions in tokenized files
//

#include "DEF.h"

#include "abc/NFA.h"
#include "abc/PRO.h"

// ============================================================
//  Language keyword tables
// ============================================================

static const char *C_DEF_KW[] = {
    "struct", "union", "enum", "typedef", "class", "namespace", NULL};
static const char *GO_DEF_KW[] = {"func", "type", "var", "const", NULL};
static const char *PY_DEF_KW[] = {"def", "class", NULL};
static const char *RS_DEF_KW[] = {
    "fn", "struct", "enum", "trait", "type", "mod", "const", "static", NULL};
static const char *JS_DEF_KW[] = {
    "function", "class", "const", "let", "var", NULL};
static const char *TS_DEF_KW[] = {
    "function", "class", "interface", "type", "enum", "const", "let", "var",
    NULL};
static const char *JA_DEF_KW[] = {
    "class", "interface", "enum", "record", NULL};
static const char *KT_DEF_KW[] = {
    "fun", "class", "interface", "enum", "object", "val", "var", NULL};
static const char *SW_DEF_KW[] = {
    "func", "class", "struct", "enum", "protocol", "let", "var", NULL};
static const char *DA_DEF_KW[] = {
    "class", "enum", "mixin", "extension", NULL};

// ============================================================
//  Definition patterns per language (table-driven)
// ============================================================
//
// Enriched alphabet:
//   s = identifier       f = defining keyword     t = typedef
//   r = other keyword    k = flow keyword         g = string
//   l = number literal   h = preprocessor
//   ( ) { } ; = , : . * < > = punctuation         p = other punct
//
// In patterns, uppercase S marks the identifier to retag as definition.
// After a successful NFA match, the name is located by S context:
//   - '}' before S in pattern → first 's' after outermost '}'
//   - lowercase letter before S → first 's' after that letter
//   - otherwise → last 's' before the first char/class after S
//
// All patterns end with .* (NFAu8Match requires full-string match).
// fresh=YES means the rule only fires at statement-start positions
// (after ; { } or stream start; cleared by = and flow keywords).

typedef struct {
    const char *regex;
    b8 fresh;
} DEFrule;

// C/C++: function defs, struct/enum/class names, typedefs, #defines
static const DEFrule C_RULES[] = {
    {"[rs*<>]+S[(][^(){};]*[)][{;].*",                         YES},
    {"[rs*<>]+S[(][^(){};]*[(][^)]*[)][^(){};]*[)][{;].*",     YES},
    {"fS[:{;].*",                                               NO},
    {"t[f]*[{].*[}][*,]*S[;].*",                               NO},
    {"t[frs*]+[(][*,rs]*S[)][^;]*[;].*",                       NO},
    {"t[frs*]+S[;].*",                                          NO},
    {"hS.*",                                                    NO},
    {NULL, NO},
};

// Java/C#: C-like + permissive class/record names (no struct-ptr issue)
static const DEFrule JA_RULES[] = {
    {"[rs*<>]+S[(][^(){};]*[)][{;].*",                         YES},
    {"[rs*<>]+S[(][^(){};]*[(][^)]*[)][^(){};]*[)][{;].*",     YES},
    {"fS[^;]*[{;].*",                                          NO},
    {NULL, NO},
};

// Go: func Name + method receivers func (r *T) Name
static const DEFrule GO_RULES[] = {
    {"fS.*",                NO},  // func Name, type Name, var/const name
    {"f[(][^)]*[)]S.*",     NO},  // func (r *Type) Method()
    {NULL, NO},
};

// Dart: C-like return-type functions + keyword-prefix classes
static const DEFrule DA_RULES[] = {
    {"[rs*]+S[(][^(){};]*[)][{;].*",     YES},  // void name() {}
    {"fS[^;]*[{;].*",                    NO},   // class Name ... {}
    {NULL, NO},
};

// Keyword-prefix languages: func/def/type/class Name
static const DEFrule KW_RULES[] = {
    {"fS.*", NO},
    {NULL, NO},
};

// ============================================================
//  Enrichment: token array → compact u8 stream + index map
// ============================================================

static b8 DEFIsWs(u8csc val) {
    for (u8c const *p = val[0]; p < val[1]; p++)
        if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
            return NO;
    return YES;
}

static const char *FLOW_KW[] = {"if",     "else",   "for",    "while",
                                "do",     "switch", "case",   "return",
                                "goto",   "break",  "continue", NULL};

static b8 DEFIsFlowKw(u8csc val) {
    u64 len = $len(val);
    for (const char *const *kw = FLOW_KW; *kw; ++kw) {
        u64 kwlen = 0;
        const char *k = *kw;
        while (k[kwlen]) ++kwlen;
        if (kwlen == len && __builtin_memcmp(val[0], k, len) == 0) return YES;
    }
    return NO;
}

static b8 DEFIsDefKw(u8csc val, const char *const *kws) {
    u64 len = $len(val);
    for (const char *const *kw = kws; *kw; ++kw) {
        u64 kwlen = 0;
        const char *k = *kw;
        while (k[kwlen]) ++kwlen;
        if (kwlen == len && __builtin_memcmp(val[0], k, len) == 0) return YES;
    }
    return NO;
}

static b8 DEFIsTypedef(u8csc val) {
    return $len(val) == 7 && __builtin_memcmp(val[0], "typedef", 7) == 0;
}

static u8 DEFPunctByte(u8csc val) {
    if ($len(val) == 1) {
        u8 ch = *val[0];
        switch (ch) {
            case '(':
            case ')':
            case '{':
            case '}':
            case ';':
            case '=':
            case ',':
            case ':':
            case '.':
            case '*':
            case '<':
            case '>':
            case '[':
            case ']':
            case '&':
            case '?':
                return ch;
        }
    }
    return 'p';
}

typedef struct {
    u8 *enr;      // enriched byte stream (allocated)
    u32 *map;     // map[i] = token index for enriched byte i
    u32 len;      // length of enriched stream
    u32 cap;      // capacity
} DEFenr;

static ok64 DEFEnrich(DEFenr *e, u32 const *const *toks, u8csc data,
                       const char *const *defkw) {
    sane(e != NULL);
    u32 ntoks = (u32)$len(toks);
    u8c *base = data[0];
    for (u32 i = 0; i < ntoks; i++) {
        u32 tok = toks[0][i];
        u8 tag = tok32Tag(tok);
        u8cs val;
        tok32Val(val,toks,base,i);

        if (tag == 'D') continue;
        if (tag == 'S' && DEFIsWs(val)) continue;

        u8 byte;
        switch (tag) {
            case 'R':
                byte = DEFIsTypedef(val)                  ? 't'
                     : (defkw && DEFIsDefKw(val, defkw))  ? 'f'
                     : DEFIsFlowKw(val)                   ? 'k'
                                                          : 'r';
                break;
            case 'S':
                byte = 's';
                break;
            case 'P':
                byte = DEFPunctByte(val);
                break;
            case 'G':
                byte = 'g';
                break;
            case 'L':
                byte = 'l';
                break;
            case 'H':
                byte = 'h';
                break;
            default:
                byte = '?';
                break;
        }

        if (e->len >= e->cap) return NFANOROOM;
        e->enr[e->len] = byte;
        e->map[e->len] = i;
        e->len++;
    }
    done;
}

// ============================================================
//  Re-tag a token S → N
// ============================================================

static void DEFRetag(u32 **toks, u32 idx) {
    u32 tok = toks[0][idx];
    if (tok32Tag(tok) == 'S')
        toks[0][idx] = tok32Pack(DEF_TAG,tok32Offset(tok));
}

// ============================================================
//  Pattern matching: find definitions in enriched stream
// ============================================================

// Try NFA pattern anchored at position i. Uses prefix match: stops as
// soon as the pattern reaches MATCH state, no need for trailing .* .
static u32 DEFTryMatch(nfau8cs nfa, u8c *enr, u32 len, u32 at) {
    u8cs text = {enr + at, enr + len};
    u32 wbuf[3 * 4096];
    u32 *ws[2] = {wbuf, wbuf + sizeof(wbuf) / sizeof(wbuf[0])};
    u16 n = NFAu8States(nfa);
    if (n == 0 || $len(ws) < 3 * (u64)n) return 0;
    return NFAu8MatchPrefix(nfa, text, ws) ? 1 : 0;
}

// Find last 's' at or before position `end` in enriched stream,
// scanning back no further than `start`.
static i64 DEFLastIdent(u8c *enr, u32 start, u32 end) {
    for (i64 j = (i64)end; j >= (i64)start; j--)
        if (enr[j] == 's') return j;
    return -1;
}

// Compile one NFA pattern, returns nstates (0 on failure)
static ok64 DEFCompile(nfau8g prog, u8csc pat, u32 *ws[2]) {
    u8cs mpat = {(u8c *)pat[0], (u8c *)pat[1]};
    return NFAu8Compile(prog, mpat, ws);
}

// --- Auto-derive name-finding rule from S position in pattern ---

static void DEFParseNameRule(const char *regex, u8 *anchor, b8 *after) {
    const char *s = regex;
    while (*s && *s != 'S') s++;
    if (*s != 'S') { *anchor = ';'; *after = NO; return; }

    // '}' before S → first 's' after '}' (typedef struct{} Name;)
    for (const char *p = regex; p < s; p++) {
        if (*p == '}') { *anchor = '}'; *after = YES; return; }
        if (*p == '[') {
            p++;
            while (*p && *p != ']') {
                if (*p == '}') { *anchor = '}'; *after = YES; return; }
                p++;
            }
        }
    }

    // concrete char before S → first 's' after that char
    char pre = (s > regex) ? *(s - 1) : 0;
    if (pre == ']') {
        // extract first char from [...] class
        const char *q = s - 2;
        while (q > regex && *q != '[') q--;
        if (*q == '[') {
            q++;
            if (*q == '^') q++;
            pre = *q;
        }
    }
    if (pre && pre != '.' && pre != '*' && pre != '+' && pre != '?'
        && pre != '[' && pre != '^') {
        *anchor = (u8)pre;
        *after = YES;
        return;
    }

    // char/class after S → last 's' before first such char
    const char *a = s + 1;
    if (*a == '[') {
        a++;
        *anchor = (*a && *a != ']') ? (u8)*a : ';';
    } else if (*a && *a != '.' && *a != '*' && *a != '+' && *a != '?') {
        *anchor = (u8)*a;
    } else {
        *anchor = ';';
    }
    *after = NO;
}

// --- Find the definition name after a successful NFA match ---

static i64 DEFFindName(u8c *enr, u32 start, u32 len, u8 anchor, b8 after) {
    if (after) {
        if (anchor == ')') {
            // depth-aware: first 's' after first ')' at paren depth 0
            i32 depth = 0;
            for (u32 j = start; j < len; j++) {
                if (enr[j] == '(') depth++;
                else if (enr[j] == ')') {
                    if (--depth <= 0) {
                        for (u32 k = j + 1; k < len; k++) {
                            if (enr[k] == 's') return (i64)k;
                            if (enr[k] == ';' || enr[k] == '{') break;
                        }
                        return -1;
                    }
                }
            }
            return -1;
        }
        if (anchor == '}') {
            // first 's' after LAST '}' (depth: nested structs)
            i64 last_anch = -1;
            for (u32 j = start; j < len; j++) {
                if (enr[j] == '}') last_anch = (i64)j;
                else if (last_anch >= 0 && (enr[j] == ';' || enr[j] == '{'))
                    break;
            }
            if (last_anch >= 0) {
                for (u32 j = (u32)(last_anch + 1); j < len; j++) {
                    if (enr[j] == 's') return (i64)j;
                    if (enr[j] == ';' || enr[j] == '{') break;
                }
            }
            return -1;
        }
        // first 's' after FIRST 'anchor'
        for (u32 j = start; j < len; j++) {
            if (enr[j] == anchor) {
                for (u32 k = j + 1; k < len; k++) {
                    if (enr[k] == 's') return (i64)k;
                    if (enr[k] == ';' || enr[k] == '{') break;
                }
                return -1;
            }
        }
        return -1;
    }
    // last 's' before first 'anchor'
    for (u32 j = start; j < len; j++) {
        if (enr[j] == anchor)
            return DEFLastIdent(enr, start, j > 0 ? j - 1 : 0);
    }
    return -1;
}

// --- Unified table-driven matcher ---

#define DEF_MAX_RULES 8
#define DEF_NFA_CAP 256

static ok64 DEFMarkRules(u32 **toks, DEFenr *e, const DEFrule *rules) {
    sane(e != NULL && rules != NULL);

    u32 nrules = 0;
    while (rules[nrules].regex) nrules++;
    if (nrules > DEF_MAX_RULES) nrules = DEF_MAX_RULES;

    nfau8 nbufs[DEF_MAX_RULES][DEF_NFA_CAP];
    nfau8c *nfa0[DEF_MAX_RULES], *nfa1[DEF_MAX_RULES];
    u8 anchors[DEF_MAX_RULES];
    b8 afters[DEF_MAX_RULES], freshs[DEF_MAX_RULES], valids[DEF_MAX_RULES];

    u32 pb[4096];
    u32 *pws[2] = {pb, pb + 4096};

    for (u32 r = 0; r < nrules; r++) {
        freshs[r] = rules[r].fresh;
        valids[r] = NO;
        DEFParseNameRule(rules[r].regex, &anchors[r], &afters[r]);

        // S → s for NFA compilation; strip trailing .* (prefix match)
        u8 pat[256];
        u32 plen = 0;
        for (const char *p = rules[r].regex; *p && plen < sizeof(pat) - 1; p++)
            pat[plen++] = (*p == 'S') ? 's' : (u8)*p;
        if (plen >= 2 && pat[plen - 2] == '.' && pat[plen - 1] == '*')
            plen -= 2;

        nfau8g pr = {nbufs[r], nbufs[r] + DEF_NFA_CAP, nbufs[r]};
        ok64 o = DEFCompile(pr, (u8cs){pat, pat + plen}, pws);
        if (o == OK) {
            nfa0[r] = (nfau8c *)pr[2];
            nfa1[r] = (nfau8c *)pr[0];
            valids[r] = YES;
        }
    }

    b8 fresh = YES;
    for (u32 i = 0; i < e->len; i++) {
        u8 ch = e->enr[i];
        if (ch == ';' || ch == '{' || ch == '}') { fresh = YES; continue; }
        if (ch == '=' || ch == 'k') { fresh = NO; continue; }

        for (u32 r = 0; r < nrules; r++) {
            if (!valids[r] || (freshs[r] && !fresh)) continue;
            nfau8cs nfa = {nfa0[r], nfa1[r]};
            if (!DEFTryMatch(nfa, e->enr, e->len, i)) continue;

            i64 name = DEFFindName(e->enr, i, e->len, anchors[r], afters[r]);
            if (name >= 0) DEFRetag(toks, e->map[name]);
            break;
        }
    }
    done;
}

// ============================================================
//  Language dispatch
// ============================================================

typedef struct {
    const char *ext;
    const char *const *defkw;
    const DEFrule *rules;
} DEFlang;

static const DEFlang DEF_LANGS[] = {
    {"c", C_DEF_KW, C_RULES},
    {"h", C_DEF_KW, C_RULES},
    {"cc", C_DEF_KW, C_RULES},
    {"cpp", C_DEF_KW, C_RULES},
    {"cxx", C_DEF_KW, C_RULES},
    {"hpp", C_DEF_KW, C_RULES},
    {"hh", C_DEF_KW, C_RULES},
    {"hxx", C_DEF_KW, C_RULES},
    {"go", GO_DEF_KW, GO_RULES},
    {"py", PY_DEF_KW, KW_RULES},
    {"rs", RS_DEF_KW, KW_RULES},
    {"js", JS_DEF_KW, KW_RULES},
    {"jsx", JS_DEF_KW, KW_RULES},
    {"ts", TS_DEF_KW, KW_RULES},
    {"tsx", TS_DEF_KW, KW_RULES},
    {"java", JA_DEF_KW, JA_RULES},
    {"cs", JA_DEF_KW, JA_RULES},
    {"kt", KT_DEF_KW, KW_RULES},
    {"swift", SW_DEF_KW, KW_RULES},
    {"dart", DA_DEF_KW, DA_RULES},
    {"zig", RS_DEF_KW, KW_RULES},
    {NULL, NULL, NULL},
};

static const DEFlang *DEFLookup(u8csc ext) {
    u64 elen = $len(ext);
    for (const DEFlang *l = DEF_LANGS; l->ext; l++) {
        u64 llen = 0;
        while (l->ext[llen]) llen++;
        if (llen == elen && __builtin_memcmp(ext[0], l->ext, llen) == 0)
            return l;
    }
    return NULL;
}

ok64 DEFMark(u32 *toks[2], u8csc data, u8csc ext) {
    sane($ok(toks) && $ok(data));
    const DEFlang *lang = DEFLookup(ext);
    if (!lang || !lang->rules) return OK;

    u32 ntoks = (u32)$len(toks);
    u32 cap = ntoks;

    u8 enr_stack[8192];
    u32 map_stack[8192];
    u8 *enr_buf = (cap <= 8192) ? enr_stack : (u8 *)malloc(cap);
    u32 *map_buf = (cap <= 8192) ? map_stack : (u32 *)malloc(cap * sizeof(u32));
    if (!enr_buf || !map_buf) return NFANOROOM;

    DEFenr e = {.enr = enr_buf, .map = map_buf, .len = 0, .cap = cap};

    u32 const *ctoks[2] = {toks[0], toks[1]};
    ok64 o = DEFEnrich(&e, ctoks, data, lang->defkw);
    if (o != OK) goto cleanup;

    o = DEFMarkRules(toks, &e, lang->rules);

cleanup:
    if (enr_buf != enr_stack) free(enr_buf);
    if (map_buf != map_stack) free(map_buf);
    return o;
}
