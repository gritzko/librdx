#include "URI.h"

ok64 URIonPath($cu8c tok, URIstate* state) {
    $mv(state->path, tok);
    return OK;
}
ok64 URIonScheme($cu8c tok, URIstate* state) {
    $mv(state->scheme, tok);
    --state->scheme[1];
    return OK;
}
ok64 URIonIPv4address($cu8c tok, URIstate* state) { return OK; }
ok64 URIonIPvFuture($cu8c tok, URIstate* state) { return OK; }
ok64 URIonIPv6address($cu8c tok, URIstate* state) { return OK; }
ok64 URIonIP_literal($cu8c tok, URIstate* state) { return OK; }
ok64 URIonUser($cu8c tok, URIstate* state) {
    $mv(state->user, tok);
    return OK;
}
ok64 URIonHost($cu8c tok, URIstate* state) {
    $mv(state->host, tok);
    return OK;
}
ok64 URIonPort($cu8c tok, URIstate* state) {
    $mv(state->port, tok);
    return OK;
}
ok64 URIonFragment($cu8c tok, URIstate* state) {
    $mv(state->fragment, tok);
    return OK;
}
ok64 URIonQuery($cu8c tok, URIstate* state) {
    $mv(state->query, tok);
    return OK;
}
ok64 URIonURI($cu8c tok, URIstate* state) { return OK; }
ok64 URIonRoot($cu8c tok, URIstate* state) { return OK; }
