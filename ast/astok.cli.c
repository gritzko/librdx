#include "BAST.h"

#include <string.h>
#include <tree_sitter/api.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

// Classify anonymous nodes: all-alpha → 'R', otherwise → 'P'
static u8 astok_anon_tag(const char *type) {
    for (const char *p = type; *p; p++) {
        u8 c = (u8)*p;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'))
            return 'P';
    }
    return 'R';
}

// Classify named leaf nodes
static u8 astok_leaf_tag(const char *type) {
    // comments
    if (strcmp(type, "comment") == 0) return 'D';
    if (strcmp(type, "line_comment") == 0) return 'D';
    if (strcmp(type, "block_comment") == 0) return 'D';
    // strings
    if (strcmp(type, "string_literal") == 0) return 'G';
    if (strcmp(type, "string_content") == 0) return 'G';
    if (strcmp(type, "char_literal") == 0) return 'G';
    if (strcmp(type, "raw_string_literal") == 0) return 'G';
    if (strcmp(type, "concatenated_string") == 0) return 'G';
    // numbers
    if (strcmp(type, "number_literal") == 0) return 'L';
    if (strcmp(type, "integer_literal") == 0) return 'L';
    if (strcmp(type, "float_literal") == 0) return 'L';
    // types
    if (strcmp(type, "type_identifier") == 0) return 'T';
    if (strcmp(type, "primitive_type") == 0) return 'T';
    if (strcmp(type, "sized_type_specifier") == 0) return 'T';
    // preprocessor
    if (strcmp(type, "preproc_arg") == 0) return 'H';
    return 'S';
}

static void astok_print(u8 tag, const u8 *src, uint32_t start, uint32_t end) {
    fputc(tag, stdout);
    fputc('\t', stdout);
    for (uint32_t i = start; i < end; ++i) {
        u8 c = src[i];
        switch (c) {
            case '\n': fputc('\\', stdout); fputc('n', stdout); break;
            case '\r': fputc('\\', stdout); fputc('r', stdout); break;
            case '\t': fputc('\\', stdout); fputc('t', stdout); break;
            case '\\': fputc('\\', stdout); fputc('\\', stdout); break;
            default:   fputc(c, stdout); break;
        }
    }
    fputc('\n', stdout);
}

// Walk tree, print leaves. pos tracks the current byte offset so we can
// emit whitespace gaps between tokens.
static void astok_walk(TSNode node, const u8 *src, uint32_t *pos) {
    uint32_t cc = ts_node_child_count(node);

    if (cc == 0) {
        // leaf node
        uint32_t s = ts_node_start_byte(node);
        uint32_t e = ts_node_end_byte(node);

        // emit whitespace gap
        if (s > *pos) {
            astok_print('S', src, *pos, s);
        }

        const char *type = ts_node_type(node);
        u8 tag;
        if (ts_node_is_named(node)) {
            tag = astok_leaf_tag(type);
        } else {
            tag = astok_anon_tag(type);
        }
        // preprocessor directives: the directive keyword itself is 'H'
        TSNode parent = ts_node_parent(node);
        if (!ts_node_is_null(parent)) {
            const char *pt = ts_node_type(parent);
            if (pt[0] == 'p' && strncmp(pt, "preproc_", 8) == 0 && tag == 'S')
                tag = 'H';
        }

        astok_print(tag, src, s, e);
        *pos = e;
        return;
    }

    for (uint32_t i = 0; i < cc; ++i) {
        TSNode child = ts_node_child(node, i);
        astok_walk(child, src, pos);
    }
}

ok64 astokcli() {
    sane($arglen >= 2);

    a$rg(arg, 1);

    // find extension
    u8cs ext = {};
    for (u8cp p = arg[1]; p > arg[0]; --p) {
        if (*(p - 1) == '/') break;
        if (*(p - 1) == '.') {
            ext[0] = p - 1;
            ext[1] = arg[1];
            break;
        }
    }
    if (ext[0] == NULL) fail(BADARG);

    const TSLanguage *lang = BASTLanguage(ext);
    if (lang == NULL) fail(BADARG);

    // read file
    a_pad(u8, path, 4096);
    call(u8bFeed, path, arg);
    u8bFeed1(path, 0);
    u8bShed1(path);

    u8bp mapped = NULL;
    call(FILEMapRO, &mapped, PATHu8cgIn(path));
    u8cp src0 = u8bDataHead(mapped);
    u8cp src1 = u8bIdleHead(mapped);
    uint32_t srclen = (uint32_t)(src1 - src0);

    // parse
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, lang);
    TSTree *tree = ts_parser_parse_string(parser, NULL, (const char *)src0, srclen);
    TSNode root = ts_tree_root_node(tree);

    uint32_t pos = 0;
    astok_walk(root, src0, &pos);

    // trailing whitespace
    if (pos < srclen) {
        astok_print('S', src0, pos, srclen);
    }

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    call(FILEUnMap, mapped);
    done;
}

MAIN(astokcli);
