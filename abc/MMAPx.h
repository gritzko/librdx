#include "MMAP.h"

#define T X(, )

fun ok64 X(MMAP, open)(X(, b) buf, size_t len) {
    return MMAPopen((voidbp)buf, len * sizeof(T));
}

fun ok64 X(MMAP, close)(X(, b) buf) { return MMAPclose((voidbp)buf); }

#undef T
