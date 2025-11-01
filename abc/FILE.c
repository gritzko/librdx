#include "FILE.h"

#include <sys/mman.h>

#include "PRO.h"

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

ok64 path8Join(path8 path, u8csc part) {
    sane(path8Sane(path) && $ok(part) && !$empty(part));
    call(u8bFeed1, path, FILE_PATH_SEP);
    u8sAte(u8bData(path));
    call(u8bFeed, path, part);
    done;
}

ok64 FILEmakedir(path8 path) {
    sane(path8Sane(path));
    int rc = mkdir(path8CStr(path), S_IRWXU);
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

ok64 FILEOpen(int *fd, path8 path, int flags) {
    sane(fd != NULL && path8Sane(path));
    *fd = open(path8CStr(path), flags);
    if (*fd < 0) fail(FILEErr(FILEnoopen));
    done;
}

fun ok64 FILEOpenDir(int *fd, path8 path) {
    return FILEOpen(fd, path, O_DIRECTORY);
}

ok64 FILEOpenAt(int *fd, int const *dirfd, path8 path, int flags) {
    sane(fd != NULL && path8Sane(path) && FILEok(*dirfd));
    *fd = openat(*dirfd, path8CStr(path), flags);
    if (*fd < 0) fail(FILEErr(FILEnoopen));
    done;
}

ok64 FILEStat(struct stat *ret, path8 path) {
    sane(ret != NULL && path8Sane(path));
    int rc = stat(path8CStr(path), ret);
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

ok64 FILEMap(u8bp buf, int const *fd, int mode) {
    sane(buf != NULL && *buf == NULL && FILEok(*fd));
    size_t size;
    call(FILESize, &size, fd);
    u8 *map = (u8 *)mmap(NULL, size, mode, MAP_FILE | MAP_SHARED, *fd, 0);
    testc(map != MAP_FAILED, FILEfail);
    uint8_t **b = (uint8_t **)buf;
    b[0] = b[1] = b[2] = b[3] = map;
    b[3] += size;
    if (0 == (mode & PROT_WRITE)) b[2] += size;
    done;
}

ok64 FILEUnMap(u8b buf) {
    sane(Bok(buf));
    u8c **b = (u8c **)buf;
    testc(-1 != munmap(buf[0], Blen(b)), FILEfail);
    b[0] = b[1] = b[2] = b[3] = NULL;
    done;
}
