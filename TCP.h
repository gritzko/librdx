
#include <sys/socket.h>

#include "NET.h"
#include "PRO.h"

con ok64 TCPfail = 0x30b65a9931d;

fun pro(TCPbind, int *fd, NETaddr addr) {
    sane(fd != nil && Bok(addr));
    int s, sfd;
    socklen_t peer_addrlen;
    struct addrinfo *result, *rp;
    struct sockaddr_storage peer_addr;

    u8$c host = NEThost(addr);
    u8$c port = NETport(addr);
    if ($len(host) > 0 && *$last(host) != 0) return NETbadaddr;
    if ($len(port) > 0 && *$last(port) != 0) return NETbadaddr;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = 0;           /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo($len(host) > 1 ? (const char *)*host : nil,
                    $len(port) > 1 ? (const char *)*port : nil, &hints,
                    &result);
    if (s != 0) {
        trace("getaddrinfo: %s\n", gai_strerror(s));
        return TCPfail;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, 0);
        if (sfd == -1) continue;

        int rc = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (rc != 0) {
            trace("getadd: %s\n", gai_strerror(s));
            if (result) freeaddrinfo(result);
            return TCPfail;
        }
        if (rc == 0) break;

        close(sfd);
    }

    freeaddrinfo(result);

    test(sfd != -1, TCPfail);
    int lrc = listen(sfd, 128);
    if (lrc != 0) {
        close(sfd);
        fail(TCPfail);
    }

    *fd = sfd;
    done;
}

fun ok64 TCPconnect(int *fd, NETaddr addr) {
    int sfd, s;
    size_t len;
    ssize_t nread;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0; /* Any protocol */

    u8$c host = NEThost(addr);
    u8$ port = NETport(addr);
    if ($len(host) > 0 && *$last(host) != 0) return NETbadaddr;
    if ($len(port) > 0 && *$last(port) != 0) return NETbadaddr;

    s = getaddrinfo($len(host) > 1 ? (const char *)*host : nil,
                    $len(port) > 1 ? (const char *)*port : nil, &hints,
                    &result);
    if (s != 0) {
        trace("getaddrinfo: %s\n", gai_strerror(s));
        return TCPfail;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;

        int rc = connect(sfd, rp->ai_addr, rp->ai_addrlen);
        if (rc == 0) break;
        perror("something");

        close(sfd);
    }

    freeaddrinfo(result); /* No longer needed */

    if (rp == NULL) return TCPfail;
    *fd = sfd;
    return OK;
}

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
