#include "UDP.h"

#include "NET.h"
#include "PRO.h"

ok64 UDPBind(int *fd, u8cs addr) {
    sane(fd != NULL && !$empty(addr));
    int s, sfd;
    struct addrinfo *result = NULL, *rp;

    URIstate uri = {};
    call(URIutf8Drain, addr, &uri);
    call(NETResolve, &result, &uri, NO);

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;

        int rc = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (rc != 0) {
            trace("bind failed: %s\n", strerror(errno));
            if (result) NETFreeAddress(&result);
            return UDPFAIL;
        }
        if (rc == 0) break;

        close(sfd);
    }

    NETFreeAddress(&result);

    test(sfd != -1, UDPFAIL);

    *fd = sfd;
    done;
}

ok64 UDPConnect(int *fd, u8cs addr) {
    sane(fd != NULL && !$empty(addr));
    int sfd, s;
    size_t len;
    ssize_t nread;
    struct addrinfo *result = NULL, *rp;

    URIstate uri = {};
    call(URIutf8Drain, addr, &uri);
    call(NETResolve, &result, &uri, NO);

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;  // Success

        close(sfd);
    }

    NETFreeAddress(&result);

    if (rp == NULL) return UDPFAIL;
    *fd = sfd;
    return OK;
}
