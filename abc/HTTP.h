#ifndef ABC_HTTP_H
#define ABC_HTTP_H

#include "BUF.h"

con ok64 HTTPfail = 0xc2d96a65d751;
con ok64 HTTPnone = 0xa72cf265d751;

#define HTTPenum 0

typedef struct {
    $u8c text;

    $u8c$ parsed;
} HTTPstate;

ok64 HTTPlexer(HTTPstate *state);

ok64 HTTPfind(u8c$ value, $cu8c key, $$u8c parsed);

#endif
