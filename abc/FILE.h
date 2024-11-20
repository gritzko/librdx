#ifndef LIBRDX_FILE_H
#define LIBRDX_FILE_H

#include "01.h"
#include "B.h"
#include "OK.h"
#include "PRO.h"

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#define __USE_XOPEN_EXTENDED 1

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>

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
con ok64 FILEnone = 0xa72cf239548f;
con ok64 FILEaccess = 0xdf7a679e539548f;
con ok64 FILEloop = 0xd33cf039548f;
con ok64 FILEname = 0xa7197239548f;
con ok64 FILEbad = 0x2896639548f;

#define FILEbad(fd) (fd < 0)
#define FILEok(fd) (fd >= 0)

#ifdef IOV_MAX
#define FILEmaxiov IOV_MAX
#elif defined __IOV_MAX
#define FILEmaxiov __IOV_MAX
#else
#define FILEmaxiov 1024
#endif

#ifndef MAP_FILE
#define MAP_FILE 0
#endif

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
    sane(fd != nil && $ok(name));
    aFILEpath(p, name);
    *fd = open(p, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    testc(*fd >= 0, FILEnoopen);
    done;
}

fun pro(FILEopen, int *fd, const path name, int flags) {
    sane(fd != nil && $ok(name));
    aFILEpath(p, name);
    *fd = open(p, flags);
    testc(*fd >= 0, FILEnoopen);
    done;
}

fun pro(FILEsync, int fd) {
    sane(FILEok(fd));
    testc(fsync(fd) == 0, FILEnosync);
    done;
}

fun pro(FILEclose, int fd) {
    sane(FILEok(fd));
    testc(0 == close(fd), FILEnoclse);
    done;
}

fun pro(FILEstat, struct stat *ret, const path name) {
    sane(ret != nil && $ok(name));
    aFILEpath(p, name);
    int rc = stat(p, ret);
    if (rc == 0) skip;
    switch (errno) {
        case ENOENT:
            fail(FILEnone);
        case EACCES:
            fail(FILEaccess);
        case ENOTDIR:
            fail(FILEbad);
        case ELOOP:
            fail(FILEloop);
        case ENAMETOOLONG:
            fail(FILEname);
        default:
            fail(FILEfail);
    }
    testc(rc == 0, FILEnostat);
    done;
}

fun pro(FILEsize, size_t *size, int fd) {
    sane(size != nil && FILEok(fd));
    struct stat sb = {};
    testc(0 == fstat(fd, &sb), FILEnostat);
    *size = sb.st_size;
    done;
}

fun pro(FILEisdir, const path name) {
    sane($ok(name));
    struct stat sb = {};
    call(FILEstat, &sb, name);
    test(sb.st_mode == S_IFDIR, FILEwrong);
    done;
}

fun pro(FILEresize, int fd, size_t new_size) {
    sane(FILEok(fd));
    testc(0 == ftruncate(fd, new_size), FILEnoresz);
    // FIXME sync the dir data (another msync?)
    done;
}

fun ok64 FILEmaptrim(Bvoid buf, int fd) {
    return FILEresize(fd, Busysize(buf));
}

// Drains the data to the file; if the slice is non empty on return, see errno!
fun ok64 FILEfeed(int fd, u8 const **data) {
    ssize_t re = write(fd, *data, $size(data));
    if (re <= 0) return FILEfail;
    *data += re;
    return OK;
}

#include "INT.h"

fun int FILE2iovec(struct iovec *io, $$u8c datav) {
    int l = 0;
    while (l < $len(datav) && l < FILEmaxiov) {
        u8c$ data = $at(datav, l);
        io[l].iov_base = (void *)data[0];
        io[l].iov_len = $len(data);
        ++l;
    }
    return l;
}

fun void $$u8cdrained($$u8c datav, size_t re) {
    while (re > 0 && !$empty(datav)) {
        u8c$ data = $at(datav, 0);
        if (re < $len(data)) {
            data[0] += re;
            re = 0;
        } else {
            re -= $len(data);
            ++datav[0];
        }
    }
}

fun ok64 FILEfeedv(int fd, $$u8c datav) {
    struct iovec io[FILEmaxiov];
    int l = FILE2iovec(io, datav);
    ssize_t re = writev(fd, io, l);
    if (re <= 0) return FILEfail;
    $$u8cdrained(datav, re);
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

fun ok64 FILEdrainv($$u8 datav, int fd) {
    struct iovec io[FILEmaxiov];
    int l = FILE2iovec(io, ($u8c$)datav);
    ssize_t re = readv(fd, io, l);
    if (re <= 0) return FILEfail;
    $$u8cdrained(($u8c$)datav, re);
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
    sane($ok(name));
    aFILEpath(p, name);
    int rc = nftw(p, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
    testc(rc == 0, FILEfail);
    done;
}

fun pro(FILEunlink, path const name) {
    sane($ok(name));
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

fun pro(FILEmap, Bvoid buf, int fd, int mode, size_t size) {
    sane(buf != nil && *buf == nil && FILEok(fd));
    if (size == 0) {
        call(FILEsize, &size, fd);
    }
    u8 *map = (u8 *)mmap(NULL, size, mode, MAP_FILE | MAP_SHARED, fd, 0);
    testc(map != MAP_FAILED, FILEfail);
    uint8_t **b = (uint8_t **)buf;
    b[0] = b[1] = b[2] = b[3] = map;
    b[3] += size;
    if (mode & PROT_READ) b[2] += size;
    done;
}

#define FILE_CLOSED -1
#define FILE_SIZE_125 -125

// Memory-map a file for reading.
fun pro(FILEmapro, voidB buf, $cu8c path) {
    sane(buf != nil && *buf == nil && $ok(path));
    int fd = FILE_CLOSED;
    size_t size = 0;
    call(FILEopen, &fd, path, O_RDONLY);
    ok64 o = FILEsize(&size, fd);
    if (o != OK) {
        FILEclose(fd);
        fail(o);
    }
    u8 *new_mem =
        (u8 *)mmap(NULL, size, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
    if (new_mem == MAP_FAILED) {
        FILEclose(fd);
        fail(FILEfail);
    }
    void **b = (void **)buf;
    b[0] = new_mem;
    b[3] = b[0] + size;
    b[1] = b[0];
    b[2] = b[3];
    return OK;
}

// Mamory-map a file for writing; use the provided size.
// If the buffer is non-nil, unmaps that first.
// See special values of new_size, e.g. FILE_SIZE_125
fun pro(FILEmapre, voidB buf, $cu8c path, size_t new_size) {
    sane(buf != nil && *buf == nil && $ok(path));
    int fd = FILE_CLOSED;
    size_t file_size = 0;
    size_t size = 0;
    struct stat st = {};
    ok64 o = FILEstat(&st, path);
    if (o == OK) {
        call(FILEopen, &fd, path, O_RDWR);
        file_size = st.st_size;
    } else if (o == FILEnone) {
        call(FILEcreate, &fd, path);
    } else {
        fail(o);
    }
    if (new_size == 0) {
        size = file_size;
    } else if (new_size > 0) {
        size = new_size;
    } else if (new_size == FILE_SIZE_125) {
        size = roundup(file_size * 5 / 4, PAGESIZE);
    }
    if (file_size != size) {
        callsafe(FILEresize(fd, size), FILEclose(fd));
    }
    u8 *new_mem = (u8 *)mmap(NULL, size, PROT_READ | PROT_WRITE,
                             MAP_FILE | MAP_SHARED, fd, 0);
    testc(new_mem != MAP_FAILED, FILEfail);
    void **b = (void **)buf;
    b[0] = new_mem;
    b[3] = b[0] + size;
    b[1] = b[0];
    b[2] = b[3];
    FILEclose(fd);
    done;
}

// Mamory-map a file for in-place writing.
fun pro(FILEmaprw, voidB buf, $u8c path) {
    sane(buf != nil && *buf == nil && $ok(path));
    call(FILEmapre, buf, path, 0);
    done;
}
// Unmaps the buffer.
// TODO Trims the idle part
fun pro(FILEunmap, voidB buf) {
    sane(Bok(buf));
    testc(-1 != munmap(buf[0], Blen(buf)), FILEfail);
    void **b = (void **)buf;
    b[0] = b[1] = b[2] = b[3] = nil;
    done;
}

// new_size==0 to use de-facto file data
fun pro(FILEremap, voidB buf, int fd, size_t new_size) {
    sane(buf != nil && *buf != nil);
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

static u8 _NL[2] = {'\n', 0};
con u8 *const NL[2] = {_NL, _NL + 1};

// todo buffered print
#define $print FILEout
#define $println(s) FILEout(s), FILEout(NL)

#define FILEfeedf(fd, fmt, ...)                  \
    {                                            \
        aBpad(u8, _pad, PAGESIZE);               \
        $feedf(Bu8idle(_pad), fmt, __VA_ARGS__); \
        FILEfeed(fd, Bu8cdata(_pad));            \
    }

#endif  // ABC_F_H
