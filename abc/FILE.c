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

ok64 FILEmakedir(path const name) {
    sane($ok(name));
    aFILEpath(p, name);
    int rc = mkdir(p, S_IRWXU);
    testc(rc == 0, FILEfail);
    done;
}

ok64 FILEunlink(path const name) {
    sane($ok(name));
    aFILEpath(p, name);
    int rc = unlink(p);
    testc(rc == 0, FILEfail);
    done;
}

ok64 FILEisdir(const path name) {
    sane($ok(name));
    struct stat sb = {};
    call(FILEstat, &sb, name);
    test(sb.st_mode & S_IFDIR, FILEwrong);
    done;
}

ok64 FILEsync(int const *fd) {
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

ok64 FILECreate(int *fd, const path name) {
    sane(fd != NULL && $ok(name));
    aFILEpath(p, name);
    *fd = open(p, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (*fd < 0) fail(FILEErr(FILEnoopen));
    done;
}

ok64 FILEOpen(int *fd, const path name, int flags) {
    sane(fd != NULL && $ok(name));
    aFILEpath(p, name);
    *fd = open(p, flags);
    if (*fd < 0) fail(FILEErr(FILEnoopen));
    done;
}

ok64 FILEOpenAt(int *fd, int const *dirfd, const path name, int flags) {
    sane(fd != NULL && $ok(name) && FILEok(*dirfd));
    aFILEpath(p, name);
    *fd = openat(*dirfd, p, flags);
    if (*fd < 0) fail(FILEErr(FILEnoopen));
    done;
}

ok64 FILEstat(struct stat *ret, const path name) {
    sane(ret != NULL && $ok(name));
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

ok64 FILEsize(size_t *size, int const *fd) {
    sane(size != NULL && FILEok(*fd));
    struct stat sb = {};
    testc(0 == fstat(*fd, &sb), FILEnostat);
    *size = sb.st_size;
    done;
}

ok64 FILEresize(int const *fd, size_t new_size) {
    sane(FILEok(*fd));
    testc(0 == ftruncate(*fd, new_size), FILEnoresz);
    // FIXME sync the dir data (another msync?)
    done;
}

ok64 FILErename(const path oldname, const path newname) {
    sane($ok(oldname) && $ok(newname));
    aFILEpath(old, oldname);
    aFILEpath(neu, newname);
    testc(0 == rename(old, neu), FILEfail);
    done;
}

fun int unlink_cb(const char *fname, const struct stat *sb, int typeflag,
                  struct FTW *ftwbuf) {
    return remove(fname);
}

ok64 FILErmrf(path const name) {
    sane($ok(name));
    aFILEpath(p, name);
    int rc = nftw(p, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
    testc(rc == 0, FILEfail);
    done;
}

ok64 FILEMap(u8bp buf, int const *fd, int mode) {
    sane(buf != NULL && *buf == NULL && FILEok(*fd));
    size_t size;
    call(FILEsize, &size, fd);
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
