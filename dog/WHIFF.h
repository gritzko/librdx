#ifndef DOG_WHIFF_H
#define DOG_WHIFF_H

//  WHIFF: 64-bit tagged word with type[4] | id[20] | offset[40].
//
//  Canonical layout used across dogs:
//    keeper: type=pack_format, id=file_number, off=byte_offset
//    graf:   type=entry_kind,  id=generation,  off=hashlet
//    sniff:  type=flags,       id=path_index,  off=mtime

#include "abc/INT.h"

typedef u64 wh64;

#define WHIFF_TYPE_SHIFT  60
#define WHIFF_TYPE_MASK   0xfULL
#define WHIFF_ID_SHIFT    40
#define WHIFF_ID_BITS     20
#define WHIFF_ID_MASK     ((1ULL << WHIFF_ID_BITS) - 1)
#define WHIFF_OFF_BITS    40
#define WHIFF_OFF_MASK    ((1ULL << WHIFF_OFF_BITS) - 1)

fun wh64 wh64Pack(u8 type, u32 id, u64 off) {
    return ((u64)(type & WHIFF_TYPE_MASK) << WHIFF_TYPE_SHIFT) |
           ((u64)(id & WHIFF_ID_MASK) << WHIFF_ID_SHIFT) |
           (off & WHIFF_OFF_MASK);
}

fun u8  wh64Type(wh64 v) { return (u8)((v >> WHIFF_TYPE_SHIFT) & WHIFF_TYPE_MASK); }
fun u32 wh64Id(wh64 v)   { return (u32)((v >> WHIFF_ID_SHIFT) & WHIFF_ID_MASK); }
fun u64 wh64Off(wh64 v)  { return v & WHIFF_OFF_MASK; }

#endif
