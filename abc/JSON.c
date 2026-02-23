
#include "JSON.h"

#include "B.h"
#include "OK.h"
#include "PRO.h"

// --- Write path ---

ok64 JSONWriteNext(slit *it) {
    sane(it != NULL && it->buf != NULL);
    if (it->plit == 0) {
        // root level: plain TLV, no key
        u8cs v = {it->val[0], it->val[1]};
        call(TLVu8sFeed, u8bIdle(it->buf), it->lit, v);
    } else {
        u8cs k = {it->key[0], it->key[1]};
        u8cs v = {it->val[0], it->val[1]};
        call(TLVFeedKeyVal, u8bIdle(it->buf), it->lit, k, v);
    }
    done;
}

ok64 JSONWriteInto(slit *it) {
    sane(it != NULL && it->buf != NULL);
    // push parent plit
    u64 plit64 = it->plit;
    call(u64bFeed1, it->stack, plit64);
    // open TLV container
    call(TLVu8bInto, it->buf, it->lit);
    // if keyed (parent is O or A), write key prefix
    if (it->plit != 0) {
        u8 klen = (u8)u8csLen(it->key);
        call(u8bFeed1, it->buf, klen);
        call(u8bFeed, it->buf, it->key);
    }
    it->plit = it->lit;
    done;
}

ok64 JSONWriteOuto(slit *it) {
    sane(it != NULL && it->buf != NULL);
    u8 saved_lit = it->lit;
    // close TLV container
    call(TLVu8bOuto, it->buf, saved_lit);
    // pop parent plit
    u64 pplit = *u64bLast(it->stack);
    call(u64bPop, it->stack);
    it->plit = (u8)pplit;
    done;
}

// --- Lexer callback helpers ---

static void JSONPendingKeyInit(JSONstate *s) {
    s->pending_key_s[0] = s->pending_key;
    s->pending_key_s[1] = s->pending_key + sizeof(s->pending_key);
}

static ok64 JSONSetKey(JSONstate *state) {
    sane(state != NULL);
    if (state->it.plit == BASON_O) {
        u8cs pk = {state->pending_key, state->pending_key_s[0]};
        state->it.key[0] = pk[0];
        state->it.key[1] = pk[1];
    } else if (state->it.plit == BASON_A) {
        JSONPendingKeyInit(state);
        u8s idx_s = {state->pending_key_s[0], state->pending_key_s[1]};
        ok64 o = RONutf8sFeed(idx_s, state->arr_index);
        if (o != OK) return o;
        state->it.key[0] = state->pending_key;
        state->it.key[1] = idx_s[0];
        ++state->arr_index;
    } else {
        // root level: empty but valid key
        state->it.key[0] = (u8c *)state->pending_key;
        state->it.key[1] = (u8c *)state->pending_key;
    }
    done;
}

// --- Lexer callbacks ---

ok64 JSONonString(u8cs tok, JSONstate *state) {
    sane($ok(tok) && state != NULL);
    u8cs inner = {tok[0] + 1, tok[1] - 1};
    if (state->it.plit == BASON_O && state->obj_state == 0) {
        JSONPendingKeyInit(state);
        u8s into = {state->pending_key_s[0], state->pending_key_s[1]};
        call(JSONUnEscapeAll, into, inner);
        state->pending_key_s[0] = into[0];
        state->obj_state = 1;
        done;
    }
    call(JSONSetKey, state);
    state->it.lit = BASON_S;
    // unescape into a temp area, then write TLKV
    a_pad(u8, tmp, 4096);
    u8s vbuf = {tmp[1], tmp[3]};
    call(JSONUnEscapeAll, vbuf, inner);
    u8cs unesc = {tmp[1], vbuf[0]};
    state->it.val[0] = unesc[0];
    state->it.val[1] = unesc[1];
    call(JSONWriteNext, &state->it);
    if (state->it.plit == BASON_O) state->obj_state = 0;
    done;
}

ok64 JSONonNumber(u8cs tok, JSONstate *state) {
    sane($ok(tok) && state != NULL);
    call(JSONSetKey, state);
    state->it.lit = BASON_N;
    state->it.val[0] = tok[0];
    state->it.val[1] = tok[1];
    call(JSONWriteNext, &state->it);
    if (state->it.plit == BASON_O) state->obj_state = 0;
    done;
}

ok64 JSONonLiteral(u8cs tok, JSONstate *state) {
    sane($ok(tok) && state != NULL);
    call(JSONSetKey, state);
    state->it.lit = BASON_B;
    state->it.val[0] = tok[0];
    state->it.val[1] = tok[1];
    call(JSONWriteNext, &state->it);
    if (state->it.plit == BASON_O) state->obj_state = 0;
    done;
}

ok64 JSONonOpenObject(u8cs tok, JSONstate *state) {
    sane($ok(tok) && state != NULL);
    call(JSONSetKey, state);
    state->it.lit = BASON_O;
    // save obj_state + arr_index
    u64 saved = ((u64)state->obj_state << 32) |
                (u64)(state->arr_index & 0xFFFFFFFF);
    call(u64bFeed1, state->it.stack, saved);
    call(JSONWriteInto, &state->it);
    state->obj_state = 0;
    state->arr_index = 0;
    done;
}

ok64 JSONonCloseObject(u8cs tok, JSONstate *state) {
    sane($ok(tok) && state != NULL);
    state->it.lit = BASON_O;
    call(JSONWriteOuto, &state->it);
    // restore saved state
    u64 saved = *u64bLast(state->it.stack);
    call(u64bPop, state->it.stack);
    state->obj_state = (u8)(saved >> 32);
    state->arr_index = saved & 0xFFFFFFFF;
    if (state->it.plit == BASON_O) state->obj_state = 0;
    done;
}

ok64 JSONonOpenArray(u8cs tok, JSONstate *state) {
    sane($ok(tok) && state != NULL);
    call(JSONSetKey, state);
    state->it.lit = BASON_A;
    u64 saved = ((u64)state->obj_state << 32) |
                (u64)(state->arr_index & 0xFFFFFFFF);
    call(u64bFeed1, state->it.stack, saved);
    call(JSONWriteInto, &state->it);
    state->obj_state = 0;
    state->arr_index = 0;
    done;
}

ok64 JSONonCloseArray(u8cs tok, JSONstate *state) {
    sane($ok(tok) && state != NULL);
    state->it.lit = BASON_A;
    call(JSONWriteOuto, &state->it);
    u64 saved = *u64bLast(state->it.stack);
    call(u64bPop, state->it.stack);
    state->obj_state = (u8)(saved >> 32);
    state->arr_index = saved & 0xFFFFFFFF;
    if (state->it.plit == BASON_O) state->obj_state = 0;
    done;
}

ok64 JSONonColon(u8cs tok, JSONstate *state) {
    sane($ok(tok) && state != NULL);
    state->obj_state = 2;
    done;
}

ok64 JSONonComma(u8cs tok, JSONstate *state) {
    sane($ok(tok) && state != NULL);
    if (state->it.plit == BASON_O) state->obj_state = 0;
    done;
}

ok64 JSONonJSON(u8cs tok, JSONstate *state) { return OK; }

ok64 JSONonRoot(u8cs tok, JSONstate *state) { return OK; }

// --- Parse entry point ---

ok64 JSONParse(u8bp buf, u64bp stack, u8cs json) {
    sane(buf != NULL && stack != NULL && $ok(json));
    JSONstate state = {};
    state.it.buf = buf;
    state.it.stack = stack;
    state.it.plit = 0;
    state.data[0] = json[0];
    state.data[1] = json[1];
    JSONPendingKeyInit(&state);
    call(JSONLexer, &state);
    done;
}

// --- Export helpers ---

static ok64 JSONExportValue(u8s json, u8cs body, u8 lit);

static ok64 JSONExportContainer(u8s json, u8cs body, u8 lit) {
    sane(u8sOK(json) && $ok(body));
    u8 open = (lit == BASON_O) ? '{' : '[';
    u8 close = (lit == BASON_O) ? '}' : ']';
    call(u8sFeed1, json, open);
    b8 first = YES;
    while (!$empty(body)) {
        u8 typ = 0;
        u8cs val = {};
        u8cs peek = {body[0], body[1]};
        ok64 o = TLVu8sDrain(peek, &typ, val);
        if (o != OK) break;
        if (typ == SLOG_K_N || typ == SLOG_C_N) {
            body[0] = peek[0];
            if (typ == SLOG_C_N) break;
            continue;
        }
        if (!first) {
            call(u8sFeed1, json, ',');
        }
        first = NO;
        u8 ktyp = 0;
        u8cs key = {};
        u8cs kval = {};
        call(TLVDrainKeyVal, &ktyp, key, kval, body);
        if (lit == BASON_O) {
            call(u8sFeed1, json, '"');
            call(JSONEscapeAll, json, key);
            call(u8sFeed2, json, '"', ':');
        }
        call(JSONExportValue, json, kval, ktyp);
    }
    call(u8sFeed1, json, close);
    done;
}

static ok64 JSONExportValue(u8s json, u8cs body, u8 lit) {
    sane(u8sOK(json));
    if (lit == BASON_O || lit == BASON_A) {
        call(JSONExportContainer, json, body, lit);
    } else if (lit == BASON_S) {
        call(u8sFeed1, json, '"');
        call(JSONEscapeAll, json, body);
        call(u8sFeed1, json, '"');
    } else {
        call(u8sFeed, json, body);
    }
    done;
}

ok64 JSONExport(u8s json, u8cs bason) {
    sane(u8sOK(json) && $ok(bason));
    u8cs body = {bason[0], bason[1]};
    while (!$empty(body)) {
        u8 typ = 0;
        u8cs val = {};
        call(TLVu8sDrain, body, &typ, val);
        call(JSONExportValue, json, val, typ);
    }
    done;
}

// --- Escape functions ---

ok64 JSONEscape(u8s txt, u8cs val) {
    sane(u8sOK(txt) && u8csOK(val));
    switch (**val) {
        case '\t':
            call(u8sFeed2, txt, '\\', 't');
            break;
        case '\r':
            call(u8sFeed2, txt, '\\', 'r');
            break;
        case '\n':
            call(u8sFeed2, txt, '\\', 'n');
            break;
        case '\b':
            call(u8sFeed2, txt, '\\', 'b');
            break;
        case '\f':
            call(u8sFeed2, txt, '\\', 'f');
            break;
        case '\\':
            call(u8sFeed2, txt, '\\', '\\');
            break;
        case '"':
            call(u8sFeed2, txt, '\\', '"');
            break;
        default:
            if (**val < 0x20) {
                static const char hex[] = "0123456789abcdef";
                u8 c = **val;
                call(u8sFeed2, txt, '\\', 'u');
                call(u8sFeed2, txt, '0', '0');
                call(u8sFeed2, txt, hex[c >> 4], hex[c & 0xf]);
            } else {
                call(u8sFeed1, txt, **val);
            }
    }
    ++*val;
    done;
}

ok64 JSONUnEscape(u8s tlv, u8cs txt) {
    if ($empty(tlv)) return NOROOM;
    if ($empty(txt)) return JSONBAD;
    if (**txt != '\\') {
        **tlv = **txt;
        ++*tlv;
        ++*txt;
        return OK;
    }
    ++*txt;
    if ($empty(txt)) return JSONBAD;
    switch (**txt) {
        case 't': **tlv = '\t'; break;
        case 'r': **tlv = '\r'; break;
        case 'n': **tlv = '\n'; break;
        case 'b': **tlv = '\b'; break;
        case 'f': **tlv = '\f'; break;
        case '0': **tlv = 0; break;
        case '\\': **tlv = '\\'; break;
        case '/': **tlv = '/'; break;
        case '"': **tlv = '"'; break;
        case 'u': {
            if ($len(txt) < 5) return HEXNODATA;
            u8cs hex = {*txt + 1, *txt + 5};
            *txt += 4;
            u64 cp = 0;
            ok64 o = u64hexdrain(&cp, hex);
            if (o != OK) return o;
            o = utf8sFeed32(tlv, cp);
            if (o != OK) return o;
            --*tlv;
            break;
        }
        default:
            return JSONBAD;
    }
    ++*tlv;
    ++*txt;
    return OK;
}

ok64 JSONEscapeAll(u8s into, u8cs from) {
    sane(u8sOK(into) && u8csOK(from));
    while (!u8csEmpty(from)) {
        call(JSONEscape, into, from);
    }
    done;
}

ok64 JSONUnEscapeAll(u8s tlv, u8cs txt) {
    sane(u8sOK(tlv) && u8csOK(txt));
    while (!$empty(txt) && !$empty(tlv)) {
        if (**txt != '\\') {
            **tlv = **txt;
            ++*tlv;
            ++*txt;
            continue;
        }
        call(JSONUnEscape, tlv, txt);
    }
    if (!$empty(txt)) return NOROOM;
    return OK;
}
