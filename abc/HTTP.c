#include "HTTP.h"

ok64 HTTPonMethod(u8cs tok, HTTPstate* state) {
    u8csMv(state->method, tok);
    return OK;
}
ok64 HTTPonRequestURI(u8cs tok, HTTPstate* state) {
    u8csMv(state->uri, tok);
    return OK;
}
ok64 HTTPonHTTPVersion(u8cs tok, HTTPstate* state) {
    u8csMv(state->version, tok);
    return OK;
}
ok64 HTTPonRequestLine(u8cs tok, HTTPstate* state) { return OK; }
ok64 HTTPonFieldName(u8cs tok, HTTPstate* state) {
    return u8cssFeed1(state->headers, tok);
}
ok64 HTTPonFieldValue(u8cs tok, HTTPstate* state) {
    return u8cssFeed1(state->headers, tok);
}
ok64 HTTPonMessageHeader(u8cs tok, HTTPstate* state) { return OK; }
ok64 HTTPonRequestHead(u8cs tok, HTTPstate* state) { return OK; }
ok64 HTTPonBody(u8cs tok, HTTPstate* state) { return OK; }
ok64 HTTPonRequest(u8cs tok, HTTPstate* state) { return OK; }
ok64 HTTPonStatusCode(u8cs tok, HTTPstate* state) {
    u8csMv(state->status_code, tok);
    return OK;
}
ok64 HTTPonReasonPhrase(u8cs tok, HTTPstate* state) {
    u8csMv(state->reason, tok);
    return OK;
}
ok64 HTTPonStatusLine(u8cs tok, HTTPstate* state) { return OK; }
ok64 HTTPonResponse(u8cs tok, HTTPstate* state) { return OK; }
ok64 HTTPonMessage(u8cs tok, HTTPstate* state) { return OK; }
ok64 HTTPonRoot(u8cs tok, HTTPstate* state) { return OK; }

ok64 HTTPutf8Drain(u8cs from, HTTPstate* http) {
    u8csMv(http->data, from);
    return HTTPLexer(http);
}

ok64 HTTPutf8Feed(u8s into, HTTPstate const* http) {
    ok64 o = OK;
    if (!u8csEmpty(http->method)) {
        // Request line
        o = u8sFeed(into, http->method);
        if (o != OK) return o;
        o = u8sFeed1(into, ' ');
        if (o != OK) return o;
        o = u8sFeed(into, http->uri);
        if (o != OK) return o;
        o = u8sFeed1(into, ' ');
        if (o != OK) return o;
        o = u8sFeed(into, http->version);
        if (o != OK) return o;
        o = u8sFeed2(into, '\r', '\n');
        if (o != OK) return o;
    } else if (!u8csEmpty(http->status_code)) {
        // Status line
        o = u8sFeed(into, http->version);
        if (o != OK) return o;
        o = u8sFeed1(into, ' ');
        if (o != OK) return o;
        o = u8sFeed(into, http->status_code);
        if (o != OK) return o;
        o = u8sFeed1(into, ' ');
        if (o != OK) return o;
        o = u8sFeed(into, http->reason);
        if (o != OK) return o;
        o = u8sFeed2(into, '\r', '\n');
        if (o != OK) return o;
    }
    // Headers
    u8css hdrs = {http->headers[0], *http->headers};
    while ($len(hdrs) >= 2) {
        o = u8sFeed(into, hdrs[0][0]);
        if (o != OK) return o;
        o = u8sFeed2(into, ':', ' ');
        if (o != OK) return o;
        o = u8sFeed(into, hdrs[0][1]);
        if (o != OK) return o;
        o = u8sFeed2(into, '\r', '\n');
        if (o != OK) return o;
        hdrs[0] += 2;
    }
    // End of headers
    o = u8sFeed2(into, '\r', '\n');
    return o;
}

ok64 HTTPfind(u8cs *value, u8cs key, u8css headers) {
    for (size_t i = 0; i + 1 < $len(headers); i += 2) {
        if ($eq($at(headers, i), key)) {
            u8csMv(*value, $at(headers, i + 1));
            return OK;
        }
    }
    return HTTPNONE;
}
