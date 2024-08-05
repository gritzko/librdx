#ifndef LIBRDX_FILE_H
#define LIBRDX_FILE_H

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "B.h"
#include "OK.h"
#include "PRO.h"

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#define __USE_XOPEN_EXTENDED 1

#include "ftw.h"

con ok64 FILEagain = 0xf4953a5ae5b72;
con ok64 FILEerror = 0xf4953a9db6cf6;
con ok64 FILEbadarg = 0xaf696896639548f;
con ok64 FILEfail = 0xc2d96a39548f;
con ok64 FILEnosync = 0x9f2f77cf239548f;
con ok64 FILEnoopen = 0xca9d33cf239548f;
con ok64 FILEnoclse = 0xa77c27cf239548f;
con ok64 FILEnostat = 0xe25e37cf239548f;
con ok64 FILEwrong = 0x2bcb3dbb39548f;
con ok64 FILEnoresz = 0xfb7a76cf239548f;
con ok64 FILEend = 0x28ca939548f;

#define FILEbad(fd) (fd < 0)
#define FILEok(fd) (fd >= 0)

typedef u8 const *path[2];

fun ok64 FILEerrno() {
    switch (errno) {
        case EAGAIN:
            return FILEagain;
        default:
            return FILEerror;
    }
}

con size_t FILEmaxpathlen = 1024;

#define aFILEpath(n, p)                        \
    char n[1024];                              \
    {                                          \
        size_t sz = $size(p);                  \
        test(sz < FILEmaxpathlen, FILEbadarg); \
        memcpy(n, *p, sz);                     \
        n[sz] = 0;                             \
    }

fun pro(FILEcreate, int *fd, const path name) {
    test(fd != nil && $ok(name), FILEbadarg);
    aFILEpath(p, name);
    *fd = open(p, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    testc(*fd >= 0, FILEnoopen);
    done;
}

fun pro(FILEopen, int *fd, const path name, int flags) {
    test(fd != nil && $ok(name), FILEbadarg);
    aFILEpath(p, name);
    *fd = open(p, flags);
    testc(*fd >= 0, FILEnoopen);
    done;
}

fun pro(FILEsync, int fd) {
    test(FILEok(fd), FILEbadarg);
    testc(fsync(fd) == 0, FILEnosync);
    done;
}

fun pro(FILEclose, int fd) {
    test(FILEok(fd), FILEbadarg);
    testc(0 == close(fd), FILEnoclse);
    done;
}

fun pro(FILEstat, struct stat *ret, const path name) {
    test(ret != nil && $ok(name), FILEbadarg);
    aFILEpath(p, name);
    int rc = stat(p, ret);
    // on(ENOENT) fail(FILEnofind);
    testc(rc == 0, FILEnostat);
    done;
}

fun pro(FILEsize, size_t *size, int fd) {
    test(size != nil && FILEok(fd), FILEbadarg);
    struct stat sb = {};
    testc(0 == fstat(fd, &sb), FILEnostat);
    *size = sb.st_size;
    done;
}

fun pro(FILEisdir, const path name) {
    test($ok(name), FILEbadarg);
    struct stat sb = {};
    call(FILEstat, &sb, name);
    test(sb.st_mode == S_IFDIR, FILEwrong);
    done;
}

fun pro(FILEresize, int fd, size_t new_size) {
    test(FILEok(fd), FILEbadarg);
    testc(0 == ftruncate(fd, new_size), FILEnoresz);
    // FIXME sync the dir data (another msync?)
    done;
}

// Drains the data to the file; if the slice is non empty on return, see errno!
fun ok64 FILEfeed(int fd, u8 const **data) {
    ssize_t re = write(fd, *data, $size(data));
    if (re <= 0) return FILEfail;
    *data += re;
    return OK;
}

fun ok64 FILEfeedall(int fd, uint8_t const *const *data) {
    if (!FILEok(fd) || !$ok(data)) return FILEbadarg;
    a$dup(u8 const, d, data);
    ok64 ret = OK;
    while (!$empty(d) && OK == ret) {
        ret = FILEfeed(fd, d);
    }
    return ret;
}

fun ok64 FILEdrain(u8 **into, int fd) {
    ssize_t ret = read(fd, *into, $size(into));
    if (ret <= 0) {
        if (ret == 0) return FILEend;
        return FILEfail;  // TODO
    }
    *into += ret;
    return OK;
}

fun ok64 FILEdrainall(u8 **into, int fd) {
    ok64 o;
    do {
        o = FILEdrain(into, fd);
    } while ($len(into) > 0 && o == OK);
    if (o == FILEend) o = OK;
    return o;
}

/*
fun proc Fpwrite1(int fd, path data, size_t offset) {
    args(fd >= 0 && offset >= 0, "%i %p %lu", fd, data, offset);
    size_t wn = pwrite(fd, *data, $size(data), offset);
    callcv(wn >= 0, FFAILWRIT0);
    data[$HEAD] += wn;
    done;
}

fun proc Fpwrite(int fd, path data, size_t offset) {
    args(fd >= 0 && offset >= 0, "%i %p %lu", fd, data, offset);
    size_t eoff = offset + $size(data);
    while (!$empty(data)) {
        call(Fpwrite1(fd, data, eoff - $size(data)));
    }
    done;
}

fun proc Fpread1(int fd, path into, size_t offset) {
    args(fd >= 0 && offset >= 0, "%i %p %lu", fd, into, offset);
    size_t wn = pread(fd, *into, $size(into), (off_t)offset);
    callcv(wn >= 0, FFAILREAD0);
    into[$HEAD] += wn;
    done;
}

fun proc Fpread(int fd, path into, size_t offset) {
    args(fd >= 0 && offset >= 0, "%i %p %lu", fd, into, offset);
    size_t eoff = offset + $size(into);
    while (!$empty(into)) {
        call(Fpread1(fd, into, eoff - $size(into)));
    }
    done;
}
*/

fun int unlink_cb(const char *fname, const struct stat *sb, int typeflag,
                  struct FTW *ftwbuf) {
    return remove(fname);
}

fun pro(FILErmrf, path const name) {
    aFILEpath(p, name);
    int rc = nftw(p, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
    testc(rc == 0, FILEfail);
    done;
}

fun pro(FILEunlink, path const name) {
    aFILEpath(p, name);
    int rc = unlink(p);
    testc(rc == 0, FILEfail);
    done;
}

fun int flags2prot(int flags) {
    int prot = PROT_READ;
    if ((flags & O_RDWR) == O_RDWR) prot |= PROT_WRITE;
    return prot;
}

fun pro(FILEmap, void$ buf, int fd, int mode, size_t size) {
    test(buf != nil && *buf == nil && FILEok(fd), FILEbadarg);
    if (size == 0) {
        call(FILEsize, &size, fd);
    }
    u8 *map = (u8 *)mmap(NULL, size, mode, MAP_FILE | MAP_SHARED, fd, 0);
    testc(map != MAP_FAILED, FILEfail);
    uint8_t **b = (uint8_t **)buf;
    b[0] = b[1] = b[2] = b[3] = map;
    b[3] += size;
    done;
}
// new_size==0 to use de-facto file data
fun pro(FILEremap, void$ buf, int fd, size_t new_size) {
    test(buf != nil && *buf != nil, FILEbadarg);
    if (new_size == 0) {
        call(FILEsize, &new_size, fd);
    } else {
        test(new_size > Bsize(buf), OK);  // TODO shrink
        call(FILEresize, fd, new_size);
    }
    int rc = munmap(*buf, Bsize(buf));
    testc(0 == rc, FILEfail);
    u8 *new_mem = (u8 *)mmap(NULL, new_size, PROT_READ | PROT_WRITE,
                             MAP_FILE | MAP_SHARED, fd, 0);
    testc(new_mem != MAP_FAILED, FILEfail);
    _Brebase(buf, new_mem, new_size);
    done;
}

fun pro(FILEunmap, void$ buf) {
    test(buf != nil && *buf != nil, FILEbadarg);
    testc(0 == munmap(*buf, Bsize(buf)), FILEfail);
    _Brebase(buf, nil, 0);
    done;
}

#define Bfmap(buf, fd, prot, len) \
    FILEmap((void$)buf, fd, prot, len * sizeof(**buf))
#define Bunfmap(buf) FILEunmap((void$)buf)

fun ok64 FILEout(u8 const *const *txt) {
    a$dup(u8 const, dup, txt);
    return FILEfeedall(STDOUT_FILENO, dup);
}

fun ok64 FILEerr(u8 const *const *txt) {
    a$dup(u8 const, dup, txt);
    return FILEfeedall(STDERR_FILENO, dup);
}

u8 _NL[2] = {'\n', 0};
con u8 *const NL[2] = {_NL, _NL + 1};

// todo buffered print
#define $print FILEout
#define $println(s) FILEout(s), FILEout(NL)

#endif  // ABC_F_H
