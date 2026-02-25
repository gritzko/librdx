#ifndef ABC_JSON_H
#define ABC_JSON_H

#include "INT.h"
#include "RON.h"
#include "SLOG.h"
#include "TLV.h"
#include "UTF8.h"
#include "HEX.h"

con ok64 JSONBAD = 0x137185cb28d;
con ok64 JSONFAIL = 0x4dc6173ca495;

// BASON type bytes
#define BASON_B 'B'  // Boolean/Null
#define BASON_A 'A'  // Array
#define BASON_S 'S'  // String
#define BASON_O 'O'  // Object
#define BASON_N 'N'  // Number

typedef struct {
    u8bp  buf;    // buffer with binary data
    u64bp stack;  // SLOG offsets + nesting state
    u8cs  key;    // current entry key
    u8cs  val;    // current entry value
    u8    lit;    // value type: O/A/S/N/B
    u8    plit;   // parent type: O/A or 0 for root
} slit;

// Parse state for the lexer callbacks
typedef struct {
    slit   it;
    u8cs   data;          // source JSON text ('data' for lex compat)
    u64    arr_index;     // current array element index
    u8     obj_state;     // 0=expect key, 1=colon, 2=value
    u8     pending_key[256];
    u8s    pending_key_s; // slice into pending_key
} JSONstate;

// Write path
ok64 JSONWriteNext(slit *it);
ok64 JSONWriteInto(slit *it, u64 saved);
ok64 JSONWriteOuto(slit *it, u64p saved);

// Read path
ok64 JSONOpen(slit *it, u8bp buf, u64bp stack);
ok64 JSONNext(slit *it);
ok64 JSONInto(slit *it);
ok64 JSONOuto(slit *it);

// API entry points
ok64 JSONParse(u8bp buf, u64bp stack, u8cs json);
ok64 JSONExport(u8s json, u8cs bason);

// Lexer (generated from JSON.c.rl)
ok64 JSONLexer(JSONstate *state);

// Escape functions
ok64 JSONEscape(u8s txt, u8cs val);
ok64 JSONUnEscape(u8s tlv, u8cs txt);
ok64 JSONEscapeAll(u8s into, u8cs from);
ok64 JSONUnEscapeAll(u8s tlv, u8cs txt);

#endif
