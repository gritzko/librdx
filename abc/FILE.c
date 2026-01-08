#include "FILE.h"

#include <dirent.h>
#include <sys/mman.h>

#include "01.h"
#include "BUF.h"
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

ok64 path8Push(path8 path, u8csc part) {
    sane(u8bOK(path) && $ok(part) && !$empty(part));
    call(u8bFeed1, path, FILE_PATH_SEP);
    size_t dl = u8bDataLen(path);
    call(u8bFeed, path, part);
    u8sUsed(u8bData(path), dl);
    done;
}

ok64 FILEMakeDir(path8 path) {
    sane(path8Sane(path));
    int rc = mkdir(path8CStr(path), S_IRWXU);
    testc(rc == 0, FILEfail);
    done;
}

// Remove directory. If recursive=true, delete contents first (rm -rf style).
ok64 FILERmDir(path8 path, bool recursive) {
    sane(path8Sane(path));
    
    // Save the original path as a C string for final rmdir
    // (path8 buffer gets modified during recursive traversal)
    a_pad(u8, saved, FILE_PATH_MAX_LEN);
    u8c const* pstart = path[0];
    u8c const* pend = *u8bDataC(path) + u8bDataLen(path);
    u8cs orig = {pstart, pend};
    u8sFeed(saved_idle, orig);
    u8sFeed1(saved_idle, 0);
    
    if (recursive) {
        // Open directory and delete contents
        DIR *dir = opendir((char*)*saved_data);
        if (dir == NULL) {
            testc(errno == ENOENT, FILEfail);  // OK if doesn't exist
            done;
        }
        
        u64 dl = u8bDataLen(path);
        u8gUsedAll(u8bPastData(path));
        struct dirent *entry = 0;
        ok64 o = OK;
        
        while (o == OK && (entry = readdir(dir))) {
            // Skip . and ..
            if (entry->d_name[0] == '.') {
                if (entry->d_name[1] == 0) continue;
                if (entry->d_name[1] == '.' && entry->d_name[2] == 0) continue;
            }
            
            a_cstr(fn, entry->d_name);
            o = path8Push(path, fn);
            if (o != OK) break;
            
            switch (entry->d_type) {
                case DT_DIR:
                    o = FILERmDir(path, true);  // recurse
                    break;
                case DT_REG:
                case DT_LNK:
                default:
                    o = FILEunlink(path);
                    break;
            }
            u8bShedAll(path);
        }
        
        u8gShed(u8bPastData(path), dl);
        closedir(dir);
        if (o != OK) return o;
    }
    
    int rc = rmdir((char*)*saved_data);
    testc(rc == 0, FILEfail);
    done;
}

ok64 FILEunlink(path8 path) {
    sane(path8Sane(path));
    int rc = unlink(path8CStr(path));
    testc(rc == 0, FILEfail);
    done;
}

ok64 FILEisdir(path8 path) {
    sane(path8Sane(path));
    struct stat sb = {};
    call(FILEStat, &sb, path);
    test(sb.st_mode & S_IFDIR, FILEwrong);
    done;
}

ok64 FILESync(int const *fd) {
    sane(FILEok(*fd));
    testc(fsync(*fd) == 0, FILEnosync);
    done;
}

pro(FILEClose, int *fd) {
    sane(FILEok(*fd));
    testc(0 == close(*fd), FILEnoclse);
    *fd = FILE_CLOSED;
    done;
}

ok64 FILECreate(int *fd, path8 path) {
    sane(fd != NULL && path8Sane(path));
    *fd = open(path8CStr(path), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (*fd < 0) fail(FILEErr(FILEnoopen));
    done;
}

ok64 FILECreateAt(int *fd, int dir, path8 path) {
    sane(fd != NULL && path8Sane(path));
    *fd = openat(dir, path8CStr(path), O_CREAT | O_RDWR | O_TRUNC,
                 S_IRUSR | S_IWUSR);
    if (*fd < 0) fail(FILEErr(FILEnoopen));
    done;
}

ok64 FILEOpen(int *fd, path8 path, int flags) {
    sane(fd != NULL && path8Sane(path));
    *fd = open(path8CStr(path), flags);
    if (*fd < 0) fail(FILEErr(FILEnoopen));
    done;
}

ok64 FILEOpenAt(int *fd, int const dirfd, path8 path, int flags) {
    sane(fd != NULL && path8Sane(path) && FILEok(dirfd));
    *fd = openat(dirfd, path8CStr(path), flags);
    if (*fd < 0) fail(FILEErr(FILEnoopen));
    done;
}

ok64 FILEStat(struct stat *ret, path8 path) {
    sane(ret != NULL && path8Sane(path));
    int rc = stat(path8CStr(path), ret);
    if (rc == 0) done;
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

ok64 FILESize(size_t *size, int const *fd) {
    sane(size != NULL && FILEok(*fd));
    struct stat sb = {};
    testc(0 == fstat(*fd, &sb), FILEnostat);
    *size = sb.st_size;
    done;
}

ok64 FILEResize(int const *fd, size_t new_size) {
    sane(FILEok(*fd));
    testc(0 == ftruncate(*fd, new_size), FILEnoresz);
    // FIXME sync the dir data (another msync?)
    done;
}

ok64 FILERename(path8 old, path8 neu) {
    sane(path8Sane(old) && path8Sane(neu));
    testc(0 == rename(path8CStr(old), path8CStr(neu)), FILEfail);
    done;
}

fun int unlink_cb(const char *fname, const struct stat *sb, int typeflag,
                  struct FTW *ftwbuf) {
    return remove(fname);
}

ok64 FILERmRF(path8 path) {
    sane(path8Sane(path));
    int rc = nftw(path8CStr(path), unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
    testc(rc == 0, FILEfail);
    done;
}

u8p *FILE_BUFS[4] = {};
u8 *FILE_RW[4] = {};

ok64 FILEFlush(int const *fd) {
    sane(fd && *fd >= 0 && *fd < FILE_MAX_OPEN);
    u8bp buf = FILE_BUFS[*fd];
    if (u8bDataLen(buf) >= PAGESIZE) {
        int r = write(*fd, *u8bDataC(buf), u8bDataLen(buf));
        if (r < 0) fail(FILEerror);  // todo
        u8bFed(buf, r);
        if (u8bPastLen(buf) >= u8bDataLen(buf)) u8bShift(buf, 0);
        // todo if grows too fast
    }
    done;
}

ok64 FILEFlushAll(int const *fd) {
    sane(fd && *fd >= 0 && *fd < FILE_MAX_OPEN && *FILE_BUFS[*fd]);
    u8bp buf = FILE_BUFS[*fd];
    u8csp data = u8bDataC(buf);
    while (u8bDataLen(buf)) {
        int r = write(*fd, *data, u8csLen(data));
        if (r < 0) fail(FILEerror);  // todo
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
        if (written < 0) return FILEerror;  // TODO vocabulary
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
        if (n < 0) return FILEerror;
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
        if (n < 0) return FILEerror;
        test(n > 0, FILEend);  // EOF before getting needed bytes
        Bump(buf, n);
    }
    done;
}

ok64 FILENoteMap(int const *fd, u8bp buf, b8 rw) {
    sane(*fd > FILE_CLOSED && u8bOK(buf));
    call(FILEInit);
    size_t fdlen = u8pbDataLen(FILE_BUFS);
    if (*fd >= fdlen) u8psFed(u8pbIdle(FILE_BUFS), *fd - fdlen + 1);
    *u8pbAtP(FILE_BUFS, *fd) = buf[0];
    rw ? BitSet(FILE_RW, *fd) : BitUnset(FILE_RW, *fd);
    done;
}

ok64 FILEFindMap(int *fd, u8bp buf) {
    sane(fd != NULL && u8bOK(buf));
    size_t l = u8pbDataLen(FILE_BUFS);
    for (int i = 0; i < l; i++)
        if (buf[0] == *u8pbAtP(FILE_BUFS, i)) {
            *fd = i;
            done;
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
    *u8pbAtP(FILE_BUFS, *fd) = NULL;
    BitUnset(FILE_RW, *fd);
    u8psp data = u8pbData(FILE_BUFS);
    while (!$empty(data) && *u8psLast(data) == NULL) u8psPuked(data, 1);
    done;
}

ok64 FILECloseAll() {
    sane(1);
    // todo unmap/close all managed fds
    call(u8pbFree, FILE_BUFS);
    call(u8bFree, FILE_RW);
    done;
}

ok64 FILEMapFD(u8bp buf, int const *fd, int mode) {
    sane(buf != NULL && *buf == NULL && FILEok(*fd));
    test(*fd >= 0 && *fd < FILE_MAX_OPEN, BADARG);
    size_t size;
    call(FILESize, &size, fd);
    u8 *map = (u8 *)mmap(NULL, size, mode, MAP_FILE | MAP_SHARED, *fd, 0);
    testc(map != MAP_FAILED, FILEfail);
    uint8_t **b = (uint8_t **)buf;
    b[0] = b[1] = b[2] = b[3] = map;
    b[3] += size;
    if (0 == (mode & PROT_WRITE)) b[2] += size;
    call(FILENoteMap, fd, buf, (mode & PROT_WRITE) != 0);
    done;
}

ok64 FILEMapRO(u8bp buf, path8 path) {
    sane(buf != NULL && path8Sane(path));
    int fd = FILE_CLOSED;
    call(FILEOpen, &fd, path, O_RDONLY);
    call(FILEMapFD, buf, &fd, PROT_READ);
    // not expecting: opened-but-map-fails
    done;
}

ok64 FILEMapROAt(u8bp buf, int dir, path8 path) {
    sane(buf != NULL && path8Sane(path));
    int fd = FILE_CLOSED;
    call(FILEOpenAt, &fd, dir, path, O_RDONLY);
    call(FILEMapFD, buf, &fd, PROT_READ);
    // not expecting: opened-but-map-fails
    done;
}

ok64 FILEMapRW(u8bp buf, path8 path) {
    sane(buf != NULL && path8Sane(path));
    int fd = FILE_CLOSED;
    call(FILEOpen, &fd, path, O_RDWR);
    call(FILEMapFD, buf, &fd, PROT_READ | PROT_WRITE);
    // not expecting: opened-but-map-fails
    done;
}

ok64 FILEMapCreate(u8bp buf, path8 path, size_t size) {
    sane(buf != NULL && path8Sane(path));
    int fd = FILE_CLOSED;
    call(FILECreate, &fd, path);
    call(FILEResize, &fd, size);
    call(FILEMapFD, buf, &fd, PROT_READ | PROT_WRITE);
    // not expecting: opened-but-map-fails
    done;
}

ok64 FILEMapCreateAt(u8bp buf, int dir, path8 path, size_t size) {
    sane(buf != NULL && path8Sane(path) && dir > FILE_CLOSED);
    int fd = FILE_CLOSED;
    call(FILECreateAt, &fd, dir, path);
    call(FILEResize, &fd, size);
    call(FILEMapFD, buf, &fd, PROT_READ | PROT_WRITE);
    // not expecting: opened-but-map-fails
    done;
}

ok64 FILEReMap(u8bp buf, size_t new_size) {
    sane(Bok(buf));
    int fd = FILE_CLOSED;
    call(FILEFindMap, &fd, buf);
    call(FILEUnMapFD, buf, &fd);  // fixme non-files
    call(FILEResize, &fd, new_size);
    int prot = PROT_READ;
    b8 rw = BitAt(FILE_RW, fd);
    if (rw) prot |= PROT_WRITE;
    call(FILEMapFD, buf, &fd, prot);
    call(FILENoteMap, &fd, buf, rw);  // the buf may change
    // mremap() is not supported in FreeBSD
    done;
}

ok64 FILEUnMapFD(u8b buf, int const *fd) {
    sane(Bok(buf));
    u8c **b = (u8c **)buf;
    testc(-1 != munmap(buf[0], Blen(b)), FILEfail);
    int fd2 = FILE_CLOSED;
    FILEDropMap(&fd2, buf);  // if any
    b[0] = b[1] = b[2] = b[3] = NULL;
    done;
}

ok64 FILEUnMap(u8b buf) {
    sane(Bok(buf));
    int fd = FILE_CLOSED;
    call(FILEDropMap, &fd, buf);
    call(FILEUnMapFD, buf, &fd);
    call(FILEClose, &fd);
    done;
}

ok64 FILEScan(path8 path, FILE_SCAN mode, path8f f, voidp arg) {
    sane(path8Sane(path) && f);
    DIR *dir = opendir((const char *)*path);
    if (dir == NULL) {
        // todo
        fail(FILEbad);
    }
    u64 dl = u8bDataLen(path);
    u8gUsedAll(u8bPastData(path));
    struct dirent *entry = 0;
    ok64 o = OK;
    while (o == OK && (entry = readdir(dir))) {
        a_cstr(fn, entry->d_name);
        o = path8Push(path, fn);
        switch (entry->d_type) {
            case DT_DIR:  // todo _BSD_SOURCE
                if (o == OK && (mode & FILE_SCAN_DIRS)) o = f(arg, path);
                if (o == OK && (mode & FILE_SCAN_DEEP))
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
        u8bShedAll(path);
    }
    u8gShed(u8bPastData(path), dl);
    closedir(dir);
    return o;
}
