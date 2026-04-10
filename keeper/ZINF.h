#ifndef XX_ZINF_H
#define XX_ZINF_H

//  ZINF: zlib inflate wrapper
//  Isolated from ABC headers to avoid voidpc typedef clash.

#include <stdint.h>

//  Inflate zlib-compressed data.
//  Returns 0 on success, -1 on init failure, -2 on inflate failure.
int ZINFInflate(
    uint8_t const *src, uint64_t srclen,
    uint8_t *dst, uint64_t dstlen,
    uint64_t *consumed, uint64_t *produced);

//  Deflate (compress) data.
//  Returns 0 on success, -1 on failure.
int ZINFDeflate(
    uint8_t const *src, uint64_t srclen,
    uint8_t *dst, uint64_t dstcap,
    uint64_t *produced);

#endif
