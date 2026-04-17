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
#include "abc/UTF8.h"
#include "dog/HUNK.h"
#include "keeper/GIT.h"
#include "keeper/REFS.h"

// --- Blob version in the chain ---

typedef struct {
    u64 blob_hashlet;
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

// --- Fetch author + date from commit via keeper ---

static void blame_fetch_author(blame_author *ba, keeper *k,
                                u64 commit_hashlet) {
    ba->author[0] = 0;
    ba->date[0] = 0;

    Bu8 cbuf = {};
    if (u8bMap(cbuf, 1UL << 20) != OK) return;
    u8 obj_type = 0;
    if (KEEPGet(k, commit_hashlet, 15, cbuf, &obj_type) != OK ||
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

// --- Walk file history via DAG index (PATH_VER + BLOB_COMMIT chain) ---
//
// For a given filepath, look up its path_id in .dogs/graf/PATHS,
// then scan PATH_VER entries to collect (gen, blob_hashlet) pairs.
// For each blob, look up BLOB_COMMIT to get the commit hashlet.
// Returns versions newest-first.

static u32 blame_walk_history(blame_ver *vers, u32 maxvers,
                               dag_stack const *idx,
                               u8cs filepath, u8cs dagdir) {
    // Read PATHS file to find path_id for filepath
    a_path(pp, dagdir);
    a_cstr(pname, "/PATHS");
    if (u8bFeed(pp, pname) != OK) return 0;
    PATHu8bTerm(pp);

    u8bp pmap = NULL;
    if (FILEMapRO(&pmap, $path(pp)) != OK) return 0;
    a_dup(u8c, pdata, u8bDataC(pmap));

    // PATHS is NUL-separated: find the index of filepath
    u32 pid = 0;
    b8 found = NO;
    a_dup(u8c, pscan, pdata);
    while (!u8csEmpty(pscan)) {
        // Find next NUL
        u8cp nul = pscan[0];
        while (nul < pscan[1] && *nul != 0) nul++;
        u8cs entry = {pscan[0], nul};
        if (u8csLen(entry) == u8csLen(filepath) &&
            memcmp(entry[0], filepath[0], u8csLen(filepath)) == 0) {
            found = YES;
            break;
        }
        pid++;
        pscan[0] = (nul < pscan[1]) ? nul + 1 : pscan[1];
    }
    FILEUnMap(pmap);
    if (!found) return 0;

    // Scan all PATH_VER entries in the index for this path_id.
    // PATH_VER: a = Pack(DAG_PATH_VER, gen, path_id), b = Pack(_, gen, blob_hashlet)
    u32 count = 0;
    for (u32 r = 0; r < idx->n && count < maxvers; r++) {
        belt128cp base = idx->runs[r][0];
        size_t len = (size_t)(idx->runs[r][1] - base);
        for (size_t i = 0; i < len && count < maxvers; i++) {
            if (DAGType(base[i].a) != DAG_PATH_VER) continue;
            u64 entry_pid = DAGHashlet(base[i].a);
            if (entry_pid != (u64)pid) continue;

            u32 gen = DAGGen(base[i].b);
            u64 blob_h = DAGHashlet(base[i].b);

            // Look up BLOB_COMMIT to get commit hashlet
            u64 commit_h = 0;
            belt128cp bc = DAGLookup(idx, DAG_BLOB_COMMIT, blob_h);
            if (bc) commit_h = DAGHashlet(bc->b);

            vers[count].blob_hashlet = blob_h;
            vers[count].commit_hashlet = commit_h;
            vers[count].gen = gen;
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

ok64 GRAFBlame(keeper *k, u8cs filepath, u8cs reporoot) {
    sane(k && $ok(filepath) && $ok(reporoot));

    call(GRAFArenaInit);

    // Open DAG index
    graf g = {};
    call(GRAFOpen, &g, reporoot, NO);

    // Walk file history via DAG index
    a_cstr(dagdir, g.dir);
    blame_ver vers[BLAME_MAX_VERS];
    u32 nvers = blame_walk_history(vers, BLAME_MAX_VERS, &g.idx,
                                    filepath, dagdir);

    // Reverse to oldest first, then deduplicate (keep first = oldest)
    for (u32 i = 0; i < nvers / 2; i++) {
        blame_ver tmp = vers[i];
        vers[i] = vers[nvers - 1 - i];
        vers[nvers - 1 - i] = tmp;
    }
    {
        u32 w = 0;
        for (u32 i = 0; i < nvers; i++) {
            b8 dup = NO;
            for (u32 j = 0; j < w; j++)
                if (vers[j].blob_hashlet == vers[i].blob_hashlet) { dup = YES; break; }
            if (!dup) vers[w++] = vers[i];
        }
        nvers = w;
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

    for (u32 i = 0; i < nvers; i++) {
        u8bReset(*cur_blob);
        u8 bt = 0;
        ok64 fo = KEEPGet(k, vers[i].blob_hashlet, 15, *cur_blob, &bt);
        if (fo != OK || bt != DOG_OBJ_BLOB) continue;

        if (vers[i].commit_hashlet && nauthors < BLAME_MAX_AUTHORS) {
            authors[nauthors].gen = vers[i].gen;
            authors[nauthors].commit_hashlet = vers[i].commit_hashlet;
            blame_fetch_author(&authors[nauthors], k,
                               vers[i].commit_hashlet);
            nauthors++;
        }

        u8cs old_data = {};
        if (i > 0) {
            old_data[0] = u8bDataHead(*prev_blobp);
            old_data[1] = u8bDataHead(*prev_blobp) + u8bDataLen(*prev_blobp);
        }
        u8cs new_data = {u8bDataHead(*cur_blob),
                         u8bDataHead(*cur_blob) + u8bDataLen(*cur_blob)};

        call(WEAVEAdd, &wv, old_data, new_data, ext, vers[i].gen);

        // Swap
        Bu8 *tmp = cur_blob; cur_blob = prev_blobp; prev_blobp = tmp;
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
                    char full[12];
                    DAGHashletToHex(full, ba->commit_hashlet);
                    memcpy(hexlet, full, BLAME_HW);
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
    GRAFClose(&g);
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
