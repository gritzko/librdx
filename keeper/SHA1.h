#ifndef XX_SHA1_H
#define XX_SHA1_H

//  SHA1: SHA-1 hash wrapper, isolated from ABC type system.
//  Used for git object IDs.

#include <stdint.h>

//  Compute SHA-1 hash of data[0..len).
void SHA1Sum(uint8_t out[20], uint8_t const *data, uint64_t len);

#endif
