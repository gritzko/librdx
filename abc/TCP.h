
#include <sys/socket.h>

#include "NET.h"

con ok64 TCPfail = 0x1d319aa5b70;

ok64 TCPBind(int *fd, NETaddr addr);

ok64 TCPConnect(int *fd, NETaddr addr, b8 nonblocking);

fun ok64 TCPAccept(int *cfd, NETaddr addr, int sfd) {
    socklen_t len = Blen(addr);
    int rc = accept(sfd, (struct sockaddr *)*addr, &len);
    if (rc == -1) return TCPfail;
    *cfd = rc;
    return OK;
}

fun ok64 TCPClose(int fd) {
    int r = close(fd);
    return r == 0 ? OK : TCPfail;
}
