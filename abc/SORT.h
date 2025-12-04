#ifndef ABC_SORT_H
#define ABC_SORT_H
#include "INT.h"

con ok64 SORTnoroom = 0x7186ddcb3db3cf1;
con ok64 SORTnodata = 0x7186ddcb3a25e25;

fun ok64 SORTu64x(u8cs s, u8cs rest) { return $u8take(s, rest, sizeof(u64)); }

fun ok64 SORTu64y($u8 into, u8css eqs) { return u8sFeed(into, (u8c$c)eqs[0]); }

fun ok64 SORTu64z($cu8c *a, $cu8c *b) { return u64Z((u64c *)**a, (u64c *)**b); }

#define _X(name) SORTu64##name
#include "Yx.h"
#undef _X

ok64 SORTu64($u64 into, $u64 from);

#endif
