
#include <sys/socket.h>

#include "NET.h"

static const ok64 TCPfail = 0x1d319aa5b70;

ok64 TCPbind(int *fd, NETaddr addr);

ok64 TCPconnect(int *fd, NETaddr addr);

fun ok64 TCPaccept(int *cfd, NETaddr addr, int sfd) {
    socklen_t len = Blen(addr);
    int rc = accept(sfd, (struct sockaddr *)*addr, &len);
    if (rc == -1) return TCPfail;
    *cfd = rc;
    return OK;
}

fun ok64 TCPclose(int fd) {
    int r = close(fd);
    return r == 0 ? OK : TCPfail;
}
