#include "DIJK.h"
#include "INT.h"
#include "PRO.h"
#include "TEST.h"

// Simple grid graph: nodes are (x,y) encoded as x*H + y + 1
// key=0 is reserved (hash empty sentinel)

#define GRID_W 4
#define GRID_H 4
#define NODE(x, y) ((u64)(x) * GRID_H + (y) + 1)

ok64 grid_next(kv64 from, kv64s nexts, void *ctx) {
    (void)ctx;
    u64 id = from.key - 1;
    u64 x = id / GRID_H;
    u64 y = id % GRID_H;
    if (x + 1 < GRID_W) {
        kv64 r = {NODE(x + 1, y), 1};
        kv64sFeedP(nexts, &r);
    }
    if (y + 1 < GRID_H) {
        kv64 d = {NODE(x, y + 1), 1};
        kv64sFeedP(nexts, &d);
    }
    if (x > 0) {
        kv64 l = {NODE(x - 1, y), 1};
        kv64sFeedP(nexts, &l);
    }
    if (y > 0) {
        kv64 u = {NODE(x, y - 1), 1};
        kv64sFeedP(nexts, &u);
    }
    return OK;
}

#define DIJK_NEXT grid_next
#define X(M, name) M##grid##name
#include "DIJKx.h"
#undef X
#undef DIJK_NEXT

ok64 DIJKtest_grid() {
    sane(1);
    aBpad(kv64, heap, 256);
    a_pad0(kv64, dist, 256);

    u64 cost = 0;
    call(DIJKgridRun, &cost, NODE(0, 0), NODE(3, 3),
         heap, dist_idle, NULL);
    testeq(cost, 6);  // Manhattan distance 3+3
    done;
}

ok64 DIJKtest_unreachable() {
    sane(1);
    aBpad(kv64, heap, 256);
    a_pad0(kv64, dist, 256);

    u64 cost = 0;
    ok64 o = DIJKgridRun(&cost, NODE(0, 0), 99, heap, dist_idle, NULL);
    testeq(o, DIJKNOPATH);
    done;
}

// Weighted graph: diamond shape
//   1 --1--> 2 --1--> 4
//   1 --2--> 3 --1--> 4
// Shortest 1->4 is 2 (via node 2)

ok64 diamond_next(kv64 from, kv64s nexts, void *ctx) {
    (void)ctx;
    switch (from.key) {
        case 1: {
            kv64 a = {2, 1}, b = {3, 2};
            kv64sFeedP(nexts, &a);
            kv64sFeedP(nexts, &b);
            break;
        }
        case 2: case 3: {
            kv64 a = {4, 1};
            kv64sFeedP(nexts, &a);
            break;
        }
        default: break;
    }
    return OK;
}

#define DIJK_NEXT diamond_next
#define X(M, name) M##diamond##name
#include "DIJKx.h"
#undef X
#undef DIJK_NEXT

ok64 DIJKtest_weighted() {
    sane(1);
    aBpad(kv64, heap, 64);
    a_pad0(kv64, dist, 64);

    u64 cost = 0;
    call(DIJKdiamondRun, &cost, 1, 4, heap, dist_idle, NULL);
    testeq(cost, 2);
    done;
}

ok64 DIJKtest() {
    sane(1);
    call(DIJKtest_grid);
    call(DIJKtest_weighted);
    call(DIJKtest_unreachable);
    done;
}

TEST(DIJKtest)
