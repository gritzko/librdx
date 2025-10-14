#include "MMAP.h"

#include "PRO.h"

ok64 MMAPopen(voidb buf, size_t size) {
    sane(!(buf == nil || *buf != nil || size == 0));
    uint8_t *p = (uint8_t *)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_ANON, -1, 0);
    testc(p != MAP_FAILED, Bmapfail);
    uint8_t **b = (uint8_t **)buf;
    b[0] = b[1] = b[2] = b[3] = p;
    b[3] += size;
    done;
}

ok64 MMAPresize(voidb buf, size_t new_size) {
    sane(!Bnil(buf) && new_size > 0);
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
