//  WALK: git object graph traversal
//
#include "WALK.h"

#include "DELT.h"
#include "GIT.h"
#include "PACK.h"
#include "SHA1.h"
#include "ZINF.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PRO.h"

// --- Open / Close ---

ok64 WALKOpen(walk *w, u8cs belt_dir) {
    sane(w && $ok(belt_dir));
    memset(w, 0, sizeof(walk));
    w->stack[0] = w->runs;
    w->stack[1] = w->runs;

    // mmap objects.log
    {
        a_pad(u8, lp, 1024);
        call(u8bFeed, lp, belt_dir);
        a_cstr(sub, "/objects.log");
        call(u8bFeed, lp, sub);
        call(PATHu8gTerm, PATHu8gIn(lp));
        call(FILEMapRO, &w->logmap, PATHu8cgIn(lp));
    }
    w->pack = u8bDataHead(w->logmap);
    w->packlen = u8bDataLen(w->logmap);

    // load index runs
    char idx_path[1024];
    snprintf(idx_path, sizeof(idx_path), "%.*s/objects.idx",
             (int)$len(belt_dir), (char *)belt_dir[0]);

    DIR *d = opendir(idx_path);
    if (d) {
        char names[BELT_MAX_LEVELS][64];
        u32 count = 0;
        struct dirent *e;
        while ((e = readdir(d)) != NULL && count < BELT_MAX_LEVELS) {
            size_t nlen = strlen(e->d_name);
            if (nlen < 5 || nlen > 63) continue;
            if (strcmp(e->d_name + nlen - 4, BELT_IDX_EXT) != 0) continue;
            memcpy(names[count], e->d_name, nlen + 1);
            count++;
        }
        closedir(d);

        for (u32 i = 0; i < count; i++) {
            a_pad(u8, fp, 1024);
            {
                a_cstr(ip, idx_path);
                u8bFeed(fp, ip);
                u8bFeed1(fp, '/');
                a_cstr(nm, names[i]);
                u8bFeed(fp, nm);
                PATHu8gTerm(PATHu8gIn(fp));
            }
            if (FILEMapRO(&w->maps[w->nmaps], PATHu8cgIn(fp)) != OK)
                continue;
            belt128cp base =
                (belt128cp)u8bDataHead(w->maps[w->nmaps]);
            u64 len = u8bDataLen(w->maps[w->nmaps]) / sizeof(belt128);
            w->runs[w->nmaps][0] = base;
            w->runs[w->nmaps][1] = base + len;
            w->nmaps++;
            w->stack[1] = w->runs + w->nmaps;
        }
    }

    w->buf1 = malloc(WALK_BUFSZ);
    w->buf2 = malloc(WALK_BUFSZ);
    if (!w->buf1 || !w->buf2) {
        WALKClose(w);
        fail(WALKFAIL);
    }
    done;
}

ok64 WALKClose(walk *w) {
    sane(w);
    free(w->buf2); w->buf2 = NULL;
    free(w->buf1); w->buf1 = NULL;
    for (u32 i = 0; i < w->nmaps; i++)
        FILEUnMap(w->maps[i]);
    w->nmaps = 0;
    if (w->logmap) { FILEUnMap(w->logmap); w->logmap = NULL; }
    done;
}

// --- Object access ---

ok64 WALKGet(walk *w, u64 hashlet, u8g out, u8p out_type) {
    sane(w && u8gOK(out) && out_type);
    belt128cp hit = BELTLookup(w->stack, hashlet);
    if (!hit) return WALKNONE;

    u64 off = BELTOffset(*hit);
    u8 otype = 0;
    u8p content = NULL;
    u64 outsz = 0;
    call(BELTResolve, w->pack, w->packlen, w->stack,
         off, &otype, w->buf1, w->buf2, WALK_BUFSZ,
         &content, &outsz);

    if (outsz > (u64)u8gRestLen(out)) return WALKNOROOM;
    memcpy(out[1], content, outsz);
    out[1] += outsz;
    *out_type = otype;
    done;
}

ok64 WALKGetSha(walk *w, u8cp sha, u8g out, u8p out_type) {
    sane(w && sha && u8gOK(out) && out_type);
    u64 hashlet = 0;
    memcpy(&hashlet, sha, 8);
    hashlet &= ~BELT_TYPE_MASK;
    return WALKGet(w, hashlet, out, out_type);
}

// --- Commit helpers ---

ok64 WALKCommitTree(u8cs commit, u8 tree_sha[20]) {
    sane($ok(commit) && tree_sha);
    u8cs body = {commit[0], commit[1]};
    u8cs field = {}, value = {};
    while (GITu8sDrainCommit(body, field, value) == OK) {
        if ($empty(field)) break;
        if ($len(field) == 4 && memcmp(field[0], "tree", 4) == 0) {
            if ($len(value) < 40) return WALKBADFMT;
            u8s bin = {tree_sha, tree_sha + 20};
            u8cs hex = {value[0], value[0] + 40};
            return HEXu8sDrainSome(bin, hex);
        }
    }
    return WALKBADFMT;
}

// --- BFS scratch for graph walks ---

typedef struct {
    u8  sha[20];
    u32 gen;
} walk_node;

#define WALK_SEEN_BITS 17
#define WALK_SEEN_SIZE (1 << WALK_SEEN_BITS)
#define WALK_SEEN_MASK (WALK_SEEN_SIZE - 1)

// Open-addressing hash set of hashlets
typedef struct {
    u64 *tab;
    u32  count;
} walk_seen;

static ok64 seen_init(walk_seen *s) {
    s->tab = calloc(WALK_SEEN_SIZE, sizeof(u64));
    s->count = 0;
    if (!s->tab) return WALKFAIL;
    return OK;
}

static void seen_free(walk_seen *s) {
    free(s->tab);
    s->tab = NULL;
}

// Returns 1 if already present, 0 if newly inserted
static b8 seen_put(walk_seen *s, u64 hashlet) {
    u64 key = hashlet | 1;  // ensure non-zero (0 = empty slot)
    u32 idx = (u32)(hashlet >> 4) & WALK_SEEN_MASK;
    for (;;) {
        if (s->tab[idx] == 0) {
            s->tab[idx] = key;
            s->count++;
            return 0;
        }
        if (s->tab[idx] == key) return 1;
        idx = (idx + 1) & WALK_SEEN_MASK;
    }
}

// Max-heap of walk_nodes by gen (newest first)
#define WALK_QUEUE_SIZE (1 << 14)

typedef struct {
    walk_node *nodes;
    u32 len;
} walk_heap;

static void heap_swap(walk_node *a, walk_node *b) {
    walk_node t = *a; *a = *b; *b = t;
}

static void heap_up(walk_node *h, u32 i) {
    while (i > 0) {
        u32 p = (i - 1) / 2;
        if (h[p].gen >= h[i].gen) break;
        heap_swap(&h[p], &h[i]);
        i = p;
    }
}

static void heap_down(walk_node *h, u32 len, u32 i) {
    for (;;) {
        u32 best = i;
        u32 l = 2 * i + 1, r = 2 * i + 2;
        if (l < len && h[l].gen > h[best].gen) best = l;
        if (r < len && h[r].gen > h[best].gen) best = r;
        if (best == i) break;
        heap_swap(&h[best], &h[i]);
        i = best;
    }
}

static ok64 heap_push(walk_heap *q, u8cp sha, u32 gen) {
    if (q->len >= WALK_QUEUE_SIZE) return WALKNOROOM;
    walk_node *n = &q->nodes[q->len];
    memcpy(n->sha, sha, 20);
    n->gen = gen;
    heap_up(q->nodes, q->len);
    q->len++;
    return OK;
}

static void heap_pop(walk_heap *q, walk_node *out) {
    *out = q->nodes[0];
    q->len--;
    if (q->len > 0) {
        q->nodes[0] = q->nodes[q->len];
        heap_down(q->nodes, q->len, 0);
    }
}

// Push all parents of a commit into the BFS queue
static ok64 push_parents(walk *w, walk_heap *q, walk_seen *seen,
                          u8cp content, u64 sz) {
    sane(w && content);
    u8cs body = {content, content + sz};
    u8cs field = {}, value = {};
    while (GITu8sDrainCommit(body, field, value) == OK) {
        if ($empty(field)) break;
        if ($len(field) != 6 || memcmp(field[0], "parent", 6) != 0)
            continue;
        if ($len(value) < 40) continue;
        u8 psha[20];
        u8s pbin = {psha, psha + 20};
        u8cs phex = {value[0], value[0] + 40};
        if (HEXu8sDrainSome(pbin, phex) != OK) continue;

        u64 phashlet = 0;
        memcpy(&phashlet, psha, 8);
        phashlet &= ~BELT_TYPE_MASK;
        if (seen_put(seen, phashlet)) continue;  // already visited

        belt128cp hit = BELTLookup(w->stack, phashlet);
        u32 pgen = hit ? BELTGen(*hit) : 0;
        call(heap_push, q, psha, pgen);
    }
    done;
}

// --- WALKCommits ---

ok64 WALKCommits(walk *w, u8cp head_sha, u8cp stop_sha,
                 walk_fn visit, voidp ctx) {
    sane(w && head_sha && visit);

    walk_seen seen = {};
    call(seen_init, &seen);

    walk_node *qmem = malloc(WALK_QUEUE_SIZE * sizeof(walk_node));
    if (!qmem) { seen_free(&seen); fail(WALKFAIL); }
    walk_heap q = {.nodes = qmem, .len = 0};

    u32 stop_gen = 0;
    u64 stop_hashlet = 0;
    if (stop_sha) {
        memcpy(&stop_hashlet, stop_sha, 8);
        stop_hashlet &= ~BELT_TYPE_MASK;
        belt128cp sh = BELTLookup(w->stack, stop_hashlet);
        if (sh) stop_gen = BELTGen(*sh);
        seen_put(&seen, stop_hashlet);
    }

    // seed with head
    u64 head_hashlet = 0;
    memcpy(&head_hashlet, head_sha, 8);
    head_hashlet &= ~BELT_TYPE_MASK;
    seen_put(&seen, head_hashlet);
    belt128cp hh = BELTLookup(w->stack, head_hashlet);
    u32 head_gen = hh ? BELTGen(*hh) : 0;
    ok64 o = heap_push(&q, head_sha, head_gen);
    if (o != OK) { free(qmem); seen_free(&seen); return o; }

    while (q.len > 0) {
        // gen-based early termination
        if (stop_sha && q.nodes[0].gen <= stop_gen) break;

        walk_node cur = {};
        heap_pop(&q, &cur);

        // resolve this commit
        u64 hashlet = 0;
        memcpy(&hashlet, cur.sha, 8);
        hashlet &= ~BELT_TYPE_MASK;
        belt128cp hit = BELTLookup(w->stack, hashlet);
        if (!hit) continue;

        u8 otype = 0;
        u8p content = NULL;
        u64 outsz = 0;
        o = BELTResolve(w->pack, w->packlen, w->stack,
                         BELTOffset(*hit), &otype,
                         w->buf1, w->buf2, WALK_BUFSZ,
                         &content, &outsz);
        if (o != OK) continue;
        if (otype != BELT_COMMIT) continue;

        u8cs cs = {content, content + outsz};
        o = visit(hashlet, otype, cs, ctx);
        if (o != OK) break;

        push_parents(w, &q, &seen, content, outsz);
    }

    free(qmem);
    seen_free(&seen);
    done;
}

// --- WALKTree ---

static ok64 walk_tree_rec(walk *w, u8cp sha, walk_fn visit, voidp ctx,
                           walk_seen *seen) {
    sane(w && sha && visit);

    u64 hashlet = 0;
    memcpy(&hashlet, sha, 8);
    hashlet &= ~BELT_TYPE_MASK;
    if (seen_put(seen, hashlet)) done;  // already visited

    belt128cp hit = BELTLookup(w->stack, hashlet);
    if (!hit) return WALKNONE;

    u8 otype = 0;
    u8p content = NULL;
    u64 outsz = 0;
    call(BELTResolve, w->pack, w->packlen, w->stack,
         BELTOffset(*hit), &otype,
         w->buf1, w->buf2, WALK_BUFSZ,
         &content, &outsz);

    u8cs cs = {content, content + outsz};
    ok64 o = visit(hashlet, otype, cs, ctx);
    if (o != OK) return o;

    if (otype != BELT_TREE) done;

    // recurse into tree entries
    // copy tree content — resolve may overwrite buf1/buf2
    u8p tcopy = malloc(outsz);
    if (!tcopy) fail(WALKFAIL);
    memcpy(tcopy, content, outsz);

    u8cs tree = {tcopy, tcopy + outsz};
    u8cs file = {}, esha = {};
    while (GITu8sDrainTree(tree, file, esha) == OK) {
        walk_tree_rec(w, esha[0], visit, ctx, seen);
    }
    free(tcopy);
    done;
}

ok64 WALKTree(walk *w, u8cp tree_sha, walk_fn visit, voidp ctx) {
    sane(w && tree_sha && visit);
    walk_seen seen = {};
    call(seen_init, &seen);
    ok64 o = walk_tree_rec(w, tree_sha, visit, ctx, &seen);
    seen_free(&seen);
    return o;
}

// --- WALKAncestor ---

#define WALK_COLOR_A 1
#define WALK_COLOR_B 2
#define WALK_COLOR_BOTH 3

typedef struct {
    u64 *tab;   // hashlet
    u8  *color; // color per slot
    u32  count;
} walk_colors;

static ok64 colors_init(walk_colors *c) {
    c->tab = calloc(WALK_SEEN_SIZE, sizeof(u64));
    c->color = calloc(WALK_SEEN_SIZE, 1);
    c->count = 0;
    if (!c->tab || !c->color) {
        free(c->color); free(c->tab);
        return WALKFAIL;
    }
    return OK;
}

static void colors_free(walk_colors *c) {
    free(c->color); free(c->tab);
}

// Returns previous color (0 if new)
static u8 colors_put(walk_colors *c, u64 hashlet, u8 clr) {
    u64 key = hashlet | 1;
    u32 idx = (u32)(hashlet >> 4) & WALK_SEEN_MASK;
    for (;;) {
        if (c->tab[idx] == 0) {
            c->tab[idx] = key;
            c->color[idx] = clr;
            c->count++;
            return 0;
        }
        if (c->tab[idx] == key) {
            u8 prev = c->color[idx];
            c->color[idx] |= clr;
            return prev;
        }
        idx = (idx + 1) & WALK_SEEN_MASK;
    }
}

ok64 WALKAncestor(walk *w, u8cp sha_a, u8cp sha_b, u8 out[20]) {
    sane(w && sha_a && sha_b && out);

    walk_colors colors = {};
    call(colors_init, &colors);

    walk_node *qmem = malloc(WALK_QUEUE_SIZE * sizeof(walk_node));
    if (!qmem) { colors_free(&colors); fail(WALKFAIL); }
    walk_heap q = {.nodes = qmem, .len = 0};

    // We store color in the unused low bit of walk_node.gen.
    // Actually, let's use a separate approach: tag the queue nodes.
    // Repurpose: use a parallel color array keyed by queue position?
    // Simpler: run two heaps? No, let's use the colors table.
    // When we pop a node, look up its color to know which side it came from.

    // Seed A
    {
        u64 h = 0; memcpy(&h, sha_a, 8); h &= ~BELT_TYPE_MASK;
        colors_put(&colors, h, WALK_COLOR_A);
        belt128cp hit = BELTLookup(w->stack, h);
        u32 gen = hit ? BELTGen(*hit) : 0;
        heap_push(&q, sha_a, gen);
    }

    // Seed B
    {
        u64 h = 0; memcpy(&h, sha_b, 8); h &= ~BELT_TYPE_MASK;
        u8 prev = colors_put(&colors, h, WALK_COLOR_B);
        if (prev & WALK_COLOR_A) {
            // same commit
            memcpy(out, sha_a, 20);
            free(qmem); colors_free(&colors);
            done;
        }
        belt128cp hit = BELTLookup(w->stack, h);
        u32 gen = hit ? BELTGen(*hit) : 0;
        heap_push(&q, sha_b, gen);
    }

    ok64 result = WALKNONE;

    while (q.len > 0) {
        walk_node cur = {};
        heap_pop(&q, &cur);

        u64 hashlet = 0;
        memcpy(&hashlet, cur.sha, 8);
        hashlet &= ~BELT_TYPE_MASK;

        // Look up the color of this node
        u64 key = hashlet | 1;
        u32 cidx = (u32)(hashlet >> 4) & WALK_SEEN_MASK;
        u8 my_color = 0;
        while (colors.tab[cidx] != 0) {
            if (colors.tab[cidx] == key) { my_color = colors.color[cidx]; break; }
            cidx = (cidx + 1) & WALK_SEEN_MASK;
        }

        if (my_color == WALK_COLOR_BOTH) {
            memcpy(out, cur.sha, 20);
            result = OK;
            break;
        }

        // resolve commit, push parents
        belt128cp hit = BELTLookup(w->stack, hashlet);
        if (!hit) continue;

        u8 otype = 0;
        u8p content = NULL;
        u64 outsz = 0;
        ok64 o = BELTResolve(w->pack, w->packlen, w->stack,
                              BELTOffset(*hit), &otype,
                              w->buf1, w->buf2, WALK_BUFSZ,
                              &content, &outsz);
        if (o != OK || otype != BELT_COMMIT) continue;

        // parse parents
        u8cs body = {content, content + outsz};
        u8cs field = {}, value = {};
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if ($empty(field)) break;
            if ($len(field) != 6 || memcmp(field[0], "parent", 6) != 0)
                continue;
            if ($len(value) < 40) continue;
            u8 psha[20];
            u8s pbin = {psha, psha + 20};
            u8cs phex = {value[0], value[0] + 40};
            if (HEXu8sDrainSome(pbin, phex) != OK) continue;

            u64 ph = 0;
            memcpy(&ph, psha, 8);
            ph &= ~BELT_TYPE_MASK;
            u8 prev = colors_put(&colors, ph, my_color);
            if ((prev | my_color) == WALK_COLOR_BOTH) {
                memcpy(out, psha, 20);
                result = OK;
                goto found;
            }
            if (prev & my_color) continue;  // already visited from this side

            belt128cp ph_hit = BELTLookup(w->stack, ph);
            u32 pgen = ph_hit ? BELTGen(*ph_hit) : 0;
            heap_push(&q, psha, pgen);
        }
    }

found:
    free(qmem);
    colors_free(&colors);
    return result;
}

// --- WALKMissing ---

ok64 WALKMissing(walk *w, u8cp head_sha, u8cp base_sha,
                 walk_fn visit, voidp ctx) {
    sane(w && head_sha && visit);

    // First: find all commit hashlets reachable from base
    walk_seen base_set = {};
    call(seen_init, &base_set);

    if (base_sha) {
        walk_node *qmem = malloc(WALK_QUEUE_SIZE * sizeof(walk_node));
        if (!qmem) { seen_free(&base_set); fail(WALKFAIL); }
        walk_heap q = {.nodes = qmem, .len = 0};

        u64 bh = 0; memcpy(&bh, base_sha, 8); bh &= ~BELT_TYPE_MASK;
        seen_put(&base_set, bh);
        belt128cp hit = BELTLookup(w->stack, bh);
        u32 bgen = hit ? BELTGen(*hit) : 0;
        heap_push(&q, base_sha, bgen);

        while (q.len > 0) {
            walk_node cur = {};
            heap_pop(&q, &cur);
            u64 hashlet = 0;
            memcpy(&hashlet, cur.sha, 8);
            hashlet &= ~BELT_TYPE_MASK;

            belt128cp ch = BELTLookup(w->stack, hashlet);
            if (!ch) continue;
            u8 otype = 0;
            u8p content = NULL;
            u64 outsz = 0;
            if (BELTResolve(w->pack, w->packlen, w->stack,
                             BELTOffset(*ch), &otype,
                             w->buf1, w->buf2, WALK_BUFSZ,
                             &content, &outsz) != OK) continue;
            if (otype != BELT_COMMIT) continue;
            push_parents(w, &q, &base_set, content, outsz);
        }
        free(qmem);
    }

    // Now walk from head, skip anything in base_set
    walk_seen head_seen = {};
    ok64 o = seen_init(&head_seen);
    if (o != OK) { seen_free(&base_set); return o; }

    walk_node *qmem2 = malloc(WALK_QUEUE_SIZE * sizeof(walk_node));
    if (!qmem2) { seen_free(&head_seen); seen_free(&base_set); fail(WALKFAIL); }
    walk_heap q2 = {.nodes = qmem2, .len = 0};

    u64 hh = 0; memcpy(&hh, head_sha, 8); hh &= ~BELT_TYPE_MASK;
    seen_put(&head_seen, hh);
    belt128cp hh_hit = BELTLookup(w->stack, hh);
    u32 hgen = hh_hit ? BELTGen(*hh_hit) : 0;
    heap_push(&q2, head_sha, hgen);

    // Walk tree objects seen from missing commits
    walk_seen tree_seen = {};
    o = seen_init(&tree_seen);
    if (o != OK) {
        free(qmem2); seen_free(&head_seen); seen_free(&base_set);
        return o;
    }

    while (q2.len > 0) {
        walk_node cur = {};
        heap_pop(&q2, &cur);

        u64 hashlet = 0;
        memcpy(&hashlet, cur.sha, 8);
        hashlet &= ~BELT_TYPE_MASK;

        // skip if in base
        u64 key = hashlet | 1;
        u32 idx = (u32)(hashlet >> 4) & WALK_SEEN_MASK;
        b8 in_base = 0;
        while (base_set.tab[idx] != 0) {
            if (base_set.tab[idx] == key) { in_base = 1; break; }
            idx = (idx + 1) & WALK_SEEN_MASK;
        }
        if (in_base) continue;

        belt128cp hit = BELTLookup(w->stack, hashlet);
        if (!hit) continue;

        u8 otype = 0;
        u8p content = NULL;
        u64 outsz = 0;
        o = BELTResolve(w->pack, w->packlen, w->stack,
                         BELTOffset(*hit), &otype,
                         w->buf1, w->buf2, WALK_BUFSZ,
                         &content, &outsz);
        if (o != OK) continue;
        if (otype != BELT_COMMIT) continue;

        u8cs cs = {content, content + outsz};
        o = visit(hashlet, otype, cs, ctx);
        if (o != OK) break;

        // walk this commit's tree
        u8 tree_sha[20];
        if (WALKCommitTree(cs, tree_sha) == OK)
            walk_tree_rec(w, tree_sha, visit, ctx, &tree_seen);

        // push parents into commit BFS queue
        // (reuse head_seen to avoid revisiting)
        u8cs body2 = {content, content + outsz};
        u8cs field2 = {}, value2 = {};
        while (GITu8sDrainCommit(body2, field2, value2) == OK) {
            if ($empty(field2)) break;
            if ($len(field2) != 6 || memcmp(field2[0], "parent", 6) != 0)
                continue;
            if ($len(value2) < 40) continue;
            u8 psha[20];
            u8s pbin = {psha, psha + 20};
            u8cs phex = {value2[0], value2[0] + 40};
            if (HEXu8sDrainSome(pbin, phex) != OK) continue;
            u64 ph = 0;
            memcpy(&ph, psha, 8);
            ph &= ~BELT_TYPE_MASK;
            if (seen_put(&head_seen, ph)) continue;
            belt128cp phit = BELTLookup(w->stack, ph);
            u32 pgen = phit ? BELTGen(*phit) : 0;
            heap_push(&q2, psha, pgen);
        }
    }

    free(qmem2);
    seen_free(&tree_seen);
    seen_free(&head_seen);
    seen_free(&base_set);
    done;
}

// --- WALKCheckout ---

ok64 WALKCheckout(walk *w, u8cp tree_sha, u8cs dest) {
    sane(w && tree_sha && $ok(dest));

    u64 hashlet = 0;
    memcpy(&hashlet, tree_sha, 8);
    hashlet &= ~BELT_TYPE_MASK;

    belt128cp hit = BELTLookup(w->stack, hashlet);
    if (!hit) return WALKNONE;

    u8 otype = 0;
    u8p content = NULL;
    u64 outsz = 0;
    call(BELTResolve, w->pack, w->packlen, w->stack,
         BELTOffset(*hit), &otype,
         w->buf1, w->buf2, WALK_BUFSZ,
         &content, &outsz);
    if (otype != BELT_TREE) return WALKBADFMT;

    u8p tcopy = malloc(outsz);
    if (!tcopy) fail(WALKFAIL);
    memcpy(tcopy, content, outsz);

    u8cs tree = {tcopy, tcopy + outsz};
    u8cs file = {}, esha = {};
    ok64 result = OK;

    while (GITu8sDrainTree(tree, file, esha) == OK) {
        // parse mode and name: "<mode> <name>"
        u8cp p = file[0];
        u8cp e = file[1];
        while (p < e && *p != ' ') p++;
        if (p >= e) continue;
        u8cs mode_s = {file[0], p};
        u8cs name_s = {p + 1, e};

        // check if directory (mode starts with '4')
        b8 is_dir = (*mode_s[0] == '4');

        // build path: dest/name
        a_pad(u8, path, 1024);
        u8bFeed(path, dest);
        u8bFeed1(path, '/');
        u8bFeed(path, name_s);
        PATHu8gTerm(PATHu8gIn(path));

        if (is_dir) {
            FILEMakeDir(PATHu8cgIn(path));
            u8cs subdir = {u8bDataHead(path), path[2]};
            result = WALKCheckout(w, esha[0], subdir);
            if (result != OK) break;
        } else {
            // resolve blob, write to file
            u64 bh = 0;
            memcpy(&bh, esha[0], 8);
            bh &= ~BELT_TYPE_MASK;
            belt128cp bhit = BELTLookup(w->stack, bh);
            if (!bhit) { result = WALKNONE; break; }

            u8 bt = 0;
            u8p bcontent = NULL;
            u64 bsz = 0;
            result = BELTResolve(w->pack, w->packlen, w->stack,
                                  BELTOffset(*bhit), &bt,
                                  w->buf1, w->buf2, WALK_BUFSZ,
                                  &bcontent, &bsz);
            if (result != OK) break;

            int fd = -1;
            result = FILECreate(&fd, PATHu8cgIn(path));
            if (result != OK) break;
            u8cs data = {bcontent, bcontent + bsz};
            result = FILEFeedall(fd, data);
            close(fd);
            if (result != OK) break;

            // set executable if mode contains 755
            if ($len(mode_s) >= 6 && mode_s[0][3] == '7')
                chmod((char const *)u8bDataHead(path), 0755);
        }
    }

    free(tcopy);
    return result;
}
