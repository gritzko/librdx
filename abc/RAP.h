//
// rapidhash wrapper - fast non-crypto hash
// See rapidhash.h for license (MIT)
//
#ifndef ABC_RAP_H
#define ABC_RAP_H

#include "INT.h"
#include "rapidhash.h"

// Standard rapidhash - balanced speed and quality
fun u64 RAPHash(u8csc data) {
    return rapidhash(*data, $len(data));
}

fun u64 RAPHashSeed(u8csc data, u64 seed) {
    return rapidhash_withSeed(*data, $len(data), seed);
}

// Micro variant - HPC/server, cache efficient, ~140 instructions
// Faster for sizes up to 512 bytes, 15-20% slower above 1kb
fun u64 RAPMicro(u8csc data) {
    return rapidhashMicro(*data, $len(data));
}

fun u64 RAPMicroSeed(u8cs data, u64 seed) {
    return rapidhashMicro_withSeed(*data, $len(data), seed);
}

// Nano variant - mobile/embedded, <100 instructions
// Fastest for sizes up to 48 bytes, slower for larger inputs
fun u64 RAPNano(u8cs data) {
    return rapidhashNano(*data, $len(data));
}

fun u64 RAPNanoSeed(u8cs data, u64 seed) {
    return rapidhashNano_withSeed(*data, $len(data), seed);
}

#endif
