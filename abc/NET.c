#include "NET.h"

#include <sys/socket.h>

#include "OK.h"
#include "PRO.h"
#include "URI.h"

ok64 NETParseAddress(struct addrinfo **result, u8cs address, b8 tcp) {
    sane(!$empty(address) && result != NULL && *result == NULL);
    socklen_t peer_addrlen;
    struct addrinfo *rp;
    struct sockaddr_storage peer_addr;

    URIstate state = {};
    $mv(state.text, address);
    call(URIlexer, &state);

    a_pad(u8, host, 64);
    a_pad(u8, port, 16);
    call($u8feed, host_idle, state.host);
    if ($len(state.port)) {
        call($u8feed, port_idle, state.port);
    } else if ($len(state.scheme)) {
        call($u8feed, port_idle, state.scheme);
    } else {
        fail(badarg);
    }
    call(u8sFeed1, host_idle, 0);
    call(u8sFeed1, port_idle, 0);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = tcp ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */

    int s = getaddrinfo((char *)*host_data, (char *)*port_data, &hints, result);
    if (s != 0) {
        trace("getaddrinfo: %s\n", gai_strerror(s));
        return NETbadaddr;
    }
    done;
}
