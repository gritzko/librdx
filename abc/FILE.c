#include "FILE.h"

#include <dirent.h>
#include <sys/mman.h>
#include <time.h>

#include "01.h"
#include "BUF.h"
#include "LSM.h"
#include "PATH.h"
#include "PRO.h"
#include "S.h"

u64 FILE_ERR_VOCAB[][2] = {
    {EBADF, 0xe2ca34f},           {EBUSY, 0xe2de722},
    {EDQUOT, 0x38d69e61d},        {EINVAL, 0x3925df295},
    {EISDIR, 0x39270d49b},        {ENAMETOOLONG, 0x729639d6185585d0},
    {ENOENT, 0x39760e5dd},        {ENOMEM, 0x397616396},
    {ENOSPC, 0x39761c64c},        {ENOTDIR, 0xe5d874d49b},
    {EOVERFLOW, 0xe61f39b3d5620}, {EPERM, 0xe64e6d6},
    {ETXTBSY, 0xe76174b722},      {0, 0}};

ok64 FILEErr(ok64 def) {
    int err = errno;
    for (int i = 0; FILE_ERR_VOCAB[i][0] != 0; i++)
        if (FILE_ERR_VOCAB[i][0] == err) return FILE_ERR_VOCAB[i][1];
    return def;
}

ok64 FILEMakeDir(path8cg path) {
    sane(PATHu8cgOK(path));
    int rc = mkdir((char const *)*path, S_IRWXU);
    FILETestC(rc == 0);
    done;
}

ok64 FILEMakeDirP(path8cg path) {
    sane(PATHu8cgOK(path));
    ok64 o = FILEMakeDir(path);
    if (o == OK || o == FILEEXIST) return OK;
    // Parent might not exist, try creating it first
    u8 pbuf[FILE_PATH_MAX_LEN];
    path8 parent = {pbuf, pbuf, pbuf, pbuf + FILE_PATH_MAX_LEN};
    u8cs dir = {};
    PATHu8gDir(dir, path);
    if ($empty(dir) || $len(dir) <= 1) return o;
    call(u8sFeed, u8bIdle(parent), dir);
    call(PATHu8gTerm, PATHu8gIn(parent));
    call(FILEMakeDirP, PATHu8cgIn(parent));
    o = FILEMakeDir(path);
    if (o == FILEEXIST) return OK;
    return o;
}

// Remove directory. If recursive=true, delete contents first (rm -rf style).
ok64 FILERmDir(path8cg path, bool recursive) {
    sane(PATHu8cgOK(path));

    if (recursive) {  // FIXME refac
        // Use internal buffer for recursion
        a_pad(u8, work, FILE_PATH_MAX_LEN);
        u8sFeed(work_idle, path);
        path8gp workp = PATHu8gIn(work);
        *workp[1] = 0;  // null terminate (at data end, not included in data)

        // Open directory and delete contents
        DIR *dir = opendir((char const *)*path);
        if (dir == NULL) {
            testc(errno == ENOENT, FILEFAIL);  // OK if doesn't exist
            done;
        }

        struct dirent *entry = 0;
        ok64 o = OK;
        u8p saved_end = workp[1];  // save data end position

        while (o == OK && (entry = readdir(dir))) {
            // Skip . and ..
            if (entry->d_name[0] == '.') {
                if (entry->d_name[1] == 0) continue;
                if (entry->d_name[1] == '.' && entry->d_name[2] == 0) continue;
            }

            a_cstr(fn, entry->d_name);
            o = PATHu8gPush(workp, fn);
            if (o != OK) break;

            // d_type can be DT_UNKNOWN on some filesystems, so use stat()
            b8 is_dir = NO;
            if (entry->d_type == DT_DIR) {
                is_dir = YES;
            } else if (entry->d_type == DT_UNKNOWN) {
                struct stat sb = {};
                if (stat((char const *)*workp, &sb) == 0) {
                    is_dir = S_ISDIR(sb.st_mode);
                }
            }

            if (is_dir) {
                o = FILERmDir((path8cgp)workp, true);  // recurse
            } else {
                o = FILEUnLink((path8cgp)workp);
            }
            // Restore path to original length
            workp[1] = saved_end;
            *workp[1] = 0;  // null terminate
        }

        closedir(dir);
        if (o != OK) return o;
    }

    int rc = rmdir((char const *)*path);
    FILETestC(rc == 0);
    done;
}

ok64 FILEUnLink(path8cg path) {
    sane(PATHu8cgOK(path));
    int rc = unlink((char const *)*path);
    FILETestC(rc == 0);
    done;
}

ok64 FILEHardLink(path8cg dst, path8cg src) {
    sane($ok(dst) && $ok(src));
    int rc = link((char const *)*src, (char const *)*dst);
    FILETestC(rc == 0);
    done;
}

ok64 FILEisdir(path8cg path) {
    sane(PATHu8cgOK(path));
    struct stat sb = {};
    call(FILEStat, &sb, path);
    test(sb.st_mode & S_IFDIR, FILEWRONG);
    done;
}

ok64 FILESync(int const *fd) {
    sane(FILEok(*fd));
    testc(fsync(*fd) == 0, FILENOSYNC);
    done;
}

ok64 FILEClose(int *fd) {
    sane(FILEok(*fd));
    testc(0 == close(*fd), FILENOCLSE);
    *fd = FILE_CLOSED;
    done;
}

ok64 FILECreate(int *fd, path8cg path) {
    sane(fd != NULL && PATHu8cgOK(path));
    *fd = open((char const *)*path, O_CREAT | O_RDWR | O_TRUNC,
               S_IRUSR | S_IWUSR);
    if (*fd < 0) fail(FILEErr(FILENOOPEN));
    done;
}

ok64 FILECreateAt(int *fd, int dir, path8cg path) {
    sane(fd != NULL && PATHu8cgOK(path));
    *fd = openat(dir, (char const *)*path, O_CREAT | O_RDWR | O_TRUNC,
                 S_IRUSR | S_IWUSR);
    if (*fd < 0) fail(FILEErr(FILENOOPEN));
    done;
}

ok64 FILEOpen(int *fd, path8cg path, int flags) {
    sane(fd != NULL && PATHu8cgOK(path));
    *fd = open((char const *)*path, flags);
    if (*fd < 0) fail(FILEErr(FILENOOPEN));
    done;
}

ok64 FILEOpenAt(int *fd, int const dirfd, path8cg path, int flags) {
    sane(fd != NULL && PATHu8cgOK(path) && FILEok(dirfd));
    *fd = openat(dirfd, (char const *)*path, flags);
    if (*fd < 0) fail(FILEErr(FILENOOPEN));
    done;
}

ok64 FILEStat(struct stat *ret, path8cg path) {
    sane(ret != NULL && PATHu8cgOK(path));
    int rc = stat((char const *)*path, ret);
    FILETestC(rc == 0);
    done;
}

ok64 FILESize(size_t *size, int const *fd) {
    sane(size != NULL && FILEok(*fd));
    struct stat sb = {};
    testc(0 == fstat(*fd, &sb), FILENOSTAT);
    *size = sb.st_size;
    done;
}

ok64 FILEResize(int const *fd, size_t new_size) {
    sane(FILEok(*fd));
    testc(0 == ftruncate(*fd, new_size), FILENORESZ);
    // FIXME sync the dir data (another msync?)
    done;
}

ok64 FILERename(path8cg old, path8cg neu) {
    sane($ok(old) && $ok(neu));
    FILETestC(0 == rename((char const *)*old, (char const *)*neu));
    done;
}

fun int unlink_cb(const char *fname, const struct stat *sb, int typeflag,
                  struct FTW *ftwbuf) {
    return remove(fname);
}

ok64 FILErmrf(path8cg path) {
    sane(PATHu8cgOK(path));
    int rc = nftw((char const *)*path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
    FILETestC(rc == 0);
    done;
}

// System page size for mmap (may differ from PAGESIZE on Apple Silicon)
fun size_t FILESysPage() {
    static size_t p = 0;
    if (!p) p = sysconf(_SC_PAGESIZE);
    return p;
}

u8 *FILE_RW[4] = {};
u8p *FILE_BOOK[4] = {};  // Booked VA range ends
Bu8 *FILE_WANT_BUFS = NULL;
u8bwantf FILE_WANTS[FILE_MAX_OPEN] = {};

// Booked buffer callback: grow mapping on idle-want
fun ok64 FILEBookWant(u8bp buf, b8 rw, size_t need) {
    sane(Bok(buf));
    if (!rw) done;  // booked (mmap'd) data is always available
    return FILEBookGrow(buf, need);
}

// Stream-mode gateway: ensure idle space
ok64 u8bWantIdleLen(u8bp buf, size_t need) {
    sane(Bok(buf));
    if (u8bIdleLen(buf) >= need) done;
    int fd = FILEBookedFD(buf);
    test(fd >= 0 && FILE_WANTS[fd], BNOROOM);
    return FILE_WANTS[fd](buf, YES, need);
}

// Stream-mode gateway: ensure data available
ok64 u8bWantDataLen(u8bp buf, size_t need) {
    sane(Bok(buf));
    if (u8bDataLen(buf) >= need) done;
    int fd = FILEBookedFD(buf);
    test(fd >= 0 && FILE_WANTS[fd], BNODATA);
    return FILE_WANTS[fd](buf, NO, need);
}

ok64 FILEBookInit() {
    sane(1);
    if (FILE_WANT_BUFS) done;
    size_t sz = sizeof(Bu8) * FILE_MAX_OPEN;
    FILE_WANT_BUFS = mmap(NULL, sz, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    test(FILE_WANT_BUFS != MAP_FAILED, MMAPFAIL);
    done;
}

ok64 FILEFlush(int const *fd) {
    sane(fd && *fd >= 0 && *fd < FILE_MAX_OPEN && FILE_WANT_BUFS);
    u8bp buf = FILE_WANT_BUFS[*fd];
    if (u8bDataLen(buf) >= PAGESIZE) {
        int r = write(*fd, *u8bDataC(buf), u8bDataLen(buf));
        if (r < 0) fail(FILEERROR);  // todo
        u8bFed(buf, r);
        if (u8bPastLen(buf) >= u8bDataLen(buf)) u8bShift(buf, 0);
        // todo if grows too fast
    }
    done;
}

ok64 FILEFlushAll(int const *fd) {
    sane(fd && *fd >= 0 && *fd < FILE_MAX_OPEN && FILE_WANT_BUFS);
    u8bp buf = FILE_WANT_BUFS[*fd];
    test(buf[0] != NULL, BADARG);
    u8csp data = u8bDataC(buf);
    while (u8bDataLen(buf)) {
        int r = write(*fd, *data, u8csLen(data));
        if (r < 0) fail(FILEERROR);  // todo
        u8csFed(data, r);
    }
    Breset(buf);
    done;
}

// Streaming I/O primitives (caller-provided buffers)

ok64 FILEFlushThreshold(int fd, u8b buf, size_t threshold) {
    sane(fd >= 0 && Bok(buf));
    if (u8bDataLen(buf) >= threshold) {
        ssize_t written = write(fd, *u8bData(buf), u8bDataLen(buf));
        if (written < 0) return FILEERROR;  // TODO vocabulary
        Bate(buf);                          // Move data -> past
    }
    done;
}

ok64 FILEEnsureSoft(int fd, u8b buf, size_t needed) {
    sane(fd >= 0 && Bok(buf));
    if (u8bPastLen(buf) > u8bDataLen(buf)) {
        u8bShift(buf, 0);
    }
    while (u8bDataLen(buf) < needed && u8bIdleLen(buf) > 0) {
        ssize_t n = read(fd, *u8bIdle(buf), u8bIdleLen(buf));
        if (n < 0) return FILEERROR;
        if (n == 0) break;  // EOF
        u8bFed(buf, n);
    }
    done;
}

ok64 FILEEnsureHard(int fd, u8b buf, size_t needed) {
    sane(fd >= 0 && Bok(buf));

    // Check if buffer can even hold 'needed' bytes
    test(u8bLen(buf) >= needed, NOROOM);

    // First, try to compact if needed
    if (u8bIdleLen(buf) + u8bDataLen(buf) < needed && u8bPastLen(buf) > 0) {
        u8bShift(buf, 0);
    }

    // Read until we have enough
    while (u8bDataLen(buf) < needed) {
        test(u8bIdleLen(buf) > 0, NOROOM);
        ssize_t n = read(fd, *u8bIdle(buf), u8bIdleLen(buf));
        if (n < 0) return FILEERROR;
        test(n > 0, FILEEND);  // EOF before getting needed bytes
        Bump(buf, n);
    }
    done;
}

ok64 FILENoteMap(int const *fd, u8bp buf, b8 rw) {
    sane(*fd > FILE_CLOSED && u8bOK(buf));
    call(FILEInit);
    rw ? BitSet(FILE_RW, *fd) : BitUnset(FILE_RW, *fd);
    done;
}

ok64 FILEFindMap(int *fd, u8bp buf) {
    sane(fd != NULL && u8bOK(buf));
    int bfd = FILEBookedFD(buf);
    if (bfd >= 0) { *fd = bfd; done; }
    // Slow path: linear search by base pointer
    if (FILE_WANT_BUFS) {
        for (int i = 0; i < FILE_MAX_OPEN; i++) {
            if (FILE_WANT_BUFS[i][0] == buf[0]) {
                *fd = i;
                done;
            }
        }
    }
    fail(NONE);
}

ok64 FILETrimMap(u8bp buf) {
    sane(u8bOK(buf));
    int fd = FILE_CLOSED;
    call(FILEFindMap, &fd, buf);
    call(FILEResize, &fd, u8bDataLen(buf));
    u8c **b = (u8c **)buf;
    b[3] = b[2];
    done;
}

ok64 FILEDropMap(int *fd, u8bp buf) {
    sane(u8bOK(buf));
    call(FILEFindMap, fd, buf);
    BitUnset(FILE_RW, *fd);
    done;
}

ok64 FILECloseAll() {
    sane(1);
    // todo unmap/close all managed fds
    call(u8bFree, FILE_RW);
    done;
}

ok64 FILEMapFD(u8bp *buf, int const *fd, int mode) {
    sane(buf != NULL && FILEok(*fd));
    test(*fd >= 0 && *fd < FILE_MAX_OPEN, BADARG);
    call(FILEBookInit);
    u8bp slot = FILE_WANT_BUFS[*fd];
    test(slot[0] == NULL, BADARG);  // slot must be free
    size_t size;
    call(FILESize, &size, fd);
    u8 *map = (u8 *)mmap(NULL, size, mode, MAP_FILE | MAP_SHARED, *fd, 0);
    FILETestC(map != MAP_FAILED);
    uint8_t **b = (uint8_t **)slot;
    b[0] = b[1] = b[2] = b[3] = map;
    b[3] += size;
    if (0 == (mode & PROT_WRITE)) b[2] += size;
    call(FILENoteMap, fd, slot, (mode & PROT_WRITE) != 0);
    *buf = slot;
    done;
}

ok64 FILEMapRO(u8bp *buf, path8cg path) {
    sane(buf != NULL && PATHu8cgOK(path));
    int fd = FILE_CLOSED;
    call(FILEOpen, &fd, path, O_RDONLY);
    call(FILEMapFD, buf, &fd, PROT_READ);
    done;
}

ok64 FILEMapROAt(u8bp *buf, int dir, path8cg path) {
    sane(buf != NULL && PATHu8cgOK(path));
    int fd = FILE_CLOSED;
    call(FILEOpenAt, &fd, dir, path, O_RDONLY);
    call(FILEMapFD, buf, &fd, PROT_READ);
    done;
}

ok64 FILEMapRW(u8bp *buf, path8cg path) {
    sane(buf != NULL && PATHu8cgOK(path));
    int fd = FILE_CLOSED;
    call(FILEOpen, &fd, path, O_RDWR);
    call(FILEMapFD, buf, &fd, PROT_READ | PROT_WRITE);
    done;
}

ok64 FILEMapCreate(u8bp *buf, path8cg path, size_t size) {
    sane(buf != NULL && PATHu8cgOK(path));
    int fd = FILE_CLOSED;
    call(FILECreate, &fd, path);
    call(FILEResize, &fd, size);
    call(FILEMapFD, buf, &fd, PROT_READ | PROT_WRITE);
    done;
}

ok64 FILEMapCreateAt(u8bp *buf, int dir, path8cg path, size_t size) {
    sane(buf != NULL && PATHu8cgOK(path) && dir > FILE_CLOSED);
    int fd = FILE_CLOSED;
    call(FILECreateAt, &fd, dir, path);
    call(FILEResize, &fd, size);
    call(FILEMapFD, buf, &fd, PROT_READ | PROT_WRITE);
    done;
}

ok64 FILEReMap(u8bp buf, size_t new_size) {
    sane(Bok(buf));
    int fd = FILEBookedFD(buf);
    test(fd >= 0, NONE);
    b8 rw = BitAt(FILE_RW, fd);
    int prot = PROT_READ;
    if (rw) prot |= PROT_WRITE;
    // Unmap old
    FILETestC(-1 != munmap(buf[0], buf[3] - buf[0]));
    u8c **b = (u8c **)buf;
    b[0] = b[1] = b[2] = b[3] = NULL;
    // Resize and re-map into same slot
    call(FILEResize, &fd, new_size);
    size_t size;
    call(FILESize, &size, &fd);
    u8 *map = (u8 *)mmap(NULL, size, prot, MAP_FILE | MAP_SHARED, fd, 0);
    FILETestC(map != MAP_FAILED);
    b[0] = b[1] = b[2] = (u8c *)map;
    b[3] = (u8c *)map + size;
    if (0 == (prot & PROT_WRITE)) b[2] = b[3];
    // mremap() is not supported in FreeBSD
    done;
}

ok64 FILEUnMapFD(u8b buf, int const *fd) {
    sane(Bok(buf));
    u8c **b = (u8c **)buf;
    FILETestC(-1 != munmap(buf[0], Blen(b)));
    BitUnset(FILE_RW, *fd);
    b[0] = b[1] = b[2] = b[3] = NULL;
    done;
}

ok64 FILEUnMap(u8bp buf) {
    sane(Bok(buf));
    int fd = FILEBookedFD(buf);
    test(fd >= 0, NONE);
    call(FILEUnMapFD, buf, &fd);
    call(FILEClose, &fd);
    done;
}

// . . . . . . . . booked mmapped buffers . . . . . . . .

// Internal: book VA range and map fd at start
fun ok64 FILEBookFD(u8bp *buf, int const *fd, size_t book_size) {
    sane(buf != NULL && FILEok(*fd));
    test(*fd >= 0 && *fd < FILE_MAX_OPEN, BADARG);

    call(FILEBookInit);

    size_t file_size;
    call(FILESize, &file_size, fd);

    // Round up to system page boundaries (16KB on Apple Silicon)
    size_t sp = FILESysPage();
    book_size = roundup(book_size, sp);
    file_size = roundup(file_size, sp);
    test(file_size <= book_size, BADARG);

    // Slot must be free
    u8bp slot = FILE_WANT_BUFS[*fd];
    test(slot[0] == NULL, BADARG);

    // Reserve entire VA range with PROT_NONE (no access yet)
    u8 *base = (u8 *)mmap(NULL, book_size, PROT_NONE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    FILETestC(base != MAP_FAILED);

    // Map the file at the start of the reserved range
    u8 *map = (u8 *)mmap(base, file_size, PROT_READ | PROT_WRITE,
                         MAP_FILE | MAP_SHARED | MAP_FIXED, *fd, 0);
    if (map == MAP_FAILED) {
        munmap(base, book_size);
        FILETestC(0);  // return errno
    }

    // Setup buffer slot
    uint8_t **b = (uint8_t **)slot;
    b[0] = b[1] = b[2] = map;
    b[3] = map + file_size;

    // Track the mapping and booked end
    call(FILEInit);
    call(FILENoteMap, fd, slot, YES);

    // Store booked end in FILE_BOOK
    size_t fdlen = u8pbDataLen(FILE_BOOK);
    if (*fd >= fdlen) u8psFed(u8pbIdle(FILE_BOOK), *fd - fdlen + 1);
    *u8pbAtP(FILE_BOOK, *fd) = base + book_size;

    FILE_WANTS[*fd] = FILEBookWant;
    *buf = slot;
    done;
}

ok64 FILEBook(u8bp *buf, path8cg path, size_t book_size) {
    sane(buf != NULL && PATHu8cgOK(path));
    int fd = FILE_CLOSED;
    call(FILEOpen, &fd, path, O_RDWR);
    call(FILEBookFD, buf, &fd, book_size);
    done;
}

ok64 FILEBookAt(u8bp *buf, int dir, path8cg path, size_t book_size) {
    sane(buf != NULL && PATHu8cgOK(path));
    int fd = FILE_CLOSED;
    call(FILEOpenAt, &fd, dir, path, O_RDWR);
    call(FILEBookFD, buf, &fd, book_size);
    done;
}

ok64 FILEBookCreate(u8bp *buf, path8cg path, size_t book_size,
                    size_t init_size) {
    sane(buf != NULL && PATHu8cgOK(path) && init_size <= book_size);
    int fd = FILE_CLOSED;
    call(FILECreate, &fd, path);
    size_t sp = FILESysPage();
    init_size = roundup(init_size, sp);
    if (init_size == 0) init_size = sp;
    call(FILEResize, &fd, init_size);
    call(FILEBookFD, buf, &fd, book_size);
    done;
}

ok64 FILEBookCreateAt(u8bp *buf, int dir, path8cg path, size_t book_size,
                      size_t init_size) {
    sane(buf != NULL && PATHu8cgOK(path) && dir > FILE_CLOSED);
    int fd = FILE_CLOSED;
    call(FILECreateAt, &fd, dir, path);
    size_t sp = FILESysPage();
    init_size = roundup(init_size, sp);
    if (init_size == 0) init_size = sp;
    call(FILEResize, &fd, init_size);
    call(FILEBookFD, buf, &fd, book_size);
    done;
}

ok64 FILEBookExtend(u8bp buf, size_t new_size) {
    sane(Bok(buf));
    int fd = FILEBookedFD(buf);
    test(fd >= 0, FILENOBOOK);

    // Get booked end
    u8p booked_end = *u8pbAtP(FILE_BOOK, fd);
    test(booked_end != NULL, FILENOBOOK);

    u8p base = buf[0];
    size_t book_size = booked_end - base;

    size_t sp = FILESysPage();
    new_size = roundup(new_size, sp);
    test(new_size <= book_size, BNOROOM);

    size_t old_size = buf[3] - base;
    if (new_size <= old_size) done;  // already big enough

    // Extend the file
    call(FILEResize, &fd, new_size);

    // Map the new portion into the reserved range
    size_t extend_size = new_size - old_size;
    u8 *extend_base = base + old_size;
    u8 *map = (u8 *)mmap(extend_base, extend_size, PROT_READ | PROT_WRITE,
                         MAP_FILE | MAP_SHARED | MAP_FIXED, fd, old_size);
    FILETestC(map != MAP_FAILED);

    // Update buffer end
    ((u8 **)buf)[3] = base + new_size;

    done;
}

ok64 FILETrimBook(u8bp buf) {
    sane(Bok(buf));
    int fd = FILEBookedFD(buf);
    test(fd >= 0, FILENOBOOK);
    call(FILEResize, &fd, u8bDataLen(buf));
    u8c **b = (u8c **)buf;
    b[3] = b[2];
    done;
}

ok64 FILEBookGrow(u8bp buf, size_t need) {
    sane(Bok(buf));
    size_t cur = buf[3] - buf[0];           // current mapped size
    size_t data = u8bBusyLen(buf);          // past + data
    size_t target = data + need;
    size_t sp = FILESysPage();
    size_t grow = cur + (cur >> 3);         // cur * 9/8
    if (grow < target) grow = target;
    if (grow < cur + sp) grow = cur + sp;
    grow = roundup(grow, sp);
    return FILEBookExtend(buf, grow);
}

ok64 FILEMSync(u8bp buf) {
    sane(Bok(buf));
    size_t size = buf[3] - buf[0];
    FILETestC(msync(buf[0], size, MS_SYNC) == 0);
    done;
}

ok64 FILEUnBook(u8bp buf) {
    sane(Bok(buf));
    int fd = FILEBookedFD(buf);
    test(fd >= 0, FILENOBOOK);

    // Get booked end
    u8p booked_end = *u8pbAtP(FILE_BOOK, fd);
    test(booked_end != NULL, FILENOBOOK);

    u8p base = buf[0];
    size_t book_size = booked_end - base;

    // Unmap the entire booked range (includes file mapping + reserved)
    FILETestC(munmap(base, book_size) == 0);

    // Clear tracking
    *u8pbAtP(FILE_BOOK, fd) = NULL;
    BitUnset(FILE_RW, fd);

    // Shrink FILE_BOOK tracking array if possible
    u8psp data = u8pbData(FILE_BOOK);
    while (!$empty(data) && *u8psLast(data) == NULL) u8psPuked(data, 1);

    FILE_WANTS[fd] = NULL;

    call(FILEClose, &fd);

    // Clear buffer slot
    u8c **b = (u8c **)buf;
    b[0] = b[1] = b[2] = b[3] = NULL;

    done;
}

ok64 FILEScan(path8 path, FILE_SCAN mode, path8f f, voidp arg) {
    sane(PATHu8bSane(path) && f);
    DIR *dir = opendir((const char *)*path);
    if (dir == NULL) {
        // todo
        fail(FILEBAD);
    }
    u8sp idle = u8bIdle(path);
    u8p saved_end = *idle;
    struct dirent *entry = 0;
    ok64 o = OK;
    while (o == OK && (entry = readdir(dir))) {
        a_cstr(fn, entry->d_name);
        // Skip . and .. entries (check length first, then value)
        u64 nlen = u8csLen(fn);
        if ((nlen == 1 && fn[0][0] == '.') ||
            (nlen == 2 && fn[0][0] == '.' && fn[0][1] == '.'))
            continue;
        o = PATHu8bPush(path, fn);
        if (o == BNOROOM) {
            // Path too long - skip this entry and continue
            o = OK;
            *idle = saved_end;
            continue;
        }
        unsigned char dtype = entry->d_type;
        if (dtype == DT_UNKNOWN) {
            struct stat st;
            if (lstat((const char *)*path, &st) == 0) {
                if (S_ISREG(st.st_mode))
                    dtype = DT_REG;
                else if (S_ISDIR(st.st_mode))
                    dtype = DT_DIR;
                else if (S_ISLNK(st.st_mode))
                    dtype = DT_LNK;
            }
        }
        switch (dtype) {
            case DT_DIR:
                if (o == OK && (mode & FILE_SCAN_DIRS)) o = f(arg, path);
                if (o == FILESKIP)
                    o = OK;  // Skip recursion but continue scan
                else if (o == OK && (mode & FILE_SCAN_DEEP))
                    o = FILEScan(path, mode, f, arg);
                break;
            case DT_LNK:
                if (o == OK && (mode & FILE_SCAN_LINKS)) o = f(arg, path);
                break;
            case DT_REG:
                if (o == OK && (mode & FILE_SCAN_FILES)) o = f(arg, path);
                break;
            default:
        }
        *idle = saved_end;
    }
    closedir(dir);
    return o;
}

// File tree iterator implementation

ok64 FILEIterOpen(fileitp it, path8gp path) {
    sane(it && path && PATHu8gOK((path8cgp)path));
    it->dir = opendir((char const *)path[0]);
    if (!it->dir) return FILEerrno(errno);
    it->path = path;
    it->dirend = path[1];
    it->type = 0;
    it->flags = 0;
    it->sort = NULL;
    it->buf = NULL;
    it->buf_mark = NULL;
    it->stream[0] = it->stream[1] = NULL;
    return OK;
}

// Helper: load and sort entries from dir into buffer using LSMSort
// Buffer layout: [entries...][tmp space for sort]
// After sort: [sorted entries...][free for children]
fun ok64 FILELoadSorted(fileitp it, DIR *dir, u8csz z) {
    sane(it && it->buf && Bok(it->buf) && z);
    u8bp buf = it->buf;

    // Save buffer position for stack pop later
    it->buf_mark = buf[2];

    // Read all entries into buffer as [type][len][name]
    u8sp idle = u8bIdle(buf);
    u8s entries = {*idle, *idle};

    struct dirent *e;
    while ((e = readdir(dir))) {
        if (e->d_name[0] == '.') {
            if (e->d_name[1] == 0) continue;
            if (e->d_name[1] == '.' && e->d_name[2] == 0) continue;
        }
        u8cs nm = u8csOf(e->d_name);
        test(u8csLen(nm) <= 0xff, FILENAMEBAD);
        // Write entry: [type][len][name]
        call(u8sFeed2, idle, e->d_type, (u8)$len(nm));
        call(u8sFeed, idle, nm);
    }

    entries[1] = *idle;

    if ($empty(entries)) {
        zero$(it->stream);
        done;
    }

    // Remaining idle is tmp for LSMSort (must be >= entries size)
    a_dup(u8, tmp, idle);
    test($len(tmp) >= $len(entries), BNOROOM);

    // Sort entries in place using LSMSort
    call(LSMSort, entries, FILEentryX, z, FILEentryY, tmp);

    // After LSMSort, entries[0..entries[1]) contains sorted data
    // (may be smaller if merger filtered, but we pass through everything)
    $mv(it->stream, entries);

    // Advance buffer past sorted entries (stack push)
    u8sMv(u8bIdle(buf), idle);

    return OK;
}

ok64 FILEIterOpenSorted(fileitp it, path8gp path, u8bp buf, u8csz z) {
    sane(it && path && PATHu8gOK((path8cgp)path) && Bok(buf) && z);
    DIR *dir = opendir((char const *)path[0]);
    if (!dir) return FILEerrno(errno);

    // Setup iterator basics
    it->path = path;
    it->dirend = path[1];
    it->type = 0;
    it->flags = 0;
    it->sort = z;
    it->buf = buf;
    it->dir = NULL;
    it->stream[0] = it->stream[1] = NULL;

    // Load and sort entries
    ok64 o = FILELoadSorted(it, dir, z);
    closedir(dir);
    return o;
}

ok64 FILEIterClose(fileitp it) {
    sane(it);
    if (it->dir) {
        closedir((DIR *)it->dir);
        it->dir = NULL;
    }
    if (it->path && it->dirend) {
        it->path[1] = it->dirend;
        call(PATHu8gTerm, it->path);
    }
    // Restore buffer position (stack pop)
    if (it->buf && it->buf_mark) {
        u8bIdle(it->buf)[0] = it->buf_mark;
    }
    it->stream[0] = it->stream[1] = NULL;
    it->sort = NULL;
    it->buf = NULL;
    it->buf_mark = NULL;
    return OK;
}

ok64 FILENext(fileitp it) {
    sane(it && it->path);

    // Sorted mode: drain entries from sorted stream via slicer
    if (it->sort) {
        it->path[1] = it->dirend;
        if ($empty(it->stream)) {
            it->type = 0;
            return END;
        }
        // Slice next entry: [type][len][name]
        u8cs entry;
        ok64 o = FILEentryX(entry, it->stream);
        if (o != OK) {
            it->type = 0;
            return END;
        }
        it->type = entry[0][0];
        u8cs name = {entry[0] + 2, entry[1]};  // skip type and len
        o = PATHu8gPush(it->path, name);
        if (o != OK) return o;
        return OK;
    }

    // Unsorted mode: use DIR*
    if (!it->dir) return BADARG;
    it->path[1] = it->dirend;
    struct dirent *e;
    while ((e = readdir((DIR *)it->dir))) {
        if (e->d_name[0] == '.') {
            if (e->d_name[1] == 0) continue;
            if (e->d_name[1] == '.' && e->d_name[2] == 0) continue;
        }
        it->type = e->d_type;
        a_cstr(name, e->d_name);
        ok64 o = PATHu8gPush(it->path, name);
        if (o == BNOROOM) continue;
        if (o != OK) return o;
        return OK;
    }
    it->type = 0;
    return END;
}

ok64 FILEInto(fileitp child, fileitp parent) {
    sane(child && parent && parent->path && parent->type == DT_DIR);
    DIR *dir = opendir((char const *)parent->path[0]);
    if (!dir) return FILEerrno(errno);

    child->path = parent->path;
    child->dirend = parent->path[1];
    child->type = 0;
    child->flags = 0;
    child->stream[0] = child->stream[1] = NULL;

    // Inherit sort and buf from parent (nullable)
    child->sort = parent->sort;
    child->buf = parent->buf;
    child->buf_mark = NULL;

    if (child->sort && child->buf) {
        // Sorted mode: load entries into inherited buffer
        ok64 o = FILELoadSorted(child, dir, child->sort);
        closedir(dir);
        child->dir = NULL;
        return o;
    } else {
        // Unsorted mode: use DIR*
        child->dir = dir;
        return OK;
    }
}

ok64 FILEOuto(fileitp child, fileitp parent) {
    sane(child && parent);
    if (child->dir) {
        closedir((DIR *)child->dir);
        child->dir = NULL;
    }
    // Restore buffer position (stack pop)
    if (child->buf && child->buf_mark) {
        u8bIdle(child->buf)[0] = child->buf_mark;
    }
    child->stream[0] = child->stream[1] = NULL;
    child->sort = NULL;
    child->buf = NULL;
    child->buf_mark = NULL;
    return OK;
}
