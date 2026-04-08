#include "LESS.h"

#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/HUNK.h"

// --- Producer-side staging state ---
Bu8      less_arena = {};
LESShunk less_hunks[LESS_MAX_HUNKS];
u8bp     less_maps[LESS_MAX_MAPS];
Bu32     less_toks[LESS_MAX_MAPS];
u32      less_nhunks = 0;
u32      less_nmaps  = 0;

int          spot_out_fd = -1;
spot_emit_fn spot_emit   = NULL;

ok64 LESSArenaInit(void) {
    less_nhunks = 0;
    less_nmaps  = 0;
    memset(less_hunks, 0, sizeof(less_hunks));
    memset(less_maps,  0, sizeof(less_maps));
    memset(less_toks,  0, sizeof(less_toks));
    if (less_arena[0] != NULL) {
        ((u8 **)less_arena)[2] = less_arena[1];  // reset idle to start
        return OK;
    }
    return u8bMap(less_arena, LESS_ARENA_SIZE);
}

void LESSArenaCleanup(void) {
    for (u32 i = 0; i < less_nmaps; i++) {
        if (less_toks[i][0] != NULL) u32bUnMap(less_toks[i]);
        if (less_maps[i] != NULL)    FILEUnMap(less_maps[i]);
    }
    less_nhunks = 0;
    less_nmaps  = 0;
}

u8p LESSArenaWrite(void const *data, size_t len) {
    if (u8bIdleLen(less_arena) < len) return NULL;
    u8p p = u8bIdleHead(less_arena);
    memcpy(p, data, len);
    u8bFed(less_arena, len);
    return p;
}

ok64 LESSArenaAlloc(u8s out, size_t len) {
    if (u8bIdleLen(less_arena) < len) return FAILSANITY;
    $mv(out, u8bIdle(less_arena));
    out[1] = out[0] + len;
    u8sZero(out);
    u8bFed(less_arena, len);
    return OK;
}

void LESSDefer(u8bp mapped, Bu32 toks) {
    if (less_nmaps >= LESS_MAX_MAPS) return;
    less_maps[less_nmaps] = mapped;
    memcpy(less_toks[less_nmaps], toks, sizeof(Bu32));
    less_nmaps++;
}

// Serialize the just-built hunk via spot_emit, write to spot_out_fd,
// rewind the staging arena so the slot is reusable.
void LESSHunkEmit(void) {
    if (spot_emit == NULL || spot_out_fd < 0) return;
    LESShunk *hk = &less_hunks[less_nhunks];

    range64 mark;
    Bu8mark(less_arena, &mark);
    u8cp start = u8bIdleHead(less_arena);
    if (spot_emit(u8bIdle(less_arena), hk) != OK) {
        Bu8rewind(less_arena, mark);
        return;
    }

    u8cs ser = {start, u8bIdleHead(less_arena)};
    while (!$empty(ser)) {
        ssize_t w = write(spot_out_fd, ser[0], $len(ser));
        if (w <= 0) break;
        u8csFed(ser, (size_t)w);
    }

    // Rewind: scratch space (title, toks, hili, serialized bytes) is reused.
    Bu8rewind(less_arena, mark);
}

ok64 LESSRun(LESShunk const *hunks, u32 nhunks) {
    sane(1);
    (void)hunks;
    (void)nhunks;
    LESSArenaCleanup();
    done;
}
