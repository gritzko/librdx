#include "NET.h"

#include <sys/socket.h>

#include "OK.h"
#include "PRO.h"

ok64 NETResolve(struct addrinfo **result, URIstate const *uri, b8 tcp) {
    sane(uri != NULL && result != NULL && *result == NULL);

    // Extract host and port from URIstate
    a_pad(u8, host, 64);
    a_pad(u8, port, 16);
    call(u8sFeed, host_idle, uri->host);
    if ($len(uri->port)) {
        call(u8sFeed, port_idle, uri->port);
    } else if ($len(uri->scheme)) {
        // Use scheme as port (e.g., "http" -> 80 via /etc/services)
        call(u8sFeed, port_idle, uri->scheme);
    } else {
        fail(BADARG);
    }
    call(u8sFeed1, host_idle, 0);  // Null terminate for getaddrinfo
    call(u8sFeed1, port_idle, 0);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = tcp ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;  // For wildcard IP address

    int s = getaddrinfo((char *)*host_data, (char *)*port_data, &hints, result);
    if (s != 0) {
        trace("getaddrinfo: %s\n", gai_strerror(s));
        return NETBADADDR;
    }
    done;
}
