#include "TOK.h"

#include "CT.h"
#include "CPPT.h"
#include "GOT.h"
#include "PYT.h"
#include "JST.h"
#include "RST.h"
#include "JAT.h"
#include "CST.h"
#include "HTMT.h"
#include "CSST.h"
#include "JSONT.h"
#include "SHT.h"
#include "RBT.h"
#include "HST.h"
#include "MLT.h"
#include "JLT.h"
#include "PHPT.h"
#include "AGDT.h"
#include "VERT.h"
#include "TST.h"
#include "KTT.h"
#include "SCLT.h"
#include "SWFT.h"
#include "DARTT.h"
#include "ZIGT.h"
#include "DT.h"
#include "LUAT.h"
#include "PRLT.h"
#include "RT.h"
#include "ELXT.h"
#include "ERLT.h"
#include "NIMT.h"
#include "NIXT.h"
#include "VIMT.h"
#include "YMLT.h"
#include "TOMLT.h"
#include "SQLT.h"
#include "GQLT.h"
#include "PRTT.h"
#include "HCLT.h"
#include "SCSST.h"
#include "LAXT.h"
#include "CLJT.h"
#include "CMKT.h"
#include "DKFT.h"
#include "FORT.h"
#include "FSHT.h"
#include "GLMT.h"
#include "GLST.h"
#include "MAKT.h"
#include "ODNT.h"
#include "PWST.h"
#include "SOLT.h"
#include "TYST.h"
#include "TXTT.h"
#include "MDT.h"
#include "MKDT.h"
#include "LLT.h"
#include "abc/PATH.h"
#include "abc/PRO.h"

fun b8 TOKIsAlpha_(u8 c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fun b8 TOKIsAlnum_(u8 c) {
    return TOKIsAlpha_(c) || (c >= '0' && c <= '9');
}

fun b8 TOKIsSpace(u8 c) {
    return c == ' ' || c == '\t' || c == '\n' ||
           c == '\r' || c == '\f' || c == '\v';
}

ok64 TOKSplitText(u8 tag, u8cs text, TOKcb cb, void *ctx) {
    u8c *p = text[0];
    u8c *e = text[1];
    u8cs sub;
    while (p < e) {
        sub[0] = p;
        u8 c = *p;
        if (TOKIsAlnum_(c)) {
            do { ++p; } while (p < e && TOKIsAlnum_(*p));
        } else if (TOKIsSpace(c)) {
            do { ++p; } while (p < e && TOKIsSpace(*p));
        } else if (c >= 0x80) {
            ++p;
            while (p < e && (*p & 0xC0) == 0x80) ++p;
        } else {
            ++p;
        }
        sub[1] = p;
        ok64 o = cb(tag, sub, ctx);
        if (o != OK) return o;
    }
    return OK;
}

typedef ok64 (*TOKfn)(TOKstate *state);

typedef struct {
    const char *ext;
    TOKfn lexer;
} TOKentry;

static const TOKentry TOK_TABLE[] = {
    {"c",          (TOKfn)CTLexer},
    {"h",          (TOKfn)CTLexer},
    {"cpp",        (TOKfn)CPPTLexer},
    {"cc",         (TOKfn)CPPTLexer},
    {"cxx",        (TOKfn)CPPTLexer},
    {"hpp",        (TOKfn)CPPTLexer},
    {"hh",         (TOKfn)CPPTLexer},
    {"hxx",        (TOKfn)CPPTLexer},
    {"go",         (TOKfn)GOTLexer},
    {"py",         (TOKfn)PYTLexer},
    {"js",         (TOKfn)JSTLexer},
    {"jsx",        (TOKfn)JSTLexer},
    {"mjs",        (TOKfn)JSTLexer},
    {"ts",         (TOKfn)TSTLexer},
    {"tsx",        (TOKfn)TSTLexer},
    {"rs",         (TOKfn)RSTLexer},
    {"java",       (TOKfn)JATLexer},
    {"kt",         (TOKfn)KTTLexer},
    {"kts",        (TOKfn)KTTLexer},
    {"scala",      (TOKfn)SCLTLexer},
    {"sc",         (TOKfn)SCLTLexer},
    {"cs",         (TOKfn)CSTLexer},
    {"fs",         (TOKfn)FSHTLexer},
    {"fsi",        (TOKfn)FSHTLexer},
    {"fsx",        (TOKfn)FSHTLexer},
    {"swift",      (TOKfn)SWFTLexer},
    {"dart",       (TOKfn)DARTTLexer},
    {"d",          (TOKfn)DTLexer},
    {"zig",        (TOKfn)ZIGTLexer},
    {"html",       (TOKfn)HTMTLexer},
    {"htm",        (TOKfn)HTMTLexer},
    {"css",        (TOKfn)CSSTLexer},
    {"scss",       (TOKfn)SCSSTLexer},
    {"json",       (TOKfn)JSONTLexer},
    {"yml",        (TOKfn)YMLTLexer},
    {"yaml",       (TOKfn)YMLTLexer},
    {"toml",       (TOKfn)TOMLTLexer},
    {"sh",         (TOKfn)SHTLexer},
    {"bash",       (TOKfn)SHTLexer},
    {"rb",         (TOKfn)RBTLexer},
    {"lua",        (TOKfn)LUATLexer},
    {"pl",         (TOKfn)PRLTLexer},
    {"pm",         (TOKfn)PRLTLexer},
    {"r",          (TOKfn)RTLexer},
    {"R",          (TOKfn)RTLexer},
    {"ex",         (TOKfn)ELXTLexer},
    {"exs",        (TOKfn)ELXTLexer},
    {"erl",        (TOKfn)ERLTLexer},
    {"hrl",        (TOKfn)ERLTLexer},
    {"hs",         (TOKfn)HSTLexer},
    {"ml",         (TOKfn)MLTLexer},
    {"mli",        (TOKfn)MLTLexer},
    {"jl",         (TOKfn)JLTLexer},
    {"nim",        (TOKfn)NIMTLexer},
    {"nims",       (TOKfn)NIMTLexer},
    {"php",        (TOKfn)PHPTLexer},
    {"clj",        (TOKfn)CLJTLexer},
    {"cljs",       (TOKfn)CLJTLexer},
    {"cljc",       (TOKfn)CLJTLexer},
    {"edn",        (TOKfn)CLJTLexer},
    {"nix",        (TOKfn)NIXTLexer},
    {"sql",        (TOKfn)SQLTLexer},
    {"graphql",    (TOKfn)GQLTLexer},
    {"gql",        (TOKfn)GQLTLexer},
    {"proto",      (TOKfn)PRTTLexer},
    {"hcl",        (TOKfn)HCLTLexer},
    {"tf",         (TOKfn)HCLTLexer},
    {"tex",        (TOKfn)LAXTLexer},
    {"sty",        (TOKfn)LAXTLexer},
    {"cls",        (TOKfn)LAXTLexer},
    {"vim",        (TOKfn)VIMTLexer},
    {"cmake",      (TOKfn)CMKTLexer},
    {"dockerfile", (TOKfn)DKFTLexer},
    {"mk",         (TOKfn)MAKTLexer},
    {"f90",        (TOKfn)FORTLexer},
    {"f95",        (TOKfn)FORTLexer},
    {"f03",        (TOKfn)FORTLexer},
    {"f08",        (TOKfn)FORTLexer},
    {"glsl",       (TOKfn)GLSTLexer},
    {"vert",       (TOKfn)GLSTLexer},
    {"frag",       (TOKfn)GLSTLexer},
    {"geom",       (TOKfn)GLSTLexer},
    {"comp",       (TOKfn)GLSTLexer},
    {"gleam",      (TOKfn)GLMTLexer},
    {"odin",       (TOKfn)ODNTLexer},
    {"ps1",        (TOKfn)PWSTLexer},
    {"psm1",       (TOKfn)PWSTLexer},
    {"psd1",       (TOKfn)PWSTLexer},
    {"sol",        (TOKfn)SOLTLexer},
    {"typ",        (TOKfn)TYSTLexer},
    {"agda",       (TOKfn)AGDTLexer},
    {"v",          (TOKfn)VERTLexer},
    {"sv",         (TOKfn)VERTLexer},
    {"ll",         (TOKfn)LLTLexer},
    {"md",         (TOKfn)MDTLexer},
    {"markdown",   (TOKfn)MDTLexer},
    {"mkd",        (TOKfn)MKDTLexer},
    {"sm",         (TOKfn)MKDTLexer},
    {"txt",        (TOKfn)TXTTLexer},
    {"rst",        (TOKfn)TXTTLexer},
    {NULL,         NULL},
};

// Filename → lexer table for files whose names are their type.
static const TOKentry TOK_NAME_TABLE[] = {
    {"CMakeLists.txt", (TOKfn)CMKTLexer},
    {"Makefile",       (TOKfn)MAKTLexer},
    {"makefile",       (TOKfn)MAKTLexer},
    {"GNUmakefile",    (TOKfn)MAKTLexer},
    {"Dockerfile",     (TOKfn)DKFTLexer},
    {"Vagrantfile",    (TOKfn)RBTLexer},
    {"Gemfile",        (TOKfn)RBTLexer},
    {"Rakefile",       (TOKfn)RBTLexer},
    {"Justfile",       (TOKfn)MAKTLexer},
    {".gitignore",     (TOKfn)SHTLexer},
    {".gitattributes", (TOKfn)SHTLexer},
    {".gitmodules",    (TOKfn)TOMLTLexer},
    {".bashrc",        (TOKfn)SHTLexer},
    {".bash_profile",  (TOKfn)SHTLexer},
    {".profile",       (TOKfn)SHTLexer},
    {".zshrc",         (TOKfn)SHTLexer},
    {".vimrc",         (TOKfn)VIMTLexer},
    {".clang-format",  (TOKfn)YMLTLexer},
    {NULL,             NULL},
};

const char *TOKExtAt(int i) {
    if (i < 0) return NULL;
    int n = (int)(sizeof(TOK_TABLE) / sizeof(TOK_TABLE[0])) - 1;
    if (i >= n) return NULL;
    return TOK_TABLE[i].ext;
}

static b8 TOKSliceMatch(u8csc s, const char *pat) {
    u64 len = u8csLen(s);
    u64 plen = 0;
    while (pat[plen]) ++plen;
    if (len != plen) return NO;
    return __builtin_memcmp(s[0], pat, len) == 0;
}

// Wrappers around abc/PATH for basename and extension extraction.

// Try the name table against the basename of `path`.
static TOKfn TOKFindByName(u8csc path) {
    u8cs base = {};
    PATHu8sBase(base, path);
    if ($empty(base)) return NULL;
    for (const TOKentry *e = TOK_NAME_TABLE; e->ext != NULL; ++e)
        if (TOKSliceMatch(base, e->ext)) return e->lexer;
    return NULL;
}

// Try the ext table against the extension of `path`.
static TOKfn TOKFindByExt(u8csc path) {
    u8cs ext = {};
    PATHu8sExt(ext, path);
    if ($empty(ext)) return NULL;
    for (const TOKentry *e = TOK_TABLE; e->ext != NULL; ++e)
        if (TOKSliceMatch(ext, e->ext)) return e->lexer;
    return NULL;
}

// Resolve lexer for a path, filename, or bare extension.
// Tries: 1) name table by basename, 2) ext table by extension.
static TOKfn TOKResolve(u8csc input) {
    if ($empty(input)) return NULL;
    // If input has no dot and no slash, treat as bare ext directly.
    b8 has_dot = NO, has_slash = NO;
    $for(u8c, p, input) {
        if (*p == '.') has_dot = YES;
        if (*p == '/') has_slash = YES;
    }
    if (!has_dot && !has_slash) {
        // Bare extension like "c" or "py" — try ext table first.
        for (const TOKentry *e = TOK_TABLE; e->ext != NULL; ++e)
            if (TOKSliceMatch(input, e->ext)) return e->lexer;
        // Then name table for extensionless filenames (Makefile, etc.).
        TOKfn fn = TOKFindByName(input);
        if (fn) return fn;
        return NULL;
    }
    // Strip leading dot if caller passed ".c" style.
    u8cs stripped = {};
    if (input[0][0] == '.' && !has_slash && $len(input) > 1) {
        // Could be ".c" (dotted ext) or ".gitignore" (dotfile name).
        // Try name table for dotfiles first.
        TOKfn fn = TOKFindByName(input);
        if (fn) return fn;
        // Then try as dotted ext.
        stripped[0] = input[0] + 1;
        stripped[1] = input[1];
        for (const TOKentry *e = TOK_TABLE; e->ext != NULL; ++e)
            if (TOKSliceMatch(stripped, e->ext)) return e->lexer;
        return NULL;
    }
    // Full path or filename: try name, then ext.
    TOKfn fn = TOKFindByName(input);
    if (fn) return fn;
    return TOKFindByExt(input);
}

b8 TOKKnownExt(u8csc ext) {
    return TOKResolve(ext) != NULL;
}

b8 TOKSameLexer(u8csc a, u8csc b) {
    TOKfn fa = TOKResolve(a);
    TOKfn fb = TOKResolve(b);
    return fa != NULL && fa == fb;
}

ok64 TOKLexer(TOKstate *state, u8csc ext) {
    sane($ok(state->data) && state != NULL);
    TOKfn fn = TOKResolve(ext);
    if (fn == NULL) fn = (TOKfn)TXTTLexer;
    call(fn, state);
    done;
}
