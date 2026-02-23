#ifndef ABC_NET_H
#define ABC_NET_H
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "INT.h"
#include "URI.h"

con ok64 NETbadaddr = 0x5ce766968968a36;
con ok64 NETnospace = 0x5ce772cf7d259e9;
con ok64 NETnone = 0x1739dcb3ca9;

typedef Bu8 NETaddr;
#define NEThost(a) u8bPast(a)
#define NETraw(a) u8bData(a)
#define NETport(a) u8bData(a)

#define NETmaxhost (256 - 32 - 2)
#define NETmaxserv (32)

#define aNETraw(name) aBpad(u8, name, 128);

#define aNETAddress(name, host, port)         \
    aBpad(u8, name, NETmaxhost + NETmaxserv); \
    {                                         \
        int hl = strlen(host) + 1;            \
        int pl = strlen(port) + 1;            \
        range64 r = {hl, hl + pl};            \
        Bu8rewind(name, r);                   \
        memcpy(name[0], host, hl);            \
        memcpy(name[1], port, pl);            \
    }

fun ok64 NETInfo(NETaddr text, NETaddr raw) {
    char host[NETmaxhost], service[NETmaxserv];
    u8$ addr = NETraw(raw);
    int s = getnameinfo((struct sockaddr*)addr[0], $len(addr), host, NETmaxhost,
                        service, NETmaxserv, NI_NUMERICSERV);
    if (s != 0) {
        return NETbadaddr;
    }
    u64 hl = strlen(host);
    u64 sl = strlen(service);
    range64 range = {hl + 1, hl + sl + 2};
    if (range.till > Blen(text)) return NETnospace;
    Bu8rewind(text, range);
    return OK;
}

fun int NETRandomPort() {
    int ret = 10000;
    ret += (int)(time(NULL) % 10000);
    ret += 10 * (getpid() % 1000);
    return ret;
}

//  Resolve host:port from URIstate to addrinfo
//  Example: parse "tcp://1.2.3.4:8080" with URIutf8Drain first, then call this
ok64 NETResolve(struct addrinfo** result, URIstate const* uri, b8 stream);

fun ok64 NETFreeAddress(struct addrinfo** addr) {
    if (*addr == NULL) return NETnone;
    freeaddrinfo(*addr);
    *addr = NULL;
    return OK;
}

#endif
