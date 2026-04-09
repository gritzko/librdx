//  BLAME: token-level blame via weave + DAG index.
//
//  Walks the PREV_BLOB chain, builds a weave, then renders
//  blame annotations per line.  Emits hunks via GRAFHunkEmit
//  (TLV to bro on tty, plain text when piped).
//
#include "GRAF.h"
#include "DAG.h"
#include "WEAVE.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/UTF8.h"
#include "dog/HUNK.h"

// --- Blob version in the chain ---

typedef struct {
    u64 blob_hashlet;
    u64 commit_hashlet;
    u32 gen;
} blame_ver;

#define BLAME_MAX_VERS 4096
#define BLAME_MAX_AUTHORS 256

// --- Author table: gen → author + date ---

typedef struct {
    u32  gen;
    u64  commit_hashlet;
    char author[48];
    char date[12];   // YYYY-MM-DD
} blame_author;

// --- UTF-8 aware fixed-width field: truncate to N codepoints, pad right ---

// Write src into out, truncated to maxcols codepoints, padded with
// spaces to exactly maxcols, followed by a trailing string `after`.
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
    // iso_date is "YYYY-MM-DD" or empty
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

// --- Fetch blob bytes from git ---

static ok64 blame_fetch_blob(u8bp buf, u64 blob_hashlet, u8cs reporoot) {
    sane(buf && $ok(reporoot));
    char prefix[12];
    DAGHashletToHex(prefix, blob_hashlet);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "git -C %.*s cat-file blob %s 2>/dev/null",
             (int)$len(reporoot), (char *)reporoot[0], prefix);
    FILE *fp = popen(cmd, "r");
    if (!fp) fail(WEAVEFAIL);

    char chunk[8192];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        u8cs data = {(u8cp)chunk, (u8cp)chunk + n};
        call(u8bFeed, buf, data);
    }
    pclose(fp);
    done;
}

// --- Fetch author + date from commit ---

static void blame_fetch_author(blame_author *ba,
                                u64 commit_hashlet, u8cs reporoot) {
    ba->author[0] = 0;
    ba->date[0] = 0;
    char prefix[12];
    DAGHashletToHex(prefix, commit_hashlet);

    // Fetch "author\ndate" in one call
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "git -C %.*s log --format='%%an%%n%%as' %s -1 2>/dev/null",
             (int)$len(reporoot), (char *)reporoot[0], prefix);
    FILE *fp = popen(cmd, "r");
    if (!fp) return;
    char lbuf[128];
    // Line 1: author name
    if (fgets(lbuf, sizeof(lbuf), fp)) {
        size_t l = strlen(lbuf);
        while (l > 0 && (lbuf[l - 1] == '\n' || lbuf[l - 1] == '\r'))
            lbuf[--l] = 0;
        if (l >= sizeof(ba->author)) l = sizeof(ba->author) - 1;
        memcpy(ba->author, lbuf, l);
        ba->author[l] = 0;
    }
    // Line 2: date (YYYY-MM-DD)
    if (fgets(lbuf, sizeof(lbuf), fp)) {
        size_t l = strlen(lbuf);
        while (l > 0 && (lbuf[l - 1] == '\n' || lbuf[l - 1] == '\r'))
            lbuf[--l] = 0;
        if (l >= sizeof(ba->date)) l = sizeof(ba->date) - 1;
        memcpy(ba->date, lbuf, l);
        ba->date[l] = 0;
    }
    pclose(fp);
}

// --- Walk PREV_BLOB chain from the index ---

static u32 blame_walk_chain(blame_ver *vers, u32 maxvers,
                             dag_stack const *st,
                             u64 start_blob, u32 start_gen) {
    u32 n = 0;
    u64 bh = start_blob;
    u32 gen = start_gen;

    while (n < maxvers) {
        vers[n].blob_hashlet = bh;
        vers[n].gen = gen;
        vers[n].commit_hashlet = 0;

        belt128cp bc = DAGLookup(st, DAG_BLOB_COMMIT, bh);
        if (bc) vers[n].commit_hashlet = DAGHashlet(bc->b);
        n++;

        // Walk backward: find PREV_BLOB with b.gen < current gen
        belt128cp prev = DAGLookup(st, DAG_PREV_BLOB, bh);
        if (!prev) break;
        u32 pgen = DAGGen(prev->b);
        if (pgen >= gen) break;  // must go backward
        bh = DAGHashlet(prev->b);
        gen = pgen;
    }
    return n;
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

ok64 GRAFBlame(u8cs filepath, u8cs reporoot) {
    sane($ok(filepath) && $ok(reporoot));

    call(GRAFArenaInit);

    // Open DAG index
    a_path(dagpath, reporoot);
    a_cstr(dagrel, "/.dogs/graf");
    call(u8bFeed, dagpath, dagrel);
    call(PATHu8gTerm, PATHu8gIn(dagpath));
    a_dup(u8c, dagdir, u8bDataC(dagpath));

    dag_stack st = {};
    call(dag_stack_open, &st, dagdir);

    // Get current blob for the file: git ls-tree HEAD -- <file>
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "git -C %.*s ls-tree HEAD -- %.*s",
             (int)$len(reporoot), (char *)reporoot[0],
             (int)$len(filepath), (char *)filepath[0]);
    FILE *fp = popen(cmd, "r");
    if (!fp) { dag_stack_close(&st); fail(DAGFAIL); }
    char lbuf[512];
    char *got = fgets(lbuf, sizeof(lbuf), fp);
    pclose(fp);
    if (!got) { dag_stack_close(&st); fail(DAGFAIL); }

    // Parse: "mode type sha\tpath"
    char *tab = strchr(lbuf, '\t');
    if (!tab) { dag_stack_close(&st); fail(DAGFAIL); }
    // Find the sha (third space-separated field)
    char *p = lbuf;
    while (*p && *p != ' ') p++; // skip mode
    while (*p == ' ') p++;
    while (*p && *p != ' ') p++; // skip type
    while (*p == ' ') p++;
    // p now points to sha
    if (p + 40 > tab) { dag_stack_close(&st); fail(DAGFAIL); }

    sha1 head_blob = {};
    DAGsha1FromHex(&head_blob, p);
    u64 head_bh = DAGsha1Hashlet(&head_blob);

    // Find gen for HEAD blob via BLOB_COMMIT
    u32 head_gen = 0;
    belt128cp bc = DAGLookup(&st, DAG_BLOB_COMMIT, head_bh);
    if (bc) head_gen = DAGGen(bc->a);

    // Walk PREV_BLOB chain
    blame_ver vers[BLAME_MAX_VERS];
    u32 nvers = blame_walk_chain(vers, BLAME_MAX_VERS, &st, head_bh, head_gen);

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
        call(blame_fetch_blob, *cur_blob, vers[i].blob_hashlet, reporoot);

        if (vers[i].commit_hashlet && nauthors < BLAME_MAX_AUTHORS) {
            authors[nauthors].gen = vers[i].gen;
            authors[nauthors].commit_hashlet = vers[i].commit_hashlet;
            blame_fetch_author(&authors[nauthors],
                               vers[i].commit_hashlet, reporoot);
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

    dag_stack_close(&st);

    // Render blame: "hashlet name date code"
    //   7-char hashlet (greenish grey), 12-char name (bluish grey),
    //   5-char date (light grey), then code. On author change show all,
    //   on continuation blank prefix.
    #define BLAME_HW 7   // hashlet width
    #define BLAME_NW 12  // name width
    #define BLAME_DW 5   // date width
    #define BLAME_PW (BLAME_HW + 1 + BLAME_NW + 1 + BLAME_DW + 1) // "hashlet name date "
    #define CLR_HASH "\033[38;5;108m"  // greenish grey
    #define CLR_NAME "\033[38;5;103m"  // bluish grey
    #define CLR_DATE "\033[38;5;245m"  // light grey
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

            char pre[128];
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
                // Build fixed-width fields
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

        // Emit token text, tracking newlines
        while (tp < te) {
            u8cp nl = tp;
            while (nl < te && *nl != '\n') nl++;
            if (nl < te) {
                u8cs chunk = {tp, nl + 1};
                u8bFeed(outbuf, chunk);
                tp = nl + 1;
                at_bol = YES;
                // If more text follows in this token, emit blank prefix
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

    // Build hunk and emit
    // Emit blame as a single hunk (no syntax highlighting on annotated text)
    {
        char title[128];
        snprintf(title, sizeof(title), "--- %.*s (blame) ---",
                 (int)$len(filepath), (char *)filepath[0]);

        u8cs fdata = {u8bDataHead(outbuf),
                      u8bDataHead(outbuf) + u8bDataLen(outbuf)};
        hunk hk = {};
        hk.title[0] = (u8cp)title;
        hk.title[1] = (u8cp)title + strlen(title);
        $mv(hk.text, fdata);
        call(GRAFHunkEmit, &hk, NULL);
    }

    u8bUnMap(outbuf);
    WEAVEFree(&wv);
    GRAFArenaCleanup();
    done;
}
