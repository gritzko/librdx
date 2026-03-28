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

    // C/C++
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

    // TypeScript
    if (TOKExtMatch(ext, "ts") || TOKExtMatch(ext, "tsx")) {
        TOK_DISPATCH(TST, TSTLexer);
    }

    // Rust
    if (TOKExtMatch(ext, "rs")) {
        TOK_DISPATCH(RST, RSTLexer);
    }

    // Java
    if (TOKExtMatch(ext, "java")) {
        TOK_DISPATCH(JAT, JATLexer);
    }

    // Kotlin
    if (TOKExtMatch(ext, "kt") || TOKExtMatch(ext, "kts")) {
        TOK_DISPATCH(KTT, KTTLexer);
    }

    // Scala
    if (TOKExtMatch(ext, "scala") || TOKExtMatch(ext, "sc")) {
        TOK_DISPATCH(SCLT, SCLTLexer);
    }

    // C#
    if (TOKExtMatch(ext, "cs")) {
        TOK_DISPATCH(CST, CSTLexer);
    }

    // F#
    if (TOKExtMatch(ext, "fs") || TOKExtMatch(ext, "fsi") ||
        TOKExtMatch(ext, "fsx")) {
        TOK_DISPATCH(FSHT, FSHTLexer);
    }

    // Swift
    if (TOKExtMatch(ext, "swift")) {
        TOK_DISPATCH(SWFT, SWFTLexer);
    }

    // Dart
    if (TOKExtMatch(ext, "dart")) {
        TOK_DISPATCH(DARTT, DARTTLexer);
    }

    // D
    if (TOKExtMatch(ext, "d")) {
        TOK_DISPATCH(DT, DTLexer);
    }

    // Zig
    if (TOKExtMatch(ext, "zig")) {
        TOK_DISPATCH(ZIGT, ZIGTLexer);
    }

    // HTML
    if (TOKExtMatch(ext, "html") || TOKExtMatch(ext, "htm")) {
        TOK_DISPATCH(HTMT, HTMTLexer);
    }

    // CSS
    if (TOKExtMatch(ext, "css")) {
        TOK_DISPATCH(CSST, CSSTLexer);
    }

    // SCSS
    if (TOKExtMatch(ext, "scss")) {
        TOK_DISPATCH(SCSST, SCSSTLexer);
    }

    // JSON
    if (TOKExtMatch(ext, "json")) {
        TOK_DISPATCH(JSONT, JSONTLexer);
    }

    // YAML
    if (TOKExtMatch(ext, "yml") || TOKExtMatch(ext, "yaml")) {
        TOK_DISPATCH(YMLT, YMLTLexer);
    }

    // TOML
    if (TOKExtMatch(ext, "toml")) {
        TOK_DISPATCH(TOMLT, TOMLTLexer);
    }

    // Bash
    if (TOKExtMatch(ext, "sh") || TOKExtMatch(ext, "bash")) {
        TOK_DISPATCH(SHT, SHTLexer);
    }

    // Ruby
    if (TOKExtMatch(ext, "rb")) {
        TOK_DISPATCH(RBT, RBTLexer);
    }

    // Lua
    if (TOKExtMatch(ext, "lua")) {
        TOK_DISPATCH(LUAT, LUATLexer);
    }

    // Perl
    if (TOKExtMatch(ext, "pl") || TOKExtMatch(ext, "pm")) {
        TOK_DISPATCH(PRLT, PRLTLexer);
    }

    // R
    if (TOKExtMatch(ext, "r") || TOKExtMatch(ext, "R")) {
        TOK_DISPATCH(RT, RTLexer);
    }

    // Elixir
    if (TOKExtMatch(ext, "ex") || TOKExtMatch(ext, "exs")) {
        TOK_DISPATCH(ELXT, ELXTLexer);
    }

    // Erlang
    if (TOKExtMatch(ext, "erl") || TOKExtMatch(ext, "hrl")) {
        TOK_DISPATCH(ERLT, ERLTLexer);
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

    // Nim
    if (TOKExtMatch(ext, "nim") || TOKExtMatch(ext, "nims")) {
        TOK_DISPATCH(NIMT, NIMTLexer);
    }

    // PHP
    if (TOKExtMatch(ext, "php")) {
        TOK_DISPATCH(PHPT, PHPTLexer);
    }

    // Clojure
    if (TOKExtMatch(ext, "clj") || TOKExtMatch(ext, "cljs") ||
        TOKExtMatch(ext, "cljc") || TOKExtMatch(ext, "edn")) {
        TOK_DISPATCH(CLJT, CLJTLexer);
    }

    // Nix
    if (TOKExtMatch(ext, "nix")) {
        TOK_DISPATCH(NIXT, NIXTLexer);
    }

    // SQL
    if (TOKExtMatch(ext, "sql")) {
        TOK_DISPATCH(SQLT, SQLTLexer);
    }

    // GraphQL
    if (TOKExtMatch(ext, "graphql") || TOKExtMatch(ext, "gql")) {
        TOK_DISPATCH(GQLT, GQLTLexer);
    }

    // Protobuf
    if (TOKExtMatch(ext, "proto")) {
        TOK_DISPATCH(PRTT, PRTTLexer);
    }

    // HCL/Terraform
    if (TOKExtMatch(ext, "hcl") || TOKExtMatch(ext, "tf")) {
        TOK_DISPATCH(HCLT, HCLTLexer);
    }

    // LaTeX
    if (TOKExtMatch(ext, "tex") || TOKExtMatch(ext, "sty") ||
        TOKExtMatch(ext, "cls")) {
        TOK_DISPATCH(LAXT, LAXTLexer);
    }

    // VimL
    if (TOKExtMatch(ext, "vim")) {
        TOK_DISPATCH(VIMT, VIMTLexer);
    }

    // CMake
    if (TOKExtMatch(ext, "cmake")) {
        TOK_DISPATCH(CMKT, CMKTLexer);
    }

    // Dockerfile
    if (TOKExtMatch(ext, "dockerfile")) {
        TOK_DISPATCH(DKFT, DKFTLexer);
    }

    // Makefile
    if (TOKExtMatch(ext, "mk")) {
        TOK_DISPATCH(MAKT, MAKTLexer);
    }

    // Fortran
    if (TOKExtMatch(ext, "f90") || TOKExtMatch(ext, "f95") ||
        TOKExtMatch(ext, "f03") || TOKExtMatch(ext, "f08")) {
        TOK_DISPATCH(FORT, FORTLexer);
    }

    // GLSL
    if (TOKExtMatch(ext, "glsl") || TOKExtMatch(ext, "vert") ||
        TOKExtMatch(ext, "frag") || TOKExtMatch(ext, "geom") ||
        TOKExtMatch(ext, "comp")) {
        TOK_DISPATCH(GLST, GLSTLexer);
    }

    // Gleam
    if (TOKExtMatch(ext, "gleam")) {
        TOK_DISPATCH(GLMT, GLMTLexer);
    }

    // Odin
    if (TOKExtMatch(ext, "odin")) {
        TOK_DISPATCH(ODNT, ODNTLexer);
    }

    // PowerShell
    if (TOKExtMatch(ext, "ps1") || TOKExtMatch(ext, "psm1") ||
        TOKExtMatch(ext, "psd1")) {
        TOK_DISPATCH(PWST, PWSTLexer);
    }

    // Solidity
    if (TOKExtMatch(ext, "sol")) {
        TOK_DISPATCH(SOLT, SOLTLexer);
    }

    // Typst
    if (TOKExtMatch(ext, "typ")) {
        TOK_DISPATCH(TYST, TYSTLexer);
    }

    // Agda
    if (TOKExtMatch(ext, "agda")) {
        TOK_DISPATCH(AGDT, AGDTLexer);
    }

    // Verilog
    if (TOKExtMatch(ext, "v") || TOKExtMatch(ext, "sv")) {
        TOK_DISPATCH(VERT, VERTLexer);
    }

    // Unknown extension: fall back to plain text
    TOK_DISPATCH(TXTT, TXTTLexer);
}
