#include "POL.h"

#include <time.h>

#include "B.h"
#include "INT.h"
#include "OK.h"
#include "PRO.h"
#include "RON.h"

fun int pollercmp(poller const* a, poller const* b) {
    if (a->callback == NULL || b->callback == NULL) {
        return a->callback == NULL ? 1 : -1;
    }
    return u64cmp(&a->deadline, &b->deadline);
}

fun b8 pollerZ(poller const* a, poller const* b) {
    if (a->callback == NULL) return NO;
    if (b->callback == NULL) return YES;
    return a->deadline < b->deadline;
}

#define X(M, name) M##poller##name
#include "Bx.h"
#undef X

#define X(M, name) M##poller##name
#include "HEAPx.h"
#undef X

thread_local Bpoller POL_QUEUE = {};
thread_local b8 POL_STOP = NO;
thread_local int POL_MAXFD = 0;

int POLMaxFiles() { return POL_MAXFD; }

b8 POLAny() { return !pollerbEmpty(POL_QUEUE); }

ok64 POLInit(int max_fd) {
    pollerbAllocate(POL_QUEUE, max_fd);
    POL_MAXFD = max_fd;
    return OK;
}

ok64 POLFree() {
    pollerbFree(POL_QUEUE);
    POL_STOP = NO;
    POL_MAXFD = 0;
    return OK;
}

ok64 POLStop() {
    POL_STOP = YES;
    return OK;
}

// Find poller by tofd in heap, returns index or -1
fun int POLFind(int tofd) {
    poller** data = pollerbData(POL_QUEUE);
    size_t len = $len(data);
    for (size_t i = 0; i < len; i++) {
        if ((*data)[i].tofd == tofd) return (int)i;
    }
    return -1;
}

ok64 POLTrackEvents(int fd, poller p) {
    sane(p.callback != NULL);  // tofd can be 0 for stdin
    u64 now = POLNow();
    p.deadline = now + (p.tofd < 0 ? -p.tofd : 1000) * POLNanosPerMSec;

    // Check if already tracked
    int idx = POLFind(p.tofd);
    if (idx >= 0) {
        // Update existing entry
        poller** data = pollerbData(POL_QUEUE);
        (*data)[idx] = p;
        pollersUpAtZ(data, idx, pollerZ);
        pollersDownAtZ(data, idx, pollerZ);
    } else {
        // Add new entry
        call(HEAPpollerPushZ, POL_QUEUE, &p, pollerZ);
    }
    done;
}

ok64 POLAddEvents(int fd, short events) {
    int idx = POLFind(fd);
    if (idx < 0) return POLNONE;
    poller** data = pollerbData(POL_QUEUE);
    (*data)[idx].events |= events;
    return OK;
}

ok64 POLIgnoreEvents(int fd) {
    int idx = POLFind(fd);
    if (idx < 0) return POLNONE;
    return HEAPpollerEjectAtZ(POL_QUEUE, idx, pollerZ);
}

short POLTimer(int fd, struct poller* p) {
    if (p->callback == NULL || p->payload == NULL) return 0;
    timercb timer = p->payload;
    u32 ms = timer(p->deadline);
    // If timer returns >= 1 hour, treat as "never" - remove from queue
    if (ms >= 3600000) {
        p->callback = NULL;
        return 0;
    }
    p->tofd = -(int)ms;  // negative for timer period
    return 0;
}

ok64 POLTrackTime(timercb callback) {
    poller t = {
        .callback = &POLTimer,
        .payload = callback,
        .tofd = -1,  // -1ms initial
    };
    return POLTrackEvents(-1, t);
}

// Find timer by callback payload (since timers use POLTimer as callback)
fun int POLFindTimer(void* payload) {
    poller** data = pollerbData(POL_QUEUE);
    size_t len = $len(data);
    for (size_t i = 0; i < len; i++) {
        if ((*data)[i].callback == POLTimer && (*data)[i].payload == payload)
            return (int)i;
    }
    return -1;
}

ok64 POLAddTime(int ms) {
    // For legacy single-timer API, find any timer
    poller** data = pollerbData(POL_QUEUE);
    size_t len = $len(data);
    for (size_t i = 0; i < len; i++) {
        if ((*data)[i].tofd >= 0) continue;  // skip file pollers
        poller* t = &(*data)[i];
        t->tofd = -ms;
        u64 now = POLNow();
        u64 deadline = now + ms * POLNanosPerMSec;
        if (t->deadline < deadline) return OK;
        t->deadline = deadline;
        pollersUpAtZ(data, i, pollerZ);
        return OK;
    }
    return POLNONE;
}

ok64 POLIgnoreTime() {
    // Remove first timer found
    poller** data = pollerbData(POL_QUEUE);
    size_t len = $len(data);
    for (size_t i = 0; i < len; i++) {
        if ((*data)[i].tofd < 0) {
            return HEAPpollerEjectAtZ(POL_QUEUE, i, pollerZ);
        }
    }
    return POLNONE;
}

ok64 POLSleep(u64 ns) {
    struct timespec duration = {.tv_sec = ns / POLNanosPerSec,
                                .tv_nsec = ns % POLNanosPerSec};
    nanosleep(&duration, NULL);
    return OK;
}

ok64 POLLoop(u64 timens) {
    u64 now = POLNow();
    u64 till = timens == POLNever ? POLNever : now + timens;

    // Local poll vector - exactly POL_MAXFD entries
    struct pollfd* vec = calloc(POL_MAXFD, sizeof(struct pollfd));
    if (!vec) return NOROOM;

    while (now < till && !pollerbEmpty(POL_QUEUE) && !POL_STOP) {
        now = POLNow();
        poller** data = pollerbData(POL_QUEUE);

        // Deliver timeouts and remove dead pollers
        while (!$empty(data) && (*data)->deadline <= now) {
            poller* at = *data;
            if (at->callback == NULL) {
                poller dummy;
                HEAPpollerPopZ(&dummy, POL_QUEUE, pollerZ);
                data = pollerbData(POL_QUEUE);
                continue;
            }
            at->deadline = now;
            at->callback(at->tofd, at);
            // Remove if callback cleared itself
            if (at->callback == NULL) {
                poller dummy;
                HEAPpollerPopZ(&dummy, POL_QUEUE, pollerZ);
                data = pollerbData(POL_QUEUE);
                continue;
            }
            int period_ms = at->tofd < 0 ? -at->tofd : 1000;
            at->deadline = now + period_ms * POLNanosPerMSec;
            pollersDownZ(data, pollerZ);
        }

        if ($empty(data)) break;

        // Remove file pollers with no events
        for (size_t i = 0; i < $len(data);) {
            poller* p = *data + i;
            if (p->tofd >= 0 && p->events == 0) {
                HEAPpollerEjectAtZ(POL_QUEUE, i, pollerZ);
                data = pollerbData(POL_QUEUE);
            } else {
                i++;
            }
        }
        if ($empty(data)) break;

        u64 next_timeout = (*data)->deadline;

        // Build poll vector from file descriptors (positive tofd)
        size_t len = $len(data);
        int pollscount = 0;
        for (size_t i = 0; i < len && pollscount < POL_MAXFD; i++) {
            poller* p = *data + i;
            if (p->tofd < 0 || p->events == 0) continue;
            vec[pollscount++] = (struct pollfd){.fd = p->tofd, .events = p->events};
        }

        if (pollscount == 0) {
            // No file descriptors, just timers
            u64 sleep = next_timeout - now;
            if (sleep > POLNanosPerMonth) break;
            POLSleep(sleep);
            continue;
        }

        int pollms = (next_timeout - now) / POLNanosPerMSec;
        poll(vec, pollscount, pollms);

        // Process poll results
        for (int i = 0; i < pollscount; i++) {
            if (!vec[i].revents) continue;
            int fd = vec[i].fd;
            int idx = POLFind(fd);
            if (idx < 0) continue;
            data = pollerbData(POL_QUEUE);
            poller* at = *data + idx;
            if (at->callback == NULL) continue;
            at->revents = vec[i].revents;
            short events = at->callback(fd, at);
            // Remove if callback cleared itself or no events requested
            if (at->callback == NULL || (at->tofd >= 0 && events == 0)) {
                HEAPpollerEjectAtZ(POL_QUEUE, idx, pollerZ);
            } else {
                at->events = events;
                int period_ms = at->tofd < 0 ? -at->tofd : 1000;
                at->deadline = now + period_ms * POLNanosPerMSec;
            }
        }

    }
    free(vec);
    return OK;
}

ron60 RONNow() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm* now = localtime(&ts.tv_sec);
    ron60 t = 0;
    RONOfTime(&t, now);
    u64 ns = ts.tv_nsec % POLNanosPerSec;
    t |= (ns >> 12);
    return t;
}
