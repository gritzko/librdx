#include "GRAF.h"

#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"

// --- Producer-side staging state (legacy globals) ---
Bu8          graf_arena   = {};
int          graf_out_fd  = -1;
graf_emit_fn graf_emit    = NULL;

// --- GRAFOpen / GRAFClose ---

ok64 GRAFOpen(graf *g, u8cs home, b8 rw) {
    sane(g);
    memset(g, 0, sizeof(*g));
    g->out_fd = -1;

    // Resolve .dogs/graf/ path
    a_path(dir);
    if ($empty(home)) {
        call(HOMEFindDogs, dir);
    } else {
        call(PATHu8bFeed, dir, home);
    }
    a_cstr(rel, "/graf");
    call(u8bFeed, dir, rel);
    call(PATHu8gTerm, PATHu8gIn(dir));

    size_t dlen = u8bDataLen(dir);
    if (dlen >= sizeof(g->dir)) dlen = sizeof(g->dir) - 1;
    memcpy(g->dir, u8bDataHead(dir), dlen);
    g->dir[dlen] = 0;

    // Create directory only in rw mode.
    if (rw) call(FILEMakeDirP, PATHu8cgIn(dir));

    // Open DAG index
    u8cs dagdir = {(u8cp)g->dir, (u8cp)g->dir + dlen};
    call(dag_stack_open, &g->idx, dagdir);

    // Map arena
    call(u8bMap, g->arena, GRAF_ARENA_SIZE);

    done;
}

// --- Update: feed a single git object into graf's DAG index ---
//
// TODO: when obj_type == KEEP_OBJ_COMMIT, thread the commit into the
// DAG; for trees, record blob→path mapping.  For now this is a stub
// so the DOG 4-fn surface is consistent across all dogs.
ok64 GRAFUpdate(graf *g, u8 obj_type, u8cs blob, u8csc path) {
    sane(g);
    (void)obj_type; (void)blob; (void)path;
    done;
}

ok64 GRAFClose(graf *g) {
    sane(g);
    dag_stack_close(&g->idx);
    if (g->arena[0]) u8bUnMap(g->arena);
    g->out_fd = -1;
    g->emit = NULL;
    done;
}

ok64 GRAFArenaInit(void) {
    if (graf_arena[0] != NULL) {
        ((u8 **)graf_arena)[2] = graf_arena[1];  // reset idle to start
        return OK;
    }
    return u8bMap(graf_arena, GRAF_ARENA_SIZE);
}

void GRAFArenaCleanup(void) {
    if (graf_arena[0] != NULL)
        ((u8 **)graf_arena)[2] = graf_arena[1];
}

ok64 GRAFHunkEmit(hunk const *hk, void *ctx) {
    sane(hk != NULL);
    (void)ctx;
    if (graf_emit == NULL || graf_out_fd < 0) return OK;

    // Reuse the trailing portion of graf_arena as TLV scratch.
    range64 mark;
    Bu8mark(graf_arena, &mark);
    u8cp start = u8bIdleHead(graf_arena);
    if (graf_emit(u8bIdle(graf_arena), hk) != OK) {
        Bu8rewind(graf_arena, mark);
        return OK;
    }

    u8cs ser = {start, u8bIdleHead(graf_arena)};
    while (!$empty(ser)) {
        ssize_t w = write(graf_out_fd, ser[0], $len(ser));
        if (w <= 0) break;
        u8csFed(ser, (size_t)w);
    }

    Bu8rewind(graf_arena, mark);
    return OK;
}
