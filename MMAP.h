#ifndef ABC_MMAP_H
#define ABC_MMAP_H
#include "B.h"
#include "01.h"
#include "PRO.h"

#define _GNU_SOURCE
#include <sys/mman.h>

con ok64 MMAPbadarg = 0xaf696896664a596;
con ok64 MMAPfail = 0xc2d96a64a596;

fun pro(MMAPopen, Bvoid buf, size_t size) {
    if (buf == nil || *buf != nil || size == 0) return MMAPbadarg;
    uint8_t *p = (uint8_t *)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_ANON, -1, 0);
    testc(p != MAP_FAILED, Bmapfail);
    uint8_t **b = (uint8_t **)buf;
    b[0] = b[1] = b[2] = b[3] = p;
    b[3] += size;
    done;
}

fun ok64 MMAPclose(Bvoid buf) {
    if (buf == nil || *buf == nil) return MMAPbadarg;
    munmap(buf[0], Bsize(buf));
    memset((void **)buf, 0, sizeof(Bvoid));
    return OK;
}

fun pro(MMAPresize, Bvoid buf, size_t new_size) {
    test(!Bnil(buf) && new_size > 0, MMAPbadarg);
    size_t old_size = Bsize(buf);
#ifdef MREMAP_MAYMOVE
    u8 *new_mem = (u8 *)mremap(buf[0], Bsize(buf), new_size, MREMAP_MAYMOVE);
    testc(new_mem != MAP_FAILED, MMAPfail);
#else
    u8 *new_mem = (u8 *)mmap(NULL, new_size, PROT_READ | PROT_WRITE,
                             MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    testc(new_mem != MAP_FAILED, MMAPfail);
    memmove(new_mem, buf[0], old_size);  // TODO linux mremap
    int rc = munmap(buf[0], old_size);
    testc(rc == 0, MMAPfail);
#endif
    _Brebase(buf, new_mem, new_size);
    done;
}

fun ok64 MMAPresize2(Bvoid buf) { return MMAPresize(buf, Bsize(buf) * 2); }

fun ok64 MMAPresize4(Bvoid buf) {
    size_t new_size = Bsize(buf);
    new_size += new_size >> 2;
    new_size = round_power_of_2(new_size);
    return MMAPresize(buf, new_size);
}

fun ok64 MMAPmayresize(Bvoid buf, size_t idle_size) {
    size_t has_idle = $size(Bidle(buf));
    if (has_idle >= idle_size) return OK;
    size_t new_size = round_power_of_2(Bsize(buf) + idle_size - has_idle);
    return MMAPresize(buf, new_size);
}

#define Bmmap(buf, len) MMAPopen((void$)buf, len * sizeof(**buf))
#define Bunmap(buf) MMAPclose((void$)buf)
#define Bremap(buf, new_len) MMAPresize((void$)buf, new_len * sizeof(**buf))
#define Bremap2(buf) MMAPresize2((void$)buf)
#define Bremap4(buf) MMAPresize4((void$)buf)
#define Bmayremap(buf, idle_len) \
    MMAPmayresize((void$)buf, idle_len * sizeof(**buf))

#endif
