#include "POL.h"

#include <time.h>

#include "INT.h"
#include "PRO.h"

fun int pollercmp(poller const* a, poller const* b) {
    if (a->callback == NULL || b->callback == NULL) {
        return a->callback == NULL ? 1 : -1;
    }
    return u64cmp(&a->timeout, &b->timeout);
}

#define X(M, name) M##poller##name
#include "Bx.h"
#include "HEAPx.h"
#undef X

Bpoller POL = {};

int POLmax = 1024;

int POLtimer = -1;

b8 POLany() { return Bdatalen(POL) > 0; }

ok64 POLinit() {
    Bpolleralloc(POL, POLmax);
    return OK;
}
ok64 POLfree() {
    Bpollerfree(POL);
    return OK;
}

poller* POLfind(int fd) {
    pollersp ps = Bdata(POL);
    eats(poller, p, ps) {
        if (p->fd.fd == fd) return p;
    }
    return NULL;
}

ok64 POLtrack(poller* poller) {
    sane(poller != nil);
    u64 to = POLnanops;
    pollersp ps = $data(POL);
    if (poller < ps[0] || poller >= ps[1]) {
        call(pollerB_feedp, POL, poller);
        poller = $last(ps);
        HEAPpollerup_at(ps, poller - *ps);
    } else {
        HEAPpollerdown_at(ps, poller - *ps);
    }
    if ($last(ps)->callback == NULL) $term(ps)--;
    done;
}

ok64 POLcancel(int fd) {
    poller* p = POLfind(fd);
    if (p == NULL) return POLnone;
    return POLtrack(p);
}

ok64 POLloop(u64 timens) {
    u64 elapsed = 0;
    u64 start = POLnow();
    pollersp ps = Bdata(POL);
    while (elapsed < timens) {
        struct pollfd polls[POLmax];
        int pollscount = 0;
        eatB(poller, j, POL) polls[pollscount++] = *(struct pollfd*)j;
        // u64 ns = timens - elapsed;  // todo
        u64 ns = POLnanops / 1000;
        // todo timeouts
        poll(polls, pollscount, ns);
        for (int i = 0; i < pollscount; i++) {
            if (!polls[i].revents) continue;
            u64 to = POLnever;
            poller* desc = $atp(ps, i);
            short e = desc->callback(desc);
            HEAPpollerdown_at(ps, desc - *ps);
            if ($last(ps)->callback == NULL) $term(ps)--;
        }
    }
    return OK;
}
