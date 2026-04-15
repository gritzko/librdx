//  ZINF: zlib inflate/deflate wrapper, isolated from ABC type system
//
#include <limits.h>
#include <stdint.h>
#include <zlib.h>

#include "ZINF.h"

int ZINFInflate(
    uint8_t const *src, uint64_t srclen,
    uint8_t *dst, uint64_t dstlen,
    uint64_t *consumed, uint64_t *produced)
{
    if (srclen > UINT_MAX || dstlen > UINT_MAX) return -1;

    z_stream zs = {0};
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)srclen;
    zs.next_out = (Bytef *)dst;
    zs.avail_out = (uInt)dstlen;

    int r = inflateInit(&zs);
    if (r != Z_OK) return -1;

    // Inflate in a loop — output buffer may be smaller than decompressed data
    for (;;) {
        r = inflate(&zs, Z_NO_FLUSH);
        if (r == Z_STREAM_END) break;
        if (r != Z_OK) { inflateEnd(&zs); return -2; }
        if (zs.avail_out == 0) {
            // Output full, discard and keep going to find stream end
            zs.next_out = (Bytef *)dst;
            zs.avail_out = (uInt)dstlen;
        }
    }

    *consumed = zs.total_in;
    *produced = zs.total_out;

    inflateEnd(&zs);
    return 0;
}

int ZINFDeflate(
    uint8_t const *src, uint64_t srclen,
    uint8_t *dst, uint64_t dstcap,
    uint64_t *produced)
{
    if (srclen > UINT_MAX || dstcap > UINT_MAX) return -1;

    z_stream zs = {0};
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)srclen;
    zs.next_out = (Bytef *)dst;
    zs.avail_out = (uInt)dstcap;

    int r = deflateInit(&zs, Z_DEFAULT_COMPRESSION);
    if (r != Z_OK) return -1;

    r = deflate(&zs, Z_FINISH);
    if (r != Z_STREAM_END) {
        deflateEnd(&zs);
        return -1;
    }

    *produced = zs.total_out;
    deflateEnd(&zs);
    return 0;
}
