#include "BRO.h"

#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "MAUS.h"
#include "abc/ANSI.h"
#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/TTY.h"
#include "abc/UTF8.h"
#include "dog/DEF.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"
#include "dog/TOK.h"

b8 BRO_COLOR = YES;

// --- BRO arena state ---
Bu8 bro_arena = {};
hunk bro_hunks[BRO_MAX_HUNKS];
u8bp bro_maps[BRO_MAX_MAPS];
Bu32 bro_toks[BRO_MAX_MAPS];
u32 bro_nhunks = 0;
u32 bro_nmaps = 0;

ok64 BROArenaInit(void) {
    bro_nhunks = 0;
    bro_nmaps = 0;
    memset(bro_hunks, 0, sizeof(bro_hunks));
    memset(bro_maps, 0, sizeof(bro_maps));
    memset(bro_toks, 0, sizeof(bro_toks));
    if (bro_arena[0] != NULL) {
        // Reset idle pointer to start
        ((u8 **)bro_arena)[2] = bro_arena[1];
        return OK;
    }
    return u8bMap(bro_arena, BRO_ARENA_SIZE);
}

void BROArenaCleanup(void) {
    for (u32 i = 0; i < bro_nmaps; i++) {
        if (bro_toks[i][0] != NULL) u32bUnMap(bro_toks[i]);
        if (bro_maps[i] != NULL) FILEUnMap(bro_maps[i]);
    }
    bro_nhunks = 0;
    bro_nmaps = 0;
}

// Write bytes into the arena, return pointer to start
u8p BROArenaWrite(void const *data, size_t len) {
    if (u8bIdleLen(bro_arena) < len) return NULL;
    u8p p = u8bIdleHead(bro_arena);
    memcpy(p, data, len);
    u8bFed(bro_arena, len);
    return p;
}

// Defer file+toks cleanup until after BRORun
void BRODefer(u8bp mapped, Bu32 toks) {
    if (bro_nmaps >= BRO_MAX_MAPS) return;
    bro_maps[bro_nmaps] = mapped;
    memcpy(bro_toks[bro_nmaps], toks, sizeof(Bu32));
    bro_nmaps++;
}

// Bump bro_nhunks after caller filled bro_hunks[bro_nhunks].
void BROHunkAdd(void) {
    bro_nhunks++;
}

// 256-color ink violet for hunk titles
#define BRO_TITLE_COLOR TTY_FG256(56)

// Tag-to-ANSI-color mapping.
// Returns fg color; sets *bold = YES for definitions.
static int BROTagColor(u8 tag, b8 *bold) {
    *bold = NO;
    switch (tag) {
        case 'D': return GRAY;
        case 'G': return DARK_GREEN;
        case 'L': return LIGHT_CYAN;
        case 'H': return DARK_PINK;
        case 'R': return LIGHT_BLUE;
        case 'N': *bold = YES; return 0;
        default:  return 0;
    }
}

// --- Line index ---
// Maps line number -> (hunk index, byte offset within hunk text).
// A "line" is range32: lo=hunk index, hi=byte offset within hunk text.
// A title separator is stored as hunk index with hi=UINT32_MAX.

// A hunk has a displayable title if it has a path or a function name.
#define hunk_has_title(hk) (!$empty((hk)->path) || !$empty((hk)->title))

// --- View stack for file navigation ---
#define BRO_MAX_VIEWS 32

// Saved state of the main view when a file is opened.
typedef struct {
    hunk const *hunks;
    u32 nhunks;
    range32 *lines;
    Brange32 linesbuf;
    u32 nlines;
    u32 scroll;
} BROsave;

// Resources owned by a file view (one opened file).
typedef struct {
    u8bp mapped;       // mmap'd file
    Bu32 toks;         // tok buffer
    hunk hunk;      // inline hunk (title + text + toks, no hili)
} BROfileview;

typedef struct {
    hunk const *hunks;
    u32 nhunks;
    range32 *lines;    // line index array (heap buffer)
    Brange32 linesbuf; // buffer owning lines memory
    u32 nlines;        // total line count
    u32 scroll;        // first visible line
    u16 rows, cols;    // terminal dimensions
    char search[256];  // current search pattern
    u32 search_len;    // length of search pattern
    struct termios orig_termios;  // saved terminal state
    int tty_fd;        // /dev/tty for keyboard (stdin may be data pipe)
    b8 raw_mode;       // whether terminal is in raw mode
    b8 mouse_on;       // mouse tracking active (toggled with 'm')
    char flash[128];   // one-shot status bar message (cleared after display)
    // View stack
    BROsave saves[BRO_MAX_VIEWS];
    BROfileview files[BRO_MAX_VIEWS];
    int nsaves;
} BROstate;

// Global for signal handler
static volatile sig_atomic_t bro_resized = 0;

static void bro_winch_handler(int sig) {
    (void)sig;
    bro_resized = 1;
}

// --- Terminal setup ---

static ok64 BRORawEnable(BROstate *st) {
    sane(st != NULL);
    // Open /dev/tty so keyboard input works even when stdin is a data pipe.
    if (st->tty_fd < 0) {
        st->tty_fd = open("/dev/tty", O_RDWR | O_NOCTTY);
        if (st->tty_fd < 0) fail(FAILSANITY);
    }
    if (tcgetattr(st->tty_fd, &st->orig_termios) < 0)
        fail(FAILSANITY);
    struct termios raw = st->orig_termios;
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= (tcflag_t)~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= (tcflag_t)~(OPOST);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;  // 100ms timeout
    if (tcsetattr(st->tty_fd, TCSAFLUSH, &raw) < 0)
        fail(FAILSANITY);
    st->raw_mode = YES;
    done;
}

static void BRORawDisable(BROstate *st) {
    if (st->raw_mode) {
        tcsetattr(st->tty_fd, TCSAFLUSH, &st->orig_termios);
        st->raw_mode = NO;
    }
    if (st->tty_fd >= 0) {
        close(st->tty_fd);
        st->tty_fd = -1;
    }
}

static void BROGetSize(BROstate *st) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) {
        st->rows = ws.ws_row;
        st->cols = ws.ws_col;
    } else {
        st->rows = 24;
        st->cols = 80;
    }
}

// Forward declarations for functions used before definition.
static u32 BROAppendLines(range32 *lines, u32 nlines, u32 maxlines,
                          hunkc const *hunks, u32 from, u32 nhunks);
static void BROScrollCenter(BROstate *st, u32 target);

// --- Build line index ---

static ok64 BROBuildIndex(BROstate *st) {
    sane(st != NULL && st->hunks != NULL);

    // Count lines to size the allocation.
    u32 total = 0;
    for (u32 h = 0; h < st->nhunks; h++) {
        if (hunk_has_title(&st->hunks[h])) total++;
        u32 tlen = (u32)$len(st->hunks[h].text);
        if (tlen == 0) continue;
        total++;  // at least one line
        $for(u8c, c, st->hunks[h].text) {
            if (*c == '\n') total++;
        }
        if (st->hunks[h].text[0][tlen - 1] == '\n') total--;
    }

    ok64 lo = range32bAlloc(st->linesbuf, total);
    if (lo != OK) fail(NOROOM);
    st->lines = st->linesbuf[0];
    st->nlines = BROAppendLines(st->lines, 0, total,
                                st->hunks, 0, st->nhunks);
    done;
}

// --- Tokenize helper (shared by CAT.c and BROOpenFile) ---
// Tokenize source into toks buffer; set hk->toks on success.
// toks buffer is allocated here (caller must u32bUnMap on cleanup).
// Returns YES if tokenized, NO otherwise (unknown ext, alloc fail, etc).
b8 BROTokenize(Bu32 toks, hunk *hk, u8csc pathslice) {
    u8cs ext = {};
    HUNKu8sExt(ext, pathslice[0], (size_t)$len(pathslice));
    u8cs ext_nodot = {};
    if (!$empty(ext) && ext[0][0] == '.') {
        ext_nodot[0] = ext[0] + 1;
        ext_nodot[1] = ext[1];
    }
    if ($empty(ext_nodot) || !TOKKnownExt(ext_nodot)) return NO;
    u32 srclen = (u32)$len(hk->text);
    if (u32bMap(toks, srclen + 1) != OK) return NO;
    u8cs source = {hk->text[0], hk->text[1]};
    if (HUNKu32bTokenize(toks, source, ext) != OK) {
        u32bUnMap(toks);
        memset(toks, 0, sizeof(Bu32));
        return NO;
    }
    u32 *dts[2] = {u32bDataHead(toks), u32bIdleHead(toks)};
    DEFMark(dts, source, ext_nodot);
    hk->toks[0] = (u32cp)u32bDataHead(toks);
    hk->toks[1] = (u32cp)u32bIdleHead(toks);
    return YES;
}

// --- File view open / back ---

// Open a file by repo-relative path, push current view onto stack.
// `repo` is the NUL-terminated repo root path.  Returns OK if the
// file was opened (st is updated in place); non-OK on error (st unchanged).
static ok64 BROOpenFile(BROstate *st, u8csc relpath, char const *repo,
                        u32 target_line) {
    sane(st != NULL && !$empty(relpath) && repo != NULL);
    if (st->nsaves >= BRO_MAX_VIEWS) fail(NOROOM);

    // Build absolute path: repo/relpath
    char abspath[FILE_PATH_MAX_LEN];
    int apn = snprintf(abspath, sizeof(abspath), "%s/%.*s",
                       repo, (int)$len(relpath), (char *)relpath[0]);
    if (apn <= 0 || (size_t)apn >= sizeof(abspath)) fail(FAILSANITY);

    // Feed into path buffer for FILEMapRO
    a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
    {
        u8cs abs = {(u8cp)abspath, (u8cp)abspath + apn};
        ok64 fo = u8bFeed(fpbuf, abs);
        if (fo != OK) fail(fo);
        fo = PATHu8gTerm(PATHu8gIn(fpbuf));
        if (fo != OK) fail(fo);
    }

    // Map file
    u8bp mapped = NULL;
    ok64 mo = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
    if (mo != OK) fail(mo);

    int idx = st->nsaves;
    BROfileview *fv = &st->files[idx];
    *fv = (BROfileview){};
    fv->mapped = mapped;

    u8cp src_head = u8bDataHead(mapped);
    u8cp src_idle = u8bIdleHead(mapped);

    fv->hunk = (hunk){};
    fv->hunk.text[0] = src_head;
    fv->hunk.text[1] = src_idle;

    // Copy path into arena (relpath may point into pipe buffer or old hunk)
    u8p pp = BROArenaWrite(relpath[0], (size_t)$len(relpath));
    if (pp) {
        fv->hunk.path[0] = pp;
        fv->hunk.path[1] = pp + $len(relpath);
    }

    BROTokenize(fv->toks, &fv->hunk, relpath);

    // Save current view
    BROsave *sv = &st->saves[idx];
    sv->hunks = st->hunks;
    sv->nhunks = st->nhunks;
    sv->lines = st->lines;
    memcpy(sv->linesbuf, st->linesbuf, sizeof(Brange32));
    sv->nlines = st->nlines;
    sv->scroll = st->scroll;

    // Switch to file view
    st->hunks = &fv->hunk;
    st->nhunks = 1;
    memset(st->linesbuf, 0, sizeof(Brange32));
    call(BROBuildIndex, st);
    // Scroll to target line (1-based file line number).
    // Line index entry i corresponds to file-line = lineno for offset 0,
    // incremented per newline. Scan for the closest match.
    if (target_line > 1 && st->nlines > 1) {
        // The file hunk starts at lineno=1. Each line index entry after
        // the title adds one file line. Find the entry for target_line.
        u32 file_ln = 1;
        u32 best = 1;  // skip title
        for (u32 i = 1; i < st->nlines; i++) {
            if (st->lines[i].hi == BRO_TITLE_LINE) continue;
            if (file_ln >= target_line) { best = i; break; }
            file_ln++;
            best = i;
        }
        BROScrollCenter(st, best);
    } else {
        st->scroll = (st->nlines > 1) ? 1 : 0;
    }
    st->nsaves = idx + 1;
    done;
}

// Go back to the previous view. Frees the current file view's resources.
static b8 BROBack(BROstate *st) {
    if (st->nsaves <= 0) return NO;
    st->nsaves--;
    int idx = st->nsaves;

    // Free file view resources
    BROfileview *fv = &st->files[idx];
    if (fv->toks[0] != NULL) u32bUnMap(fv->toks);
    if (fv->mapped != NULL) FILEUnMap(fv->mapped);
    *fv = (BROfileview){};

    // Free current line index
    range32bFree(st->linesbuf);

    // Restore saved view
    BROsave *sv = &st->saves[idx];
    st->hunks = sv->hunks;
    st->nhunks = sv->nhunks;
    st->lines = sv->lines;
    memcpy(st->linesbuf, sv->linesbuf, sizeof(Brange32));
    st->nlines = sv->nlines;
    st->scroll = sv->scroll;
    return YES;
}

// Try to open the file referenced by the hunk at the given line index.
// Returns YES if a file was opened, NO otherwise.
// On failure, flashes the error briefly on the status bar.
static b8 BROTryOpen(BROstate *st, u32 line, char const *repo) {
    if (line >= st->nlines) return NO;
    u32 hunk_idx = st->lines[line].lo;
    hunk const *hk = &st->hunks[hunk_idx];
    if ($empty(hk->path)) {
        snprintf(st->flash, sizeof(st->flash), "no path in hunk");
        return NO;
    }
    if (repo == NULL || repo[0] == 0) {
        snprintf(st->flash, sizeof(st->flash), "no repo root found");
        return NO;
    }
    ok64 o = BROOpenFile(st, hk->path, repo, hk->lineno);
    if (o != OK) {
        snprintf(st->flash, sizeof(st->flash),
                 "open: %.*s: %s",
                 (int)$len(hk->path), (char *)hk->path[0],
                 ok64str(o));
        return NO;
    }
    return YES;
}

// --- Navigation primitives ---
// These walk the line index built by BROBuildIndex (or
// BROExtendIndex). They are exposed via BRO.h for tests, so they take
// raw arrays rather than BROstate.

static b8 bro_hili_real(u8 tag) {
    return (tag != 0 && tag != 'A') ? YES : NO;
}

// Find the line that contains byte offset `off` in hunk `h`.
// Returns the largest line index i with lines[i].lo == h,
// lines[i].hi != BRO_TITLE_LINE, and lines[i].hi <= off.
// BRO_NONE if no such line exists.
static u32 bro_line_for_off(range32 const *lines, u32 nlines,
                            u32 h, u32 off) {
    u32 best = BRO_NONE;
    for (u32 i = 0; i < nlines; i++) {
        if (lines[i].lo < h) continue;
        if (lines[i].lo > h) break;  // hunks are appended in order
        if (lines[i].hi == BRO_TITLE_LINE) continue;
        if (lines[i].hi <= off) best = i;
        else break;
    }
    return best;
}

u32 BROHunkNextLine(range32 const *lines, u32 nlines, u32 from) {
    if (nlines == 0) return BRO_NONE;
    u32 start = (from + 1 < nlines) ? (from + 1) : nlines;
    for (u32 i = start; i < nlines; i++) {
        if (lines[i].lo != lines[i - 1].lo) return i;
    }
    return BRO_NONE;
}

u32 BROHunkPrevLine(range32 const *lines, u32 nlines, u32 from) {
    if (nlines == 0 || from == 0) return BRO_NONE;
    if (from >= nlines) from = nlines - 1;
    // Walk back to the start line of the current hunk.
    u32 i = from;
    while (i > 0 && lines[i - 1].lo == lines[i].lo) i--;
    if (i < from) return i;  // mid-hunk: jump to start of current
    // Already at the current hunk's start; step into the previous hunk.
    if (i == 0) return BRO_NONE;
    i--;
    while (i > 0 && lines[i - 1].lo == lines[i].lo) i--;
    return i;
}

u32 BROHunkCount(range32 const *lines, u32 nlines) {
    if (nlines == 0) return 0;
    u32 n = 1;
    for (u32 i = 1; i < nlines; i++)
        if (lines[i].lo != lines[i - 1].lo) n++;
    return n;
}

u32 BROHunkIndexAt(range32 const *lines, u32 nlines, u32 at) {
    if (nlines == 0) return 0;
    if (at >= nlines) at = nlines - 1;
    u32 n = 1;
    for (u32 i = 1; i <= at; i++)
        if (lines[i].lo != lines[i - 1].lo) n++;
    return n;
}

u32 BROHiliCount(hunk const *hunks, u32 nhunks) {
    u32 n = 0;
    for (u32 h = 0; h < nhunks; h++) {
        u32 nh = (u32)$len(hunks[h].hili);
        for (u32 j = 0; j < nh; j++)
            if (bro_hili_real(tok32Tag(hunks[h].hili[0][j]))) n++;
    }
    return n;
}

u32 BROHiliIndexAt(hunk const *hunks, u32 nhunks,
                   range32 const *lines, u32 nlines, u32 at) {
    u32 n = 0;
    for (u32 h = 0; h < nhunks; h++) {
        u32 nh = (u32)$len(hunks[h].hili);
        for (u32 j = 0; j < nh; j++) {
            tok32 t = hunks[h].hili[0][j];
            if (!bro_hili_real(tok32Tag(t))) continue;
            u32 start_off = (j == 0) ? 0
                          : tok32Offset(hunks[h].hili[0][j - 1]);
            u32 ln = bro_line_for_off(lines, nlines, h, start_off);
            if (ln == BRO_NONE) continue;
            if (ln <= at) n++;
            else return n;
        }
    }
    return n;
}

u32 BROHiliNextLine(hunk const *hunks, u32 nhunks,
                    range32 const *lines, u32 nlines, u32 mid) {
    for (u32 h = 0; h < nhunks; h++) {
        u32 nh = (u32)$len(hunks[h].hili);
        for (u32 j = 0; j < nh; j++) {
            tok32 t = hunks[h].hili[0][j];
            if (!bro_hili_real(tok32Tag(t))) continue;
            u32 start_off = (j == 0) ? 0
                          : tok32Offset(hunks[h].hili[0][j - 1]);
            u32 ln = bro_line_for_off(lines, nlines, h, start_off);
            if (ln == BRO_NONE) continue;
            if (ln > mid) return ln;
        }
    }
    return BRO_NONE;
}

u32 BROHiliPrevLine(hunk const *hunks, u32 nhunks,
                    range32 const *lines, u32 nlines, u32 mid) {
    u32 best = BRO_NONE;
    for (u32 h = 0; h < nhunks; h++) {
        u32 nh = (u32)$len(hunks[h].hili);
        for (u32 j = 0; j < nh; j++) {
            tok32 t = hunks[h].hili[0][j];
            if (!bro_hili_real(tok32Tag(t))) continue;
            u32 start_off = (j == 0) ? 0
                          : tok32Offset(hunks[h].hili[0][j - 1]);
            u32 ln = bro_line_for_off(lines, nlines, h, start_off);
            if (ln == BRO_NONE) continue;
            if (ln >= mid) return best;
            best = ln;
        }
    }
    return best;
}

// --- Rendering ---
// All screen output is built into bro_scr[] buffer, flushed once.

#define BRO_SCR_SIZE (1UL << 20)  // 1MB screen buffer
static Bu8 bro_scr = {};

static ok64 BROScreenInit(void) {
    if (bro_scr[0] != NULL) { u8bReset(bro_scr); return OK; }
    return u8bMap(bro_scr, BRO_SCR_SIZE);
}

static void BROScreenFlush(void) {
    if (u8bDataLen(bro_scr) > 0)
        (void)write(STDOUT_FILENO, u8bDataHead(bro_scr), u8bDataLen(bro_scr));
    u8bReset(bro_scr);
}

// Feed string literal
static void scr_puts(char const *s) {
    u8sFeed(u8bIdle(bro_scr), (u8csc){(u8cp)s, (u8cp)s + strlen(s)});
}

// Feed goto escape: \033[row;colH
static void scr_goto(int row, int col) {
    a_pad(u8, tmp, 32);
    u8sFeed1(tmp_idle, 033);
    u8sFeed1(tmp_idle, '[');
    utf8sFeed10(tmp_idle, (u64)row);
    u8sFeed1(tmp_idle, ';');
    utf8sFeed10(tmp_idle, (u64)col);
    u8sFeed1(tmp_idle, 'H');
    u8bFeed(bro_scr, u8bDataC(tmp));
}

// Direct write helpers (for prompts/setup, not screen rendering)
static void bro_puts(char const *s) {
    (void)write(STDOUT_FILENO, s, strlen(s));
}
static void bro_write(u8csc s) {
    (void)write(STDOUT_FILENO, s[0], $len(s));
}
static void bro_goto(int row, int col) {
    a_pad(u8, tmp, 32);
    u8sFeed1(tmp_idle, 033);
    u8sFeed1(tmp_idle, '[');
    utf8sFeed10(tmp_idle, (u64)row);
    u8sFeed1(tmp_idle, ';');
    utf8sFeed10(tmp_idle, (u64)col);
    u8sFeed1(tmp_idle, 'H');
    bro_write(u8bDataC(tmp));
}

// Feed one byte with fg tag, bg tag, and search highlight
static void scr_emit_char(u8cp p, u32 n, u8 fg_tag, u8 bg_tag, b8 in_search) {
    b8 bold = NO;
    int fg = BROTagColor(fg_tag, &bold);
    b8 is_ins = (bg_tag == 'I');
    b8 is_del = (bg_tag == 'D');

    u8sp out = u8bIdle(bro_scr);
    if (fg != 0 || bold || is_ins || is_del || in_search) {
        if (bold) escfeed(out, BOLD);
        if (fg != 0) escfeed(out, (u8)fg);
        if (is_ins) escfeedBG256(out, 157);
        else if (is_del) escfeedBG256(out, 217);
        else if (in_search) escfeed(out, 7);
        u8cs chars = {p, p + n};
        u8sFeed(out, chars);
        escfeed(out, 0);  // reset
    } else {
        u8cs chars = {p, p + n};
        u8sFeed(out, chars);
    }
}

// Check if search pattern matches at position pos
static b8 bro_search_at(BROstate *st, u8csc text, u32 pos) {
    if (st->search_len == 0) return NO;
    if (pos + st->search_len > (u32)$len(text)) return NO;
    return memcmp(&text[0][pos], st->search, st->search_len) == 0 ? YES : NO;
}

static void BROStatusBar(BROstate *st);

// Format display title "--- path :: func ---" into buf.
// Returns the number of bytes written (excl NUL).
static int bro_format_title(char *buf, size_t bufsz, hunkc const *hk) {
    char pathz[FILE_PATH_MAX_LEN] = {};
    char funcz[256] = {};
    if (!$empty(hk->path)) {
        size_t pl = (size_t)$len(hk->path);
        if (pl >= sizeof(pathz)) pl = sizeof(pathz) - 1;
        memcpy(pathz, hk->path[0], pl);
    }
    if (!$empty(hk->title)) {
        size_t fl = (size_t)$len(hk->title);
        if (fl >= sizeof(funcz)) fl = sizeof(funcz) - 1;
        memcpy(funcz, hk->title[0], fl);
    }
    // Use the same format as HUNKu8sFormatTitle but into a char buf.
    int n = 0;
    if (pathz[0] && funcz[0])
        n = snprintf(buf, bufsz, "--- %s :: %s ---", pathz, funcz);
    else if (pathz[0])
        n = snprintf(buf, bufsz, "--- %s ---", pathz);
    else if (funcz[0])
        n = snprintf(buf, bufsz, "--- %s ---", funcz);
    if (n < 0) n = 0;
    if ((size_t)n >= bufsz) n = (int)(bufsz - 1);
    return n;
}

static void BRORender(BROstate *st) {
    u8bReset(bro_scr);
    scr_puts(TTY_CUR_HOME);

    u32 visible = (u32)(st->rows - 1);
    if (st->nlines > visible) {
        if (st->scroll > st->nlines - visible)
            st->scroll = st->nlines - visible;
    } else {
        st->scroll = 0;
    }
    u32 end = st->scroll + visible;
    if (end > st->nlines) end = st->nlines;

    for (u32 vi = st->scroll; vi < end; vi++) {
        scr_goto((int)(vi - st->scroll + 1), 1);
        scr_puts(TTY_ERASE_LINE);

        range32 *ln = &st->lines[vi];
        hunk const *hk = &st->hunks[ln->lo];

        if (ln->hi == BRO_TITLE_LINE) {
            scr_puts(BRO_TITLE_COLOR);
            char dtitle[HUNK_TITLE_MAX + 1];
            int dtlen = bro_format_title(dtitle, sizeof(dtitle), hk);
            u32 w = (u32)dtlen < st->cols ? (u32)dtlen : st->cols;
            u8cs ttl = {(u8cp)dtitle, (u8cp)dtitle + w};
            u8sFeed(u8bIdle(bro_scr), ttl);
            scr_puts(TTY_RESET);
            continue;
        }

        u32 textlen = (u32)$len(hk->text);
        u32 off = ln->hi;
        u32 line_end = off;
        while (line_end < textlen && hk->text[0][line_end] != '\n')
            line_end++;

        u32 w = line_end - off;
        if (w > st->cols) w = st->cols;

        // Find tok cursor for this offset
        int ntoks = (int)$len(hk->toks);
        int tok_i = 0;
        while (tok_i < ntoks &&
               tok32Offset(hk->toks[0][tok_i]) <= off)
            tok_i++;

        int nhili = (int)$len(hk->hili);
        int hili_i = 0;
        while (hili_i < nhili &&
               tok32Offset(hk->hili[0][hili_i]) <= off)
            hili_i++;

        // Carry-over byte counter so a multi-byte search match stays
        // highlighted across the whole run, not just its first char.
        u32 search_left = 0;
        for (u32 j = 0; j < w; ) {
            u32 pos = off + j;
            u8 ch = hk->text[0][pos];
            u32 clen = UTF8_LEN[ch >> 4];
            if (clen > w - j) clen = w - j;  // clamp to line end
            // Advance cursors past pos
            while (tok_i < ntoks &&
                   tok32Offset(hk->toks[0][tok_i]) <= pos)
                tok_i++;
            u8 fg_tag = (tok_i < ntoks)
                            ? tok32Tag(hk->toks[0][tok_i]) : 'S';
            while (hili_i < nhili &&
                   tok32Offset(hk->hili[0][hili_i]) <= pos)
                hili_i++;
            u8 bg_tag = (hili_i < nhili)
                            ? tok32Tag(hk->hili[0][hili_i]) : 'A';
            if (search_left == 0 && bro_search_at(st, hk->text, pos))
                search_left = st->search_len;
            b8 in_search = (search_left > 0) ? YES : NO;
            scr_emit_char(hk->text[0] + pos, clen, fg_tag, bg_tag, in_search);
            if (search_left >= clen) search_left -= clen;
            else search_left = 0;
            j += clen;
        }
    }

    for (u32 row = end - st->scroll + 1; row < st->rows; row++) {
        scr_goto((int)row, 1);
        scr_puts(TTY_ERASE_LINE);
    }

    BROStatusBar(st);
    BROScreenFlush();
}

// Set st->scroll so that line `target` lands on the middle row.
// Clamps to [0, nlines - page].
static void BROScrollCenter(BROstate *st, u32 target) {
    u32 page = (st->rows > 1) ? (u32)(st->rows - 1) : 1;
    u32 half = page / 2;
    u32 s = (target > half) ? (target - half) : 0;
    if (st->nlines > page && s > st->nlines - page)
        s = st->nlines - page;
    st->scroll = s;
}

static void BROStatusBar(BROstate *st) {
    scr_goto(st->rows, 1);
    scr_puts(TTY_INVERSE TTY_ERASE_LINE);

    // Flash message: show it and clear for next render.
    if (st->flash[0]) {
        u8sp out = u8bIdle(bro_scr);
        int slen = snprintf((char *)out[0], $len(out),
                            " %s", st->flash);
        if (slen > 0) {
            if ((u32)slen > st->cols) slen = (int)st->cols;
            u8sFed(out, (size_t)slen);
        }
        scr_puts(TTY_RESET);
        st->flash[0] = 0;
        return;
    }

    u32 cur_hunk = 0;
    if (st->scroll < st->nlines)
        cur_hunk = st->lines[st->scroll].lo;

    u32 hunk_tot = BROHunkCount(st->lines, st->nlines);
    u32 hunk_idx = BROHunkIndexAt(st->lines, st->nlines, st->scroll);
    u32 hili_tot = BROHiliCount(st->hunks, st->nhunks);
    u32 hili_idx = BROHiliIndexAt(st->hunks, st->nhunks,
                                  st->lines, st->nlines, st->scroll);

    hunk const *ch = &st->hunks[cur_hunk];

    // Build the right-side stats first so we know how much room the
    // title gets.  Use a small local buffer (stats are short).
    char stats[128];
    int sn;
    if (hili_tot > 0 && st->search_len > 0)
        sn = snprintf(stats, sizeof(stats),
                      "  L%u/%u  H%u/%u  C%u/%u  [/%.*s]",
                      st->scroll + 1, st->nlines,
                      hunk_idx, hunk_tot, hili_idx, hili_tot,
                      (int)st->search_len, st->search);
    else if (hili_tot > 0)
        sn = snprintf(stats, sizeof(stats),
                      "  L%u/%u  H%u/%u  C%u/%u",
                      st->scroll + 1, st->nlines,
                      hunk_idx, hunk_tot, hili_idx, hili_tot);
    else if (st->search_len > 0)
        sn = snprintf(stats, sizeof(stats),
                      "  L%u/%u  H%u/%u  [/%.*s]",
                      st->scroll + 1, st->nlines,
                      hunk_idx, hunk_tot,
                      (int)st->search_len, st->search);
    else
        sn = snprintf(stats, sizeof(stats),
                      "  L%u/%u  H%u/%u",
                      st->scroll + 1, st->nlines,
                      hunk_idx, hunk_tot);
    if (sn < 0) sn = 0;
    if ((size_t)sn >= sizeof(stats)) sn = (int)(sizeof(stats) - 1);

    // Format display title for current hunk.
    char dtitle[HUNK_TITLE_MAX + 1];
    int dtlen = bro_format_title(dtitle, sizeof(dtitle), ch);

    // Truncate title to fit: " <title><stats>" within cols.
    u32 cols = st->cols;
    u32 stats_w = (u32)sn;
    u32 title_max = (cols > stats_w + 2) ? (cols - stats_w - 1) : 0;
    u32 title_len = (u32)dtlen;
    if (title_len > title_max) title_len = title_max;

    // Layout: " <title><padding><stats>" exactly cols wide.
    u32 left_len = 1 + title_len;
    u32 pad = (cols > left_len + stats_w) ? (cols - left_len - stats_w) : 0;

    u8sp out = u8bIdle(bro_scr);
    int slen = snprintf((char *)out[0], $len(out),
                        " %.*s%*s%s",
                        (int)title_len, dtitle,
                        (int)pad, "",
                        stats);
    if (slen > 0) {
        if ((u32)slen > cols) slen = (int)cols;
        u8sFed(out, (size_t)slen);
    }
    scr_puts(TTY_RESET);
}

// --- Search ---

// Find next occurrence of search pattern starting from line `from` (direction=+1)
// or backwards from `from` (direction=-1). Returns line index or UINT32_MAX.
static u32 BROSearchNext(BROstate *st, u32 from, int direction) {
    if (st->search_len == 0) return UINT32_MAX;
    u32 i = from;
    for (;;) {
        if (direction > 0) {
            i++;
            if (i >= st->nlines) return UINT32_MAX;
        } else {
            if (i == 0) return UINT32_MAX;
            i--;
        }
        range32 *ln = &st->lines[i];
        if (ln->hi == BRO_TITLE_LINE) continue;
        hunk const *hk = &st->hunks[ln->lo];
        u32 textlen = (u32)$len(hk->text);
        u32 off = ln->hi;
        u32 line_end = off;
        while (line_end < textlen && hk->text[0][line_end] != '\n')
            line_end++;
        // Search within this line
        u32 w = line_end - off;
        if (w >= st->search_len) {
            u32 limit = w - st->search_len + 1;
            for (u32 j = 0; j < limit; j++) {
                if (memcmp(&hk->text[0][off + j], st->search,
                           st->search_len) == 0)
                    return i;
            }
        }
    }
}

// Read search pattern from user (displayed in status bar)
static void BROReadSearch(BROstate *st) {
    st->search_len = 0;
    memset(st->search, 0, sizeof(st->search));

    for (;;) {
        // Render prompt on status bar
        bro_goto(st->rows, 1);
        bro_puts(TTY_INVERSE TTY_ERASE_LINE);
        bro_puts(" /");
        if (st->search_len > 0) {
            u8cs srch = {(u8cp)st->search, (u8cp)st->search + st->search_len};
            bro_write(srch);
        }
        bro_puts(TTY_RESET);

        u8 ch = 0;
        ssize_t n = read(st->tty_fd, &ch, 1);
        if (n <= 0) continue;
        if (ch == '\n' || ch == '\r') break;  // confirm
        if (ch == 27) {  // escape: cancel
            st->search_len = 0;
            break;
        }
        if (ch == 127 || ch == 8) {  // backspace
            if (st->search_len > 0) st->search_len--;
            continue;
        }
        if (ch >= 32 && st->search_len < sizeof(st->search) - 1) {
            st->search[st->search_len++] = (char)ch;
        }
    }
}

// Read line number from user (displayed in status bar)
static void BROReadGoto(BROstate *st) {
    char buf[32] = {};
    u32 len = 0;

    for (;;) {
        bro_goto(st->rows, 1);
        bro_puts(TTY_INVERSE TTY_ERASE_LINE);
        bro_puts(" :");
        if (len > 0) {
            u8cs bs = {(u8cp)buf, (u8cp)buf + len};
            bro_write(bs);
        }
        bro_puts(TTY_RESET);

        u8 ch = 0;
        ssize_t n = read(st->tty_fd, &ch, 1);
        if (n <= 0) continue;
        if (ch == '\n' || ch == '\r') break;
        if (ch == 27) { len = 0; break; }
        if (ch == 127 || ch == 8) {
            if (len > 0) len--;
            continue;
        }
        if (ch >= '0' && ch <= '9' && len < sizeof(buf) - 1)
            buf[len++] = (char)ch;
    }

    if (len > 0) {
        buf[len] = 0;
        u32 target = (u32)atoi(buf);
        if (target > 0) target--;  // 1-based to 0-based
        if (target >= st->nlines) target = st->nlines > 0 ? st->nlines - 1 : 0;
        st->scroll = target;
    }
}

// --- Fallback: plain output (when piped) ---

static ok64 BROPlain(hunk const *hunks, u32 nhunks) {
    sane(hunks != NULL);
    call(BROScreenInit);
    for (u32 h = 0; h < nhunks; h++) {
        u8bReset(bro_scr);
        if (hunk_has_title(&hunks[h])) {
            char dtitle[HUNK_TITLE_MAX + 1];
            int dtlen = bro_format_title(dtitle, sizeof(dtitle), &hunks[h]);
            if (BRO_COLOR) scr_puts(BRO_TITLE_COLOR);
            u8cs dts = {(u8cp)dtitle, (u8cp)dtitle + dtlen};
            u8bFeed(bro_scr, dts);
            if (BRO_COLOR) scr_puts(TTY_RESET);
            u8sFeed1(u8bIdle(bro_scr), '\n');
        }
        if (!$empty(hunks[h].text)) {
            u32 tlen = (u32)$len(hunks[h].text);
            if (!BRO_COLOR) {
                // No colors: plain text
                u8bFeed(bro_scr, hunks[h].text);
            } else {
                int ntoks = (int)$len(hunks[h].toks);
                int nhili = (int)$len(hunks[h].hili);
                int tok_i = 0, hili_i = 0;
                int prev_fg = 0;
                int prev_bg = 0;
                b8 prev_bold = NO;
                for (u32 i = 0; i < tlen; i++) {
                    // Advance tok cursor
                    while (tok_i < ntoks &&
                           tok32Offset(hunks[h].toks[0][tok_i]) <= i)
                        tok_i++;
                    u8 fg_tag = (tok_i < ntoks)
                                    ? tok32Tag(hunks[h].toks[0][tok_i])
                                    : 'S';
                    // Advance hili cursor
                    while (hili_i < nhili &&
                           tok32Offset(hunks[h].hili[0][hili_i]) <= i)
                        hili_i++;
                    u8 bg_tag = (hili_i < nhili)
                                    ? tok32Tag(hunks[h].hili[0][hili_i])
                                    : 'A';
                    b8 bold = NO;
                    int fg = BROTagColor(fg_tag, &bold);
                    int bg = 0;
                    if (bg_tag == 'I') bg = 157;
                    else if (bg_tag == 'D') bg = 217;
                    u8sp out = u8bIdle(bro_scr);
                    if (fg != prev_fg || bg != prev_bg ||
                        bold != prev_bold) {
                        if (prev_fg != 0 || prev_bg != 0 || prev_bold)
                            escfeed(out, 0);  // reset
                        if (bold) escfeed(out, BOLD);
                        if (fg != 0) escfeed(out, (u8)fg);
                        if (bg != 0) escfeedBG256(out, (u8)bg);
                        prev_fg = fg;
                        prev_bg = bg;
                        prev_bold = bold;
                    }
                    u8sFeed1(out, hunks[h].text[0][i]);
                    if (hunks[h].text[0][i] == '\n' &&
                        (prev_fg != 0 || prev_bg != 0 || prev_bold)) {
                        escfeed(out, 0);  // reset at EOL
                        prev_fg = 0;
                        prev_bg = 0;
                        prev_bold = NO;
                    }
                }
                if (prev_fg != 0 || prev_bg != 0 || prev_bold)
                    escfeed(u8bIdle(bro_scr), 0);
            }
            // Trailing newline if text doesn't end with one
            if (tlen > 0 && hunks[h].text[0][tlen - 1] != '\n')
                u8sFeed1(u8bIdle(bro_scr), '\n');
            if (h + 1 >= nhunks || hunk_has_title(&hunks[h + 1]))
                u8sFeed1(u8bIdle(bro_scr), '\n');
        }
        // Flush per hunk (hunks can be large)
        BROScreenFlush();
    }
    done;
}

// --- Spot invocation ---

// Resolve spot binary path (lazy, cached).
static char bro_spot_path[FILE_PATH_MAX_LEN] = {};
static void bro_resolve_spot(void) {
    if (bro_spot_path[0]) return;
    a$rg(a0, 0);
    HOMEResolveSibling(bro_spot_path, sizeof(bro_spot_path),
                       "spot", (char const *)a0[0]);
}

// Collect unique words from hunk text matching a prefix.
// Words = runs of [a-zA-Z0-9_] starting with [a-zA-Z_].
// Returns count of matches written to out[0..maxout).
static int bro_collect_words(hunkc const *hunks, u32 nhunks,
                             char const *prefix, int pfxlen,
                             char out[][64], int maxout) {
    int n = 0;
    for (u32 h = 0; h < nhunks && n < maxout; h++) {
        u32 tlen = (u32)$len(hunks[h].text);
        if (tlen == 0) continue;
        u8cp txt = hunks[h].text[0];
        u32 i = 0;
        while (i < tlen && n < maxout) {
            if (!isalpha(txt[i]) && txt[i] != '_') { i++; continue; }
            u32 ws = i;
            while (i < tlen && (isalnum(txt[i]) || txt[i] == '_')) i++;
            u32 wlen = i - ws;
            if (wlen < 2 || wlen >= 64) continue;
            if ((int)wlen < pfxlen) continue;
            if (pfxlen > 0 && memcmp(txt + ws, prefix, (size_t)pfxlen) != 0)
                continue;
            // Dedup
            char word[64];
            memcpy(word, txt + ws, wlen);
            word[wlen] = 0;
            b8 dup = NO;
            for (int j = 0; j < n; j++)
                if (strcmp(out[j], word) == 0) { dup = YES; break; }
            if (!dup) {
                memcpy(out[n], word, wlen + 1);
                n++;
            }
        }
    }
    return n;
}

// Interactive spot prompt with Tab completion.
// Returns the accepted token in buf (NUL-terminated), or buf[0]==0 on cancel.
static void BROReadSpot(BROstate *st, char *buf, int bufsz, b8 file_scoped) {
    int len = 0;
    buf[0] = 0;

    // Completion state
    char matches[256][64];
    int nmatch = 0, match_idx = -1;
    b8 matches_valid = NO;

    for (;;) {
        // Render prompt
        bro_goto(st->rows, 1);
        bro_puts(TTY_INVERSE TTY_ERASE_LINE);
        bro_puts(file_scoped ? " spot> " : " SPOT> ");
        if (len > 0) {
            u8cs bs = {(u8cp)buf, (u8cp)buf + len};
            bro_write(bs);
        }
        bro_puts(TTY_RESET);

        u8 ch = 0;
        ssize_t nr = read(st->tty_fd, &ch, 1);
        if (nr <= 0) continue;
        if (ch == '\n' || ch == '\r') break;
        if (ch == 27) { len = 0; buf[0] = 0; break; }
        if (ch == 127 || ch == 8) {
            if (len > 0) { len--; buf[len] = 0; matches_valid = NO; }
            continue;
        }
        if (ch == '\t') {
            // Tab completion
            if (!matches_valid) {
                nmatch = bro_collect_words(st->hunks, st->nhunks,
                                           buf, len, matches, 256);
                match_idx = -1;
                matches_valid = YES;
            }
            if (nmatch > 0) {
                match_idx = (match_idx + 1) % nmatch;
                int mlen = (int)strlen(matches[match_idx]);
                if (mlen >= bufsz) mlen = bufsz - 1;
                memcpy(buf, matches[match_idx], (size_t)mlen);
                buf[mlen] = 0;
                len = mlen;
            }
            continue;
        }
        if (ch >= 32 && len < bufsz - 1) {
            buf[len++] = (char)ch;
            buf[len] = 0;
            matches_valid = NO;
        }
    }
}

// Fork spot, drain all TLV hunks, push as new view.
// Returns OK if hunks were produced and view pushed.
static ok64 BROForkSpot(BROstate *st, char const *token,
                        char const *filepath, char const *repo) {
    sane(st != NULL && token != NULL && token[0] != 0);
    if (st->nsaves >= BRO_MAX_VIEWS) fail(NOROOM);

    bro_resolve_spot();

    int pfd[2];
    if (pipe(pfd) != 0) fail(FAILSANITY);

    pid_t pid = fork();
    if (pid < 0) { close(pfd[0]); close(pfd[1]); fail(FAILSANITY); }

    if (pid == 0) {
        // Child: run spot -g <token> [filepath]
        close(pfd[0]);
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]);
        if (filepath)
            execlp(bro_spot_path, "spot", "-g", token, filepath, (char *)NULL);
        else
            execlp(bro_spot_path, "spot", "-g", token, (char *)NULL);
        _exit(127);
    }

    // Parent: read TLV hunks from pipe
    close(pfd[1]);

    // Use a temporary arena-like approach: drain into bro_arena at current pos.
    // Save the arena position to restore if we fail.
    u8p arena_save = u8bIdleHead(bro_arena);
    u32 hunks_save = bro_nhunks;

    // Read buffer
    u8 rdbuf[1 << 16];
    u8 pending[1 << 16];
    int pend_len = 0;

    for (;;) {
        ssize_t nr = read(pfd[0], rdbuf, sizeof(rdbuf));
        if (nr <= 0) break;
        // Append to pending
        if (pend_len + (int)nr > (int)sizeof(pending)) {
            nr = (ssize_t)(sizeof(pending) - pend_len);
            if (nr <= 0) break;
        }
        memcpy(pending + pend_len, rdbuf, (size_t)nr);
        pend_len += (int)nr;

        // Drain complete TLV records
        u8cs from = {(u8cp)pending, (u8cp)pending + pend_len};
        while (!$empty(from) && bro_nhunks < BRO_MAX_HUNKS) {
            u8cs save = {from[0], from[1]};
            hunk tlv_hk = {};
            ok64 o = HUNKu8sDrain(from, &tlv_hk);
            if (o != OK) { $mv(from, save); break; }
            hunk *hk = &bro_hunks[bro_nhunks];
            *hk = (hunk){};
            if (!$empty(tlv_hk.title)) {
                u8p tp = BROArenaWrite(tlv_hk.title[0], (size_t)$len(tlv_hk.title));
                if (tp) { hk->title[0] = tp; hk->title[1] = u8bIdleHead(bro_arena); }
            }
            if (!$empty(tlv_hk.text)) {
                u8p xp = BROArenaWrite(tlv_hk.text[0], (size_t)$len(tlv_hk.text));
                if (xp) { hk->text[0] = xp; hk->text[1] = u8bIdleHead(bro_arena); }
            }
            if (!$empty(tlv_hk.toks)) {
                u8p tkp = BROArenaWrite(tlv_hk.toks[0],
                              (size_t)((u8cp)tlv_hk.toks[1] - (u8cp)tlv_hk.toks[0]));
                if (tkp) { hk->toks[0] = (u32cp)tkp; hk->toks[1] = (u32cp)u8bIdleHead(bro_arena); }
            }
            if (!$empty(tlv_hk.hili)) {
                u8p hp = BROArenaWrite(tlv_hk.hili[0],
                             (size_t)((u8cp)tlv_hk.hili[1] - (u8cp)tlv_hk.hili[0]));
                if (hp) { hk->hili[0] = (u32cp)hp; hk->hili[1] = (u32cp)u8bIdleHead(bro_arena); }
            }
            if (!$empty(tlv_hk.path)) {
                u8p pp = BROArenaWrite(tlv_hk.path[0], (size_t)$len(tlv_hk.path));
                if (pp) { hk->path[0] = pp; hk->path[1] = u8bIdleHead(bro_arena); }
            }
            hk->lineno = tlv_hk.lineno;
            bro_nhunks++;
        }
        // Compact pending
        int consumed = pend_len - (int)$len(from);
        if (consumed > 0) {
            pend_len -= consumed;
            memmove(pending, pending + consumed, (size_t)pend_len);
        }
    }

    close(pfd[0]);
    int status = 0;
    waitpid(pid, &status, 0);

    u32 new_nhunks = bro_nhunks - hunks_save;
    if (new_nhunks == 0) {
        // No results — restore arena, flash message
        bro_nhunks = hunks_save;
        // Reset arena idle pointer
        ((u8 **)bro_arena)[2] = arena_save;
        snprintf(st->flash, sizeof(st->flash), "spot: no results");
        fail(FAILSANITY);
    }

    // Save current view and switch to spot results
    int idx = st->nsaves;
    BROsave *sv = &st->saves[idx];
    sv->hunks = st->hunks;
    sv->nhunks = st->nhunks;
    sv->lines = st->lines;
    memcpy(sv->linesbuf, st->linesbuf, sizeof(Brange32));
    sv->nlines = st->nlines;
    sv->scroll = st->scroll;

    st->hunks = bro_hunks + hunks_save;
    st->nhunks = new_nhunks;
    memset(st->linesbuf, 0, sizeof(Brange32));
    call(BROBuildIndex, st);
    st->scroll = (st->nlines > 1) ? 1 : 0;
    st->nsaves = idx + 1;
    done;
}

// --- Unified key handler ---
// Returns: -1 = quit, 0 = no change, 1 = changed (needs render).

#define BRO_KEY_QUIT    (-1)
#define BRO_KEY_NONE    0
#define BRO_KEY_CHANGED 1

static int BROHandleKey(BROstate *st, u8 ch, char const *repo) {
    u32 page = (st->rows > 1) ? (u32)(st->rows - 1) : 1;

    if (ch == 'q' || ch == 'Q') {
        if (st->nsaves > 0) { BROBack(st); return BRO_KEY_CHANGED; }
        return BRO_KEY_QUIT;
    }
    if (ch == 'h') return BROBack(st) ? BRO_KEY_CHANGED : BRO_KEY_NONE;
    if (ch == 'l' || ch == '\r' || ch == '\n') {
        BROTryOpen(st, st->scroll, repo);
        return BRO_KEY_CHANGED;
    }
    if (ch == ' ' || ch == 'f') {
        if (st->scroll + page < st->nlines) st->scroll += page;
        else if (st->nlines > page) st->scroll = st->nlines - page;
        return BRO_KEY_CHANGED;
    }
    if (ch == 'b') {
        if (st->scroll >= page) st->scroll -= page;
        else st->scroll = 0;
        return BRO_KEY_CHANGED;
    }
    if (ch == 'j') {
        if (st->scroll + 1 < st->nlines) { st->scroll++; return BRO_KEY_CHANGED; }
        return BRO_KEY_NONE;
    }
    if (ch == 'k') {
        if (st->scroll > 0) { st->scroll--; return BRO_KEY_CHANGED; }
        return BRO_KEY_NONE;
    }
    if (ch == 'g') { st->scroll = 0; return BRO_KEY_CHANGED; }
    if (ch == 'G') {
        st->scroll = (st->nlines > page) ? st->nlines - page : 0;
        return BRO_KEY_CHANGED;
    }
    if (ch == ':') { BROReadGoto(st); return BRO_KEY_CHANGED; }
    if (ch == '/') {
        BROReadSearch(st);
        if (st->search_len > 0) {
            u32 f = BROSearchNext(st, st->scroll, +1);
            if (f != UINT32_MAX) st->scroll = f;
        }
        return BRO_KEY_CHANGED;
    }
    if (ch == 'n') {
        u32 f = BROSearchNext(st, st->scroll, +1);
        if (f != UINT32_MAX) { st->scroll = f; return BRO_KEY_CHANGED; }
        return BRO_KEY_NONE;
    }
    if (ch == 'N') {
        u32 f = BROSearchNext(st, st->scroll, -1);
        if (f != UINT32_MAX) { st->scroll = f; return BRO_KEY_CHANGED; }
        return BRO_KEY_NONE;
    }
    if (ch == 'd') {
        u32 half = page / 2;
        if (st->scroll + half < st->nlines) st->scroll += half;
        else if (st->nlines > page) st->scroll = st->nlines - page;
        return BRO_KEY_CHANGED;
    }
    if (ch == 'u') {
        u32 half = page / 2;
        if (st->scroll >= half) st->scroll -= half;
        else st->scroll = 0;
        return BRO_KEY_CHANGED;
    }
    if (ch == ']' || ch == '}') {
        u32 nx = BROHunkNextLine(st->lines, st->nlines, st->scroll);
        if (nx != BRO_NONE) { st->scroll = nx; return BRO_KEY_CHANGED; }
        return BRO_KEY_NONE;
    }
    if (ch == '[' || ch == '{') {
        u32 pv = BROHunkPrevLine(st->lines, st->nlines, st->scroll);
        if (pv != BRO_NONE) { st->scroll = pv; return BRO_KEY_CHANGED; }
        return BRO_KEY_NONE;
    }
    if (ch == ')' || ch == '(') {
        u32 mid = st->scroll + (page > 0 ? (page - 1) / 2 : 0);
        u32 hl = (ch == ')')
            ? BROHiliNextLine(st->hunks, st->nhunks, st->lines, st->nlines, mid)
            : BROHiliPrevLine(st->hunks, st->nhunks, st->lines, st->nlines, mid);
        if (hl != BRO_NONE) { BROScrollCenter(st, hl); return BRO_KEY_CHANGED; }
        return BRO_KEY_NONE;
    }
    if (ch == 'm') {
        st->mouse_on = !st->mouse_on;
        if (st->mouse_on) MAUSEnable(STDOUT_FILENO);
        else MAUSDisable(STDOUT_FILENO);
        return BRO_KEY_NONE;
    }
    if (ch == 's' || ch == 'S') {
        b8 file_scoped = (ch == 's');
        char token[256] = {};
        BROReadSpot(st, token, sizeof(token), file_scoped);
        if (token[0] == 0) return BRO_KEY_CHANGED;
        // Get file path for file-scoped search
        char const *fpath = NULL;
        char fpathz[FILE_PATH_MAX_LEN] = {};
        if (file_scoped && st->scroll < st->nlines) {
            u32 hi = st->lines[st->scroll].lo;
            hunkc const *hk = &st->hunks[hi];
            if (!$empty(hk->path)) {
                size_t pl = (size_t)$len(hk->path);
                if (pl >= sizeof(fpathz)) pl = sizeof(fpathz) - 1;
                memcpy(fpathz, hk->path[0], pl);
                fpath = fpathz;
            }
        }
        ok64 o = BROForkSpot(st, token, fpath, repo);
        if (o != OK && st->flash[0] == 0)
            snprintf(st->flash, sizeof(st->flash), "spot failed: %s", ok64str(o));
        return BRO_KEY_CHANGED;
    }
    if (ch == 033) {
        u8 seq[2] = {};
        ssize_t n1 = read(st->tty_fd, &seq[0], 1);
        if (n1 <= 0) return BRO_KEY_NONE;
        if (seq[0] != '[') return BRO_KEY_NONE;
        ssize_t n2 = read(st->tty_fd, &seq[1], 1);
        if (n2 <= 0) return BRO_KEY_NONE;
        if (seq[1] == '<') {
            // SGR mouse
            u8 mbuf[32];
            mbuf[0] = 033; mbuf[1] = '['; mbuf[2] = '<';
            int mi = 3;
            for (;;) {
                if (mi >= (int)sizeof(mbuf)) break;
                ssize_t r = read(st->tty_fd, &mbuf[mi], 1);
                if (r <= 0) break;
                if (mbuf[mi] == 'M' || mbuf[mi] == 'm') { mi++; break; }
                mi++;
            }
            MAUSevent mev = {};
            if (MAUSParse(&mev, mbuf, mi)) {
                if (mev.type == MAUS_WHEEL) {
                    u32 step = 3;
                    if (mev.button == MAUS_UP) {
                        if (st->scroll >= step) st->scroll -= step;
                        else st->scroll = 0;
                    } else if (mev.button == MAUS_DOWN) {
                        if (st->scroll + step < st->nlines) st->scroll += step;
                        else if (st->nlines > page) st->scroll = st->nlines - page;
                    }
                    return BRO_KEY_CHANGED;
                }
                if (mev.type == MAUS_PRESS && mev.button == MAUS_LEFT) {
                    u32 line = st->scroll + mev.row - 1;
                    if (line < st->nlines) {
                        BROTryOpen(st, line, repo);
                        return BRO_KEY_CHANGED;
                    }
                }
            }
            return BRO_KEY_NONE;
        }
        switch (seq[1]) {
        case 'A': if (st->scroll > 0) st->scroll--; return BRO_KEY_CHANGED;
        case 'B': if (st->scroll + 1 < st->nlines) st->scroll++; return BRO_KEY_CHANGED;
        case '5':
            (void)read(st->tty_fd, &seq[0], 1);
            if (st->scroll >= page) st->scroll -= page;
            else st->scroll = 0;
            return BRO_KEY_CHANGED;
        case '6':
            (void)read(st->tty_fd, &seq[0], 1);
            if (st->scroll + page < st->nlines) st->scroll += page;
            else if (st->nlines > page) st->scroll = st->nlines - page;
            return BRO_KEY_CHANGED;
        case 'H': st->scroll = 0; return BRO_KEY_CHANGED;
        case 'F':
            st->scroll = (st->nlines > page) ? st->nlines - page : 0;
            return BRO_KEY_CHANGED;
        }
    }
    return BRO_KEY_NONE;
}

// --- Main entry ---

ok64 BRORun(hunk const *hunks, u32 nhunks) {
    sane(hunks != NULL && nhunks > 0);

    // Fallback: plain output when stdout is not a terminal
    if (!isatty(STDOUT_FILENO))
        return BROPlain(hunks, nhunks);

    BROstate st = {};
    st.tty_fd = -1;
    st.hunks = hunks;
    st.nhunks = nhunks;

    // Resolve repo root for file navigation
    a_path(repo_path);
    char repo[FILE_PATH_MAX_LEN] = {};
    if (HOMEFind(repo_path) == OK) {
        u8cs rr = {};
        $mv(rr, u8bDataC(repo_path));
        size_t rl = (size_t)$len(rr);
        if (rl < sizeof(repo)) { memcpy(repo, rr[0], rl); repo[rl] = 0; }
    }

    BROGetSize(&st);
    call(BROBuildIndex, &st);
    call(BROScreenInit);
    call(BRORawEnable, &st);

    // Install SIGWINCH handler
    struct sigaction sa = {}, old_sa = {};
    sa.sa_handler = bro_winch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, &old_sa);

    // Hide cursor
    bro_puts("\033[?25l");

    // Alternate screen buffer
    bro_puts("\033[?1049h");
    // Mouse tracking starts disabled (toggle with 'm').

    // Start at line 1: skip the first hunk title (shown in status bar)
    if (st.nlines > 1) st.scroll = 1;

    BRORender(&st);

    b8 quit = NO;
    while (!quit) {
        if (bro_resized) {
            bro_resized = 0;
            BROGetSize(&st);
            BRORender(&st);
        }
        u8 ch = 0;
        ssize_t nr = read(st.tty_fd, &ch, 1);
        if (nr <= 0) continue;
        int r = BROHandleKey(&st, ch, repo);
        if (r == BRO_KEY_QUIT) quit = YES;
        else if (r == BRO_KEY_CHANGED) BRORender(&st);
    }

    // Restore: disable mouse, leave alternate screen, show cursor, reset
    if (st.mouse_on) MAUSDisable(STDOUT_FILENO);
    bro_puts("\033[?1049l");
    bro_puts("\033[?25h");
    BRORawDisable(&st);
    sigaction(SIGWINCH, &old_sa, NULL);
    // Free any stacked file views
    while (st.nsaves > 0) BROBack(&st);
    range32bFree(st.linesbuf);

    done;
}

// --- Pipe pager: incremental display of TLV hunks from a pipe ---

#define PIPE_RDBUF_INIT (1UL << 22)  // 4MB initial read buffer
#define PIPE_MAX_LINES  (1UL << 20)  // 1M lines max

// Append lines for hunks [from..nhunks) into lines[nlines..maxlines).
// Returns new total nlines.
static u32 BROAppendLines(range32 *lines, u32 nlines, u32 maxlines,
                          hunkc const *hunks, u32 from, u32 nhunks) {
    u32 li = nlines;
    for (u32 h = from; h < nhunks; h++) {
        if (hunk_has_title(&hunks[h])) {
            if (li < maxlines)
                lines[li++] = (range32){h, BRO_TITLE_LINE};
        }
        u32 tlen = (u32)$len(hunks[h].text);
        if (tlen == 0) continue;
        if (li < maxlines)
            lines[li++] = (range32){h, 0};
        for (u32 i = 0; i < tlen; i++) {
            if (hunks[h].text[0][i] == '\n' && i + 1 < tlen) {
                if (li < maxlines)
                    lines[li++] = (range32){h, i + 1};
            }
        }
    }
    return li;
}

ok64 BROPipeRun(int pipefd) {
    sane(pipefd >= 0);

    call(BROArenaInit);

    // Allocate growable read buffer
    Bu8 rdbuf = {};
    call(u8bMap, rdbuf, PIPE_RDBUF_INIT);

    // Resolve repo root
    a_path(repo_path2);
    char repo[FILE_PATH_MAX_LEN] = {};
    if (HOMEFind(repo_path2) == OK) {
        u8cs rr = {};
        $mv(rr, u8bDataC(repo_path2));
        size_t rl = (size_t)$len(rr);
        if (rl < sizeof(repo)) { memcpy(repo, rr[0], rl); repo[rl] = 0; }
    }

    // Allocate line index
    BROstate st = {};
    st.tty_fd = -1;
    ok64 lo = range32bAlloc(st.linesbuf, PIPE_MAX_LINES);
    if (lo != OK) {
        u8bUnMap(rdbuf);
        return NOROOM;
    }
    st.lines = st.linesbuf[0];
    st.hunks = bro_hunks;
    st.nhunks = 0;

    BROGetSize(&st);
    st.nlines = 0;

    call(BROScreenInit);
    if (BRORawEnable(&st) != OK) {
        u8bUnMap(rdbuf);
        range32bFree(st.linesbuf);
        return FAILSANITY;
    }

    struct sigaction sa = {}, old_sa = {};
    sa.sa_handler = bro_winch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, &old_sa);

    bro_puts("\033[?25l");    // hide cursor
    bro_puts("\033[?1049h");  // alt screen
    // Mouse tracking starts disabled (toggle with 'm').

    // Show initial "nothing..." while waiting for data
    u8bReset(bro_scr);
    scr_goto(st.rows, 1);
    scr_puts(TTY_INVERSE TTY_ERASE_LINE " nothing..." TTY_RESET);
    BROScreenFlush();

    b8 pipe_eof = NO;
    b8 quit = NO;
    u32 indexed_nhunks = 0;
    u32 rendered_nhunks = 0;

    while (!quit) {
        if (bro_resized) {
            bro_resized = 0;
            BROGetSize(&st);
        }

        struct pollfd fds[2];
        int nfds = 0;
        fds[0].fd = st.tty_fd;
        fds[0].events = POLLIN;
        nfds = 1;
        if (!pipe_eof) {
            fds[1].fd = pipefd;
            fds[1].events = POLLIN;
            nfds = 2;
        }

        int pr = poll(fds, (nfds_t)nfds, 16);

        b8 changed = NO;
        b8 key_pressed = NO;

        // Read from pipe
        if (!pipe_eof && (fds[1].revents & (POLLIN | POLLHUP))) {
            size_t space = u8bIdleLen(rdbuf);
            if (space > 0) {
                ssize_t nr = read(pipefd, u8bIdleHead(rdbuf), space);
                if (nr > 0) {
                    u8bFed(rdbuf, (size_t)nr);
                } else if (nr == 0) {
                    pipe_eof = YES;
                }
            }

            // Drain complete TLV records
            a_dup(u8 const, from, u8bDataC(rdbuf));
            while (!$empty(from) && bro_nhunks < BRO_MAX_HUNKS) {
                a_dup(u8 const, save, from);
                hunk tlv_hk = {};
                ok64 o = HUNKu8sDrain(from, &tlv_hk);
                if (o != OK) {
                    $mv(from, save);
                    // Buffer full with incomplete record — grow
                    if ($len(from) >= u8bIdleLen(rdbuf)) {
                        // Compact first, then grow
                        size_t eaten = u8bDataLen(rdbuf) - $len(from);
                        if (eaten > 0) {
                            u8bUsed(rdbuf, eaten);
                            u8bShift(rdbuf, 0);
                        }
                        ok64 ro = u8bReMap(rdbuf, u8bSize(rdbuf) * 2);
                        if (ro == OK) {
                            $mv(from, u8bDataC(rdbuf));
                            continue;  // retry drain
                        }
                    }
                    break;
                }
                // Copy fields into arena
                hunk *hk = &bro_hunks[bro_nhunks];
                *hk = (hunk){};
                if (!$empty(tlv_hk.title)) {
                    u8p tp = BROArenaWrite(tlv_hk.title[0],
                                            (size_t)$len(tlv_hk.title));
                    if (tp) {
                        hk->title[0] = tp;
                        hk->title[1] = u8bIdleHead(bro_arena);
                    }
                }
                if (!$empty(tlv_hk.text)) {
                    u8p xp = BROArenaWrite(tlv_hk.text[0],
                                            (size_t)$len(tlv_hk.text));
                    if (xp) {
                        hk->text[0] = xp;
                        hk->text[1] = u8bIdleHead(bro_arena);
                    }
                }
                if (!$empty(tlv_hk.toks)) {
                    u8p tkp = BROArenaWrite(tlv_hk.toks[0],
                                  (size_t)((u8cp)tlv_hk.toks[1] -
                                           (u8cp)tlv_hk.toks[0]));
                    if (tkp) {
                        hk->toks[0] = (u32cp)tkp;
                        hk->toks[1] = (u32cp)u8bIdleHead(bro_arena);
                    }
                }
                if (!$empty(tlv_hk.hili)) {
                    u8p hp = BROArenaWrite(tlv_hk.hili[0],
                                 (size_t)((u8cp)tlv_hk.hili[1] -
                                          (u8cp)tlv_hk.hili[0]));
                    if (hp) {
                        hk->hili[0] = (u32cp)hp;
                        hk->hili[1] = (u32cp)u8bIdleHead(bro_arena);
                    }
                }
                if (!$empty(tlv_hk.path)) {
                    u8p pp = BROArenaWrite(tlv_hk.path[0],
                                           (size_t)$len(tlv_hk.path));
                    if (pp) {
                        hk->path[0] = pp;
                        hk->path[1] = u8bIdleHead(bro_arena);
                    }
                }
                hk->lineno = tlv_hk.lineno;
                bro_nhunks++;
            }

            // Compact: shift remaining to buffer start
            size_t consumed = u8bDataLen(rdbuf) - $len(from);
            if (consumed > 0) {
                u8bUsed(rdbuf, consumed);
                u8bShift(rdbuf, 0);
            }
        }

        // Handle keyboard input
        if (fds[0].revents & POLLIN) {
            u8 ch = 0;
            ssize_t nr = read(st.tty_fd, &ch, 1);
            if (nr > 0) {
                int r = BROHandleKey(&st, ch, repo);
                if (r == BRO_KEY_QUIT) quit = YES;
                else if (r == BRO_KEY_CHANGED) changed = YES;
                key_pressed = YES;
            }
        }

        // Extend line index for any new hunks
        if (bro_nhunks > indexed_nhunks) {
            st.nlines = BROAppendLines(st.lines, st.nlines,
                                         PIPE_MAX_LINES, bro_hunks,
                                         indexed_nhunks, bro_nhunks);
            st.nhunks = bro_nhunks;
            indexed_nhunks = bro_nhunks;
        }
        // Render whenever new hunks have been indexed.  The 16ms poll
        // timeout caps refresh at ~60 fps even when the producer
        // streams quickly, so explicit debouncing isn't needed.
        if (indexed_nhunks > rendered_nhunks) {
            // Skip the first title line on the first render so the
            // user lands on actual content, not the title separator.
            if (rendered_nhunks == 0 && st.nlines > 1) st.scroll = 1;
            changed = YES;
            rendered_nhunks = indexed_nhunks;
        }
        if ((changed || key_pressed) && st.nlines > 0)
            BRORender(&st);

        // Pipe done, no results — show "nothing!" and wait for quit
        if (pipe_eof && st.nlines == 0 && !quit) {
            u8bReset(bro_scr);
            scr_goto(st.rows, 1);
            scr_puts(TTY_INVERSE TTY_ERASE_LINE " nothing!" TTY_RESET);
            BROScreenFlush();
        }
    }

    // Teardown
    if (st.mouse_on) MAUSDisable(STDOUT_FILENO);
    bro_puts("\033[?1049l");
    bro_puts("\033[?25h");
    BRORawDisable(&st);
    sigaction(SIGWINCH, &old_sa, NULL);

    while (st.nsaves > 0) BROBack(&st);
    range32bFree(st.linesbuf);
    u8bUnMap(rdbuf);
    BROArenaCleanup();

    done;
}
