#include "PAGE.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

// Simple ensure callback: just marks pages loaded
ok64 testEnsure(pagep p, b8 rw, u64 pos, size_t len) {
    // For tests, we just mark pages as loaded for both read and write
    return PAGEMarkLoaded(p, pos, len);
}

ok64 PAGEtestBasic() {
    sane(1);

    // Init registry
    call(PAGEInit, 4);

    // Create paged buffer
    pagep p = NULL;
    call(PAGECreate, &p, PAGESIZE * 4, testEnsure, NULL);

    // PAGETracked should work
    test(PAGETracked(p->buf) == YES, PAGEFAIL);

    // PAGEFind should work
    test(PAGEFind(p->buf) == p, PAGEFAIL);

    // Not present initially
    test(PAGEPresent(p, 0, 100) == NO, PAGEFAIL);

    // Ensure page 0
    call(PAGEEnsure, p, 0, 100);
    test(PAGEPresent(p, 0, 100) == YES, PAGEFAIL);
    test(PAGEIdxRead(p, 0) == PAGE_LOADED, PAGEFAIL);

    // Ensure spanning pages 1-2
    call(PAGEEnsure, p, PAGESIZE + 500, PAGESIZE * 2);
    test(PAGEIdxRead(p, 1) == PAGE_LOADED, PAGEFAIL);
    test(PAGEIdxRead(p, 2) == PAGE_LOADED, PAGEFAIL);

    // Mark dirty
    call(PAGEDirty, p, 0, 100);
    test(PAGEIdxWrite(p, 0) == PAGE_DIRTY, PAGEFAIL);

    // Clean up
    call(PAGEClose, p);

    done;
}

ok64 PAGEtestFlush() {
    sane(1);

    pagep p = NULL;
    call(PAGECreate, &p, PAGESIZE * 4, testEnsure, NULL);

    // Mark some pages dirty (first mark them loaded)
    PAGEIdxSetRead(p, 0, PAGE_LOADED);
    PAGEIdxSetRead(p, 2, PAGE_LOADED);
    PAGEIdxSetWrite(p, 0, PAGE_DIRTY);
    PAGEIdxSetWrite(p, 2, PAGE_DIRTY);

    test(PAGEIdxWrite(p, 0) == PAGE_DIRTY, PAGEFAIL);
    test(PAGEIdxWrite(p, 2) == PAGE_DIRTY, PAGEFAIL);

    // Flush
    call(PAGEFlush, p);

    // Should be clean now
    test(PAGEIdxWrite(p, 0) == PAGE_CLEAN, PAGEFAIL);
    test(PAGEIdxWrite(p, 2) == PAGE_CLEAN, PAGEFAIL);

    call(PAGEClose, p);

    done;
}

ok64 PAGEtestCoalesce() {
    sane(1);

    // Test that PAGEFlush coalesces consecutive dirty pages
    pagep p = NULL;
    call(PAGECreate, &p, PAGESIZE * 8, testEnsure, NULL);

    // Mark pages 1,2,3 and 5,6 dirty (two ranges)
    for (int i = 1; i <= 3; i++) {
        PAGEIdxSetRead(p, i, PAGE_LOADED);
        PAGEIdxSetWrite(p, i, PAGE_DIRTY);
    }
    for (int i = 5; i <= 6; i++) {
        PAGEIdxSetRead(p, i, PAGE_LOADED);
        PAGEIdxSetWrite(p, i, PAGE_DIRTY);
    }

    // Flush should coalesce into 2 ranges
    call(PAGEFlush, p);

    // All should be clean now
    for (int i = 1; i <= 3; i++) {
        test(PAGEIdxWrite(p, i) == PAGE_CLEAN, PAGEFAIL);
    }
    for (int i = 5; i <= 6; i++) {
        test(PAGEIdxWrite(p, i) == PAGE_CLEAN, PAGEFAIL);
    }

    call(PAGEClose, p);

    done;
}

// Streaming test: send 1GB of incrementing u64s over localhost
#include <sys/socket.h>
#include <sys/wait.h>

#define STREAM_BUF_SIZE (1 * MB)
#define STREAM_TOTAL (1 * GB)
#define STREAM_COUNT (STREAM_TOTAL / sizeof(u64))

ok64 PAGEtestStream() {
    sane(1);

    int sv[2];
    test(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, PAGEFAIL);

    pid_t pid = fork();
    test(pid >= 0, PAGEFAIL);

    if (pid == 0) {
        // Child: sender
        close(sv[1]);
        page sender = {};
        if (u8bMap(sender.buf, STREAM_BUF_SIZE) != OK) _exit(1);
        sender.ensure = PAGEStreamFd;
        sender.ctx = (void *)(intptr_t)sv[0];

        for (u64 seq = 0; seq < STREAM_COUNT; seq++) {
            PAGEEnsureIdle(&sender, sizeof(u64));
            u8sp idle = u8bIdle(sender.buf);
            *(u64 *)(*idle) = seq;
            *idle += sizeof(u64);
        }
        PAGEEnsureIdle(&sender, STREAM_BUF_SIZE);

        close(sv[0]);
        u8bUnMap(sender.buf);
        _exit(0);
    }

    // Parent: receiver
    close(sv[0]);
    page receiver = {};
    call(u8bMap, receiver.buf, STREAM_BUF_SIZE);
    receiver.ensure = PAGEStreamFd;
    receiver.ctx = (void *)(intptr_t)sv[1];

    u64 recv_count = 0;
    while (recv_count < STREAM_COUNT) {
        ok64 o = PAGEEnsureData(&receiver, sizeof(u64));
        if (o == END) break;
        test(o == OK, PAGEFAIL);
        u8csp data = u8bDataC(receiver.buf);
        test(*(u64 *)(*data) == recv_count, PAGEFAIL);
        recv_count++;
        u8bUsed(receiver.buf, sizeof(u64));
    }

    test(recv_count == STREAM_COUNT, PAGEFAIL);

    int status;
    waitpid(pid, &status, 0);
    test(WIFEXITED(status) && WEXITSTATUS(status) == 0, PAGEFAIL);

    close(sv[1]);
    u8bUnMap(receiver.buf);

    done;
}

ok64 PAGEtestEnsure() {
    sane(1);

    pagep p = NULL;
    call(PAGECreate, &p, PAGESIZE * 4, testEnsure, NULL);

    // u8bEnsure1: single page, not loaded
    test(PAGEIdxRead(p, 0) == PAGE_ABSENT, PAGEFAIL);
    call(u8bEnsure1, p->buf, 100);
    test(PAGEIdxRead(p, 0) == PAGE_LOADED, PAGEFAIL);

    // u8bEnsure1: already loaded - fast path
    call(u8bEnsure1, p->buf, 200);
    test(PAGEIdxRead(p, 0) == PAGE_LOADED, PAGEFAIL);

    // u8bEnsure2: two pages, page 1 not loaded
    test(PAGEIdxRead(p, 1) == PAGE_ABSENT, PAGEFAIL);
    call(u8bEnsure2, p->buf, PAGESIZE - 100);  // crosses into page 1
    test(PAGEIdxRead(p, 1) == PAGE_LOADED, PAGEFAIL);

    // u8bEnsure2: both pages loaded - fast path
    call(u8bEnsure2, p->buf, PAGESIZE - 50);
    test(PAGEIdxRead(p, 0) == PAGE_LOADED, PAGEFAIL);
    test(PAGEIdxRead(p, 1) == PAGE_LOADED, PAGEFAIL);

    // u8bEnsure: multi-page range
    test(PAGEIdxRead(p, 2) == PAGE_ABSENT, PAGEFAIL);
    test(PAGEIdxRead(p, 3) == PAGE_ABSENT, PAGEFAIL);
    call(u8bEnsure, p->buf, PAGESIZE * 2, PAGESIZE * 2);
    test(PAGEIdxRead(p, 2) == PAGE_LOADED, PAGEFAIL);
    test(PAGEIdxRead(p, 3) == PAGE_LOADED, PAGEFAIL);

    // Non-paged buffer: u8bEnsure should be no-op
    a_pad(u8, plain, 256);
    call(u8bEnsure1, plain, 0);
    call(u8bEnsure2, plain, 0);
    call(u8bEnsure, plain, 0, 100);

    call(PAGEClose, p);

    done;
}

ok64 PAGEtest() {
    sane(1);
    call(PAGEtestBasic);
    call(PAGEtestFlush);
    call(PAGEtestCoalesce);
    call(PAGEtestEnsure);
    call(PAGEtestStream);
    done;
}

TEST(PAGEtest);
