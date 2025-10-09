
#include "JSON.h"

#include "B.h"
#include "JSON.rl.h"
#include "OK.h"
#include "PRO.h"

const char* JSON_NODE_NAMES[] = {
    "JSON_NODE_ROOT",   "JSON_NODE_LITERAL", "JSON_NODE_NUMBER",
    "JSON_NODE_STRING", "JSON_NODE_ARRAY",   "JSON_NODE_OBJECT",
};

ok64 JSONonLiteral($cu8c tok, JSONstate* state) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    u32 ndx = Blast(state->stack);
    js64* prnt = (js64*)Batp(state->json, ndx);
    test((prnt->node == JSON_NODE_OBJECT && (prnt->toks & 1) == 0) ||
             (prnt->node == JSON_NODE_ARRAY && (prnt->toks & 1) == 0) ||
             (prnt->node == JSON_NODE_ROOT && prnt->toks == 0),
         JSONbad);
    ++prnt->toks;
    js64 child = {.pos = $u8offset(state->text, tok),
                  .node = JSON_NODE_LITERAL};
    call(u64BFeedP, state->json, (u64*)&child);
    done;
}

ok64 JSONonString($cu8c tok, JSONstate* state) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    u32 ndx = Blast(state->stack);
    js64* prnt = (js64*)Batp(state->json, ndx);
    test((prnt->node == JSON_NODE_OBJECT && (prnt->toks & 1) == 0) ||
             (prnt->node == JSON_NODE_ARRAY && (prnt->toks & 1) == 0) ||
             (prnt->node == JSON_NODE_ROOT && prnt->toks == 0),
         JSONbad);
    ++prnt->toks;
    js64 child = {.pos = $u8offset(state->text, tok), .node = JSON_NODE_STRING};
    call(u64BFeedP, state->json, (u64*)&child);
    done;
}

ok64 JSONonNumber($cu8c tok, JSONstate* state) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    u32 ndx = Blast(state->stack);
    js64* prnt = (js64*)Batp(state->json, ndx);
    test((prnt->node == JSON_NODE_OBJECT && (prnt->toks & 3) == 2) ||
             (prnt->node == JSON_NODE_ARRAY && (prnt->toks & 1) == 0) ||
             (prnt->node == JSON_NODE_ROOT && prnt->toks == 0),
         JSONbad);
    ++prnt->toks;
    js64 child = {.pos = $u8offset(state->text, tok), .node = JSON_NODE_NUMBER};
    call(u64BFeedP, state->json, (u64*)&child);
    done;
}

ok64 JSONonOpen($cu8c tok, JSONstate* state, int node) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    u32 ndx = Blast(state->stack);
    js64* prnt = (js64*)Batp(state->json, ndx);
    test((prnt->node == JSON_NODE_OBJECT && (prnt->toks & 3) == 2) ||
             (prnt->node == JSON_NODE_ARRAY && (prnt->toks & 1) == 0) ||
             (prnt->node == JSON_NODE_ROOT && prnt->toks == 0),
         JSONbad);
    ++prnt->toks;
    js64 child = {.pos = $u8offset(state->text, tok), .node = node};
    u32 toklen = Bdatalen(state->json);
    call(u64BFeedP, state->json, (u64*)&child);
    call(u32BFeed1, state->stack, toklen);
    done;
}

ok64 JSONonClose($cu8c tok, JSONstate* state, int node) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    u32 ndx = Blast(state->stack);
    u32 toklen = Bdatalen(state->json);
    js64* arr = (js64*)Bu64atp(state->json, ndx);
    arr->toks = toklen - ndx - 1;
    call(Bu32pop, state->stack);
    done;
}

ok64 JSONonOpenObject($cu8c tok, JSONstate* state) {
    return JSONonOpen(tok, state, JSON_NODE_OBJECT);
}

ok64 JSONonCloseObject($cu8c tok, JSONstate* state) {
    return JSONonClose(tok, state, JSON_NODE_OBJECT);
}

ok64 JSONonOpenArray($cu8c tok, JSONstate* state) {
    return JSONonOpen(tok, state, JSON_NODE_ARRAY);
}

ok64 JSONonCloseArray($cu8c tok, JSONstate* state) {
    return JSONonClose(tok, state, JSON_NODE_ARRAY);
}

ok64 JSONonComma($cu8c tok, JSONstate* state) {
    sane($ok(tok) && state != nil);
    u32 ndx = Blast(state->stack);
    js64* prnt = (js64*)Batp(state->json, ndx);
    test((prnt->node == JSON_NODE_OBJECT && (prnt->toks & 3) == 3) ||
             (prnt->node == JSON_NODE_ARRAY && (prnt->toks & 1) == 1),
         JSONbad);
    ++prnt->toks;
    done;
}

ok64 JSONonColon($cu8c tok, JSONstate* state) {
    sane($ok(tok) && state != nil);
    u32 ndx = Blast(state->stack);
    js64* prnt = (js64*)Batp(state->json, ndx);
    test((prnt->node == JSON_NODE_OBJECT && (prnt->toks & 3) == 1), JSONbad);
    ++prnt->toks;
    done;
}

ok64 JSONonJSON($cu8c tok, JSONstate* state) { return OK; }

ok64 JSONonRoot($cu8c tok, JSONstate* state) {
    js64* root = (js64*)Batp(state->json, 0);
    u32 toklen = Bdatalen(state->json);
    root->toks = toklen - 1;
    return OK;
}
