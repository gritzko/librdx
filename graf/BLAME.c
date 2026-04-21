//  BLAME: token-level blame via keeper object store + DAG index.
//
//  Walks file history via DAG index (PATH_VER + PREV_BLOB chain),
//  fetches blobs via KEEPGet, builds a weave from successive blob
//  versions, renders blame annotations per line.
//
//  WEAVE DIFF: resolves refs via KEEPWalk, fetches blobs, runs
//  pairwise token-level diff via DIFFu8cs.
//
#include "GRAF.h"
#include "DAG.h"
#include "TDIFF.h"
#include "WEAVE.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/RAP.h"
#include "abc/UTF8.h"
#include "dog/HUNK.h"
#include "dog/WHIFF.h"
#include "keeper/GIT.h"
#include "keeper/REFS.h"

// --- Blob version in the chain ---

typedef struct {
    u64 commit_hashlet;
    u32 gen;
} blame_ver;

#define BLAME_MAX_VERS 256
#define BLAME_MAX_AUTHORS 256

// --- Author table: gen → author + date ---

typedef struct {
    u32  gen;
    u64  commit_hashlet;
    char author[48];
    char date[12];   // YYYY-MM-DD
} blame_author;

// --- UTF-8 aware fixed-width field: truncate to N codepoints, pad right ---

static void blame_fixfield(char *out, size_t outsz,
                            char const *src, int maxcols, char const *after) {
    char *w = out;
    char *wend = out + outsz - 1;
    int cols = 0;
    char const *p = src;
    while (*p && cols < maxcols) {
        u8 len = UTF8_LEN[((u8)*p) >> 4];
        if (w + len >= wend) break;
        for (u8 j = 0; j < len && *p; j++) *w++ = *p++;
        cols++;
    }
    while (cols < maxcols && w < wend) { *w++ = ' '; cols++; }
    while (*after && w < wend) *w++ = *after++;
    *w = 0;
}

// --- Compact date: "3Jun" if same year, "2023" if different ---

static char const *MONTH_ABBR[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};

static void blame_compact_date(char *out, size_t outsz,
                                char const *iso_date, int current_year) {
    out[0] = 0;
    if (!iso_date || strlen(iso_date) < 10) return;
    int y = 0, m = 0, d = 0;
    if (sscanf(iso_date, "%d-%d-%d", &y, &m, &d) != 3) return;
    if (y == current_year) {
        if (m >= 1 && m <= 12)
            snprintf(out, outsz, "%d%s", d, MONTH_ABBR[m - 1]);
        else
            snprintf(out, outsz, "%d/%d", d, m);
    } else {
        snprintf(out, outsz, "%d", y);
    }
}

//  graf stores 40-bit hashlets (top 40 bits of SHA-1); keeper stores
//  60-bit hashlets (top 60 bits) in its LSM keys.  To resolve a graf
//  hashlet in keeper, left-align into the 60-bit space and do a
//  40-bit prefix match (hexlen=10).  For the small test repos that
//  drive us today, 40-bit collisions are vanishingly rare; the
//  caller further narrows false positives by checking obj_type.
fun u64 blame_h40_to_h60_prefix(u64 h40) { return h40 << 20; }
#define BLAME_HEXLEN_40 10

// --- Fetch author + date from commit via keeper ---

static void blame_fetch_author(blame_author *ba, keeper *k,
                                u64 commit_hashlet) {
    ba->author[0] = 0;
    ba->date[0] = 0;

    Bu8 cbuf = {};
    if (u8bMap(cbuf, 1UL << 20) != OK) return;
    u8 obj_type = 0;
    if (KEEPGet(k, blame_h40_to_h60_prefix(commit_hashlet),
                BLAME_HEXLEN_40, cbuf, &obj_type) != OK ||
        obj_type != DOG_OBJ_COMMIT) {
        u8bUnMap(cbuf);
        return;
    }

    // Parse commit headers looking for "author"
    
a_dup(u8c, scan, u8bDataC(cbuf));
    u8cs field = {}, value = {};
    while (GITu8sDrainCommit(scan, field, value) == OK) {
        if (u8csEmpty(field)) break;  // blank line = body
        a_cstr(author_f, "author");
        if (!$eq(field, author_f)) continue;

        // value = "Name <email> timestamp tz"
        // Find last '<' to split name from rest
        u8cp lt = value[1];
        while (lt > value[0] && *(lt - 1) != '<') lt--;
        if (lt > value[0]) {
            // Name is before '<', trim trailing space
            u8cp ne = lt - 1;
            while (ne > value[0] && *(ne - 1) == ' ') ne--;
            size_t nl = (size_t)(ne - value[0]);
            if (nl >= sizeof(ba->author)) nl = sizeof(ba->author) - 1;
            memcpy(ba->author, value[0], nl);
            ba->author[nl] = 0;
        }

        // Extract timestamp (after '> ')
        u8cp gt = lt;
        while (gt < value[1] && *gt != '>') gt++;
        if (gt < value[1]) gt++;  // past '>'
        while (gt < value[1] && *gt == ' ') gt++;
        // gt now at timestamp digits
        if (gt < value[1]) {
            long ts = 0;
            while (gt < value[1] && *gt >= '0' && *gt <= '9') {
                ts = ts * 10 + (*gt - '0');
                gt++;
            }
            if (ts > 0) {
                time_t t = (time_t)ts;
                struct tm *tm = gmtime(&t);
                if (tm)
                    snprintf(ba->date, sizeof(ba->date), "%04d-%02d-%02d",
                             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
            }
        }
        break;
    }
    u8bUnMap(cbuf);
}

// Forward decls for helpers defined after GRAFBlame.
static ok64 blame_tree_step(keeper *k, sha1 *cur, u8cs name);
static ok64 blame_read_blob(u8bp buf, keeper *k, u8cs ref, u8cs filepath);

#define BLAME_ANC_SIZE  (1u << 18)   // 256K slots ≈ 4MB, power of 2

// --- Resolve (commit_hashlet, filepath) → blob content ---
//
// Fetches the commit object by hashlet, parses its tree SHA from
// the header, then descends the tree one path segment at a time
// (same mechanism as blame_read_blob, but keyed by commit_h instead
// of a ref string).  Returns KEEPNONE if the path is absent in this
// commit's tree (e.g. hashlet collision or rename) — caller should
// treat as "skip this version".

static ok64 blame_blob_at_commit(u8bp buf, keeper *k,
                                  u64 commit_hashlet, u8cs filepath) {
    sane(buf && k);

    Bu8 cbuf = {};
    call(u8bAllocate, cbuf, 1UL << 20);
    u8 ct = 0;
    ok64 o = KEEPGet(k, blame_h40_to_h60_prefix(commit_hashlet),
                     BLAME_HEXLEN_40, cbuf, &ct);
    if (o != OK || ct != DOG_OBJ_COMMIT) { u8bFree(cbuf); return KEEPNONE; }

    // Parse "tree <40-hex>" from header.
    sha1 tree_sha = {};
    b8 got_tree = NO;
    {
        a_dup(u8c, scan, u8bDataC(cbuf));
        u8cs field = {}, value = {};
        while (GITu8sDrainCommit(scan, field, value) == OK) {
            if (u8csEmpty(field)) break;
            a_cstr(ft, "tree");
            if ($eq(field, ft) && u8csLen(value) >= 40) {
                DAGsha1FromHex(&tree_sha, (char const *)value[0]);
                got_tree = YES;
                break;
            }
        }
    }
    u8bFree(cbuf);
    if (!got_tree) return KEEPNONE;

    sha1 cur = tree_sha;
    u8cs rest = {filepath[0], filepath[1]};
    while (!$empty(rest)) {
        u8cp slash = rest[0];
        while (slash < rest[1] && *slash != '/') slash++;
        u8cs name = {rest[0], slash};
        ok64 s = blame_tree_step(k, &cur, name);
        if (s != OK) return s;
        rest[0] = (slash < rest[1]) ? slash + 1 : slash;
    }

    u8 btype = 0;
    call(KEEPGetExact, k, &cur, buf, &btype);
    if (btype != DOG_OBJ_BLOB) return KEEPNONE;
    done;
}

// --- Walk file history via DAG index (PATH_VER scan on path hashlet) ---
//
// PATH_VER: a = pack(PATH_VER, gen, path_hashlet40),
//           b = pack(PATH_VER, gen, commit_hashlet40)
// Scans all runs for matching path_hashlet, returns newest-first.
// path_hashlet = RAPHash(filepath) & WHIFF_OFF_MASK — 40 bits;
// collisions possible but verified by the caller (actual path must
// exist in the commit's tree, else the candidate is dropped).

// `ancestors` may be an empty-sized Bwh128 (no filter) or a populated
// set; when populated, candidates whose commit is not in the set are
// dropped.
// `ancestors` empty → no filter; populated → drop non-ancestor hits.
static u32 blame_walk_history(blame_ver *vers, u32 maxvers,
                               dag_stack const *idx,
                               u8cs filepath,
                               Bwh128 ancestors) {
    u64 path_h = RAPHash(filepath) & WHIFF_OFF_MASK;
    b8 has_filter = wh128bHead(ancestors) != wh128bTerm(ancestors);
    u32 count = 0;
    for (u32 r = 0; r < idx->n && count < maxvers; r++) {
        wh128cp base = idx->runs[r][0];
        size_t len = (size_t)(idx->runs[r][1] - base);
        for (size_t i = 0; i < len && count < maxvers; i++) {
            if (DAGType(base[i].key) != DAG_PATH_VER) continue;
            if (DAGHashlet(base[i].key) != path_h) continue;
            u64 c_h = DAGHashlet(base[i].val);
            if (has_filter && !DAGAncestorsHas(ancestors, c_h)) continue;
            vers[count].gen = DAGGen(base[i].key);
            vers[count].commit_hashlet = c_h;
            count++;
        }
    }

    // Sort by gen descending (newest first)
    for (u32 i = 1; i < count; i++) {
        for (u32 j = i; j > 0 && vers[j].gen > vers[j - 1].gen; j--) {
            blame_ver tmp = vers[j];
            vers[j] = vers[j - 1];
            vers[j - 1] = tmp;
        }
    }
    return count;
}

// --- Find author for a gen in the author table ---

static blame_author const *blame_lookup_gen(blame_author const *authors,
                                             u32 nauthors, u32 gen) {
    for (u32 i = 0; i < nauthors; i++)
        if (authors[i].gen == gen) return &authors[i];
    return NULL;
}

static blame_author const blame_unknown = {.gen = 0, .commit_hashlet = 0, .author = "?", .date = ""};

// --- Public entry ---

ok64 GRAFBlame(keeper *k, u8cs filepath, u64 tip_h, u8cs reporoot) {
    sane(k && $ok(filepath) && $ok(reporoot));

    call(GRAFArenaInit);

    //  Open the DAG index.  The CLI entry point may already have
    //  opened graf in rw mode — GRAFOpen then returns GRAFOPEN (or
    //  GRAFOPENRO on a downgrade attempt), which is NOT an error.
    //  We only own the handle (and must close it ourselves) when the
    //  open actually succeeded here.
    ok64 go = GRAFOpen(k->h, NO);
    b8 own_open = (go == OK);
    if (go != OK && go != GRAFOPEN && go != GRAFOPENRO) return go;

    // Ancestry filter — thin wrapper over DAGAncestors.  tip_h == 0
    // leaves the set empty; blame_walk_history treats empty as "no
    // filter".
    Bwh128 ancestors = {};
    if (tip_h != 0) {
        call(wh128bAllocate, ancestors, BLAME_ANC_SIZE);
        DAGAncestors(ancestors, &GRAF.idx, tip_h, 0);
    }

    blame_ver vers[BLAME_MAX_VERS];
    u32 nvers = blame_walk_history(vers, BLAME_MAX_VERS, &GRAF.idx,
                                    filepath, ancestors);

    // Reverse to oldest first.  Byte-level dedup happens in the
    // fetch loop below (collision or unchanged-path commits produce
    // identical bytes and are skipped there).
    for (u32 i = 0; i < nvers / 2; i++) {
        blame_ver tmp = vers[i];
        vers[i] = vers[nvers - 1 - i];
        vers[nvers - 1 - i] = tmp;
    }

    // Build author table + fetch blob bytes + build weave
    blame_author authors[BLAME_MAX_AUTHORS] = {};
    u32 nauthors = 0;

    weave wv = {};
    call(WEAVEInit, &wv, 0);

    // Two mapped blob buffers, swap each iteration
    #define BLAME_BLOB_MAX (16UL << 20)  // 16 MB per blob
    Bu8 blob_a = {}, blob_b = {};
    call(u8bMap, blob_a, BLAME_BLOB_MAX);
    call(u8bMap, blob_b, BLAME_BLOB_MAX);
    Bu8 *cur_blob = &blob_a, *prev_blobp = &blob_b;

    u8cs ext = {};
    HUNKu8sExt(ext, filepath[0], $len(filepath));
    if (!$empty(ext) && *ext[0] == '.') ext[0]++;

    b8 have_prev = NO;
    for (u32 i = 0; i < nvers; i++) {
        u8bReset(*cur_blob);
        // Resolve (commit, filepath) → blob via keeper.  Drops
        // false-positive PATH_VER hits (collisions, or commits
        // where the path doesn't actually exist).
        ok64 fo = blame_blob_at_commit(*cur_blob, k,
                                        vers[i].commit_hashlet, filepath);
        if (fo != OK) continue;

        // Byte-level dedup: if this blob is identical to the
        // immediately-preceding kept version, skip — same content,
        // no new weave event.  This catches rollbacks where the
        // path's blob hash reverts to a prior value.
        if (have_prev) {
            size_t cl = u8bDataLen(*cur_blob);
            size_t pl = u8bDataLen(*prev_blobp);
            if (cl == pl && (cl == 0 ||
                memcmp(u8bDataHead(*cur_blob),
                       u8bDataHead(*prev_blobp), cl) == 0)) {
                continue;
            }
        }

        if (vers[i].commit_hashlet && nauthors < BLAME_MAX_AUTHORS) {
            authors[nauthors].gen = vers[i].gen;
            authors[nauthors].commit_hashlet = vers[i].commit_hashlet;
            blame_fetch_author(&authors[nauthors], k,
                               vers[i].commit_hashlet);
            nauthors++;
        }

        u8cs old_data = {};
        if (have_prev) {
            old_data[0] = u8bDataHead(*prev_blobp);
            old_data[1] = u8bDataHead(*prev_blobp) + u8bDataLen(*prev_blobp);
        }
        u8cs new_data = {u8bDataHead(*cur_blob),
                         u8bDataHead(*cur_blob) + u8bDataLen(*cur_blob)};

        call(WEAVEAdd, &wv, old_data, new_data, ext, vers[i].gen);

        // Swap
        Bu8 *tmp = cur_blob; cur_blob = prev_blobp; prev_blobp = tmp;
        have_prev = YES;
    }
    u8bUnMap(blob_a);
    u8bUnMap(blob_b);

    // Render blame: "hashlet name date code"
    #define BLAME_HW 7   // hashlet width
    #define BLAME_NW 12  // name width
    #define BLAME_DW 5   // date width
    #define BLAME_PW (BLAME_HW + 1 + BLAME_NW + 1 + BLAME_DW + 1)
    #define CLR_HASH "\033[38;5;108m"
    #define CLR_NAME "\033[38;5;103m"
    #define CLR_DATE "\033[38;5;245m"
    #define CLR_OFF  "\033[0m"

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    int cur_year = tm ? tm->tm_year + 1900 : 2026;
    b8 tty = graf_out_fd >= 0 && graf_emit == HUNKu8sFeed;

    u32 wlen = WEAVELen(&wv);
    wtokcp wtoks = WEAVETokens(&wv);

    Bu8 outbuf = {};
    call(u8bMap, outbuf, 16UL << 20);

    char const *prev_author = NULL;
    b8 at_bol = YES;

    char blank_pre[BLAME_PW + 1];
    memset(blank_pre, ' ', BLAME_PW);
    blank_pre[BLAME_PW] = 0;

    #define EMIT_BLANK do { \
        u8cs _s = {(u8cp)blank_pre, (u8cp)blank_pre + BLAME_PW}; \
        u8bFeed(outbuf, _s); \
    } while(0)

    for (u32 wi = 0; wi < wlen; wi++) {
        if (wtoks[wi].del_gen != 0) continue;

        u8cp tp = wtoks[wi].tok[0];
        u8cp te = wtoks[wi].tok[1];

        if (at_bol) {
            u32 tgen = wtoks[wi].intro_gen;
            blame_author const *ba = blame_lookup_gen(authors, nauthors, tgen);
            if (!ba) ba = &blame_unknown;
            b8 diff_auth = !prev_author || strcmp(prev_author, ba->author) != 0;

            char pre[256];
            if (diff_auth) {
                char hexlet[12] = "       ";
                if (ba->commit_hashlet) {
                    snprintf(hexlet, sizeof(hexlet), "%010llx",
                             (unsigned long long)ba->commit_hashlet);
                    hexlet[BLAME_HW] = 0;
                }
                char cd[16];
                blame_compact_date(cd, sizeof(cd), ba->date, cur_year);
                char fhash[16], fname[32], fdate[16];
                blame_fixfield(fhash, sizeof(fhash), hexlet, BLAME_HW, " ");
                blame_fixfield(fname, sizeof(fname), ba->author, BLAME_NW, " ");
                blame_fixfield(fdate, sizeof(fdate), cd, BLAME_DW, " ");
                if (tty)
                    snprintf(pre, sizeof(pre),
                             CLR_HASH "%s" CLR_NAME "%s" CLR_DATE "%s" CLR_OFF,
                             fhash, fname, fdate);
                else
                    snprintf(pre, sizeof(pre), "%s%s%s", fhash, fname, fdate);
                prev_author = ba->author;
            } else {
                snprintf(pre, sizeof(pre), "%s", blank_pre);
            }
            u8cs ps = {(u8cp)pre, (u8cp)pre + strlen(pre)};
            u8bFeed(outbuf, ps);
            at_bol = NO;
        }

        while (tp < te) {
            u8cp nl = tp;
            while (nl < te && *nl != '\n') nl++;
            if (nl < te) {
                u8cs chunk = {tp, nl + 1};
                u8bFeed(outbuf, chunk);
                tp = nl + 1;
                at_bol = YES;
                if (tp < te) { EMIT_BLANK; at_bol = NO; }
            } else {
                u8cs chunk = {tp, te};
                u8bFeed(outbuf, chunk);
                tp = te;
            }
        }
    }

    if (!at_bol) {
        u8cs nl = {(u8cp)"\n", (u8cp)"\n" + 1};
        u8bFeed(outbuf, nl);
    }

    #undef EMIT_BLANK

    {
        char title[128];
        snprintf(title, sizeof(title), "%.*s (blame)",
                 (int)$len(filepath), (char *)filepath[0]);

        u8cs fdata = {u8bDataHead(outbuf),
                      u8bDataHead(outbuf) + u8bDataLen(outbuf)};
        hunk hk = {};
        hk.uri[0] = (u8cp)title;
        hk.uri[1] = (u8cp)title + strlen(title);
        $mv(hk.text, fdata);
        call(GRAFHunkEmit, &hk, NULL);
    }

    u8bUnMap(outbuf);
    WEAVEFree(&wv);
    if (wh128bHead(ancestors) != wh128bTerm(ancestors)) wh128bFree(ancestors);
    if (own_open) GRAFClose();
    GRAFArenaCleanup();
    done;
}

// --- Weave diff: resolve (ref, filepath) → blob via path descent ---

// Given the current tree SHA in `cur`, find the entry named `name` and
// set `cur` to its raw SHA.  Returns KEEPNONE if not found, KEEPFAIL on
// malformed tree.
static ok64 blame_tree_step(keeper *k, sha1 *cur, u8cs name) {
    sane(k && cur);
    Bu8 tbuf = {};
    call(u8bAllocate, tbuf, 1UL << 20);
    u8 otype = 0;
    ok64 o = KEEPGetExact(k, cur, tbuf, &otype);
    if (o != OK) { u8bFree(tbuf); return o; }
    if (otype != DOG_OBJ_TREE) { u8bFree(tbuf); fail(KEEPFAIL); }

    u8cs body = {u8bDataHead(tbuf), u8bIdleHead(tbuf)};
    u8cs field = {}, esha = {};
    ok64 result = KEEPNONE;
    while (GITu8sDrainTree(body, field, esha) == OK) {
        u8cs scan = {field[0], field[1]};
        if (u8csFind(scan, ' ') != OK) continue;
        u8cs entry_name = {scan[0] + 1, field[1]};
        if ($len(entry_name) != $len(name)) continue;
        if (memcmp(entry_name[0], name[0], $len(name)) != 0) continue;
        memcpy(cur->data, esha[0], 20);
        result = OK;
        break;
    }
    u8bFree(tbuf);
    return result;
}

// Resolve `ref` + `filepath` to the blob content at that path.
// Descends path segments one at a time (O(depth) tree loads) rather
// than walking the full tree.
static ok64 blame_read_blob(u8bp buf, keeper *k, u8cs ref, u8cs filepath) {
    sane(buf && k);

    uri target = {};
    a_pad(u8, ubuf, 512);
    u8bFeed1(ubuf, '?');
    call(u8bFeed, ubuf, ref);
    a_dup(u8c, udata, u8bData(ubuf));
    target.data[0] = udata[0];
    target.data[1] = udata[1];
    target.query[0] = udata[0] + 1;
    target.query[1] = udata[1];

    sha1 cur = {};
    call(KEEPResolveTree, k, &target, &cur);

    u8cs rest = {filepath[0], filepath[1]};
    while (!$empty(rest)) {
        u8cp slash = rest[0];
        while (slash < rest[1] && *slash != '/') slash++;
        u8cs name = {rest[0], slash};
        call(blame_tree_step, k, &cur, name);
        rest[0] = (slash < rest[1]) ? slash + 1 : slash;
    }

    u8 btype = 0;
    call(KEEPGetExact, k, &cur, buf, &btype);
    if (btype != DOG_OBJ_BLOB) fail(KEEPNONE);
    done;
}

ok64 GRAFWeaveDiff(keeper *k, u8cs filepath, u8cs reporoot,
                   u8cs from, u8cs to) {
    sane(k && $ok(filepath) && $ok(reporoot));

    call(GRAFArenaInit);

    u8cs ext = {};
    HUNKu8sExt(ext, filepath[0], $len(filepath));
    if (!$empty(ext) && *ext[0] == '.') ext[0]++;

    Bu8 from_buf = {}, to_buf = {};
    call(u8bMap, from_buf, 16UL << 20);
    call(u8bMap, to_buf, 16UL << 20);

    // Fetch to-blob (HEAD or specified ref)
    {
        u8cs to_ref = {};
        if (!$empty(to))
            u8csMv(to_ref, to);
        else {
            a_cstr(head, "HEAD");
            to_ref[0] = head[0];
            to_ref[1] = head[1];
        }
        blame_read_blob(to_buf, k, to_ref, filepath);
    }

    // Fetch from-blob (if specified)
    if (!$empty(from))
        blame_read_blob(from_buf, k, from, filepath);

    // Run the standard token-level diff
    {
        u8cs old_data = {u8bDataHead(from_buf),
                         u8bDataHead(from_buf) + u8bDataLen(from_buf)};
        u8cs new_data = {u8bDataHead(to_buf),
                         u8bDataHead(to_buf) + u8bDataLen(to_buf)};

        char dispname[512];
        snprintf(dispname, sizeof(dispname), "%.*s",
                 (int)$len(filepath), (char *)filepath[0]);

        call(DIFFu8cs, graf_arena, old_data, new_data,
             ext, dispname, GRAFHunkEmit, NULL);
    }

    u8bUnMap(from_buf);
    u8bUnMap(to_buf);
    GRAFArenaCleanup();
    done;
}
