#ifndef ABC_HTTP_H
#define ABC_HTTP_H

#include "BUF.h"

static const ok64 HTTPfail = 0x45d759aa5b70;
static const ok64 HTTPnone = 0x45d759cb3ca9;

#define HTTPenum 0

typedef struct {
    $u8c text;

    $u8c$ parsed;
} HTTPstate;

ok64 HTTPlexer(HTTPstate *state);

ok64 HTTPfind(u8c$ value, $cu8c key, $$u8c parsed);

#endif
