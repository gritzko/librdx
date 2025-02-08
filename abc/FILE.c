#include "FILE.h"

#include <sys/mman.h>

ok64 FILEisdir(const path name) {
    sane($ok(name));
    struct stat sb = {};
    call(FILEstat, &sb, name);
    test(sb.st_mode == S_IFDIR, FILEwrong);
    done;
}

ok64 FILEsync(int const *fd) {
    sane(FILEok(*fd));
    testc(fsync(*fd) == 0, FILEnosync);
    done;
}

pro(FILEclose, int *fd) {
    sane(FILEok(*fd));
    testc(0 == close(*fd), FILEnoclse);
    *fd = FILE_CLOSED;
    done;
}

ok64 FILEcreate(int *fd, const path name) {
    sane(fd != nil && $ok(name));
    aFILEpath(p, name);
    *fd = open(p, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    testc(*fd >= 0, FILEnoopen);
    done;
}

ok64 FILEopen(int *fd, const path name, int flags) {
    sane(fd != nil && $ok(name));
    aFILEpath(p, name);
    *fd = open(p, flags);
    testc(*fd >= 0, FILEnoopen);
    done;
}

ok64 FILEstat(struct stat *ret, const path name) {
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

ok64 FILEsize(size_t *size, int const *fd) {
    sane(size != nil && FILEok(fd));
    struct stat sb = {};
    testc(0 == fstat(*fd, &sb), FILEnostat);
    *size = sb.st_size;
    done;
}

ok64 FILEresize(int const *fd, size_t new_size) {
    sane(FILEok(fd));
    testc(0 == ftruncate(*fd, new_size), FILEnoresz);
    // FIXME sync the dir data (another msync?)
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

ok64 FILEmap(Bu8 buf, int const *fd, int mode) {
    sane(buf != nil && *buf == nil && FILEok(fd));
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

ok64 FILEunmap(Bu8 buf) {
    sane(Bok(buf));
    u8c **b = (u8c **)buf;
    testc(-1 != munmap(buf[0], Blen(b)), FILEfail);
    b[0] = b[1] = b[2] = b[3] = nil;
    done;
}
