#ifndef ABC_PAGE_H
#define ABC_PAGE_H

#include "BUF.h"

// u64 buffer type (like u8b but for u64)
typedef u64 *const u64b[4];
typedef u64 *const *u64bp;

// Error codes
con ok64 PAGEnoroom = 0x1a9acfb3db3cf1;
con ok64 PAGEnodata = 0x1a9acf3834a74a;
con ok64 PAGEfail = 0x1a9acfaa5b70;
con ok64 PAGEbadarg = 0x1a9acf2ca34a6d0;

// Page progress values (0-255 range for read/write)
#define PAGE_ABSENT 0    // read=0: not loaded
#define PAGE_LOADED 255  // read=255: fully loaded
#define PAGE_CLEAN 0     // write=0: not dirty
#define PAGE_DIRTY 255   // write=255: fully dirty

// Default page size
#define PAGE_SIZE_DEFAULT 4096

typedef struct page page;
typedef page *pagep;
typedef page const *pagecp;

// Callback: ensure [pos, len) is available/flushed
// For reads (rw=NO): load pages into buf
// For writes (rw=YES, flush): write dirty pages, clear dirty
// ctx is typically fd cast to (void*)(intptr_t)
typedef ok64 (*pagef)(pagep p, b8 rw, u64 pos, size_t len);

// Paged buffer for streamed/positioned reads/writes.
// Stream readers/writers may invoke the callback to avoid NODATA/NOROOM
// Positioned readers/writers may check page status, then invoke if needed
typedef struct page {
    u8b buf;       // mapped data region
    u64b idx;      // page index: [48:55] read progress, [56:63] write progress
    pagef ensure;  // callback
    void *ctx;     // callback context
} page;

// Global registry
extern pagep PAGE_REGISTRY;
extern u32 PAGE_REGISTRY_LEN;

// Initialize registry (call once at startup)
ok64 PAGEInit(u32 maxpages);

// Create paged buffer (allocates and maps)
ok64 PAGECreate(pagep *newpage, u64 maxlen, pagef ensure, void *ctx);

// Close and unmap
ok64 PAGEClose(pagep p);

fun b8 PAGETracked(u8bp buf) {
    return ((pagep)buf >= PAGE_REGISTRY) &&
           ((pagep)buf < (PAGE_REGISTRY + PAGE_REGISTRY_LEN));
}

// Find page for buffer (returns pagep or NULL)
fun pagep PAGEFind(u8bp buf) { return PAGETracked(buf) ? (pagep)buf : NULL; }

// Check if buffer is paged (has idx tracking)
fun b8 PAGEIsPaged(pagecp p) { return p != NULL && p->idx[0] != NULL; }

// Get read progress byte (0=absent, 255=loaded)
fun u8 PAGEIdxRead(pagecp p, u64 pg) { return (p->idx[0][pg] >> 48) & 0xFF; }

// Set read progress
fun void PAGEIdxSetRead(pagep p, u64 pg, u8 val) {
    u64 *e = &p->idx[0][pg];
    *e = (*e & ~(0xFFULL << 48)) | ((u64)val << 48);
}

// Get write progress byte (0=clean, 255=dirty)
fun u8 PAGEIdxWrite(pagecp p, u64 pg) { return (p->idx[0][pg] >> 56) & 0xFF; }

// Set write progress
fun void PAGEIdxSetWrite(pagep p, u64 pg, u8 val) {
    u64 *e = &p->idx[0][pg];
    *e = (*e & ~(0xFFULL << 56)) | ((u64)val << 56);
}

// Check if range is present
b8 PAGEPresent(pagecp p, u64 pos, size_t len);

// Ensure range is available for read
ok64 PAGEEnsure(pagep p, u64 pos, size_t len);

// Ensure slice is available (slice must be within buf)
fun ok64 PAGEEnsureSlice(pagep p, u8csc slice) {
    if (!Bwithin(p->buf, slice)) return PAGEbadarg;
    u64 pos = slice[0] - p->buf[0];
    size_t len = slice[1] - slice[0];
    return PAGEEnsure(p, pos, len);
}

// Mark range as dirty
ok64 PAGEDirty(pagep p, u64 pos, size_t len);

// Mark range as loaded
ok64 PAGEMarkLoaded(pagep p, u64 pos, size_t len);

// Invokes callback on all dirty *ranges*
ok64 PAGEFlush(pagep p);

// Standard fd-based callback for streaming I/O
// p->ctx = (void*)(intptr_t)fd
// rw=NO: read from fd into buf idle region
// rw=YES: write buf data region to fd, consume written bytes
ok64 PAGEStreamFd(pagep p, b8 rw, u64 pos, size_t need);

// Ensure idle space for writing (invokes callback if needed)
fun ok64 PAGEEnsureIdle(pagep p, size_t need) {
    if (u8bIdleLen(p->buf) >= need) return OK;
    if (p->ensure == NULL) return PAGEnoroom;
    return p->ensure(p, YES, 0, need);
}

// Ensure data available for reading (invokes callback if needed)
fun ok64 PAGEEnsureData(pagep p, size_t need) {
    if (u8bDataLen(p->buf) >= need) return OK;
    if (p->ensure == NULL) return PAGEnodata;
    return p->ensure(p, NO, 0, need);
}

// Single-page ensure - hot path fully inlined
fun ok64 u8bEnsure1(u8bp buf, u64 pos) {
    if (!PAGETracked(buf)) return OK;
    pagep p = (pagep)buf;
    if (!p->idx[0]) return OK;
    u64 pg = pos >> 12;
    if ((p->idx[0][pg] >> 48) & 0xFF) return OK;  // loaded
    return PAGEEnsure(p, pos, 256);
}

// Two-page ensure - for reads that may cross page boundary
fun ok64 u8bEnsure2(u8bp buf, u64 pos) {
    if (!PAGETracked(buf)) return OK;
    pagep p = (pagep)buf;
    if (!p->idx[0]) return OK;
    u64 pg = pos >> 12;
    if (((p->idx[0][pg] & p->idx[0][pg + 1]) >> 48) & 0xFF) return OK;
    return PAGEEnsure(p, pos, 256);
}

// Multi-page ensure for ranges that may span pages
fun ok64 u8bEnsure(u8bp buf, u64 pos, size_t len) {
    if (!PAGETracked(buf)) return OK;
    pagep p = (pagep)buf;
    if (p->idx[0]) return PAGEEnsure(p, pos, len);
    return OK;
}

#endif
