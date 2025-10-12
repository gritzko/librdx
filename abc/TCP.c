#include "TCP.h"

#include <sys/socket.h>

#include "NET.h"
#include "PRO.h"
#include "S.h"

ok64 TCPConnect(int *fd, u8csc address, b8 nonblocking) {
    sane(fd != NULL && !$empty(address));
    int sfd, s;
    struct addrinfo *result = NULL, *rp;

    call(NETParseAddress, &result, address, YES);

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;

        int rc = connect(sfd, rp->ai_addr, rp->ai_addrlen);
        if (rc == 0) break;
        perror("something");

        close(sfd);
    }

    NETFreeAddress(&result);

    if (rp == NULL) return TCPfail;
    *fd = sfd;
    return OK;
}

pro(TCPListen, int *fd, u8cs addr) {
    sane(fd != nil && !$empty(addr));
    int s, sfd;
    struct addrinfo *result = NULL, *rp;

    call(NETParseAddress, &result, addr, YES);

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, 0);
        if (sfd == -1) continue;

        int rc = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (rc != 0) {
            trace("getadd: %s\n", gai_strerror(s));
            if (result) NETFreeAddress(&result);
            return TCPfail;
        }
        if (rc == 0) break;

        close(sfd);
    }

    NETFreeAddress(&result);

    test(sfd != -1, TCPfail);
    int lrc = listen(sfd, 128);
    if (lrc != 0) {
        close(sfd);
        fail(TCPfail);
    }

    *fd = sfd;
    done;
}
