#ifndef ABC_AREN_H
#define ABC_AREN_H

#include "INT.h"

#define align32(s)                       \
    {                                    \
        size_t rem = ((size_t)s[0]) & 3; \
        rem = (4 - rem) & 3;             \
        s[0] += rem;                     \
    }

#define align64(s)                       \
    {                                    \
        size_t rem = ((size_t)s[0]) & 7; \
        rem = (8 - rem) & 7;             \
        s[0] += rem;                     \
    }

#define a32(n, val, arena)   \
    align32(arena);          \
    u32* n = (u32*)arena[0]; \
    *n = val;                \
    arena[0] += 4;

#define a64(n, val, arena)   \
    align64(arena);          \
    u64* n = (u64*)arena[0]; \
    *n = val;                \
    arena[0] += 8;

#define arec(T, n, arena)           \
    align64(arena);                 \
    T* n = (T*)arena[0];            \
    memset(arena[0], 0, sizeof(T)); \
    arena[0] += sizeof(T);

#define afed(n, feed, s, ...) \
    align64(s);               \
    $u8 n = {s[0], NULL};      \
    feed(s, __VA_ARGS__);     \
    n[1] = s[0];

#define afedc(n, feed, s, ...) \
    align64(s);                \
    u8cs n = {s[0], NULL};      \
    feed(s, __VA_ARGS__);      \
    n[1] = s[0];

#endif
