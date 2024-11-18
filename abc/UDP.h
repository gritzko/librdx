
#include "NET.h"
#include "PRO.h"

con ok64 UDPfail = 0x30b65a9935e;
#define a$c(name, s, len)          \
    char name[len] = {0};          \
    if (!$empty(s)) {              \
        int l = $len(s);           \
        if (l >= len) l = len - 1; \
        memcpy(name, *s, l);       \
        name[l] = 0;               \
    }

fun pro(UDPbind, int *fd, NETaddr addr) {
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
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */

    s = getaddrinfo($len(host) > 1 ? (const char *)*host : nil,
                    $len(port) > 1 ? (const char *)*port : nil, &hints,
                    &result);
    if (s != 0) {
        trace("getaddrinfo: %s\n", gai_strerror(s));
        return UDPfail;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;

        int rc = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (rc != 0) {
            trace("getadd: %s\n", gai_strerror(s));
            if (result) freeaddrinfo(result);
            return UDPfail;
        }
        if (rc == 0) break;

        close(sfd);
    }

    freeaddrinfo(result);

    test(sfd != -1, UDPfail);

    *fd = sfd;
    done;
}

fun ok64 UDPconnect(int *fd, NETaddr addr) {
    int sfd, s;
    size_t len;
    ssize_t nread;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */

    u8$c host = NEThost(addr);
    u8$ port = NETport(addr);
    if ($len(host) == 0 || *$last(host) != 0) return NETbadaddr;
    if ($len(port) > 0 && *$last(port) != 0) return NETbadaddr;

    s = getaddrinfo((const char *)*host,
                    $len(port) > 1 ? (const char *)*port : nil, &hints,
                    &result);
    if (s != 0) {
        trace("getaddrinfo: %s\n", gai_strerror(s));
        return UDPfail;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; /* Success */

        close(sfd);
    }

    freeaddrinfo(result); /* No longer needed */

    if (rp == NULL) return UDPfail;
    *fd = sfd;
    return OK;
}

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
