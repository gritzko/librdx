#ifndef DOG_W64_H
#define DOG_W64_H

//  WHIFF: 64-bit tagged word with type[4] | id[20] | offset[40].
//
//  Canonical layout used across dogs:
//    keeper: type=pack_format, id=file_number, off=byte_offset
//    graf:   type=entry_kind,  id=generation,  off=hashlet
//    sniff:  type=flags,       id=path_index,  off=mtime

#include "abc/INT.h"

typedef u64 w64;

#define W64_TYPE_SHIFT  60
#define W64_TYPE_MASK   0xfULL
#define W64_ID_SHIFT    40
#define W64_ID_BITS     20
#define W64_ID_MASK     ((1ULL << W64_ID_BITS) - 1)
#define W64_OFF_BITS    40
#define W64_OFF_MASK    ((1ULL << W64_OFF_BITS) - 1)

fun w64 w64Pack(u8 type, u32 id, u64 off) {
    return ((u64)(type & W64_TYPE_MASK) << W64_TYPE_SHIFT) |
           ((u64)(id & W64_ID_MASK) << W64_ID_SHIFT) |
           (off & W64_OFF_MASK);
}

fun u8  w64Type(w64 v) { return (u8)((v >> W64_TYPE_SHIFT) & W64_TYPE_MASK); }
fun u32 w64Id(w64 v)   { return (u32)((v >> W64_ID_SHIFT) & W64_ID_MASK); }
fun u64 w64Off(w64 v)  { return v & W64_OFF_MASK; }

#endif
