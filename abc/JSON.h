#ifndef ABC_JSON_H
#define ABC_JSON_H

#include "INT.h"

#define JSONenum 0

con ok64 JSONbad = 0x137185e6968;
con ok64 JSONfail = 0x4dc617aa5b70;

typedef enum {
    JSON_NODE_ROOT = 0,
    JSON_NODE_LITERAL = 1,
    JSON_NODE_NUMBER = 2,
    JSON_NODE_STRING = 3,
    JSON_NODE_ARRAY = 4,
    JSON_NODE_OBJECT = 5,
} JSONnode;

extern const char* JSON_NODE_NAMES[];

typedef struct {
    u32 pos;
    u32 toks : 28, node : 4;
} js64;

typedef struct {
    u64B json;
    u32B stack;
    $u8c text;
} JSONstate;

ok64 JSONlexer(JSONstate* state);

#endif
