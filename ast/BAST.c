#include "BAST.h"

#include <string.h>
#include <tree_sitter/api.h>

#include "abc/PRO.h"
#include "abc/RON.h"
#include "sm/SM.h"

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
extern const TSLanguage *tree_sitter_markdown(void);
extern const TSLanguage *tree_sitter_markdown_inline(void);

// --- Extension-to-language mapping ---

typedef struct {
    char ext[8];
    char codec[8];
    const TSLanguage *(*lang)(void);
} BASTLangMap;

static const BASTLangMap BAST_LANGS[] = {
    {".agda", "agda", tree_sitter_agda},
    {".sh", "bash", tree_sitter_bash},
    {".bash", "bash", tree_sitter_bash},
    {".c", "c", tree_sitter_c},
    {".h", "c", tree_sitter_c},
    {".cs", "cs", tree_sitter_c_sharp},
    {".cpp", "cpp", tree_sitter_cpp},
    {".cc", "cpp", tree_sitter_cpp},
    {".cxx", "cpp", tree_sitter_cpp},
    {".hpp", "cpp", tree_sitter_cpp},
    {".css", "css", tree_sitter_css},
    {".erb", "erb", tree_sitter_embedded_template},
    {".go", "go", tree_sitter_go},
    {".hs", "hs", tree_sitter_haskell},
    {".html", "html", tree_sitter_html},
    {".htm", "html", tree_sitter_html},
    {".java", "java", tree_sitter_java},
    {".js", "js", tree_sitter_javascript},
    {".jsx", "js", tree_sitter_javascript},
    {".mjs", "js", tree_sitter_javascript},
    {".json", "json", tree_sitter_json},
    {".jl", "jl", tree_sitter_julia},
    {".ml", "ml", tree_sitter_ocaml},
    {".mli", "ml", tree_sitter_ocaml_interface},
    {".php", "php", tree_sitter_php},
    {".py", "py", tree_sitter_python},
    {".rb", "rb", tree_sitter_ruby},
    {".rs", "rust", tree_sitter_rust},
    {".v", "v", tree_sitter_verilog},
    {".sv", "v", tree_sitter_verilog},
    {".md", "md", tree_sitter_markdown},
    {".mkd", "mkd", NULL},
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

void BASTCodec(u8csp codec, u8csc ext) {
    size_t elen = $len(ext);
    for (size_t i = 0; i < sizeof(BAST_LANGS) / sizeof(BAST_LANGS[0]); i++) {
        size_t mlen = strlen(BAST_LANGS[i].ext);
        if (mlen == elen && memcmp(ext[0], BAST_LANGS[i].ext, mlen) == 0) {
            const char *c = BAST_LANGS[i].codec;
            codec[0] = (u8cp)c;
            codec[1] = (u8cp)c + strlen(c);
            return;
        }
    }
    codec[0] = (u8cp)"text";
    codec[1] = (u8cp)"text" + 4;
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
    u8 kb[11];
    u8cs prevk = {NULL, NULL};

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

        u8s ki = {kb, kb + sizeof(kb)};
        call(BASONFeedInfInc, ki, prevk);
        u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
        prevk[0] = (u8cp)kb;
        prevk[1] = (u8cp)ki[0];
        u8cs val = {start, p};
        call(BASONFeed, idx, buf, 'S', ck, val);
    }

    call(BASONFeedOuto, idx, buf);
    done;
}

// --- AST node type tagging ---
//
// Vowels = containers, consonants = leaves.
// The TLKV type letter carries the AST node kind:
//   'E' = function/method definition (Entry)
//   'I' = class/struct/type declaration (Implementation)
//   'A' = default container
//
// Leaf consonant tags (from BASTLeafTag / BASTAnonTag):
//   'F' = definition name identifier
//   'D' = comment
//   'G' = string literal
//   'L' = number literal
//   'T' = type name
//   'H' = preprocessor
//   'R' = reserved word / keyword
//   'P' = punctuation / operator
//   'S' = plain source text (default)

#define BAST_TAG_FUNC  'E'
#define BAST_TAG_CLASS 'I'

typedef struct {
    const char *tstype;
    u8 tag;
} BASTTagEntry;

static const BASTTagEntry bast_tags[] = {
    // --- functions ---
    {"function_definition", BAST_TAG_FUNC},   // C, C++, Python, Bash
    {"function_declaration", BAST_TAG_FUNC},   // Go
    {"function_item", BAST_TAG_FUNC},          // Rust
    {"method_declaration", BAST_TAG_FUNC},     // Java, C#, Go
    {"method", BAST_TAG_FUNC},                 // Ruby
    // --- classes / type decls ---
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
    // --- Markdown block ---
    {"atx_heading", 'Y'},                         // heading (bold)
    {"setext_heading", 'Y'},                      // heading (bold)
    {"block_quote", 'I'},                         // quote
    {"fenced_code_block", 'I'},                   // code block
    {"indented_code_block", 'I'},                 // code block
    {"list", 'U'},                                // list
    // --- Markdown inline ---
    {"emphasis", 'A'},                            // container for styled text
    {"strong_emphasis", 'A'},
    {"strikethrough", 'A'},
    {"code_span", 'A'},
    {"inline_link", 'A'},
    {"shortcut_link", 'A'},
    {"full_reference_link", 'A'},
    {"collapsed_reference_link", 'A'},
    {"image", 'A'},
    {NULL, 0}
};

static u8 BASTNodeTag(const char *tstype) {
    for (const BASTTagEntry *e = bast_tags; e->tstype; e++) {
        if (strcmp(e->tstype, tstype) == 0) return e->tag;
    }
    return 0;
}

// --- Leaf classification ---

// Classify anonymous (unnamed) tree-sitter nodes.
// All-alphabetic → 'R' (reserved word/keyword), otherwise → 'P' (punctuation).
static u8 BASTAnonTag(const char *type) {
    for (const char *p = type; *p; p++) {
        u8 c = (u8)*p;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'))
            return 'P';
    }
    return 'R';
}

// Classify named leaf nodes by tree-sitter type string.
typedef struct {
    const char *tstype;
    u8 tag;
} BASTLeafEntry;

static const BASTLeafEntry bast_leaf_tags[] = {
    // Comments
    {"comment", 'D'},
    {"line_comment", 'D'},
    {"block_comment", 'D'},
    // String literals
    {"string_literal", 'G'},
    {"string", 'G'},
    {"string_content", 'G'},
    {"char_literal", 'G'},
    {"raw_string_literal", 'G'},
    {"interpreted_string_literal", 'G'},
    {"rune_literal", 'G'},
    {"template_string", 'G'},
    // Number literals
    {"number_literal", 'L'},
    {"integer_literal", 'L'},
    {"float_literal", 'L'},
    {"integer", 'L'},
    // Type names
    {"type_identifier", 'T'},
    {"primitive_type", 'T'},
    {"builtin_type", 'T'},
    {"sized_type_specifier", 'T'},
    // Preprocessor
    {"preproc_arg", 'H'},
    // Markdown block leaves
    {"code_fence_content", 'G'},
    {"info_string", 'T'},
    {"atx_h1_marker", 'P'},
    {"atx_h2_marker", 'P'},
    {"atx_h3_marker", 'P'},
    {"atx_h4_marker", 'P'},
    {"atx_h5_marker", 'P'},
    {"atx_h6_marker", 'P'},
    {"setext_h1_underline", 'P'},
    {"setext_h2_underline", 'P'},
    {"fenced_code_block_delimiter", 'P'},
    {"block_quote_marker", 'P'},
    {"list_marker_minus", 'P'},
    {"list_marker_plus", 'P'},
    {"list_marker_star", 'P'},
    {"list_marker_dot", 'P'},
    {"list_marker_parenthesis", 'P'},
    {"block_continuation", 'S'},
    // Markdown inline leaves
    {"code_span_delimiter", 'P'},
    {"emphasis_delimiter", 'P'},
    {"link_destination", 'P'},
    {"link_title", 'P'},
    {"uri_autolink", 'T'},
    {"email_autolink", 'T'},
    {NULL, 0}
};

static u8 BASTLeafTag(const char *tstype) {
    for (const BASTLeafEntry *e = bast_leaf_tags; e->tstype; e++) {
        if (strcmp(e->tstype, tstype) == 0) return e->tag;
    }
    return 'S';
}

// --- Tree-to-BASON conversion ---

// Chase tree-sitter "name" / "declarator" fields down to the identifier leaf.
static TSNode BASTFindName(TSNode node) {
    TSNode n = ts_node_child_by_field_name(node, "name", 4);
    if (!ts_node_is_null(n)) {
        if (ts_node_named_child_count(n) == 0) return n;
        return BASTFindName(n);
    }
    n = ts_node_child_by_field_name(node, "declarator", 10);
    if (!ts_node_is_null(n)) {
        if (ts_node_named_child_count(n) == 0) return n;
        return BASTFindName(n);
    }
    return (TSNode){{0}, NULL, NULL};
}

// Emit one node as BASON, walking ALL children (named + anonymous).
// Containers (nodes with children) get vowel tags.
// Leaves (no children) get consonant tags based on classification.
// name_s/name_e: byte range of the symbol name leaf (from enclosing E/I).
static ok64 BASTFeedNode(u8bp buf, u64bp idx, u8csc src, TSNode node,
                         u8cs key, u8 tag,
                         uint32_t name_s, uint32_t name_e) {
    sane(buf != NULL);
    uint32_t cc = ts_node_child_count(node);
    uint32_t s = ts_node_start_byte(node);
    uint32_t e = ts_node_end_byte(node);

    // When entering E/I node, find the name identifier range
    if (tag == BAST_TAG_FUNC || tag == BAST_TAG_CLASS) {
        TSNode nm = BASTFindName(node);
        if (!ts_node_is_null(nm)) {
            name_s = ts_node_start_byte(nm);
            name_e = ts_node_end_byte(nm);
        }
    }

    if (cc == 0) {
        // Terminal leaf — classify by type
        u8 lt;
        if (name_s < name_e && s == name_s && e == name_e) {
            lt = BAST_TAG_NAME;
        } else if (ts_node_is_named(node)) {
            lt = BASTLeafTag(ts_node_type(node));
        } else {
            lt = BASTAnonTag(ts_node_type(node));
        }
        u8cs val = {src[0] + s, src[0] + e};
        call(BASONFeed, idx, buf, lt, key, val);
        done;
    }

    call(BASONFeedInto, idx, buf, tag, key);
    uint32_t pos = s;
    u8 kb[11];
    u8cs prevk = {NULL, NULL};

    for (uint32_t i = 0; i < cc; i++) {
        TSNode child = ts_node_child(node, i);
        uint32_t cs = ts_node_start_byte(child);

        // Gap text (whitespace between children)
        if (cs > pos) {
            u8s ki = {kb, kb + sizeof(kb)};
            call(BASONFeedInfInc, ki, prevk);
            u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
            prevk[0] = (u8cp)kb;
            prevk[1] = (u8cp)ki[0];
            u8cs gap = {src[0] + pos, src[0] + cs};
            call(BASONFeed, idx, buf, 'S', ck, gap);
        }

        {
            u8s ki = {kb, kb + sizeof(kb)};
            call(BASONFeedInfInc, ki, prevk);
            u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
            prevk[0] = (u8cp)kb;
            prevk[1] = (u8cp)ki[0];

            uint32_t child_cc = ts_node_child_count(child);
            u8 ctag;
            if (child_cc > 0) {
                // Container child — get tag from table or default 'A'
                ctag = BASTNodeTag(ts_node_type(child));
                if (ctag == 0) ctag = 'A';
            } else {
                // Leaf child — classify
                if (name_s < name_e &&
                    ts_node_start_byte(child) == name_s &&
                    ts_node_end_byte(child) == name_e) {
                    ctag = BAST_TAG_NAME;
                } else if (ts_node_is_named(child)) {
                    ctag = BASTLeafTag(ts_node_type(child));
                } else {
                    ctag = BASTAnonTag(ts_node_type(child));
                }
            }
            call(BASTFeedNode, buf, idx, src, child, ck, ctag,
                 name_s, name_e);
        }

        pos = ts_node_end_byte(child);
    }

    if (e > pos) {
        u8s ki = {kb, kb + sizeof(kb)};
        call(BASONFeedInfInc, ki, prevk);
        u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
        u8cs gap = {src[0] + pos, src[0] + e};
        call(BASONFeed, idx, buf, 'S', ck, gap);
    }

    call(BASONFeedOuto, idx, buf);
    done;
}

// --- Markdown two-pass walker ---

// Map inline container type to style tag for leaf children.
static u8 BASTMDInlineStyle(const char *tstype) {
    if (strcmp(tstype, "emphasis") == 0) return 'W';
    if (strcmp(tstype, "strong_emphasis") == 0) return 'V';
    if (strcmp(tstype, "strikethrough") == 0) return 'J';
    if (strcmp(tstype, "code_span") == 0) return 'G';
    if (strcmp(tstype, "link_text") == 0) return 'T';
    return 0;
}

// Walk the inline parse tree, emitting styled leaves.
// style: inherited style from enclosing container (default 'S').
static ok64 BASTFeedMDInline(u8bp buf, u64bp idx, u8csc src,
                              TSNode node, u8cs key, u8 style) {
    sane(buf != NULL);
    uint32_t cc = ts_node_child_count(node);
    uint32_t s = ts_node_start_byte(node);
    uint32_t e = ts_node_end_byte(node);

    if (cc == 0) {
        // Leaf node
        u8 lt;
        if (!ts_node_is_named(node)) {
            lt = 'P';  // delimiters: *, **, ~~, `, [, ] etc
        } else {
            lt = BASTLeafTag(ts_node_type(node));
            if (lt == 'S') {
                // Check inline style (e.g. link_text leaf → 'T')
                u8 ms = BASTMDInlineStyle(ts_node_type(node));
                lt = ms ? ms : style;
            }
        }
        u8cs val = {src[0] + s, src[0] + e};
        call(BASONFeed, idx, buf, lt, key, val);
        done;
    }

    // Container: check if it defines a new style
    u8 child_style = BASTMDInlineStyle(ts_node_type(node));
    if (child_style == 0) child_style = style;

    // Check for link containers: link_destination/link_label → 'P'
    const char *ttype = ts_node_type(node);
    b8 is_link_meta = (strcmp(ttype, "link_destination") == 0 ||
                       strcmp(ttype, "link_title") == 0 ||
                       strcmp(ttype, "link_label") == 0 ||
                       strcmp(ttype, "image_description") == 0);
    if (is_link_meta) child_style = 'P';

    u8 ctag = BASTNodeTag(ttype);
    if (ctag == 0) ctag = 'A';

    call(BASONFeedInto, idx, buf, ctag, key);
    uint32_t pos = s;
    u8 kb[11];
    u8cs prevk = {NULL, NULL};

    for (uint32_t i = 0; i < cc; i++) {
        TSNode child = ts_node_child(node, i);
        uint32_t cs = ts_node_start_byte(child);

        if (cs > pos) {
            u8s ki = {kb, kb + sizeof(kb)};
            call(BASONFeedInfInc, ki, prevk);
            u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
            prevk[0] = (u8cp)kb;
            prevk[1] = (u8cp)ki[0];
            u8cs gap = {src[0] + pos, src[0] + cs};
            call(BASONFeed, idx, buf, child_style, ck, gap);
        }

        {
            u8s ki = {kb, kb + sizeof(kb)};
            call(BASONFeedInfInc, ki, prevk);
            u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
            prevk[0] = (u8cp)kb;
            prevk[1] = (u8cp)ki[0];
            call(BASTFeedMDInline, buf, idx, src, child, ck, child_style);
        }

        pos = ts_node_end_byte(child);
    }

    if (e > pos) {
        u8s ki = {kb, kb + sizeof(kb)};
        call(BASONFeedInfInc, ki, prevk);
        u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
        u8cs gap = {src[0] + pos, src[0] + e};
        call(BASONFeed, idx, buf, child_style, ck, gap);
    }

    call(BASONFeedOuto, idx, buf);
    done;
}

// Walk the block-level markdown tree.  When an "inline" node is found,
// re-parse its byte range with the inline grammar and walk that tree.
static ok64 BASTFeedMDNode(u8bp buf, u64bp idx, u8csc src, TSNode node,
                            u8cs key, u8 tag,
                            const TSLanguage *inline_lang) {
    sane(buf != NULL);
    uint32_t cc = ts_node_child_count(node);
    uint32_t s = ts_node_start_byte(node);
    uint32_t e = ts_node_end_byte(node);

    // Detect "inline" node — re-parse with inline grammar
    if (strcmp(ts_node_type(node), "inline") == 0 && inline_lang != NULL) {
        TSParser *ip = ts_parser_new();
        if (ip == NULL) fail(FAILsanity);
        if (!ts_parser_set_language(ip, inline_lang)) {
            ts_parser_delete(ip);
            fail(FAILsanity);
        }
        u8cp range_start = src[0] + s;
        uint32_t range_len = e - s;
        TSTree *itree = ts_parser_parse_string(
            ip, NULL, (const char *)range_start, range_len);
        if (itree == NULL) {
            ts_parser_delete(ip);
            fail(FAILsanity);
        }
        TSNode iroot = ts_tree_root_node(itree);
        // The inline parser wraps everything in an "inline" root node.
        // We need to offset: inline tree positions are 0-based relative
        // to range_start, but src positions are absolute.
        // We create a shifted source slice for the inline walker.
        u8cs isrc = {src[0], src[1]};
        // However, inline parser positions start at 0, not at s.
        // So we shift the source base back by s bytes so that
        // inline position 0 maps to src[0]+s.
        // Actually, the inline parser returns byte offsets relative
        // to the input it received (range_start), starting from 0.
        // We pass range_start as the source base.
        u8cs inline_src = {range_start, range_start + range_len};
        // Walk inline tree children (skip the root "inline" node wrapper)
        uint32_t icc = ts_node_child_count(iroot);
        if (icc > 0) {
            call(BASONFeedInto, idx, buf, tag, key);
            uint32_t ipos = 0;
            u8 ikb[11];
            u8cs iprevk = {NULL, NULL};
            for (uint32_t i = 0; i < icc; i++) {
                TSNode ichild = ts_node_child(iroot, i);
                uint32_t ics = ts_node_start_byte(ichild);
                if (ics > ipos) {
                    u8s iki = {ikb, ikb + sizeof(ikb)};
                    call(BASONFeedInfInc, iki, iprevk);
                    u8cs ick = {(u8cp)ikb, (u8cp)iki[0]};
                    iprevk[0] = (u8cp)ikb;
                    iprevk[1] = (u8cp)iki[0];
                    u8cs gap = {inline_src[0] + ipos, inline_src[0] + ics};
                    call(BASONFeed, idx, buf, 'S', ick, gap);
                }
                {
                    u8s iki = {ikb, ikb + sizeof(ikb)};
                    call(BASONFeedInfInc, iki, iprevk);
                    u8cs ick = {(u8cp)ikb, (u8cp)iki[0]};
                    iprevk[0] = (u8cp)ikb;
                    iprevk[1] = (u8cp)iki[0];
                    call(BASTFeedMDInline, buf, idx, inline_src, ichild,
                         ick, 'S');
                }
                ipos = ts_node_end_byte(ichild);
            }
            uint32_t iend = ts_node_end_byte(iroot);
            if (iend > ipos) {
                u8s iki = {ikb, ikb + sizeof(ikb)};
                call(BASONFeedInfInc, iki, iprevk);
                u8cs ick = {(u8cp)ikb, (u8cp)iki[0]};
                u8cs gap = {inline_src[0] + ipos, inline_src[0] + iend};
                call(BASONFeed, idx, buf, 'S', ick, gap);
            }
            call(BASONFeedOuto, idx, buf);
        } else {
            // Empty inline — emit as leaf
            u8cs val = {range_start, range_start + range_len};
            call(BASONFeed, idx, buf, 'S', key, val);
        }
        ts_tree_delete(itree);
        ts_parser_delete(ip);
        done;
    }

    if (cc == 0) {
        // Terminal leaf
        u8 lt;
        if (ts_node_is_named(node)) {
            lt = BASTLeafTag(ts_node_type(node));
        } else {
            lt = BASTAnonTag(ts_node_type(node));
        }
        u8cs val = {src[0] + s, src[0] + e};
        call(BASONFeed, idx, buf, lt, key, val);
        done;
    }

    call(BASONFeedInto, idx, buf, tag, key);
    uint32_t pos = s;
    u8 kb[11];
    u8cs prevk = {NULL, NULL};

    for (uint32_t i = 0; i < cc; i++) {
        TSNode child = ts_node_child(node, i);
        uint32_t cs = ts_node_start_byte(child);

        if (cs > pos) {
            u8s ki = {kb, kb + sizeof(kb)};
            call(BASONFeedInfInc, ki, prevk);
            u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
            prevk[0] = (u8cp)kb;
            prevk[1] = (u8cp)ki[0];
            u8cs gap = {src[0] + pos, src[0] + cs};
            call(BASONFeed, idx, buf, 'S', ck, gap);
        }

        {
            u8s ki = {kb, kb + sizeof(kb)};
            call(BASONFeedInfInc, ki, prevk);
            u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
            prevk[0] = (u8cp)kb;
            prevk[1] = (u8cp)ki[0];

            uint32_t child_cc = ts_node_child_count(child);
            u8 ctag;
            if (child_cc > 0) {
                ctag = BASTNodeTag(ts_node_type(child));
                if (ctag == 0) ctag = 'A';
            } else {
                if (ts_node_is_named(child)) {
                    ctag = BASTLeafTag(ts_node_type(child));
                } else {
                    ctag = BASTAnonTag(ts_node_type(child));
                }
            }
            call(BASTFeedMDNode, buf, idx, src, child, ck, ctag,
                 inline_lang);
        }

        pos = ts_node_end_byte(child);
    }

    if (e > pos) {
        u8s ki = {kb, kb + sizeof(kb)};
        call(BASONFeedInfInc, ki, prevk);
        u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
        u8cs gap = {src[0] + pos, src[0] + e};
        call(BASONFeed, idx, buf, 'S', ck, gap);
    }

    call(BASONFeedOuto, idx, buf);
    done;
}

static ok64 BASTParseMarkdown(u8bp buf, u64bp idx, u8csc source) {
    sane(buf != NULL);
    TSParser *parser = ts_parser_new();
    test(parser != NULL, FAILsanity);

    const TSLanguage *block_lang = tree_sitter_markdown();
    if (!ts_parser_set_language(parser, block_lang)) {
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
    const TSLanguage *inline_lang = tree_sitter_markdown_inline();
    __ = BASTFeedMDNode(buf, idx, source, root, key, 'A', inline_lang);

    ts_tree_delete(tree);
    ts_parser_delete(parser);
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

    // StrictMark: custom parser, no tree-sitter
    if ($len(ext) == 4 && memcmp(ext[0], ".mkd", 4) == 0)
        return SMParse(buf, idx, source);

    // Markdown: two-pass parser (block + inline)
    if ($len(ext) == 3 && memcmp(ext[0], ".md", 3) == 0)
        return BASTParseMarkdown(buf, idx, source);

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
    __ = BASTFeedNode(buf, idx, source, root, key, 'A', 0, 0);

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    done;
}
