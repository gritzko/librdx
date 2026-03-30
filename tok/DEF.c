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
static const char *JA_DEF_KW[] = {
    "class", "interface", "enum", "record", NULL};

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
        u8 tag = TOK_TAG(tok);
        u8cs val;
        TOK_VAL(val, toks, base, i);

        if (tag == 'D') continue;
        if (tag == 'S' && DEFIsWs(val)) continue;

        u8 byte;
        switch (tag) {
            case 'R':
                byte = (defkw && DEFIsDefKw(val, defkw)) ? 'f'
                     : DEFIsFlowKw(val)                  ? 'k'
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
    if (TOK_TAG(tok) == 'S')
        toks[0][idx] = TOK_PACK(DEF_TAG, TOK_OFF(tok));
}

// ============================================================
//  Pattern matching: find definitions in enriched stream
// ============================================================

// Try NFA pattern anchored at position i. Returns match length or 0.
// Pattern must end with .* to consume the tail (NFAu8Match is whole-string).
static u32 DEFTryMatch(nfau8cs nfa, u8c *enr, u32 len, u32 at) {
    u8cs text = {enr + at, enr + len};
    u32 wbuf[3 * 4096];
    u32 *ws[2] = {wbuf, wbuf + sizeof(wbuf) / sizeof(wbuf[0])};
    u16 n = NFAu8States(nfa);
    if (n == 0 || $len(ws) < 3 * (u64)n) return 0;
    return NFAu8Search(nfa, text, ws) ? 1 : 0;
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

// ============================================================
//  C-family: definition = s before ( at depth 0, ) then {;
//  Also: struct/enum/union/class NAME {;
// ============================================================

// C function def patterns (appended with .* for whole-string match):
//   simple:  s+s[(][^(){};]*[)][{;].*
//   nested:  s+s[(][^(){};]*[(][^)]*[)][^(){};]*[)][{;].*
//   struct:  fs[{;].*

static ok64 DEFMarkC(u32 **toks, DEFenr *e) {
    sane(e != NULL);
    nfau8 nb1[512], nb2[512], nb3[512];
    u32 pb[2048];
    u32 *pws[2] = {pb, pb + 2048};

    // fn def: type+ name( args ) { or ;
    // simple args (no nested parens)
    u8 p1[] = "[rs*]+s[(][^(){};]*[)][{;].*";
    nfau8g pr1 = {nb1, nb1 + 512, nb1};
    ok64 o = DEFCompile(pr1, (u8cs){p1, p1 + sizeof(p1) - 1}, pws);
    if (o != OK) return o;
    nfau8cs nfa1 = {pr1[2], pr1[0]};

    // nested fn ptr in args: type+ name([^(){};]*([^)]*)[^(){};]*)[{;]
    u8 p2[] = "[rs*]+s[(][^(){};]*[(][^)]*[)][^(){};]*[)][{;].*";
    nfau8g pr2 = {nb2, nb2 + 512, nb2};
    o = DEFCompile(pr2, (u8cs){p2, p2 + sizeof(p2) - 1}, pws);
    if (o != OK) return o;
    nfau8cs nfa2 = {pr2[2], pr2[0]};

    // struct/enum/union/class NAME {;
    u8 p3[] = "fs[{;].*";
    nfau8g pr3 = {nb3, nb3 + 512, nb3};
    o = DEFCompile(pr3, (u8cs){p3, p3 + sizeof(p3) - 1}, pws);
    if (o != OK) return o;
    nfau8cs nfa3 = {pr3[2], pr3[0]};

    b8 fresh = YES;
    for (u32 i = 0; i < e->len; i++) {
        u8 ch = e->enr[i];
        if (ch == ';' || ch == '{' || ch == '}') {
            fresh = YES;
            continue;
        }
        if (ch == '=' || ch == 'k') {
            fresh = NO;
            continue;
        }

        // struct/enum/union/class NAME {;
        if (ch == 'f' && i + 1 < e->len && e->enr[i + 1] == 's') {
            if (DEFTryMatch(nfa3, e->enr, e->len, i))
                DEFRetag(toks, e->map[i + 1]);
            continue;
        }

        // function def: s( at a fresh statement position
        if (ch == 's' && fresh && i + 1 < e->len && e->enr[i + 1] == '(') {
            // scan back for start of type prefix to anchor NFA
            u32 start = i;
            while (start > 0 && (e->enr[start - 1] == 's' ||
                                  e->enr[start - 1] == 'r' ||
                                  e->enr[start - 1] == '*'))
                start--;
            if (start < i &&
                (DEFTryMatch(nfa1, e->enr, e->len, start) ||
                 DEFTryMatch(nfa2, e->enr, e->len, start)))
                DEFRetag(toks, e->map[i]);
        }
    }
    done;
}

// ============================================================
//  Keyword-prefix languages: Go, Python, Rust, JS, Java
//  Pattern: f s ... (defining keyword followed by name)
// ============================================================

static ok64 DEFMarkKwPrefix(u32 **toks, DEFenr *e) {
    sane(e != NULL);
    for (u32 i = 0; i + 1 < e->len; i++) {
        if (e->enr[i] == 'f' && e->enr[i + 1] == 's')
            DEFRetag(toks, e->map[i + 1]);
    }
    done;
}

// ============================================================
//  Language dispatch
// ============================================================

typedef struct {
    const char *ext;
    const char *const *defkw;
    b8 cfamily;  // use C-style fn detection
} DEFlang;

static const DEFlang DEF_LANGS[] = {
    {"c", C_DEF_KW, YES},
    {"h", C_DEF_KW, YES},
    {"cc", C_DEF_KW, YES},
    {"cpp", C_DEF_KW, YES},
    {"cxx", C_DEF_KW, YES},
    {"hpp", C_DEF_KW, YES},
    {"hh", C_DEF_KW, YES},
    {"hxx", C_DEF_KW, YES},
    {"go", GO_DEF_KW, NO},
    {"py", PY_DEF_KW, NO},
    {"rs", RS_DEF_KW, NO},
    {"js", JS_DEF_KW, NO},
    {"jsx", JS_DEF_KW, NO},
    {"ts", JS_DEF_KW, NO},
    {"tsx", JS_DEF_KW, NO},
    {"java", JA_DEF_KW, YES},
    {"cs", JA_DEF_KW, YES},
    {"kt", JA_DEF_KW, NO},
    {"swift", JS_DEF_KW, NO},
    {"dart", JS_DEF_KW, NO},
    {"zig", RS_DEF_KW, NO},
    {NULL, NULL, NO},
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
    if (!lang) return OK;  // unknown language, nothing to mark

    u32 ntoks = (u32)$len(toks);
    u32 cap = ntoks;  // enriched can't be larger than token count

    // stack-allocate for small files, heap for large
    u8 enr_stack[8192];
    u32 map_stack[8192];
    u8 *enr_buf = (cap <= 8192) ? enr_stack : (u8 *)malloc(cap);
    u32 *map_buf = (cap <= 8192) ? map_stack : (u32 *)malloc(cap * sizeof(u32));
    if (!enr_buf || !map_buf) return NFANOROOM;

    DEFenr e = {.enr = enr_buf, .map = map_buf, .len = 0, .cap = cap};

    u32 const *ctoks[2] = {toks[0], toks[1]};
    ok64 o = DEFEnrich(&e, ctoks, data, lang->defkw);
    if (o != OK) goto cleanup;

    if (lang->cfamily)
        o = DEFMarkC(toks, &e);
    else
        o = DEFMarkKwPrefix(toks, &e);

cleanup:
    if (enr_buf != enr_stack) free(enr_buf);
    if (map_buf != map_stack) free(map_buf);
    return o;
}
