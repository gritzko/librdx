#include "BRO.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "abc/ANSI.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/TTY.h"
#include "abc/UTF8.h"
#include "dog/HUNK.h"
#include "dog/TOK.h"

b8 BRO_COLOR = YES;

// --- BRO arena state ---
Bu8 bro_arena = {};
BROhunk bro_hunks[BRO_MAX_HUNKS];
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

// Reserve len bytes in the arena (zeroed), return slice
ok64 BROArenaAlloc(u8s out, size_t len) {
    if (u8bIdleLen(bro_arena) < len) return FAILSANITY;
    $mv(out, u8bIdle(bro_arena));
    out[1] = out[0] + len;
    u8sZero(out);
    u8bFed(bro_arena, len);
    return OK;
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

#define BRO_TITLE_LINE UINT32_MAX

typedef struct {
    BROhunk const *hunks;
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

// --- Build line index ---

static ok64 BROBuildIndex(BROstate *st) {
    sane(st != NULL && st->hunks != NULL);

    // Count lines: walk all hunks, count '\n' in text, plus title lines
    u32 total = 0;
    for (u32 h = 0; h < st->nhunks; h++) {
        if (!$empty(st->hunks[h].title)) total++;  // title separator
        u32 tlen = (u32)$len(st->hunks[h].text);
        if (tlen == 0) continue;
        total++;  // at least one line
        $for(u8c, c, st->hunks[h].text) {
            if (*c == '\n') total++;
        }
        // If text ends with '\n', we overcounted
        if (st->hunks[h].text[0][tlen - 1] == '\n') total--;
    }

    ok64 lo = range32bAlloc(st->linesbuf, total);
    if (lo != OK) fail(NOROOM);
    st->lines = st->linesbuf[0];

    u32 li = 0;
    for (u32 h = 0; h < st->nhunks; h++) {
        if (!$empty(st->hunks[h].title)) {
            st->lines[li++] = (range32){h, BRO_TITLE_LINE};
        }
        u32 tlen = (u32)$len(st->hunks[h].text);
        if (tlen == 0) continue;
        st->lines[li++] = (range32){h, 0};
        for (u32 i = 0; i < tlen; i++) {
            if (st->hunks[h].text[0][i] == '\n' && i + 1 < tlen) {
                st->lines[li++] = (range32){h, i + 1};
            }
        }
    }
    st->nlines = li;
    done;
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

u32 BROHiliCount(BROhunk const *hunks, u32 nhunks) {
    u32 n = 0;
    for (u32 h = 0; h < nhunks; h++) {
        u32 nh = (u32)$len(hunks[h].hili);
        for (u32 j = 0; j < nh; j++)
            if (bro_hili_real(tok32Tag(hunks[h].hili[0][j]))) n++;
    }
    return n;
}

u32 BROHiliIndexAt(BROhunk const *hunks, u32 nhunks,
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

u32 BROHiliNextLine(BROhunk const *hunks, u32 nhunks,
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

u32 BROHiliPrevLine(BROhunk const *hunks, u32 nhunks,
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
        BROhunk const *hk = &st->hunks[ln->lo];

        if (ln->hi == BRO_TITLE_LINE) {
            scr_puts(BRO_TITLE_COLOR);
            u32 tlen = (u32)$len(hk->title);
            u32 w = tlen < st->cols ? tlen : st->cols;
            a_dup(u8 const, ttl, hk->title);
            u8sFeedN(u8bIdle(bro_scr), ttl, w);
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

    u32 cur_hunk = 0;
    if (st->scroll < st->nlines)
        cur_hunk = st->lines[st->scroll].lo;

    u32 hunk_tot = BROHunkCount(st->lines, st->nlines);
    u32 hunk_idx = BROHunkIndexAt(st->lines, st->nlines, st->scroll);
    u32 hili_tot = BROHiliCount(st->hunks, st->nhunks);
    u32 hili_idx = BROHiliIndexAt(st->hunks, st->nhunks,
                                  st->lines, st->nlines, st->scroll);

    BROhunk const *ch = &st->hunks[cur_hunk];
    // Build status text into idle space, snprintf is fine for one-off
    u8sp out = u8bIdle(bro_scr);
    int slen;
    if (hili_tot > 0 && st->search_len > 0) {
        slen = snprintf((char *)out[0], $len(out),
                        " %.*s  L%u/%u  H%u/%u  C%u/%u  [/%.*s]",
                        (int)$len(ch->title), (char *)ch->title[0],
                        st->scroll + 1, st->nlines,
                        hunk_idx, hunk_tot, hili_idx, hili_tot,
                        (int)st->search_len, st->search);
    } else if (hili_tot > 0) {
        slen = snprintf((char *)out[0], $len(out),
                        " %.*s  L%u/%u  H%u/%u  C%u/%u",
                        (int)$len(ch->title), (char *)ch->title[0],
                        st->scroll + 1, st->nlines,
                        hunk_idx, hunk_tot, hili_idx, hili_tot);
    } else if (st->search_len > 0) {
        slen = snprintf((char *)out[0], $len(out),
                        " %.*s  L%u/%u  H%u/%u  [/%.*s]",
                        (int)$len(ch->title), (char *)ch->title[0],
                        st->scroll + 1, st->nlines,
                        hunk_idx, hunk_tot,
                        (int)st->search_len, st->search);
    } else {
        slen = snprintf((char *)out[0], $len(out),
                        " %.*s  L%u/%u  H%u/%u",
                        (int)$len(ch->title), (char *)ch->title[0],
                        st->scroll + 1, st->nlines,
                        hunk_idx, hunk_tot);
    }
    if (slen > 0) u8sFed(out, (size_t)slen);
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
        BROhunk const *hk = &st->hunks[ln->lo];
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

static ok64 BROPlain(BROhunk const *hunks, u32 nhunks) {
    sane(hunks != NULL);
    call(BROScreenInit);
    for (u32 h = 0; h < nhunks; h++) {
        u8bReset(bro_scr);
        if (!$empty(hunks[h].title)) {
            if (BRO_COLOR) scr_puts(BRO_TITLE_COLOR);
            u8bFeed(bro_scr, hunks[h].title);
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
            if (h + 1 >= nhunks || !$empty(hunks[h + 1].title))
                u8sFeed1(u8bIdle(bro_scr), '\n');
        }
        // Flush per hunk (hunks can be large)
        BROScreenFlush();
    }
    done;
}

// --- Main entry ---

ok64 BRORun(BROhunk const *hunks, u32 nhunks) {
    sane(hunks != NULL && nhunks > 0);

    // Fallback: plain output when stdout is not a terminal
    if (!isatty(STDOUT_FILENO))
        return BROPlain(hunks, nhunks);

    BROstate st = {};
    st.tty_fd = -1;
    st.hunks = hunks;
    st.nhunks = nhunks;

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

        u32 page = (u32)(st.rows - 1);

        if (ch == 'q' || ch == 'Q') {
            quit = YES;
        } else if (ch == ' ' || ch == 'f') {
            // Page down
            if (st.scroll + page < st.nlines)
                st.scroll += page;
            else if (st.nlines > page)
                st.scroll = st.nlines - page;
            BRORender(&st);
        } else if (ch == 'b') {
            // Page up
            if (st.scroll >= page)
                st.scroll -= page;
            else
                st.scroll = 0;
            BRORender(&st);
        } else if (ch == 'j') {
            // Line down
            if (st.scroll + 1 < st.nlines) {
                st.scroll++;
                BRORender(&st);
            }
        } else if (ch == 'k') {
            // Line up
            if (st.scroll > 0) {
                st.scroll--;
                BRORender(&st);
            }
        } else if (ch == 'g') {
            st.scroll = 0;
            BRORender(&st);
        } else if (ch == 'G') {
            if (st.nlines > page)
                st.scroll = st.nlines - page;
            else
                st.scroll = 0;
            BRORender(&st);
        } else if (ch == ':') {
            BROReadGoto(&st);
            BRORender(&st);
        } else if (ch == '/') {
            BROReadSearch(&st);
            if (st.search_len > 0) {
                u32 found = BROSearchNext(&st, st.scroll, +1);
                if (found != UINT32_MAX) st.scroll = found;
            }
            BRORender(&st);
        } else if (ch == 'n') {
            u32 found = BROSearchNext(&st, st.scroll, +1);
            if (found != UINT32_MAX) {
                st.scroll = found;
                BRORender(&st);
            }
        } else if (ch == 'N') {
            u32 found = BROSearchNext(&st, st.scroll, -1);
            if (found != UINT32_MAX) {
                st.scroll = found;
                BRORender(&st);
            }
        } else if (ch == 'd') {
            // Half page down
            u32 half = page / 2;
            if (st.scroll + half < st.nlines)
                st.scroll += half;
            else if (st.nlines > page)
                st.scroll = st.nlines - page;
            BRORender(&st);
        } else if (ch == 'u') {
            // Half page up
            u32 half = page / 2;
            if (st.scroll >= half)
                st.scroll -= half;
            else
                st.scroll = 0;
            BRORender(&st);
        } else if (ch == ']' || ch == '}') {
            u32 nx = BROHunkNextLine(st.lines, st.nlines, st.scroll);
            if (nx != BRO_NONE) {
                st.scroll = nx;
                BRORender(&st);
            }
        } else if (ch == '[' || ch == '{') {
            u32 pv = BROHunkPrevLine(st.lines, st.nlines, st.scroll);
            if (pv != BRO_NONE) {
                st.scroll = pv;
                BRORender(&st);
            }
        } else if (ch == ')' || ch == '(') {
            u32 mid = st.scroll + (page > 0 ? (page - 1) / 2 : 0);
            u32 hl = (ch == ')')
                ? BROHiliNextLine(st.hunks, st.nhunks,
                                  st.lines, st.nlines, mid)
                : BROHiliPrevLine(st.hunks, st.nhunks,
                                  st.lines, st.nlines, mid);
            if (hl != BRO_NONE) {
                BROScrollCenter(&st, hl);
                BRORender(&st);
            }
        } else if (ch == 033) {
            // Escape sequence: read next bytes
            u8 seq[2] = {};
            ssize_t n1 = read(st.tty_fd, &seq[0], 1);
            if (n1 <= 0) continue;
            if (seq[0] == '[') {
                ssize_t n2 = read(st.tty_fd, &seq[1], 1);
                if (n2 <= 0) continue;
                switch (seq[1]) {
                    case 'A':  // Up
                        if (st.scroll > 0) st.scroll--;
                        BRORender(&st);
                        break;
                    case 'B':  // Down
                        if (st.scroll + 1 < st.nlines) st.scroll++;
                        BRORender(&st);
                        break;
                    case '5':  // PgUp: \033[5~
                        (void)read(st.tty_fd, &seq[0], 1);  // consume '~'
                        if (st.scroll >= page)
                            st.scroll -= page;
                        else
                            st.scroll = 0;
                        BRORender(&st);
                        break;
                    case '6':  // PgDn: \033[6~
                        (void)read(st.tty_fd, &seq[0], 1);  // consume '~'
                        if (st.scroll + page < st.nlines)
                            st.scroll += page;
                        else if (st.nlines > page)
                            st.scroll = st.nlines - page;
                        BRORender(&st);
                        break;
                    case 'H':  // Home
                        st.scroll = 0;
                        BRORender(&st);
                        break;
                    case 'F':  // End
                        if (st.nlines > page)
                            st.scroll = st.nlines - page;
                        else
                            st.scroll = 0;
                        BRORender(&st);
                        break;
                }
            }
        }
    }

    // Restore: leave alternate screen, show cursor, reset terminal
    bro_puts("\033[?1049l");
    bro_puts("\033[?25h");
    BRORawDisable(&st);
    sigaction(SIGWINCH, &old_sa, NULL);
    range32bFree(st.linesbuf);

    done;
}

// --- Pipe pager: incremental display of TLV hunks from a pipe ---

#define PIPE_RDBUF_INIT (1UL << 22)  // 4MB initial read buffer
#define PIPE_MAX_LINES  (1UL << 20)  // 1M lines max

// Count lines for hunks [from..nhunks), append to lines array.
// Returns new total nlines.
static u32 BROExtendIndex(range32 *lines, u32 nlines,
                            BROhunk const *hunks,
                            u32 from, u32 nhunks) {
    u32 li = nlines;
    for (u32 h = from; h < nhunks; h++) {
        if (!$empty(hunks[h].title)) {
            if (li < PIPE_MAX_LINES)
                lines[li++] = (range32){h, BRO_TITLE_LINE};
        }
        u32 tlen = (u32)$len(hunks[h].text);
        if (tlen == 0) continue;
        if (li < PIPE_MAX_LINES)
            lines[li++] = (range32){h, 0};
        for (u32 i = 0; i < tlen; i++) {
            if (hunks[h].text[0][i] == '\n' && i + 1 < tlen) {
                if (li < PIPE_MAX_LINES)
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
                HUNKhunk h = {};
                ok64 o = HUNKu8sDrain(from, &h);
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
                BROhunk *hk = &bro_hunks[bro_nhunks];
                *hk = (BROhunk){};
                if (!$empty(h.title)) {
                    u8p tp = BROArenaWrite(h.title[0],
                                            (size_t)$len(h.title));
                    if (tp) {
                        hk->title[0] = tp;
                        hk->title[1] = u8bIdleHead(bro_arena);
                    }
                }
                if (!$empty(h.text)) {
                    u8p xp = BROArenaWrite(h.text[0],
                                            (size_t)$len(h.text));
                    if (xp) {
                        hk->text[0] = xp;
                        hk->text[1] = u8bIdleHead(bro_arena);
                    }
                }
                if (!$empty(h.toks)) {
                    u8p tkp = BROArenaWrite(h.toks[0],
                                  (size_t)((u8cp)h.toks[1] -
                                           (u8cp)h.toks[0]));
                    if (tkp) {
                        hk->toks[0] = (u32cp)tkp;
                        hk->toks[1] = (u32cp)u8bIdleHead(bro_arena);
                    }
                }
                if (!$empty(h.hili)) {
                    u8p hp = BROArenaWrite(h.hili[0],
                                 (size_t)((u8cp)h.hili[1] -
                                          (u8cp)h.hili[0]));
                    if (hp) {
                        hk->hili[0] = (u32cp)hp;
                        hk->hili[1] = (u32cp)u8bIdleHead(bro_arena);
                    }
                }
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
                key_pressed = YES;
                u32 page = (u32)(st.rows - 1);
                if (ch == 'q' || ch == 'Q') {
                    quit = YES;
                } else if (ch == ' ' || ch == 'f') {
                    if (st.scroll + page < st.nlines)
                        st.scroll += page;
                    else if (st.nlines > page)
                        st.scroll = st.nlines - page;
                    changed = YES;
                } else if (ch == 'b') {
                    if (st.scroll >= page) st.scroll -= page;
                    else st.scroll = 0;
                    changed = YES;
                } else if (ch == 'j') {
                    if (st.scroll + 1 < st.nlines) { st.scroll++; changed = YES; }
                } else if (ch == 'k') {
                    if (st.scroll > 0) { st.scroll--; changed = YES; }
                } else if (ch == 'g') {
                    st.scroll = 0; changed = YES;
                } else if (ch == 'G') {
                    if (st.nlines > page) st.scroll = st.nlines - page;
                    else st.scroll = 0;
                    changed = YES;
                } else if (ch == ':') {
                    BROReadGoto(&st);
                    changed = YES;
                } else if (ch == '/') {
                    BROReadSearch(&st);
                    if (st.search_len > 0) {
                        u32 found = BROSearchNext(&st, st.scroll, +1);
                        if (found != UINT32_MAX) st.scroll = found;
                    }
                    changed = YES;
                } else if (ch == 'n') {
                    u32 found = BROSearchNext(&st, st.scroll, +1);
                    if (found != UINT32_MAX) { st.scroll = found; changed = YES; }
                } else if (ch == 'N') {
                    u32 found = BROSearchNext(&st, st.scroll, -1);
                    if (found != UINT32_MAX) { st.scroll = found; changed = YES; }
                } else if (ch == 'd') {
                    u32 half = page / 2;
                    if (st.scroll + half < st.nlines) st.scroll += half;
                    else if (st.nlines > page) st.scroll = st.nlines - page;
                    changed = YES;
                } else if (ch == 'u') {
                    u32 half = page / 2;
                    if (st.scroll >= half) st.scroll -= half;
                    else st.scroll = 0;
                    changed = YES;
                } else if (ch == ']' || ch == '}') {
                    u32 nx = BROHunkNextLine(st.lines, st.nlines, st.scroll);
                    if (nx != BRO_NONE) { st.scroll = nx; changed = YES; }
                } else if (ch == '[' || ch == '{') {
                    u32 pv = BROHunkPrevLine(st.lines, st.nlines, st.scroll);
                    if (pv != BRO_NONE) { st.scroll = pv; changed = YES; }
                } else if (ch == ')' || ch == '(') {
                    u32 mid = st.scroll + (page > 0 ? (page - 1) / 2 : 0);
                    u32 hl = (ch == ')')
                        ? BROHiliNextLine(st.hunks, st.nhunks,
                                          st.lines, st.nlines, mid)
                        : BROHiliPrevLine(st.hunks, st.nhunks,
                                          st.lines, st.nlines, mid);
                    if (hl != BRO_NONE) {
                        BROScrollCenter(&st, hl);
                        changed = YES;
                    }
                } else if (ch == 033) {
                    u8 seq[2] = {};
                    ssize_t n1 = read(st.tty_fd, &seq[0], 1);
                    if (n1 > 0 && seq[0] == '[') {
                        ssize_t n2 = read(st.tty_fd, &seq[1], 1);
                        if (n2 > 0) {
                            switch (seq[1]) {
                            case 'A': if (st.scroll > 0) st.scroll--; changed = YES; break;
                            case 'B': if (st.scroll + 1 < st.nlines) st.scroll++; changed = YES; break;
                            case '5':
                                (void)read(st.tty_fd, &seq[0], 1);
                                if (st.scroll >= page) st.scroll -= page;
                                else st.scroll = 0;
                                changed = YES; break;
                            case '6':
                                (void)read(st.tty_fd, &seq[0], 1);
                                if (st.scroll + page < st.nlines) st.scroll += page;
                                else if (st.nlines > page) st.scroll = st.nlines - page;
                                changed = YES; break;
                            case 'H': st.scroll = 0; changed = YES; break;
                            case 'F':
                                if (st.nlines > page) st.scroll = st.nlines - page;
                                else st.scroll = 0;
                                changed = YES; break;
                            }
                        }
                    }
                }
            }
        }

        // Extend line index for any new hunks
        if (bro_nhunks > indexed_nhunks) {
            st.nlines = BROExtendIndex(st.lines, st.nlines,
                                         bro_hunks,
                                         indexed_nhunks,
                                         bro_nhunks);
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
    bro_puts("\033[?1049l");
    bro_puts("\033[?25h");
    BRORawDisable(&st);
    sigaction(SIGWINCH, &old_sa, NULL);

    range32bFree(st.linesbuf);
    u8bUnMap(rdbuf);
    BROArenaCleanup();

    done;
}
