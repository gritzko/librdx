#ifndef ABC_POLL2_H
#define ABC_POLL2_H
#include <poll.h>
#include <time.h>

#include "abc/OK.h"

typedef struct poller poller;

typedef short (*pollcb)(int fd, struct poller* p);

typedef u32 (*timercb)(u64 ns);

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

b8 POLAny();

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
#define POLNanosPerMSec (POLNanosPerSec / 1000)
#define POLNanosPerYear (POLNanosPerSec * 60 * 60 * 24 * 365)
#define POLNanosPerMonth (POLNanosPerYear / 12)
#define POLNever u64max

fun u64 u64timespec(struct timespec ts) {
    return ((u64)ts.tv_sec * POLNanosPerSec) + ts.tv_nsec;
}

fun u64 POLNow() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return u64timespec(ts);
}

fun u64 RONNow() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm* now = localtime(&ts.tv_sec);
    u64 t = 0;
    u64 y = now->tm_year - 2000;
    t = t | ((y / 10) << (9 * 6));
    t = t | ((y % 10) << (8 * 6));
    t = t | ((u64)(now->tm_mon) << (7 * 6));
    t = t | ((u64)(now->tm_mday) << (6 * 6));
    t = t | ((u64)(now->tm_hour) << (5 * 6));
    t = t | ((u64)(now->tm_min) << (4 * 6));
    t = t | ((u64)(now->tm_sec) << (3 * 6));
    u64 ns = ts.tv_nsec % POLNanosPerSec;
    t = t | ((u64)(ns >> 2));
    return t;
}

int POLMaxFiles();

#endif
