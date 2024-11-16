#ifndef ABC_NET_H
#define ABC_NET_H
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "INT.h"

con ok64 NETbadaddr = 0xda8a25a2599d397;
con ok64 NETnospace = 0xa67974df3c9d397;

typedef Bu8 NETaddr;
#define NEThost(a) Bu8past(a)
#define NETraw(a) Bu8data(a)
#define NETport(a) Bu8data(a)

#define aNETraw(name) aBpad(u8, name, 128);

#define aNETaddr(name, host, port)            \
    aBpad(u8, name, NI_MAXHOST + NI_MAXSERV); \
    {                                         \
        int hl = strlen(host) + 1;            \
        int pl = strlen(port) + 1;            \
        range64 r = {hl, hl + pl};            \
        Bu8rewind(name, r);                   \
        memcpy(name[0], host, hl);            \
        memcpy(name[1], port, pl);            \
    }

fun ok64 NETinfo(NETaddr text, NETaddr raw) {
    char host[NI_MAXHOST], service[NI_MAXSERV];
    u8$ addr = NETraw(raw);
    int s = getnameinfo((struct sockaddr *)addr[0], $len(addr), host,
                        NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);
    if (s != 0) {
        trace("getnameinfo: %s\n", gai_strerror(s));
        return NETbadaddr;
    }
    int hl = strlen(host);
    int sl = strlen(service);
    range64 range = {hl + 1, hl + sl + 2};
    if (range.till > Blen(text)) return NETnospace;
    Bu8rewind(text, range);
    return OK;
}
#endif
