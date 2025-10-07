#ifndef LIBRDX_FILE_H
#define LIBRDX_FILE_H

#include "01.h"
#include "BUF.h"
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
con ok64 FILEbadarg = 0x3d254e9a5a25dab;
con ok64 FILEfail = 0x3d254eaa5b70;
con ok64 FILEnosync = 0x3d254ecb3dfdca7;
con ok64 FILEnoopen = 0x3d254ecb3cf4a72;
con ok64 FILEnoclse = 0x3d254ecb39f0de9;
con ok64 FILEnostat = 0x3d254ecb3df8978;
con ok64 FILEwrong = 0xf4953bbdb3cab;
con ok64 FILEnoresz = 0x3d254ecb3da9dfe;
con ok64 FILEend = 0xf4953a9ca8;
con ok64 FILEnone = 0x3d254ecb3ca9;
con ok64 FILEaccess = 0x3d254e9679e9df7;
con ok64 FILEloop = 0x3d254ec33cf4;
con ok64 FILEname = 0x3d254eca5c69;
con ok64 FILEbad = 0xf4953a6968;

#define FILEok(fd) (fd >= 0)

/*
typedef int *FILE;

#define aFILE(name)            \
    int _##name = FILE_CLOSED; \
    FILE name = &_##name;
*/

#ifdef IOV_MAX
#define FILEmaxiov IOV_MAX
#elif defined __IOV_MAX
#define FILEmaxiov __IOV_MAX
#else
#define FILEmaxiov 1024
#endif

#define FILE_CLOSED -1

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

ok64 FILEcreate(int *fd, const path name);

ok64 FILEopen(int *fd, const path name, int flags);
ok64 FILEopenat(int *fd, int const *dirfd, const path name, int flags);

ok64 FILEsync(int const *fd);

ok64 FILEclose(int *fd);

ok64 FILEstat(struct stat *ret, const path name);

ok64 FILEsize(size_t *size, int const *fd);

ok64 FILEisdir(const path name);

ok64 FILEresize(int const *fd, size_t new_size);

ok64 FILErename(const path oldname, const path newname);

// Drains the data to the file; if the slice is non empty on return, see errno!
fun ok64 FILEfeed(int fd, u8 const **data) {
    ssize_t re = write(fd, *data, $size(data));
    if (re <= 0) return FILEfail;
    *data += re;
    return OK;
}

fun int FILE2iovec(struct iovec *io, u8css datav) {
    int l = 0;
    while (l < $len(datav) && l < FILEmaxiov) {
        u8c$ data = $at(datav, l);
        io[l].iov_base = (void *)data[0];
        io[l].iov_len = $len(data);
        ++l;
    }
    return l;
}

fun void u8cssdrained(u8css datav, size_t re) {
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

fun ok64 FILEfeedv(int fd, u8css datav) {
    struct iovec io[FILEmaxiov];
    int l = FILE2iovec(io, datav);
    ssize_t re = writev(fd, io, l);
    if (re <= 0) return FILEfail;
    u8cssdrained(datav, re);
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
    int l = FILE2iovec(io, (u8cssp)datav);
    ssize_t re = readv(fd, io, l);
    if (re <= 0) return FILEfail;
    u8cssdrained((u8cssp)datav, re);
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

fun ok64 FILEmakedir(path const name) {
    sane($ok(name));
    aFILEpath(p, name);
    int rc = mkdir(p, S_IRWXU);
    testc(rc == 0, FILEfail);
    done;
}

ok64 FILErmrf(path const name);

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

ok64 FILEmap(Bu8 buf, int const *fd, int mode);

// Memory-map a file for reading.
fun ok64 FILEmapro2(Bu8 buf, int *fd) { return FILEmap(buf, fd, PROT_READ); }

fun ok64 FILEmapro(Bu8 buf, u8csc path) {
    if (buf == nil || Bok(buf) || !$ok(path)) return FILEbadarg;
    int fd = FILE_CLOSED;
    ok64 o = FILEopen(&fd, path, O_RDONLY);
    if (o == OK) {
        o = FILEmap(buf, &fd, PROT_READ);
        FILEclose(&fd);
    }
    return o;
}

// Memory-map a file for reading and writing.
fun ok64 FILEmaprw(Bu8 buf, int *fd, $cu8c path) {
    if (buf == nil || Bok(buf) || !$ok(path) || fd == nil) return FILEbadarg;
    ok64 o = FILEopen(fd, path, O_RDWR);
    if (o == OK) o = FILEmap(buf, fd, PROT_READ | PROT_WRITE);
    if (o != OK) FILEclose(fd);
    return o;
}

// Memory-map a file for reading and writing.
fun ok64 FILEmapnew(Bu8 buf, int *fd, $cu8c path, size_t size) {
    if (buf == nil || Bok(buf) || !$ok(path) || fd == nil) return FILEbadarg;
    ok64 o = FILEcreate(fd, path);
    if (o == OK) o = FILEresize(fd, size);
    if (o == OK) o = FILEmap(buf, fd, PROT_READ | PROT_WRITE);
    if (o != OK) FILEclose(fd);
    return o;
}

// Unmaps the buffer.
ok64 FILEunmap(Bu8 buf);

// Resize the file and update the mapping.
fun ok64 FILEremap(Bu8 buf, int const *fd, size_t new_size) {
    if (!Bok(buf) || fd == nil) return FILEbadarg;
    ok64 o = FILEunmap(buf);
    if (o == OK) o = FILEresize(fd, new_size);
    if (o == OK) o = FILEmap(buf, fd, PROT_WRITE | PROT_READ);
    // TODO mremap()
    return o;
}

// Extend the mapped file by 1/4
fun ok64 FILEremap125(Bu8 buf, int const *fd) {
    size_t new_size = roundup(Bsize(buf) * 5 / 4, PAGESIZE);
    return FILEremap(buf, fd, new_size);
}

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
