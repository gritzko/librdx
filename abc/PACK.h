#ifndef ABC_PACK_H
#define ABC_PACK_H

#include "FILE.h"
#include "PAGE.h"
#include <lz4.h>

// Error codes
con ok64 PACKfail = 0x1932c3aa5b70;
con ok64 PACKnoroom = 0x1932c3db3cf1;
con ok64 PACKbadarg = 0x1932c32ca34a6d0;
con ok64 PACKnodata = 0x1932c33834a74a;
con ok64 PACKcorrupt = 0x1932c38c3dde619;

// Index block: 32 bytes, 12 pages
// u64[0]: file offset of first page in block
// u64[1]: len[0..3] as 4 x u16
// u64[2]: len[4..7] as 4 x u16
// u64[3]: len[8..11] as 4 x u16
#define PACK_PAGES_PER_BLOCK 12
#define PACK_BLOCK_SIZE 32

// Max compressed size for a page (LZ4 worst case)
#define PACK_MAX_COMPRESSED (LZ4_COMPRESSBOUND(PAGESIZE))

typedef struct {
    pagep pg;           // PAGE for buffer tracking
    int fd;             // file descriptor
    u64b idx;           // index buffer (pre-mmapped)
    u64 datalen;        // uncompressed data length
    u64 foff;           // current file offset (write mode)
    b8 writing;         // YES = write mode, NO = read mode
} pack;

typedef pack *packp;
typedef pack const *packcp;

// Get compressed length for page within block
fun u16 PACKIdxLen(u64cp block, u32 pos) {
    // pos 0..11 within block
    // u64[1] has pos 0-3, u64[2] has pos 4-7, u64[3] has pos 8-11
    u32 word = 1 + pos / 4;
    u32 shift = (pos % 4) * 16;
    return (block[word] >> shift) & 0xFFFF;
}

// Set compressed length for page within block
fun void PACKIdxSetLen(u64p block, u32 pos, u16 len) {
    u32 word = 1 + pos / 4;
    u32 shift = (pos % 4) * 16;
    block[word] &= ~(0xFFFFULL << shift);
    block[word] |= ((u64)len << shift);
}

// Get file offset for a page
fun u64 PACKIdxOffset(u64cp idx, u64 page_idx) {
    u64 block = page_idx / PACK_PAGES_PER_BLOCK;
    u64 pos = page_idx % PACK_PAGES_PER_BLOCK;
    u64cp blk = idx + block * 4;
    u64 off = blk[0];  // block's base offset
    // Add lengths of preceding pages in this block
    for (u64 i = 0; i < pos; i++) {
        off += PACKIdxLen(blk, i);
    }
    return off;
}

// Get compressed length for a page
fun u16 PACKIdxPageLen(u64cp idx, u64 page_idx) {
    u64 block = page_idx / PACK_PAGES_PER_BLOCK;
    u64 pos = page_idx % PACK_PAGES_PER_BLOCK;
    return PACKIdxLen(idx + block * 4, pos);
}

// Number of index blocks needed for npages
fun u64 PACKIdxBlocks(u64 npages) {
    return (npages + PACK_PAGES_PER_BLOCK - 1) / PACK_PAGES_PER_BLOCK;
}

// Size of index in bytes
fun u64 PACKIdxSize(u64 npages) {
    return PACKIdxBlocks(npages) * PACK_BLOCK_SIZE;
}

// === Write API ===

// Create pack for writing
// Allocates buffer based on maxlen, opens file
ok64 PACKCreate(packp p, const char *path, u64 maxlen);

// Flush complete pages to file
// Compresses and writes all complete 4K pages, shifts buffer
ok64 PACKFlush(packp p);

// Close pack (write mode)
// Flushes remaining data, writes index, closes file
ok64 PACKClose(packp p);

// === Read API ===

// Open pack for reading
// Opens file, reads index from end
ok64 PACKOpen(packp p, const char *path);

// PAGE callback for decompression/compression
ok64 PACKEnsure(pagep pg, b8 rw, u64 pos, size_t len);

#endif
