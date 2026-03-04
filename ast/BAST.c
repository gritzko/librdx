#include "BAST.h"

#include <string.h>
#include <tree_sitter/api.h>

#include "abc/PRO.h"
#include "abc/RON.h"

// --- Tree-sitter language declarations ---

extern const TSLanguage *tree_sitter_agda(void);
extern const TSLanguage *tree_sitter_bash(void);
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_c_sharp(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_css(void);
extern const TSLanguage *tree_sitter_embedded_template(void);
extern const TSLanguage *tree_sitter_go(void);
extern const TSLanguage *tree_sitter_haskell(void);
extern const TSLanguage *tree_sitter_html(void);
extern const TSLanguage *tree_sitter_java(void);
extern const TSLanguage *tree_sitter_javascript(void);
extern const TSLanguage *tree_sitter_json(void);
extern const TSLanguage *tree_sitter_julia(void);
extern const TSLanguage *tree_sitter_ocaml(void);
extern const TSLanguage *tree_sitter_ocaml_interface(void);
extern const TSLanguage *tree_sitter_ocaml_type(void);
extern const TSLanguage *tree_sitter_php(void);
extern const TSLanguage *tree_sitter_php_only(void);
extern const TSLanguage *tree_sitter_python(void);
extern const TSLanguage *tree_sitter_regex(void);
extern const TSLanguage *tree_sitter_ruby(void);
extern const TSLanguage *tree_sitter_rust(void);
extern const TSLanguage *tree_sitter_verilog(void);

// --- Extension-to-language mapping ---

typedef struct {
    char ext[8];
    const TSLanguage *(*lang)(void);
} BASTLangMap;

static const BASTLangMap BAST_LANGS[] = {
    {".agda", tree_sitter_agda},
    {".sh", tree_sitter_bash},
    {".bash", tree_sitter_bash},
    {".c", tree_sitter_c},
    {".h", tree_sitter_c},
    {".cs", tree_sitter_c_sharp},
    {".cpp", tree_sitter_cpp},
    {".cc", tree_sitter_cpp},
    {".cxx", tree_sitter_cpp},
    {".hpp", tree_sitter_cpp},
    {".css", tree_sitter_css},
    {".erb", tree_sitter_embedded_template},
    {".go", tree_sitter_go},
    {".hs", tree_sitter_haskell},
    {".html", tree_sitter_html},
    {".htm", tree_sitter_html},
    {".java", tree_sitter_java},
    {".js", tree_sitter_javascript},
    {".jsx", tree_sitter_javascript},
    {".mjs", tree_sitter_javascript},
    {".json", tree_sitter_json},
    {".jl", tree_sitter_julia},
    {".ml", tree_sitter_ocaml},
    {".mli", tree_sitter_ocaml_interface},
    {".php", tree_sitter_php},
    {".py", tree_sitter_python},
    {".rb", tree_sitter_ruby},
    {".rs", tree_sitter_rust},
    {".v", tree_sitter_verilog},
    {".sv", tree_sitter_verilog},
};

const TSLanguage *BASTLanguage(u8csc ext) {
    size_t elen = $len(ext);
    for (size_t i = 0; i < sizeof(BAST_LANGS) / sizeof(BAST_LANGS[0]); i++) {
        size_t mlen = strlen(BAST_LANGS[i].ext);
        if (mlen == elen && memcmp(ext[0], BAST_LANGS[i].ext, mlen) == 0)
            return BAST_LANGS[i].lang();
    }
    return NULL;
}

u32 BASTFtype(u8csc ext) {
    if (!$ok(ext) || $empty(ext)) return 0;
    u8cp p = ext[0];
    if (*p == '.') p++;
    size_t n = ext[1] - p;
    if (n == 0 || n > 3) return 0;
    u8 chars[3] = {0, 0, 0};
    for (size_t i = 0; i < n; i++) {
        u8 v = RON64_REV[p[i]];
        if (v == 0xff) return 0;
        chars[i] = v;
    }
    return ((u32)chars[0] << 12) | ((u32)chars[1] << 6) | (u32)chars[2];
}

ok64 BASTFtypeExt(u8s ext, u32 ftype) {
    if (ftype == 0) return OK;
    u8 c0 = (ftype >> 12) & 0x3f;
    u8 c1 = (ftype >> 6) & 0x3f;
    u8 c2 = ftype & 0x3f;
    u8sFeed1(ext, '.');
    u8sFeed1(ext, RON64_CHARS[c0]);
    if (c1 != 0 || c2 != 0) {
        u8sFeed1(ext, RON64_CHARS[c1]);
        if (c2 != 0) {
            u8sFeed1(ext, RON64_CHARS[c2]);
        }
    }
    return OK;
}

// --- Plain-text tokenizer (fallback for unknown extensions) ---
// Tokens: \s*\w+ (whitespace + word), \s*\d+ (whitespace + number),
// or individual punctuation chars.  Concatenating all tokens = original.

fun b8 BASTIsWord(u8 c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fun b8 BASTIsDigit(u8 c) { return c >= '0' && c <= '9'; }

fun b8 BASTIsBlank(u8 c) { return c == ' ' || c == '\t'; }

static ok64 BASTParseText(u8bp buf, u64bp idx, u8csc source) {
    sane(buf != NULL);
    u8cs nokey = {(u8cp) "", (u8cp) ""};
    call(BASONFeedInto, idx, buf, 'A', nokey);

    u8cp p = source[0];
    u8cp end = source[1];
    u64 ci = 0;

    while (p < end) {
        u8cp start = p;

        if (*p == '\n' || *p == '\r') {
            // newline as its own token (\r\n together)
            if (*p == '\r' && p + 1 < end && *(p + 1) == '\n') p++;
            p++;
        } else {
            // consume leading blanks (spaces/tabs)
            while (p < end && BASTIsBlank(*p)) p++;

            if (p < end && (BASTIsWord(*p) || BASTIsDigit(*p))) {
                // consume word/number
                while (p < end && (BASTIsWord(*p) || BASTIsDigit(*p)))
                    p++;
            } else if (p == start) {
                // single punctuation / non-word byte
                p++;
            }
            // else: blank-only tail, p advanced past it
        }

        u8 kb[11];
        u8s ki = {kb, kb + 11};
        call(RONutf8sFeed, ki, ci++);
        u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
        u8cs val = {start, p};
        call(BASONFeed, idx, buf, 'S', ck, val);
    }

    call(BASONFeedOuto, idx, buf);
    done;
}

// --- AST node type tagging ---
//
// BASON keys get a suffix letter indicating the AST node type:
//   'f' = function/method definition
//   'c' = class/struct/type declaration
//   'b' = block/compound statement (bracketed)
// If the RON64 key naturally ends with a reserved letter, '0' is
// appended first to disambiguate.  So the last character of the
// final key tells the type: 'f'/'c'/'b' = tagged, anything else = untagged.
//
// Tree-sitter node type strings are language-dependent.  No cross-language
// conflicts exist, so one flat table covers all supported languages.

#define BAST_TAG_FUNC  'f'
#define BAST_TAG_CLASS 'c'
#define BAST_TAG_BLOCK 'b'

typedef struct {
    const char *tstype;
    u8 tag;
} BASTTagEntry;

static const BASTTagEntry bast_tags[] = {
    // --- functions (f) ---
    {"function_definition", BAST_TAG_FUNC},   // C, C++, Python, Bash
    {"function_declaration", BAST_TAG_FUNC},   // Go
    {"function_item", BAST_TAG_FUNC},          // Rust
    {"method_declaration", BAST_TAG_FUNC},     // Java, C#, Go
    {"method", BAST_TAG_FUNC},                 // Ruby
    // --- classes / type decls (c) ---
    {"struct_specifier", BAST_TAG_CLASS},       // C, C++
    {"union_specifier", BAST_TAG_CLASS},        // C
    {"enum_specifier", BAST_TAG_CLASS},         // C, C++
    {"class_specifier", BAST_TAG_CLASS},        // C++
    {"class_definition", BAST_TAG_CLASS},       // Python
    {"class_declaration", BAST_TAG_CLASS},      // Java, C#
    {"type_declaration", BAST_TAG_CLASS},       // Go
    {"struct_item", BAST_TAG_CLASS},            // Rust
    {"enum_item", BAST_TAG_CLASS},              // Rust
    {"impl_item", BAST_TAG_CLASS},              // Rust
    {"trait_item", BAST_TAG_CLASS},             // Rust
    {"class", BAST_TAG_CLASS},                  // Ruby
    {"module", BAST_TAG_CLASS},                 // Ruby
    // --- blocks (b) ---
    {"compound_statement", BAST_TAG_BLOCK},     // C, C++, Bash
    {"block", BAST_TAG_BLOCK},                  // Python, Go, Rust
    {NULL, 0}
};

static u8 BASTNodeTag(const char *tstype) {
    for (const BASTTagEntry *e = bast_tags; e->tstype; e++) {
        if (strcmp(e->tstype, tstype) == 0) return e->tag;
    }
    return 0;
}

fun b8 BASTReserved(u8 ch) {
    return ch == BAST_TAG_FUNC || ch == BAST_TAG_CLASS || ch == BAST_TAG_BLOCK;
}

// Append disambiguation '0' if needed, then type tag suffix.
fun void BASTKeySuffix(u8s ki, u8 tag) {
    if (BASTReserved(*(ki[0] - 1))) *(ki[0])++ = '0';
    if (tag) *(ki[0])++ = tag;
}

// --- Tree-to-BASON conversion ---

// Emit one named node as BASON.
// Leaf (no named children): string with source text.
// Branch: array of interleaved text gaps and child arrays.
static ok64 BASTFeedNode(u8bp buf, u64bp idx, u8csc src, TSNode node,
                         u8cs key) {
    sane(buf != NULL);
    uint32_t ncc = ts_node_named_child_count(node);
    uint32_t s = ts_node_start_byte(node);
    uint32_t e = ts_node_end_byte(node);

    if (ncc == 0) {
        u8cs val = {src[0] + s, src[0] + e};
        call(BASONFeed, idx, buf, 'S', key, val);
        done;
    }

    call(BASONFeedInto, idx, buf, 'A', key);
    uint32_t pos = s;
    u64 ci = 0;

    for (uint32_t i = 0; i < ncc; i++) {
        TSNode child = ts_node_named_child(node, i);
        uint32_t cs = ts_node_start_byte(child);

        if (cs > pos) {
            u8 kb[14];
            u8s ki = {kb, kb + sizeof(kb)};
            call(RONutf8sFeed, ki, ci++);
            BASTKeySuffix(ki, 0);
            u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
            u8cs gap = {src[0] + pos, src[0] + cs};
            call(BASONFeed, idx, buf, 'S', ck, gap);
        }

        {
            u8 kb[14];
            u8s ki = {kb, kb + sizeof(kb)};
            call(RONutf8sFeed, ki, ci++);
            BASTKeySuffix(ki, BASTNodeTag(ts_node_type(child)));
            u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
            call(BASTFeedNode, buf, idx, src, child, ck);
        }

        pos = ts_node_end_byte(child);
    }

    if (e > pos) {
        u8 kb[14];
        u8s ki = {kb, kb + sizeof(kb)};
        call(RONutf8sFeed, ki, ci++);
        BASTKeySuffix(ki, 0);
        u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
        u8cs gap = {src[0] + pos, src[0] + e};
        call(BASONFeed, idx, buf, 'S', ck, gap);
    }

    call(BASONFeedOuto, idx, buf);
    done;
}

ok64 BASTParse(u8bp buf, u64bp idx, u8csc source, u8csc ext) {
    sane(buf != NULL);

    // Reject binary files: check for null bytes in first 256 bytes
    size_t check = $len(source);
    if (check > 256) check = 256;
    for (size_t i = 0; i < check; i++) {
        if (source[0][i] == 0) fail(FAILsanity);
    }

    const TSLanguage *lang = BASTLanguage(ext);

    if (lang == NULL) return BASTParseText(buf, idx, source);

    TSParser *parser = ts_parser_new();
    test(parser != NULL, FAILsanity);

    if (!ts_parser_set_language(parser, lang)) {
        ts_parser_delete(parser);
        fail(FAILsanity);
    }

    TSTree *tree = ts_parser_parse_string(parser, NULL,
                                          (const char *)source[0],
                                          (uint32_t)$len(source));
    if (tree == NULL) {
        ts_parser_delete(parser);
        fail(FAILsanity);
    }

    TSNode root = ts_tree_root_node(tree);
    u8cs key = {(u8cp)"", (u8cp)""};
    __ = BASTFeedNode(buf, idx, source, root, key);

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    done;
}
