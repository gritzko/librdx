
#include "NET.h"

con ok64 UDPfail = 0x30b65a9935e;

#define a$c(name, s, len)          \
    char name[len] = {0};          \
    if (!$empty(s)) {              \
        int l = $len(s);           \
        if (l >= len) l = len - 1; \
        memcpy(name, *s, l);       \
        name[l] = 0;               \
    }

ok64 UDPbind(int *fd, NETaddr addr);

ok64 UDPconnect(int *fd, NETaddr addr);

fun ok64 UDPdrain($u8 into, NETaddr addr, int fd) {
    socklen_t len = Blen(addr);
    ssize_t nread =
        recvfrom(fd, *into, $len(into), 0, (struct sockaddr *)*addr, &len);
    if (nread == -1) return UDPfail;
    range64 range = {0, len};
    Bu8rewind(addr, range);
    *into += nread;
    return OK;
}

fun ok64 UDPfeed(int fd, NETaddr addr, $u8c data) {
    u8$ raw = NETraw(addr);
    ssize_t sz =
        sendto(fd, *data, $len(data), 0, (struct sockaddr *)*raw, $len(raw));
    if (sz == -1) return UDPfail;
    data[0] += sz;
    return OK;
}

fun ok64 UDPclose(int fd) {
    int r = close(fd);
    return r == 0 ? OK : UDPfail;
}
