#include "HEX.h"

#define T X(, )

fun ok64 X(HEX, put)($u8 hex, T const* x) {
    u8cs bin = {(u8*)x, (u8*)(x + 1)};
    return HEXfeed(hex, bin);
}
