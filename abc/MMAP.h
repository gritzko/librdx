#ifndef ABC_MMAP_H
#define ABC_MMAP_H
#include "01.h"
#include "B.h"

con ok64 MMAPbadarg = 0x5962999a5a25dab;
con ok64 MMAPfail = 0x596299aa5b70;

ok64 MMAPopen(voidb buf, size_t size);

fun ok64 MMAPclose(voidb buf) {
    if (buf == nil || *buf == nil) return MMAPbadarg;
    munmap(buf[0], Bsize(buf));
    memset((void **)buf, 0, sizeof(voidb));
    return OK;
}

ok64 MMAPresize(voidb buf, size_t new_size);

fun ok64 MMAPresize2(voidb buf) { return MMAPresize(buf, Bsize(buf) * 2); }

fun ok64 MMAPresize4(voidb buf) {
    size_t new_size = Bsize(buf);
    new_size += new_size >> 2;
    new_size = round_power_of_2(new_size);
    return MMAPresize(buf, new_size);
}

fun ok64 MMAPmayresize(voidb buf, size_t idle_size) {
    size_t has_idle = $size(Bidle(buf));
    if (has_idle >= idle_size) return OK;
    size_t new_size = round_power_of_2(Bsize(buf) + idle_size - has_idle);
    return MMAPresize(buf, new_size);
}

#define Bmmap(buf, len) MMAPopen((voidbp)buf, len * sizeof(**buf))
#define Bunmap(buf) MMAPclose((voidbp)buf)
#define Bremap(buf, new_len) MMAPresize((voidbp)buf, new_len * sizeof(**buf))
#define Bremap2(buf) MMAPresize2((voidbp)buf)
#define Bremap4(buf) MMAPresize4((voidbp)buf)
#define Bmayremap(buf, idle_len) \
    MMAPmayresize((voidbp)buf, idle_len * sizeof(**buf))

#endif
