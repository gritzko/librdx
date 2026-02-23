#include "PAGE.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "FILE.h"
#include "PRO.h"

// Global registry
pagep PAGE_REGISTRY = NULL;
u32 PAGE_REGISTRY_LEN = 0;

ok64 PAGEInit(u32 maxpages) {
    sane(maxpages > 0);

    PAGE_REGISTRY = calloc(maxpages, sizeof(page));
    test(PAGE_REGISTRY != NULL, PAGEnoroom);
    PAGE_REGISTRY_LEN = maxpages;

    done;
}

ok64 PAGECreate(pagep *newpage, u64 maxlen, pagef ensure, void *ctx) {
    sane(newpage != NULL && maxlen > 0 && PAGE_REGISTRY != NULL);

    // Find free slot in registry
    pagep p = NULL;
    for (u32 i = 0; i < PAGE_REGISTRY_LEN; i++) {
        if (PAGE_REGISTRY[i].buf[0] == NULL) {
            p = &PAGE_REGISTRY[i];
            break;
        }
    }
    test(p != NULL, PAGEnoroom);

    // Round up to page boundary
    u64 npages = (maxlen + PAGESIZE - 1) / PAGESIZE;
    u64 buflen = npages * PAGESIZE;

    // Map data region
    void *buf = mmap(NULL, buflen, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) fail(PAGEfail);

    // Allocate index array: u64 per page (+1 sentinel for u8bEnsure2)
    u64 *idx = calloc(npages + 1, sizeof(u64));
    if (idx == NULL) {
        munmap(buf, buflen);
        fail(PAGEnoroom);
    }

    // Init struct (cast to assign to const pointers)
    ((u8 **)p->buf)[0] = buf;
    ((u8 **)p->buf)[1] = buf;
    ((u8 **)p->buf)[2] = buf;
    ((u8 **)p->buf)[3] = (u8 *)buf + buflen;

    ((u64 **)p->idx)[0] = idx;
    ((u64 **)p->idx)[1] = idx;
    ((u64 **)p->idx)[2] = idx;
    ((u64 **)p->idx)[3] = idx + npages;

    p->ensure = ensure;
    p->ctx = ctx;

    *newpage = p;
    done;
}

ok64 PAGEClose(pagep p) {
    sane(p != NULL);

    u64 buflen = p->buf[3] - p->buf[0];
    if (p->buf[0]) munmap(p->buf[0], buflen);
    if (p->idx[0]) free(p->idx[0]);

    // Clear slot
    ((u8 **)p->buf)[0] = NULL;
    ((u8 **)p->buf)[3] = NULL;
    ((u64 **)p->idx)[0] = NULL;
    ((u64 **)p->idx)[3] = NULL;
    p->ensure = NULL;
    p->ctx = NULL;

    done;
}

// Number of pages in buffer
fun u64 PAGEnpages(pagecp p) {
    return (p->buf[3] - p->buf[0]) / PAGESIZE;
}

b8 PAGEPresent(pagecp p, u64 pos, size_t len) {
    if (p == NULL || len == 0) return YES;
    if (!PAGEIsPaged(p)) return YES;  // streaming, no tracking

    u64 first = pos / PAGESIZE;
    u64 last = (pos + len - 1) / PAGESIZE;
    u64 npages_idx = p->idx[3] - p->idx[0];

    if (last >= npages_idx) return NO;

    for (u64 i = first; i <= last; i++) {
        if (PAGEIdxRead(p, i) != PAGE_LOADED) return NO;
    }

    return YES;
}

ok64 PAGEEnsure(pagep p, u64 pos, size_t len) {
    sane(p != NULL);

    if (len == 0) done;
    if (PAGEPresent(p, pos, len)) done;

    test(p->ensure != NULL, PAGEnodata);

    // Align to page boundaries
    u64 start = (pos / PAGESIZE) * PAGESIZE;
    u64 end = ((pos + len - 1) / PAGESIZE + 1) * PAGESIZE;

    call(p->ensure, p, NO, start, end - start);

    test(PAGEPresent(p, pos, len), PAGEfail);

    done;
}

ok64 PAGEDirty(pagep p, u64 pos, size_t len) {
    sane(p != NULL);

    if (len == 0) done;
    if (!PAGEIsPaged(p)) done;  // streaming, no tracking

    u64 first = pos / PAGESIZE;
    u64 last = (pos + len - 1) / PAGESIZE;
    u64 npages_idx = p->idx[3] - p->idx[0];

    test(last < npages_idx, PAGEbadarg);

    for (u64 i = first; i <= last; i++) {
        PAGEIdxSetWrite(p, i, PAGE_DIRTY);
    }

    done;
}

ok64 PAGEMarkLoaded(pagep p, u64 pos, size_t len) {
    sane(p != NULL);

    if (len == 0) done;
    if (!PAGEIsPaged(p)) done;  // streaming, no tracking

    u64 first = pos / PAGESIZE;
    u64 last = (pos + len - 1) / PAGESIZE;
    u64 npages_idx = p->idx[3] - p->idx[0];

    test(last < npages_idx, PAGEbadarg);

    for (u64 i = first; i <= last; i++) {
        PAGEIdxSetRead(p, i, PAGE_LOADED);
        PAGEIdxSetWrite(p, i, PAGE_CLEAN);
    }

    done;
}

ok64 PAGEFlush(pagep p) {
    sane(p != NULL);

    if (p->ensure == NULL) done;
    if (!PAGEIsPaged(p)) done;  // streaming handles flush externally

    u64 npages_idx = p->idx[3] - p->idx[0];

    // Scan for dirty ranges and coalesce
    u64 range_start = 0;
    b8 in_range = NO;

    for (u64 i = 0; i < npages_idx; i++) {
        u8 write = PAGEIdxWrite(p, i);

        if (write == PAGE_DIRTY) {
            if (!in_range) {
                range_start = i;
                in_range = YES;
            }
        } else {
            if (in_range) {
                // Flush range [range_start, i)
                u64 pos = range_start * PAGESIZE;
                size_t len = (i - range_start) * PAGESIZE;
                call(p->ensure, p, YES, pos, len);

                // Mark range as clean
                for (u64 j = range_start; j < i; j++) {
                    PAGEIdxSetWrite(p, j, PAGE_CLEAN);
                }
                in_range = NO;
            }
        }
    }

    // Flush final range if any
    if (in_range) {
        u64 pos = range_start * PAGESIZE;
        size_t len = (npages_idx - range_start) * PAGESIZE;
        call(p->ensure, p, YES, pos, len);

        for (u64 j = range_start; j < npages_idx; j++) {
            PAGEIdxSetWrite(p, j, PAGE_CLEAN);
        }
    }

    done;
}

ok64 PAGEStreamFd(pagep p, b8 rw, u64 pos, size_t need) {
    sane(p);
    int fd = (int)(intptr_t)p->ctx;
    if (rw) {
        // Write: flush data to fd until we have enough idle space
        while (u8bIdleLen(p->buf) < need) {
            u8csp data = u8bDataC(p->buf);
            if (u8csEmpty(data)) {
                // No data to write - shift to reclaim PAST
                u8bShift(p->buf, 0);
                if (u8bIdleLen(p->buf) < need) fail(PAGEnoroom);
                done;
            }
            ssize_t n = write(fd, *data, u8csLen(data));
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) done;
                fail(FILEerrno(errno));
            }
            if (n == 0) fail(PAGEfail);
            u8bUsed(p->buf, n);  // consume written data from start
        }
    } else {
        // Read: fill buffer until we have enough data
        while (u8bDataLen(p->buf) < need) {
            u8sp idle = u8bIdle(p->buf);
            if (u8sEmpty(idle)) {
                // No idle space - shift data to reclaim PAST
                u8bShift(p->buf, 0);
                idle = u8bIdle(p->buf);
                if (u8sEmpty(idle)) fail(PAGEnoroom);
            }
            ssize_t n = read(fd, *idle, u8sLen(idle));
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) done;
                fail(FILEerrno(errno));
            }
            if (n == 0) return END;
            *idle += n;
        }
    }
    done;
}
