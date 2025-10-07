#ifndef ABC_POLL2_H
#define ABC_POLL2_H
#include <poll.h>
#include <time.h>

#include "abc/OK.h"

typedef struct poller poller;

typedef short (*pollcb)(struct poller* p);

typedef struct poller {
    struct pollfd fd;  // fd.fd is -1 for timers
    u64 timeout;
    pollcb callback;
    void* payload;
} poller;

con ok64 POLnone = 0x19615cb3ca9;

extern int POLmax;

#define POLL_DONE 0x8000

ok64 POLinit();
ok64 POLfree();

b8 POLany();

poller* POLfind(int fd);
ok64 POLtrack(poller* p);
ok64 POLcancel(int fd);

ok64 POLloop(u64 ns);

#define POLnanops 1000000000UL
#define POLnever u64max

fun u64 u64timespec(struct timespec ts) {
    return ((u64)ts.tv_sec * POLnanops) + ts.tv_nsec;
}

fun u64 POLnow() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return u64timespec(ts);
}

#endif
