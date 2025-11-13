#include "POL.h"

#include <threads.h>
#include <time.h>

#include "B.h"
#include "INT.h"
#include "OK.h"
#include "PRO.h"

fun int pollercmp(poller const* a, poller const* b) {
    if (a->callback == NULL || b->callback == NULL) {
        return a->callback == NULL ? 1 : -1;
    }
    return u64cmp(&a->deadline, &b->deadline);
}

#define X(M, name) M##poller##name
#include "Bx.h"
#undef X

#define X(M, name) M##i32##name
#include "HEAPx.h"
#undef X

thread_local Bpoller POL_FILES = {};
thread_local poller POL_TIMER = {};
thread_local Bi32 POL_QUEUE = {};
thread_local struct pollfd* POL_VEC;

const u64 POLTimeMask = (1UL << 48) - 1;

fun poller* POLAt(i32 fd) { return fd < 0 ? &POL_TIMER : Batp(POL_FILES, fd); }

int POLMaxFiles() { return Blen(POL_FILES); }

fun int fd32cmp(const i32* a, const i32* b) {
    u64 ans = POLAt(*a)->deadline;
    u64 bns = POLAt(*b)->deadline;
    return ans < bns ? -1 : 1;
}

b8 POLAny() { return !Bempty(POL_FILES) || !Bempty(POL_QUEUE); }

ok64 POLInit(int max_fd) {
    pollerbAllocate(POL_FILES, max_fd);
    Bzero(POL_FILES);
    POL_FILES[2] = POL_FILES[3];
    zero(POL_TIMER);
    POL_VEC = malloc(max_fd);
    i32bAllocate(POL_QUEUE, max_fd);
    return OK;
}

ok64 POLFree() {
    pollerbFree(POL_FILES);
    i32bFree(POL_QUEUE);
    free(POL_VEC);
    POL_VEC = NULL;
    return OK;
}

ok64 POLTrackEvents(int fd, poller poller) {
    sane(poller.callback != NULL && poller.timeout_ms != 0);
    u64 now = POLNow();
    b8 preex = 1;
    if (fd < 0) {
        preex = POL_TIMER.callback != NULL;
        POL_TIMER = poller;
        fd = -1;
    } else if (fd < Blen(POL_FILES)) {
        preex = Bat(POL_FILES, fd).callback != NULL;
        Bat(POL_FILES, fd) = poller;
    } else {
        return POLnone;
    }
    if (poller.callback != NULL && !preex) {
        call(HEAPi32Push1, POL_QUEUE, fd);
    }
    done;
}

ok64 POLAddEvents(int fd, short events) {
    if (fd >= Blen(POL_FILES) || Bat(POL_FILES, fd).callback == NULL)
        return POLnone;
    Bat(POL_FILES, fd).events |= events;
    return OK;
}

ok64 POLIgnoreEvents(int fd) {
    sane(fd < 0 || fd >= Blen(POL_FILES));
    zerop(Batp(POL_FILES, fd));
    done;
}

short POLTimer(int fd, struct poller* p) {
    if (p->callback == NULL) return 0;
    timercb timer = p->payload;
    int ms = timer(p->deadline);
    p->timeout_ms = ms;
    return 0;
}

ok64 POLTrackTime(timercb callback) {
    poller t = {
        .callback = &POLTimer,
        .payload = callback,
        .timeout_ms = 1,
    };
    return POLTrackEvents(-1, t);
}

ok64 POLAddTime(int ms) {
    if (POL_TIMER.callback == NULL) return POLnone;
    POL_TIMER.timeout_ms = ms;
    u64 now = POLNow();
    u64 deadline = now + ms * POLNanosPerMSec;
    if (POL_TIMER.deadline < deadline) return OK;
    POL_TIMER.deadline = deadline;
    int ndx = 0;
    while (ndx < Bdatalen(POL_QUEUE) && Bat(POL_QUEUE, ndx) != -1) ndx++;
    HEAPi32UpAtZ(POL_QUEUE, fd32cmp, ndx);
    return OK;
}

ok64 POLSleep(u64 ns) {
    struct timespec duration = {.tv_sec = ns / POLNanosPerSec,
                                .tv_nsec = ns % POLNanosPerSec};
    nanosleep(&duration, NULL);
    return OK;
}

// @return OK/POLnone/etc
ok64 POLLoop(u64 timens) {
    u64 now = POLNow();
    u64 till = timens == POLNever ? POLNever : now + timens;
    while (now < till && !Bempty(POL_QUEUE)) {
        now = POLNow();
        // deliver timeouts
        poller* at = POLAt(**POL_QUEUE);
        while (at->deadline <= now) {
            at->deadline = now;
            at->callback(**POL_QUEUE, at);
            at->deadline = now + at->timeout_ms * (POLNanosPerSec / 1000);
            HEAPi32DownZ(POL_QUEUE, fd32cmp);
            at = POLAt(**POL_QUEUE);
        }
        u64 next_timeout = at->deadline;
        int pollscount = 0;
        eatB(poller, j, POL_FILES) {
            if (0 == j->events) continue;
            struct pollfd p = {.fd = j - *POL_FILES, .events = j->events};
            POL_VEC[pollscount++] = p;
        }
        if (pollscount == 0) {
            if (POL_TIMER.callback == NULL) break;
            u64 sleep = next_timeout - now;
            if (sleep > POLNanosPerMonth) break;
            POLSleep(sleep);
            continue;
        }
        int pollms = (next_timeout - now) / (POLNanosPerSec / 1000);
        poll(POL_VEC, pollscount, pollms);
        for (int i = 0; i < pollscount; i++) {
            if (!POL_VEC[i].revents) continue;
            int fd = POL_VEC[i].fd;
            at = Batp(POL_FILES, fd);
            at->revents = POL_VEC[i].revents;
            at->events = at->callback(fd, at);
            at->deadline = now + at->timeout_ms * (POLNanosPerSec / 1000);
        }
    }
    return OK;
}

ron60 RONNow() {
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
