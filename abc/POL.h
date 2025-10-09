#ifndef ABC_POLL2_H
#define ABC_POLL2_H
#include <poll.h>
#include <time.h>

#include "abc/OK.h"

typedef struct poller poller;

typedef short (*pollcb)(int fd, struct poller* p);

typedef int (*timercb)();

typedef struct poller {
    pollcb callback;
    void* payload;
    u64 deadline;  // event timeout, ns
    u32 timeout_ms;
    u16 events, revents;
} poller;

con ok64 POLnone = 0x19615cb3ca9;

ok64 POLInit(int max_fd);
ok64 POLFree();

b8 POLany();

// track events on a file descriptor
ok64 POLTrackEvents(int fd, poller p);
// track additional events on a descriptor (prior POLTrack is mandatory)
ok64 POLAddEvents(int fd, short events);
// stop tracking events on a file descriptor
ok64 POLIgnoreEvents(int fd);

// assign the timer callback
ok64 POLTrackTime(timercb callback);
// wake the timer earlier than previously set (prior POLTimer is mandatory)
ok64 POLAddTime(int ms);
// cancel the timer completely
fun ok64 POLIgnoreTime() { return POLTrackTime(NULL); }

ok64 POLLoop(u64 ns);
ok64 POLSleep(u64 ns);

#define POLNanosPerSec 1000000000UL
#define POLNever u64max

fun u64 u64timespec(struct timespec ts) {
    return ((u64)ts.tv_sec * POLNanosPerSec) + ts.tv_nsec;
}

fun u64 POLNow() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return u64timespec(ts);
}

#endif
