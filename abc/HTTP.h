#ifndef ABC_HTTP_H
#define ABC_HTTP_H

#include "BUF.h"

con ok64 HTTPFAIL = 0x45d7593ca495;
con ok64 HTTPBAD = 0x45d759a25dab;
con ok64 HTTPNONE = 0x45d7595d85ce;

#define HTTPenum 0

typedef struct {
    u8cs data;

    // Request line
    u8cs method;
    u8cs uri;
    u8cs version;

    // Status line (responses)
    u8cs status_code;
    u8cs reason;

    // Headers (name, value pairs)
    u8cssp headers;
} HTTPstate;

ok64 HTTPLexer(HTTPstate *state);

ok64 HTTPutf8Drain(u8cs from, HTTPstate *http);

ok64 HTTPutf8Feed(u8s into, HTTPstate const *http);

ok64 HTTPfind(u8cs *value, u8cs key, u8css headers);

#endif
