#include "PACK.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "PRO.h"

ok64 PACKCreate(packp p, const char *path, u64 maxlen) {
    sane(p != NULL && path != NULL && maxlen > 0);

    memset(p, 0, sizeof(pack));

    // Calculate pages needed
    u64 npages = (maxlen + PAGESIZE - 1) / PAGESIZE;
    u64 nblocks = PACKIdxBlocks(npages);

    // Create PAGE for buffer (use 16 pages as working buffer)
    u64 bufpages = 16;
    if (bufpages > npages) bufpages = npages;
    call(PAGECreate, &p->pg, bufpages * PAGESIZE, NULL, NULL);

    // Map index buffer
    size_t idxsize = nblocks * PACK_BLOCK_SIZE;
    u64 *idx = mmap(NULL, idxsize, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    test(idx != MAP_FAILED, PACKfail);
    ((u64 **)p->idx)[0] = idx;
    ((u64 **)p->idx)[1] = idx;
    ((u64 **)p->idx)[2] = idx;
    ((u64 **)p->idx)[3] = idx + nblocks * 4;

    // Open file for writing
    p->fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    test(p->fd >= 0, PACKfail);

    p->datalen = 0;
    p->foff = 0;
    p->writing = YES;

    done;
}

// Compress and write one page, update index
fun ok64 PACKWritePage(packp p, u8cp data, size_t len) {
    sane(p != NULL && data != NULL && len > 0);

    // Compression buffer on stack
    u8 compressed[PACK_MAX_COMPRESSED];

    int clen = LZ4_compress_default((const char *)data, (char *)compressed,
                                    (int)len, PACK_MAX_COMPRESSED);
    test(clen > 0, PACKfail);

    // Write compressed data
    ssize_t w = write(p->fd, compressed, clen);
    test(w == clen, PACKfail);

    // Update index
    u64 pagenum = p->datalen / PAGESIZE;
    u64 block = pagenum / PACK_PAGES_PER_BLOCK;
    u64 pos = pagenum % PACK_PAGES_PER_BLOCK;
    u64p blk = p->idx[0] + block * 4;

    // Set block offset for first page in block
    if (pos == 0) {
        blk[0] = p->foff;
    }

    // Set compressed length
    PACKIdxSetLen(blk, pos, (u16)clen);

    p->foff += clen;
    p->datalen += len;

    done;
}

ok64 PACKFlush(packp p) {
    sane(p != NULL && p->writing);

    // Get data from PAGE buffer
    u8p data = p->pg->buf[1];
    u8p end = p->pg->buf[2];
    size_t datalen = end - data;

    // Compress and write complete pages
    while (datalen >= PAGESIZE) {
        call(PACKWritePage, p, data, PAGESIZE);
        data += PAGESIZE;
        datalen -= PAGESIZE;
    }

    // Shift remaining data to start
    if (datalen > 0 && data != p->pg->buf[0]) {
        memmove(p->pg->buf[0], data, datalen);
    }

    // Update buffer pointers
    ((u8 **)p->pg->buf)[1] = p->pg->buf[0];
    ((u8 **)p->pg->buf)[2] = p->pg->buf[0] + datalen;

    done;
}

ok64 PACKClose(packp p) {
    sane(p != NULL);

    if (p->writing) {
        // Flush complete pages
        call(PACKFlush, p);

        // Write remaining partial page (if any)
        u8p data = p->pg->buf[1];
        size_t remaining = p->pg->buf[2] - data;
        if (remaining > 0) {
            call(PACKWritePage, p, data, remaining);
        }

        // Write index
        u64 npages = (p->datalen + PAGESIZE - 1) / PAGESIZE;
        u64 nblocks = PACKIdxBlocks(npages);
        u64 idxsize = nblocks * PACK_BLOCK_SIZE;
        ssize_t w = write(p->fd, p->idx[0], idxsize);
        test(w == (ssize_t)idxsize, PACKfail);

        // Write trailer: uncompressed length (u64) + index size (u64)
        u64 trailer[2] = {p->datalen, idxsize};
        w = write(p->fd, trailer, sizeof(trailer));
        test(w == sizeof(trailer), PACKfail);
    }

    // Cleanup
    if (p->fd >= 0) {
        close(p->fd);
        p->fd = -1;
    }
    if (p->pg) {
        PAGEClose(p->pg);
        p->pg = NULL;
    }
    if (p->idx[0]) {
        size_t idxsize = (u8 *)p->idx[3] - (u8 *)p->idx[0];
        munmap(p->idx[0], idxsize);
        ((u64 **)p->idx)[0] = NULL;
        ((u64 **)p->idx)[3] = NULL;
    }

    done;
}

ok64 PACKOpen(packp p, const char *path) {
    sane(p != NULL && path != NULL);

    memset(p, 0, sizeof(pack));

    // Open file for reading
    p->fd = open(path, O_RDONLY);
    test(p->fd >= 0, PACKfail);

    // Get file size
    off_t fsize = lseek(p->fd, 0, SEEK_END);
    test(fsize > 16, PACKcorrupt);  // At least trailer size

    // Read trailer: uncompressed length (u64) + index size (u64)
    u64 trailer[2];
    test(lseek(p->fd, fsize - 16, SEEK_SET) >= 0, PACKfail);
    test(read(p->fd, trailer, 16) == 16, PACKfail);

    p->datalen = trailer[0];
    u64 idxsize = trailer[1];

    u64 npages = (p->datalen + PAGESIZE - 1) / PAGESIZE;
    test(idxsize == PACKIdxSize(npages), PACKcorrupt);

    // Map index buffer
    u64 nblocks = PACKIdxBlocks(npages);
    u64 *idx = mmap(NULL, idxsize, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    test(idx != MAP_FAILED, PACKfail);
    ((u64 **)p->idx)[0] = idx;
    ((u64 **)p->idx)[1] = idx;
    ((u64 **)p->idx)[2] = idx;
    ((u64 **)p->idx)[3] = idx + nblocks * 4;

    // Read index
    off_t idxoff = fsize - 16 - idxsize;
    test(lseek(p->fd, idxoff, SEEK_SET) >= 0, PACKfail);
    test(read(p->fd, p->idx[0], idxsize) == (ssize_t)idxsize, PACKfail);

    // Create PAGE for decompressed data, set buf[2] to actual data length
    call(PAGECreate, &p->pg, npages * PAGESIZE, PACKEnsure, p);
    ((u8 **)p->pg->buf)[2] = p->pg->buf[0] + p->datalen;

    p->writing = NO;

    done;
}

ok64 PACKEnsure(pagep pg, b8 rw, u64 pos, size_t len) {
    packp p = (packp)pg->ctx;
    sane(p != NULL && !p->writing);

    // Currently only read (decompress) is implemented
    test(rw == NO, PACKbadarg);

    // Calculate page range
    u64 first = pos / PAGESIZE;
    u64 last = (pos + len - 1) / PAGESIZE;

    u64 npages = (p->datalen + PAGESIZE - 1) / PAGESIZE;
    test(last < npages, PACKbadarg);

    // Compression buffer for reading
    u8 compressed[PACK_MAX_COMPRESSED];

    for (u64 pg = first; pg <= last; pg++) {
        // Skip if already loaded
        if (PAGEIdxRead(p->pg, pg) == PAGE_LOADED) continue;

        // Get offset and length from index
        u64 off = PACKIdxOffset(p->idx[0], pg);
        u16 clen = PACKIdxPageLen(p->idx[0], pg);

        test(clen > 0 && clen <= PACK_MAX_COMPRESSED, PACKcorrupt);

        // Read compressed data
        test(lseek(p->fd, off, SEEK_SET) >= 0, PACKfail);
        test(read(p->fd, compressed, clen) == clen, PACKfail);

        // Decompress into page buffer
        u8p dst = p->pg->buf[0] + pg * PAGESIZE;
        int dlen = LZ4_decompress_safe((const char *)compressed, (char *)dst,
                                       clen, PAGESIZE);
        test(dlen > 0, PACKcorrupt);

        // Mark page as loaded
        PAGEIdxSetRead(p->pg, pg, PAGE_LOADED);
    }

    done;
}
