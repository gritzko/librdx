#ifndef ABC_SORT_H
#define ABC_SORT_H
#include "INT.h"

con ok64 SORTnoroom = 0xc73cf6cf275b61c;
con ok64 SORTnodata = 0x978968cf275b61c;

fun ok64 SORTu64x($u8c s, $u8c rest) { return $u8take(s, rest, sizeof(u64)); }

fun ok64 SORTu64y($u8 into, $$u8c eqs) { return $u8feed(into, (u8c$c)eqs[0]); }

fun int SORTu64z($cu8c *a, $cu8c *b) {
    return u64cmp((u64c *)**a, (u64c *)**b);
}

#define _X(name) SORTu64##name
#include "Yx.h"
#undef _X

ok64 SORTu64($u64 into, $u64 from);

#endif
