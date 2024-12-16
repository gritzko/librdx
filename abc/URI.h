#ifndef ABC_URI_H
#define ABC_URI_H

#include "BUF.h"

con ok64 URIfail = 0x30b65a926de;

#define URIenum 0

typedef struct {
    $u8c text;

    $u8c scheme;
    $u8c user;
    $u8c host;
    $u8c port;
    $u8c path;
    $u8c query;
    $u8c fragment;
} URIstate;

ok64 URIlexer(URIstate *state);

#endif
