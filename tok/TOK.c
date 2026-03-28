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
#include "abc/PRO.h"

static b8 TOKExtMatch(u8csc ext, const char *pat) {
    u64 len = u8csLen(ext);
    u64 plen = 0;
    while (pat[plen]) ++plen;
    if (len != plen) return NO;
    return __builtin_memcmp(ext[0], pat, len) == 0;
}

// Dispatch macro: declare local state, call lexer, update position
#define TOK_DISPATCH(TYPE, LEXER, ...) do { \
    TYPE##state st = { \
        .data = {state->data[0], state->data[1]}, \
        .cb = state->cb, \
        .ctx = state->ctx, \
    }; \
    call(LEXER, &st); \
    state->data[0] = st.data[0]; \
    done; \
} while(0)

ok64 TOKLexer(TOKstate *state, u8csc ext) {
    sane($ok(state->data) && state != NULL);

    // C/C++ (CT handles .c .h .rl; CPPT handles C++ extensions)
    if (TOKExtMatch(ext, "c") || TOKExtMatch(ext, "h") ||
        TOKExtMatch(ext, "rl")) {
        TOK_DISPATCH(CT, CTLexer);
    }
    if (TOKExtMatch(ext, "cpp") || TOKExtMatch(ext, "cc") ||
        TOKExtMatch(ext, "cxx") || TOKExtMatch(ext, "hpp") ||
        TOKExtMatch(ext, "hh") || TOKExtMatch(ext, "hxx")) {
        TOK_DISPATCH(CPPT, CPPTLexer);
    }

    // Go
    if (TOKExtMatch(ext, "go")) {
        TOK_DISPATCH(GOT, GOTLexer);
    }

    // Python
    if (TOKExtMatch(ext, "py")) {
        TOK_DISPATCH(PYT, PYTLexer);
    }

    // JavaScript
    if (TOKExtMatch(ext, "js") || TOKExtMatch(ext, "jsx") ||
        TOKExtMatch(ext, "mjs")) {
        TOK_DISPATCH(JST, JSTLexer);
    }

    // Rust
    if (TOKExtMatch(ext, "rs")) {
        TOK_DISPATCH(RST, RSTLexer);
    }

    // Java
    if (TOKExtMatch(ext, "java")) {
        TOK_DISPATCH(JAT, JATLexer);
    }

    // C#
    if (TOKExtMatch(ext, "cs")) {
        TOK_DISPATCH(CST, CSTLexer);
    }

    // HTML
    if (TOKExtMatch(ext, "html") || TOKExtMatch(ext, "htm")) {
        TOK_DISPATCH(HTMT, HTMTLexer);
    }

    // CSS
    if (TOKExtMatch(ext, "css")) {
        TOK_DISPATCH(CSST, CSSTLexer);
    }

    // JSON
    if (TOKExtMatch(ext, "json")) {
        TOK_DISPATCH(JSONT, JSONTLexer);
    }

    // Bash
    if (TOKExtMatch(ext, "sh") || TOKExtMatch(ext, "bash")) {
        TOK_DISPATCH(SHT, SHTLexer);
    }

    // Ruby
    if (TOKExtMatch(ext, "rb")) {
        TOK_DISPATCH(RBT, RBTLexer);
    }

    // Haskell
    if (TOKExtMatch(ext, "hs")) {
        TOK_DISPATCH(HST, HSTLexer);
    }

    // OCaml
    if (TOKExtMatch(ext, "ml") || TOKExtMatch(ext, "mli")) {
        TOK_DISPATCH(MLT, MLTLexer);
    }

    // Julia
    if (TOKExtMatch(ext, "jl")) {
        TOK_DISPATCH(JLT, JLTLexer);
    }

    // PHP
    if (TOKExtMatch(ext, "php")) {
        TOK_DISPATCH(PHPT, PHPTLexer);
    }

    // Agda
    if (TOKExtMatch(ext, "agda")) {
        TOK_DISPATCH(AGDT, AGDTLexer);
    }

    // Verilog
    if (TOKExtMatch(ext, "v") || TOKExtMatch(ext, "sv")) {
        TOK_DISPATCH(VERT, VERTLexer);
    }

    fail(TOKBAD);
}
