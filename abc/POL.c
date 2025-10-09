#include "POL.h"

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

fun poller* POLat(i32 fd) { return fd < 0 ? &POL_TIMER : Batp(POL_FILES, fd); }

fun int fd32cmp(const i32* a, const i32* b) {
    u64 ans = POLat(*a)->deadline;
    u64 bns = POLat(*b)->deadline;
    return ans < bns ? -1 : 1;
}

b8 POLany() { return !Bempty(POL_FILES) || !Bempty(POL_QUEUE); }

ok64 POLInit(int max_fd) {
    Bpolleralloc(POL_FILES, max_fd);
    Bzero(POL_FILES);
    POL_FILES[2] = POL_FILES[3];
    zero(POL_TIMER);
    POL_VEC = malloc(max_fd);
    return OK;
}

ok64 POLFree() {
    Bpollerfree(POL_FILES);
    Bi32free(POL_QUEUE);
    free(POL_VEC);
    return OK;
}

ok64 POLTrackEvents(int fd, poller poller) {
    sane(poller.callback != NULL && poller.timeout_ms != 0);
    u64 now = POLnow();
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
    if (poller.callback != NULL && !preex) call(i32s_feed1, POL_QUEUE, fd);
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
    int ms = timer();
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
    u64 now = POLnow();
    u64 deadline = now + ms * (POLnanops / 1000);
    if (POL_TIMER.deadline < deadline) return OK;
    POL_TIMER.deadline = deadline;
    int ndx = 0;
    while (ndx < Bdatalen(POL_QUEUE) && Bat(POL_QUEUE, ndx) != -1) ndx++;
    HEAPi32UpAtZ(POL_QUEUE, fd32cmp, ndx);
    return OK;
}

// @return OK/POLnone/etc
ok64 POLloop(u64 timens) {
    u64 now = POLnow();
    u64 till = timens == POLnever ? POLnever : now + timens;
    while (now < till && !Bempty(POL_QUEUE)) {
        // deliver timeouts
        poller* at = POLat(**POL_QUEUE);
        while (at->deadline <= now) {
            at->callback(**POL_QUEUE, at);
            at->deadline = now + at->timeout_ms * (POLnanops / 1000);
            HEAPi32DownZ(POL_QUEUE, fd32cmp);
            at = POLat(**POL_QUEUE);
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
            continue;
        }
        poll(POL_VEC, pollscount, (next_timeout - now) / (POLnanops / 1000));
        for (int i = 0; i < pollscount; i++) {
            if (!POL_VEC[i].revents) continue;
            int fd = POL_VEC[i].fd;
            at = Batp(POL_FILES, fd);
            short e = at->callback(fd, at);
            at->deadline = now + at->timeout_ms * (POLnanops / 1000);
        }
        now = POLnow();
    }
    return OK;
}
