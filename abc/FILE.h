#ifndef LIBRDX_FILE_H
#define LIBRDX_FILE_H

#include "01.h"
#include "BUF.h"
#include "OK.h"
#include "PATH.h"

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

// Generic FILE error codes
con ok64 FILEERROR = 0xf49538e6db61b;
con ok64 FILEBADARG = 0x3d254e2ca34a6d0;
con ok64 FILEFAIL = 0x3d254e3ca495;
con ok64 FILENOSYNC = 0x3d254e5d87225cc;
con ok64 FILENOOPEN = 0x3d254e5d8619397;
con ok64 FILENOCLSE = 0x3d254e5d831570e;
con ok64 FILENOSTAT = 0x3d254e5d871d29d;
con ok64 FILEWRONG = 0xf4953a06d85d0;
con ok64 FILENORESZ = 0x3d254e5d86ce723;
con ok64 FILEEND = 0xf49538e5cd;
con ok64 FILENONE = 0x3d254e5d85ce;
con ok64 FILEACCESS = 0x3d254e28c30e71c;
con ok64 FILENAME = 0x3d254e5ca58e;
con ok64 FILEBAD = 0xf49538b28d;
con ok64 FILESKIP = 0x3d254e714499;  // Skip this directory (don't recurse)
con ok64 FILENOBOOK = 0x3d254e5d82d8614;  // Not a booked mapping

// errno ok64 codes (FILE + E* from errno.h, E doubled to single)
con ok64 FILEACCES = 0xf49538a30c39c;  // EACCES: permission denied
con ok64 FILEAGAIN =
    0xf49538a40a497;                 // EAGAIN: resource temporarily unavailable
con ok64 FILEBADF = 0x3d254e2ca34f;  // EBADF: bad file descriptor
con ok64 FILEBUSY = 0x3d254e2de722;  // EBUSY: device or resource busy
con ok64 FILEEXIST = 0xf49538e85271d;     // EEXIST: file exists
con ok64 FILEFAULT = 0xf49538f29e55d;     // EFAULT: bad address
con ok64 FILEFBIG = 0x3d254e3cb490;       // EFBIG: file too large
con ok64 FILEINTR = 0x3d254e49775b;       // EINTR: interrupted system call
con ok64 FILEINVAL = 0xf4953925df295;     // EINVAL: invalid argument
con ok64 FILEIO = 0x3d254e498;            // EIO: I/O error
con ok64 FILEISDIR = 0xf49539270d49b;     // EISDIR: is a directory
con ok64 FILELOOP = 0x3d254e558619;       // ELOOP: too many symbolic links
con ok64 FILEMFILE = 0xf4953963d254e;     // EMFILE: too many open files
con ok64 FILE2LONG = 0xf4953825585d0;     // ENAMETOOLONG: filename too long
con ok64 FILENFILE = 0xf4953973d254e;     // ENFILE: file table overflow
con ok64 FILENODEV = 0xf49539760d39f;     // ENODEV: no such device
con ok64 FILENOENT = 0xf49539760e5dd;     // ENOENT: no such file or directory
con ok64 FILENOMEM = 0xf495397616396;     // ENOMEM: out of memory
con ok64 FILENOSPC = 0xf49539761c64c;     // ENOSPC: no space left on device
con ok64 FILENOTDIR = 0x3d254e5d874d49b;  // ENOTDIR: not a directory
con ok64 FILENOTEMP = 0x3d254e5d874e599;  // ENOTEMPTY: directory not empty
con ok64 FILEPERM = 0xf49538e64e6d6;      // EPERM: operation not permitted
con ok64 FILEROFS = 0x3d254e6d83dc;       // EROFS: read-only file system
con ok64 FILEXDEV = 0x3d254e84d39f;       // EXDEV: cross-device link
con ok64 FILENAMEBAD = 0xf49539729638b28d;

// Translate errno to ok64
fun ok64 FILEerrno(int e) {
    switch (e) {
        case 0:
            return OK;
        case EACCES:
            return FILEACCES;
        case EAGAIN:
            return FILEAGAIN;
        case EBADF:
            return FILEBADF;
        case EBUSY:
            return FILEBUSY;
        case EEXIST:
            return FILEEXIST;
        case EFAULT:
            return FILEFAULT;
        case EFBIG:
            return FILEFBIG;
        case EINTR:
            return FILEINTR;
        case EINVAL:
            return FILEINVAL;
        case EIO:
            return FILEIO;
        case EISDIR:
            return FILEISDIR;
        case ELOOP:
            return FILELOOP;
        case EMFILE:
            return FILEMFILE;
        case ENAMETOOLONG:
            return FILE2LONG;
        case ENFILE:
            return FILENFILE;
        case ENODEV:
            return FILENODEV;
        case ENOENT:
            return FILENOENT;
        case ENOMEM:
            return FILENOMEM;
        case ENOSPC:
            return FILENOSPC;
        case ENOTDIR:
            return FILENOTDIR;
        case ENOTEMPTY:
            return FILENOTEMP;
        case EPERM:
            return FILEPERM;
        case EROFS:
            return FILEROFS;
        case EXDEV:
            return FILEXDEV;
        default:
            return FILEFAIL;
    }
}

#define FILEok(fd) (fd >= 0)

// Test syscall return; on failure return errno as ok64
#define FILETestC(cond)                       \
    do {                                      \
        if (!(cond)) return FILEerrno(errno); \
    } while (0)

extern u8 *FILE_RW[4];
extern u8p *FILE_BOOK[4];       // Booked VA range ends
extern Bu8 *FILE_WANT_BUFS;   // Per-fd booked buffer slots [FILE_MAX_OPEN]

// Stream-mode callback: rw=YES → ensure idle (flush/grow), rw=NO → ensure data (read)
typedef ok64 (*u8bwantf)(u8bp buf, b8 rw, size_t need);
extern u8bwantf FILE_WANTS[];   // [FILE_MAX_OPEN], parallel to FILE_WANT_BUFS

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
#define FILE_NAME_MAX_LEN 255
#define FILE_PATH_MAX_LEN 1024

#ifndef FILE_MAX_OPEN
#define FILE_MAX_OPEN 1024
#endif

// Legacy path8 buffer type (for code that needs to own/modify paths)
typedef u8b path8;
typedef u8bp path8p;
typedef ok64 (*path8f)(voidp arg, path8p path);

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

fun ok64 path8bAlloc(path8p path) {
    return u8bAllocate(path, FILE_PATH_MAX_LEN);
}

fun ok64 path8bFree(path8p path) { return u8bFree(path); }

ok64 FILECreate(int *fd, path8cg path);

ok64 FILEOpen(int *fd, path8cg path, int flags);
ok64 FILEOpenAt(int *fd, int const dirfd, path8cg path, int flags);
fun ok64 FILEOpenDir(int *fd, path8cg path) {
    return FILEOpen(fd, path, O_RDONLY | O_DIRECTORY);
}
// dir entries have / appended

// scan modes
typedef enum {
    FILE_SCAN_DEEP = 1,
    FILE_SCAN_FILES = 2,
    FILE_SCAN_DIRS = 4,
    FILE_SCAN_LINKS = 8,
    FILE_SCAN_ALL = 14,
} FILE_SCAN;
ok64 FILEScan(path8p path, FILE_SCAN mode, path8f f, voidp arg);

// File tree iterator (into/next/outo pattern)
// Usage:
//   fileit it = {};
//   call(FILEIterOpen, &it, path);
//   scan(FILENext, &it) {
//       if (it.type == DT_DIR) {
//           fileit child = {};
//           call(FILEInto, &child, &it);
//           // ... recurse ...
//           call(FILEOuto, &child, &it);
//       }
//   }
//   call(FILEIterClose, &it);
//
// For sorted traversal, provide buffer and comparator (inherited by children).
// Buffer is used as a stack: each level uses space after parent's sorted
// entries. Uses LSMSort for sorting, stream-based iteration (no slice array
// overhead).
typedef struct fileit {
    u8 type;       // DT_REG, DT_DIR, DT_LNK, etc (from dirent.h)
    u8 flags;      // reserved
    u8p dirend;    // saved path end (dir portion, before entry name)
    voidp dir;     // DIR* handle (void* to avoid dirent.h in header)
    path8gp path;  // shared path gauge
    // Sorted mode fields (NULL = unsorted):
    u8csz sort;    // comparator (inherited by children)
    u8bp buf;      // shared buffer for entries (inherited)
    u8p buf_mark;  // buffer position when this level started (for stack pop)
    u8cs stream;   // position in sorted entry stream [type][name\0]...
} fileit;
typedef fileit *fileitp;
typedef fileit const *fileitcp;

// Entry comparator for sorted iteration (compares [type][len][name] slices)
// Default: alphabetical by name
fun b8 FILEentryZ(u8cscp a, u8cscp b) {
    // Skip type and len bytes, compare names
    u8cs na = {(*a)[0] + 2, (*a)[1]};
    u8cs nb = {(*b)[0] + 2, (*b)[1]};
    return $cmp(na, nb) < 0;
}

// Entry slicer for LSMSort - extracts [type][len][name] record from stream
// TLV-like format: O(1) slicing, no null scan
fun ok64 FILEentryX(u8csp rec, u8cs stream) {
    if ($len(stream) < 2) return NODATA;
    size_t reclen = 2 + stream[0][1];  // type + len + name
    if ($len(stream) < reclen) return NODATA;
    rec[0] = stream[0];
    rec[1] = stream[0] + reclen;
    stream[0] += reclen;
    return OK;
}

// Entry merger for LSMSort - pass through (no duplicates expected in dirs)
fun ok64 FILEentryY(u8s into, u8css recs) { return u8sFeed(into, **recs); }

// Open iterator on directory path
ok64 FILEIterOpen(fileitp it, path8gp path);
// Open sorted iterator (buf holds entries, z compares them)
ok64 FILEIterOpenSorted(fileitp it, path8gp path, u8bp buf, u8csz z);
// Close iterator
ok64 FILEIterClose(fileitp it);
// Next entry in current directory (returns END when exhausted)
ok64 FILENext(fileitp it);
// Descend into directory (current entry must be DT_DIR)
ok64 FILEInto(fileitp child, fileitp parent);
// Ascend from directory
ok64 FILEOuto(fileitp child, fileitp parent);
fun ok64 FILEScanDir(path8p path, path8f f, voidp arg) {
    return FILEScan(path, FILE_SCAN_ALL, f, arg);
}
fun ok64 FILEScanFiles(path8p path, path8f f, voidp arg) {
    return FILEScan(path, FILE_SCAN_FILES, f, arg);
}
fun ok64 FILEDeepScanFiles(path8p path, path8f f, voidp arg) {
    return FILEScan(path, (FILE_SCAN)(FILE_SCAN_FILES | FILE_SCAN_DEEP), f,
                    arg);
}
fun ok64 FILEDeepScanDir(path8p path, path8f f, voidp arg) {
    return FILEScan(path, (FILE_SCAN)(FILE_SCAN_ALL | FILE_SCAN_DEEP), f, arg);
}

ok64 FILESync(int const *fd);

ok64 FILEFlush(int const *fd);
ok64 FILEFlushAll(int const *fd);

// Streaming I/O primitives (caller-provided buffers)
ok64 FILEFlushThreshold(int fd, u8b buf, size_t threshold);
ok64 FILEEnsureSoft(int fd, u8b buf, size_t needed);
ok64 FILEEnsureHard(int fd, u8b buf, size_t needed);

ok64 FILEClose(int *fd);

// ok64 FILEExists(path8 path);

ok64 FILEStat(struct stat *ret, path8cg path);

ok64 FILESize(size_t *size, int const *fd);

ok64 FILEisdir(path8cg path);

ok64 FILEResize(int const *fd, size_t new_size);

ok64 FILERename(path8cg oldname, path8cg newname);

// Drains the data to the file; if the slice is non empty on return, see errno!
fun ok64 FILEFeed(int fd, u8 const **data) {
    ssize_t re = write(fd, *data, $size(data));
    if (re <= 0) return FILEFAIL;
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
    if (re <= 0) return FILEFAIL;
    u8cssdrained(datav, re);
    return OK;
}

fun ok64 FILEFeedall(int fd, uint8_t const *const *data) {
    if (!FILEok(fd) || !$ok(data)) return FILEBADARG;
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
        if (ret == 0) return FILEEND;
        return FILEFAIL;  // TODO
    }
    *into += ret;
    return OK;
}

// FILEDrain: read from fd into slice (fd-first arg order, matches FILEFeed)
fun ok64 FILEDrain(int fd, $u8 into) {
    ssize_t ret = read(fd, *into, $len(into));
    if (ret <= 0) {
        if (ret == 0) return FILEEND;
        return FILEFAIL;
    }
    *into += ret;
    return OK;
}

fun ok64 FILEdrainv($$u8 datav, int fd) {
    struct iovec io[FILEmaxiov];
    int l = FILE2iovec(io, (u8cssp)datav);
    ssize_t re = readv(fd, io, l);
    if (re <= 0) return FILEFAIL;
    u8cssdrained((u8cssp)datav, re);
    return OK;
}

fun ok64 FILEdrainall(u8 **into, int fd) {
    ok64 o;
    do {
        o = FILEdrain(into, fd);
    } while ($len(into) > 0 && o == OK);
    if (o == FILEEND) o = OK;
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

ok64 FILEMakeDir(path8cg path);

// Create directory and parents recursively (like mkdir -p)
ok64 FILEMakeDirP(path8cg path);

ok64 FILERmDir(path8cg path, bool recursive);

ok64 FILEHardLink(path8cg dst, path8cg src);

ok64 FILErmrf(path8cg name);

ok64 FILEUnLink(path8cg name);

fun int flags2prot(int flags) {
    int prot = PROT_READ;
    if ((flags & O_RDWR) == O_RDWR) prot |= PROT_WRITE;
    return prot;
}

// . . . . . . . . mmapped buffers . . . . . . . .

ok64 FILEMapFD(u8bp *buf, int const *fd, int mode);

ok64 FILEUnMapFD(u8b buf, int const *fd);

ok64 FILEMapRO(u8bp *buf, path8cg path);

ok64 FILEMapROAt(u8bp *buf, int dir, path8cg path);

// Memory-map a file for reading and writing.
ok64 FILEMapRW(u8bp *buf, path8cg path);

// Create and map a file for reading and writing.
ok64 FILEMapCreate(u8bp *buf, path8cg path, size_t size);

ok64 FILEMapCreateAt(u8bp *buf, int dir, path8cg path, size_t size);

// Unmaps the buffer (buf must point into FILE_WANT_BUFS).
ok64 FILEUnMap(u8bp buf);

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

// . . . . . . . . booked mmapped buffers . . . . . . . .
//
// Book reserves a large VA range but only maps the file at the start.
// When more space is needed, extend the file and mapping without changing
// the base address - all pointers remain valid.
//
// Typical usage:
//   FILEBookCreate(buf, path, 1*GB, 4*KB);  // reserve 1GB, start with 4KB
//   ... write data ...
//   if (need more) FILEBookExtend(buf, new_size);
//   ... pointers into buf still valid ...
//   FILEUnBook(buf);

// Book VA range and map existing file at start
ok64 FILEBook(u8bp *buf, path8cg path, size_t book_size);

// Book VA range and map existing file at dir
ok64 FILEBookAt(u8bp *buf, int dir, path8cg path, size_t book_size);

// Create file, book VA range, map with initial size
ok64 FILEBookCreate(u8bp *buf, path8cg path, size_t book_size, size_t init_size);

// Create file at dir, book VA range, map with initial size
ok64 FILEBookCreateAt(u8bp *buf, int dir, path8cg path, size_t book_size,
                      size_t init_size);

// Extend file and mapping within booked range (base address unchanged)
ok64 FILEBookExtend(u8bp buf, size_t new_size);

// Grow booked buffer to accommodate 'need' idle bytes (slow path)
ok64 FILEBookGrow(u8bp buf, size_t need);

// Ensure at least 'need' idle bytes in booked buffer (inline fast path)
fun ok64 FILEBookEnsure(u8bp buf, size_t need) {
    if (u8bIdleLen(buf) >= need) return OK;
    return FILEBookGrow(buf, need);
}

// Sync mapped region to disk
ok64 FILEMSync(u8bp buf);

// O(1) fd lookup for booked buffers (buf points into FILE_WANT_BUFS)
fun int FILEBookedFD(u8bp buf) {
    if (!FILE_WANT_BUFS || !buf) return FILE_CLOSED;
    size_t off = (u8cp)buf - (u8cp)FILE_WANT_BUFS;
    if (off % sizeof(Bu8) != 0) return FILE_CLOSED;
    size_t idx = off / sizeof(Bu8);
    if (idx >= FILE_MAX_OPEN) return FILE_CLOSED;
    return (int)idx;
}

// Check if buffer is booked
fun b8 FILEIsBooked(u8bp buf) {
    return FILEBookedFD(buf) >= 0;
}

// Unmap booked buffer and close file
ok64 FILEUnBook(u8bp buf);

// Truncate booked file to actual data length
ok64 FILETrimBook(u8bp buf);

// . . . . . . . .

ok64 FILECloseAll();
ok64 FILEBookInit();

fun ok64 FILEInit() {
    if (*FILE_RW != NULL) return OK;
    ok64 o = u8bAllocate(FILE_RW, roundup(FILE_MAX_OPEN >> 3, 64));
    if (o == OK) o = u8pbAllocate(FILE_BOOK, FILE_MAX_OPEN);
    if (o == OK) o = FILEBookInit();
    return o;
}
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
