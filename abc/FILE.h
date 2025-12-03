#ifndef LIBRDX_FILE_H
#define LIBRDX_FILE_H

#include "01.h"
#include "BUF.h"
#include "OK.h"

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

#define FILE_PATH_SEP '/'
#define FILE_PATH_CWD '.'
#define FILE_PATH_MAX_LEN 1024

#ifndef FILE_MAX_OPEN
#define FILE_MAX_OPEN 1024
#endif

typedef u8b path8;

#define a_path(n, p)                 \
    a_pad(u8, n, FILE_PATH_MAX_LEN); \
    u8sFeedCStr(n##_idle, p);

fun ok64 path8Sane(path8 path) {
    if (!u8bOK(path) || u8bDataLen(path) == 0) return NO;
    if (u8bIdleLen(path) == 0) return NO;
    u8c *tp = $term(u8bData(path));
    if (*tp != 0) *(u8 *)tp = 0;  // :/
    return YES;
}

fun const char *path8CStr(path8 path) { return (char *)(path[0]); }

ok64 path8Push(path8 path, u8csc part);

ok64 path8Pop(path8 path);  // todo

fun void path8ResetToCWD(path8 path) {
    Breset(path);
    u8bFeed1(path, FILE_PATH_CWD);
}

fun void path8ResetToRoot(path8 path) { Breset(path); }

fun ok64 FILEerrno() {
    switch (errno) {
        case EAGAIN:
            return FILEagain;
        default:
            return FILEerror;
    }
}

ok64 FILECreate(int *fd, path8 path);

ok64 FILEOpen(int *fd, path8 path, int flags);
ok64 FILEOpenAt(int *fd, int const dirfd, path8 path, int flags);
fun ok64 FILEOpenDir(int *fd, path8 path) {
    return FILEOpen(fd, path, O_DIRECTORY);
}

ok64 FILESync(int const *fd);

ok64 FILEClose(int *fd);

ok64 FILEStat(struct stat *ret, path8 path);

ok64 FILESize(size_t *size, int const *fd);

ok64 FILEisdir(path8 path);

ok64 FILEResize(int const *fd, size_t new_size);

ok64 FILERename(path8 oldname, path8 newname);

// Drains the data to the file; if the slice is non empty on return, see errno!
fun ok64 FILEFeed(int fd, u8 const **data) {
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

fun ok64 FILEFeedv(int fd, u8css datav) {
    struct iovec io[FILEmaxiov];
    int l = FILE2iovec(io, datav);
    ssize_t re = writev(fd, io, l);
    if (re <= 0) return FILEfail;
    u8cssdrained(datav, re);
    return OK;
}

fun ok64 FILEFeedall(int fd, uint8_t const *const *data) {
    if (!FILEok(fd) || !$ok(data)) return FILEbadarg;
    a_dup(u8 const, d, data);
    ok64 ret = OK;
    while (!$empty(d) && OK == ret) {
        ret = FILEFeed(fd, d);
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

ok64 FILEMakeDir(path8 path);

ok64 FILErmrf(path8 name);

ok64 FILEunlink(path8 name);

fun int flags2prot(int flags) {
    int prot = PROT_READ;
    if ((flags & O_RDWR) == O_RDWR) prot |= PROT_WRITE;
    return prot;
}

// . . . . . . . . mmapped buffers . . . . . . . .

ok64 FILEMapFD(u8bp buf, int const *fd, int mode);

ok64 FILEUnMapFD(u8b buf, int const *fd);

ok64 FILEMapRO(u8bp buf, path8 path);

ok64 FILEMapROAt(u8bp buf, int dir, path8 path);

// ? fun ok64 FILEMapTmp(u8bp buf) {}

// Memory-map a file for reading and writing.
ok64 FILEMapRW(u8bp buf, path8 path);

// Create and map a file for reading and writing.
ok64 FILEMapCreate(u8bp buf, path8 path, size_t size);

ok64 FILEMapCreateAt(u8bp buf, int dir, path8 path, size_t size);

// Unmaps the buffer.
ok64 FILEUnMap(u8b buf);

// Resize the file and update the mapping.
ok64 FILEReMap(u8bp buf, size_t new_size);

// Resize the file to th busy part of the buffer (trim the idle part).
ok64 FILETrimMap(u8bp buf);

// Extend the mapped file by 1/2
fun ok64 FILEremap15(Bu8 buf) {
    size_t new_size = roundup(Bsize(buf) * 3 / 2, PAGESIZE);
    return FILEReMap(buf, new_size);
}

// Extend the mapped file by 1/4
fun ok64 FILEremap125(Bu8 buf) {
    size_t new_size = roundup(Bsize(buf) * 5 / 4, PAGESIZE);
    return FILEReMap(buf, new_size);
}

ok64 FILECloseAll();

// . . . . . . . .

fun ok64 FILEout(u8 const *const *txt) {
    a_dup(u8 const, dup, txt);
    return FILEFeedall(STDOUT_FILENO, dup);
}

fun ok64 FILEerr(u8 const *const *txt) {
    a_dup(u8 const, dup, txt);
    return FILEFeedall(STDERR_FILENO, dup);
}

static u8 _NL[2] = {'\n', 0};
con u8 *const NL[2] = {_NL, _NL + 1};

// todo buffered print
#define $print FILEout
#define $println(s) FILEout(s), FILEout(NL)

#define FILEFeedf(fd, fmt, ...)                  \
    {                                            \
        aBpad(u8, _pad, PAGESIZE);               \
        $feedf(u8bIdle(_pad), fmt, __VA_ARGS__); \
        FILEFeed(fd, u8cbData(_pad));            \
    }

#endif  // ABC_F_H
