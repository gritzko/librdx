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
            u8 kb[11];
            u8s ki = {kb, kb + 11};
            call(RONutf8sFeed, ki, ci++);
            u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
            u8cs gap = {src[0] + pos, src[0] + cs};
            call(BASONFeed, idx, buf, 'S', ck, gap);
        }

        {
            u8 kb[11];
            u8s ki = {kb, kb + 11};
            call(RONutf8sFeed, ki, ci++);
            u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
            call(BASTFeedNode, buf, idx, src, child, ck);
        }

        pos = ts_node_end_byte(child);
    }

    if (e > pos) {
        u8 kb[11];
        u8s ki = {kb, kb + 11};
        call(RONutf8sFeed, ki, ci++);
        u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
        u8cs gap = {src[0] + pos, src[0] + e};
        call(BASONFeed, idx, buf, 'S', ck, gap);
    }

    call(BASONFeedOuto, idx, buf);
    done;
}

ok64 BASTParse(u8bp buf, u64bp idx, u8csc source, u8csc ext) {
    sane(buf != NULL);
    const TSLanguage *lang = BASTLanguage(ext);
    test(lang != NULL, BADARG);

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
