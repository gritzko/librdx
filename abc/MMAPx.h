#include "MMAP.h"

#define T X(, )

fun ok64 X(MMAP, open)(X(, B) buf, size_t len) {
    return MMAPopen((voidB)buf, len * sizeof(T));
}

fun ok64 X(MMAP, close)(X(, B) buf) { return MMAPclose((voidB)buf); }

#undef T
