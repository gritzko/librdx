#ifndef ABC_URI_H
#define ABC_URI_H

#include "BUF.h"

con ok64 URIfail = 0x1e6d2aa5b70;

#define URIenum 0

typedef struct {
    u8cs text;

    u8cs scheme;
    u8cs user;
    u8cs host;
    u8cs port;
    u8cs path;
    u8cs query;
    u8cs fragment;
} URIstate;

ok64 URIlexer(URIstate *state);

#endif
