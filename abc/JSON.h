#ifndef ABC_JSON_H
#define ABC_JSON_H

#include "INT.h"
#include "UTF8.h"
#include "HEX.h"

con ok64 JSONBAD = 0x137185cb28d;
con ok64 JSONFAIL = 0x4dc6173ca495;

// SAX event handler type: receives token slice and user context
typedef ok64 (*JSONsax_fn)(u8cs tok, void *ctx);

// Parse state for the lexer callbacks
typedef struct {
    u8cs       data;            // source JSON text ('data' for lex compat)
    void      *ctx;             // user context passed to handlers
    JSONsax_fn on_string;
    JSONsax_fn on_number;
    JSONsax_fn on_literal;
    JSONsax_fn on_open_object;
    JSONsax_fn on_close_object;
    JSONsax_fn on_open_array;
    JSONsax_fn on_close_array;
    JSONsax_fn on_colon;
    JSONsax_fn on_comma;
} JSONstate;

// Lexer (generated from JSON.c.rl)
ok64 JSONLexer(JSONstate *state);

// Escape functions
ok64 JSONEscape(u8s txt, u8cs val);
ok64 JSONUnEscape(u8s tlv, u8cs txt);
ok64 JSONEscapeAll(u8s into, u8cs from);
ok64 JSONUnEscapeAll(u8s tlv, u8cs txt);

// JSON formatter context (SAX consumer that writes formatted JSON)
typedef struct {
    u8s  out;      // output buffer
    u8cs indent;   // indent per level ("" for compact)
    u8   depth;    // current nesting depth
    b8   need_nl;  // pending newline before next content
} JSONfmt;

// Wire formatter callbacks into a JSONstate
ok64 JSONfmtInit(JSONstate *state, JSONfmt *fmt);

// Convenience: lex JSON, format into output buffer
ok64 JSONFmt(u8s out, u8cs json, u8cs indent);

#endif
