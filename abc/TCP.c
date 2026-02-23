#include "TCP.h"

#include <sys/socket.h>

#include "NET.h"
#include "PRO.h"
#include "S.h"

ok64 TCPConnect(int *fd, u8csc address, b8 nonblocking) {
    sane(fd != NULL && !$empty(address));
    int sfd, s;
    struct addrinfo *result = NULL, *rp;

    URIstate uri = {};
    a_dup(u8c, addr, address);
    call(URIutf8Drain, addr, &uri);
    call(NETResolve, &result, &uri, YES);

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;

        int rc = connect(sfd, rp->ai_addr, rp->ai_addrlen);
        if (rc == 0) break;
        perror("something");

        close(sfd);
    }

    NETFreeAddress(&result);

    if (rp == NULL) return TCPFAIL;
    *fd = sfd;
    return OK;
}

ok64 TCPListen(int *fd, u8cs addr) {
    sane(fd != NULL && !$empty(addr));
    int s, sfd;
    struct addrinfo *result = NULL, *rp;

    URIstate uri = {};
    call(URIutf8Drain, addr, &uri);
    call(NETResolve, &result, &uri, YES);

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, 0);
        if (sfd == -1) continue;

        int rc = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (rc != 0) {
            trace("bind failed: %s\n", strerror(errno));
            if (result) NETFreeAddress(&result);
            return TCPFAIL;
        }
        if (rc == 0) break;

        close(sfd);
    }

    NETFreeAddress(&result);

    test(sfd != -1, TCPFAIL);
    int lrc = listen(sfd, 128);
    if (lrc != 0) {
        close(sfd);
        fail(TCPFAIL);
    }

    *fd = sfd;
    done;
}
