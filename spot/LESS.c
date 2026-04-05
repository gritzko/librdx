#include "LESS.h"

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
#include "spot/HUNK.h"
#include "tok/TOK.h"

extern b8 CAPO_COLOR;

// --- LESS arena state ---
Bu8 less_arena = {};
LESShunk less_hunks[LESS_MAX_HUNKS];
u8bp less_maps[LESS_MAX_MAPS];
Bu32 less_toks[LESS_MAX_MAPS];
u32 less_nhunks = 0;
u32 less_nmaps = 0;
int less_pipe_fd = -1;

ok64 LESSArenaInit(void) {
    less_nhunks = 0;
    less_nmaps = 0;
    memset(less_hunks, 0, sizeof(less_hunks));
    memset(less_maps, 0, sizeof(less_maps));
    memset(less_toks, 0, sizeof(less_toks));
    if (less_arena[0] != NULL) {
        // Reset idle pointer to start
        ((u8 **)less_arena)[2] = less_arena[1];
        return OK;
    }
    return u8bMap(less_arena, LESS_ARENA_SIZE);
}

void LESSArenaCleanup(void) {
    for (u32 i = 0; i < less_nmaps; i++) {
        if (less_toks[i][0] != NULL) u32bUnMap(less_toks[i]);
        if (less_maps[i] != NULL) FILEUnMap(less_maps[i]);
    }
    less_nhunks = 0;
    less_nmaps = 0;
}

// Write bytes into the arena, return pointer to start
u8p LESSArenaWrite(void const *data, size_t len) {
    if (u8bIdleLen(less_arena) < len) return NULL;
    u8p p = u8bIdleHead(less_arena);
    memcpy(p, data, len);
    u8bFed(less_arena, len);
    return p;
}

// Reserve len bytes in the arena (zeroed), return slice
ok64 LESSArenaAlloc(u8s out, size_t len) {
    if (u8bIdleLen(less_arena) < len) return FAILSANITY;
    $mv(out, u8bIdle(less_arena));
    out[1] = out[0] + len;
    u8sZero(out);
    u8bFed(less_arena, len);
    return OK;
}

// Defer file+toks cleanup until after LESSRun
void LESSDefer(u8bp mapped, Bu32 toks) {
    if (less_nmaps >= LESS_MAX_MAPS) return;
    less_maps[less_nmaps] = mapped;
    memcpy(less_toks[less_nmaps], toks, sizeof(Bu32));
    less_nmaps++;
}

// --- LESSClipToks: clip file-level toks to [lo,hi), arena-copy rebased ---

void LESSClipToks(u32cs out, u32cs toks, u8cp base, u32 lo, u32 hi) {
    out[0] = NULL; out[1] = NULL;
    if ($empty(toks) || lo >= hi) return;
    int n = (int)$len(toks);
    // Find first tok overlapping [lo, hi)
    int first = 0;
    while (first < n && tok32Offset(toks[0][first]) <= lo) first++;
    // Find last tok overlapping [lo, hi)
    int last = first;
    while (last < n) {
        u32 tlo = (last > 0) ? tok32Offset(toks[0][last - 1]) : 0;
        if (tlo >= hi) break;
        last++;
    }
    if (first >= last) return;
    // Arena-write rebased entries
    u32gp g = u32aOpen(less_arena);
    for (int i = first; i < last; i++) {
        u8 tag = tok32Tag(toks[0][i]);
        u32 off = tok32Offset(toks[0][i]);
        if (off > hi) off = hi;
        u32 rebased = off - lo;
        u32gFeed1(g, tok32Pack(tag, rebased));
    }
    u32aClose(less_arena, out);
}

// --- LESSHunkEmit: serialize + write to pipe in fork mode ---

void LESSHunkEmit(void) {
    u32 idx = less_nhunks++;
    if (less_pipe_fd < 0) return;  // sync mode: nothing more to do

    LESShunk *hk = &less_hunks[idx];
    HUNKhunk h = {};
    $mv(h.title, hk->title);
    $mv(h.text, hk->text);
    h.toks[0] = hk->toks[0];
    h.toks[1] = hk->toks[1];
    h.hili[0] = hk->hili[0];
    h.hili[1] = hk->hili[1];

    // Serialize into arena idle space, write to pipe, then rewind
    range64 mark;
    Bu8mark(less_arena, &mark);
    u8cp start = u8bIdleHead(less_arena);
    if (HUNKu8sFeed(u8bIdle(less_arena), &h) != OK) {
        Bu8rewind(less_arena, mark);
        less_nhunks--;
        return;
    }

    u8cs ser = {start, u8bIdleHead(less_arena)};
    while (!$empty(ser)) {
        ssize_t w = write(less_pipe_fd, ser[0], $len(ser));
        if (w <= 0) break;
        u8csFed(ser, (size_t)w);
    }

    Bu8rewind(less_arena, mark);  // rewind: scratch space reused
    less_nhunks--;
}

// 256-color ink violet for hunk titles
#define LESS_TITLE_COLOR TTY_FG256(56)

// Tag-to-ANSI-color mapping.
// Returns fg color; sets *bold = YES for definitions.
static int LESSTagColor(u8 tag, b8 *bold) {
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

#define LESS_TITLE_LINE UINT32_MAX

typedef struct {
    LESShunk const *hunks;
    u32 nhunks;
    range32 *lines;    // line index array (heap buffer)
    Brange32 linesbuf; // buffer owning lines memory
    u32 nlines;        // total line count
    u32 scroll;        // first visible line
    u16 rows, cols;    // terminal dimensions
    char search[256];  // current search pattern
    u32 search_len;    // length of search pattern
    struct termios orig_termios;  // saved terminal state
    b8 raw_mode;       // whether terminal is in raw mode
} LESSstate;

// Global for signal handler
static volatile sig_atomic_t less_resized = 0;

static void less_winch_handler(int sig) {
    (void)sig;
    less_resized = 1;
}

// --- Terminal setup ---

static ok64 LESSRawEnable(LESSstate *st) {
    sane(st != NULL);
    if (tcgetattr(STDIN_FILENO, &st->orig_termios) < 0)
        fail(FAILSANITY);
    struct termios raw = st->orig_termios;
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= (tcflag_t)~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= (tcflag_t)~(OPOST);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;  // 100ms timeout
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0)
        fail(FAILSANITY);
    st->raw_mode = YES;
    done;
}

static void LESSRawDisable(LESSstate *st) {
    if (st->raw_mode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &st->orig_termios);
        st->raw_mode = NO;
    }
}

static void LESSGetSize(LESSstate *st) {
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

static ok64 LESSBuildIndex(LESSstate *st) {
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
            st->lines[li++] = (range32){h, LESS_TITLE_LINE};
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

// --- Rendering ---
// All screen output is built into less_scr[] buffer, flushed once.

#define LESS_SCR_SIZE (1UL << 20)  // 1MB screen buffer
static Bu8 less_scr = {};

static ok64 LESSScreenInit(void) {
    if (less_scr[0] != NULL) { u8bReset(less_scr); return OK; }
    return u8bMap(less_scr, LESS_SCR_SIZE);
}

static void LESSScreenFlush(void) {
    if (u8bDataLen(less_scr) > 0)
        (void)write(STDOUT_FILENO, u8bDataHead(less_scr), u8bDataLen(less_scr));
    u8bReset(less_scr);
}

// Feed string literal
static void scr_puts(char const *s) {
    u8sFeed(u8bIdle(less_scr), (u8csc){(u8cp)s, (u8cp)s + strlen(s)});
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
    u8bFeed(less_scr, u8bDataC(tmp));
}

// Direct write helpers (for prompts/setup, not screen rendering)
static void less_puts(char const *s) {
    (void)write(STDOUT_FILENO, s, strlen(s));
}
static void less_write(u8csc s) {
    (void)write(STDOUT_FILENO, s[0], $len(s));
}
static void less_goto(int row, int col) {
    a_pad(u8, tmp, 32);
    u8sFeed1(tmp_idle, 033);
    u8sFeed1(tmp_idle, '[');
    utf8sFeed10(tmp_idle, (u64)row);
    u8sFeed1(tmp_idle, ';');
    utf8sFeed10(tmp_idle, (u64)col);
    u8sFeed1(tmp_idle, 'H');
    less_write(u8bDataC(tmp));
}

// Feed one byte with fg tag, bg tag, and search highlight
// UTF-8 sequence length from lead byte
static u32 utf8len(u8 ch) {
    if (ch < 0x80) return 1;
    if ((ch & 0xE0) == 0xC0) return 2;
    if ((ch & 0xF0) == 0xE0) return 3;
    if ((ch & 0xF8) == 0xF0) return 4;
    return 1;  // invalid lead, emit as single byte
}

static void scr_emit_char(u8cp p, u32 n, u8 fg_tag, u8 bg_tag, b8 in_search) {
    b8 bold = NO;
    int fg = LESSTagColor(fg_tag, &bold);
    b8 is_ins = (bg_tag == 'I');
    b8 is_del = (bg_tag == 'D');

    u8sp out = u8bIdle(less_scr);
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
static b8 less_search_at(LESSstate *st, u8csc text, u32 pos) {
    if (st->search_len == 0) return NO;
    if (pos + st->search_len > (u32)$len(text)) return NO;
    return memcmp(&text[0][pos], st->search, st->search_len) == 0 ? YES : NO;
}

static void LESSStatusBar(LESSstate *st);

static void LESSRender(LESSstate *st) {
    u8bReset(less_scr);
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
        LESShunk const *hk = &st->hunks[ln->lo];

        if (ln->hi == LESS_TITLE_LINE) {
            scr_puts(LESS_TITLE_COLOR);
            u32 tlen = (u32)$len(hk->title);
            u32 w = tlen < st->cols ? tlen : st->cols;
            a_dup(u8 const, ttl, hk->title);
            u8sFeedN(u8bIdle(less_scr), ttl, w);
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

        for (u32 j = 0; j < w; ) {
            u32 pos = off + j;
            u8 ch = hk->text[0][pos];
            u32 clen = utf8len(ch);
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
            b8 in_search = less_search_at(st, hk->text, pos);
            scr_emit_char(hk->text[0] + pos, clen, fg_tag, bg_tag, in_search);
            j += clen;
        }
    }

    for (u32 row = end - st->scroll + 1; row < st->rows; row++) {
        scr_goto((int)row, 1);
        scr_puts(TTY_ERASE_LINE);
    }

    LESSStatusBar(st);
    LESSScreenFlush();
}

static void LESSStatusBar(LESSstate *st) {
    scr_goto(st->rows, 1);
    scr_puts(TTY_INVERSE TTY_ERASE_LINE);

    u32 cur_hunk = 0;
    if (st->scroll < st->nlines)
        cur_hunk = st->lines[st->scroll].lo;

    LESShunk const *ch = &st->hunks[cur_hunk];
    // Build status text into idle space, snprintf is fine for one-off
    u8sp out = u8bIdle(less_scr);
    int slen;
    if (st->search_len > 0) {
        slen = snprintf((char *)out[0], $len(out),
                        " %.*s  line %u/%u  [/%.*s]",
                        (int)$len(ch->title), (char *)ch->title[0],
                        st->scroll + 1, st->nlines,
                        (int)st->search_len, st->search);
    } else {
        slen = snprintf((char *)out[0], $len(out),
                        " %.*s  line %u/%u",
                        (int)$len(ch->title), (char *)ch->title[0],
                        st->scroll + 1, st->nlines);
    }
    if (slen > 0) u8sFed(out, (size_t)slen);
    scr_puts(TTY_RESET);
}

// --- Search ---

// Find next occurrence of search pattern starting from line `from` (direction=+1)
// or backwards from `from` (direction=-1). Returns line index or UINT32_MAX.
static u32 LESSSearchNext(LESSstate *st, u32 from, int direction) {
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
        if (ln->hi == LESS_TITLE_LINE) continue;
        LESShunk const *hk = &st->hunks[ln->lo];
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
static void LESSReadSearch(LESSstate *st) {
    st->search_len = 0;
    memset(st->search, 0, sizeof(st->search));

    for (;;) {
        // Render prompt on status bar
        less_goto(st->rows, 1);
        less_puts(TTY_INVERSE TTY_ERASE_LINE);
        less_puts(" /");
        if (st->search_len > 0) {
            u8cs srch = {(u8cp)st->search, (u8cp)st->search + st->search_len};
            less_write(srch);
        }
        less_puts(TTY_RESET);

        u8 ch = 0;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
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
static void LESSReadGoto(LESSstate *st) {
    char buf[32] = {};
    u32 len = 0;

    for (;;) {
        less_goto(st->rows, 1);
        less_puts(TTY_INVERSE TTY_ERASE_LINE);
        less_puts(" :");
        if (len > 0) {
            u8cs bs = {(u8cp)buf, (u8cp)buf + len};
            less_write(bs);
        }
        less_puts(TTY_RESET);

        u8 ch = 0;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
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

static ok64 LESSPlain(LESShunk const *hunks, u32 nhunks) {
    sane(hunks != NULL);
    call(LESSScreenInit);
    for (u32 h = 0; h < nhunks; h++) {
        u8bReset(less_scr);
        if (!$empty(hunks[h].title)) {
            if (CAPO_COLOR) scr_puts(LESS_TITLE_COLOR);
            u8bFeed(less_scr, hunks[h].title);
            if (CAPO_COLOR) scr_puts(TTY_RESET);
            u8sFeed1(u8bIdle(less_scr), '\n');
        }
        if (!$empty(hunks[h].text)) {
            u32 tlen = (u32)$len(hunks[h].text);
            if (!CAPO_COLOR) {
                // No colors: plain text
                u8bFeed(less_scr, hunks[h].text);
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
                    int fg = LESSTagColor(fg_tag, &bold);
                    int bg = 0;
                    if (bg_tag == 'I') bg = 157;
                    else if (bg_tag == 'D') bg = 217;
                    u8sp out = u8bIdle(less_scr);
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
                    escfeed(u8bIdle(less_scr), 0);
            }
            // Trailing newline if text doesn't end with one
            if (tlen > 0 && hunks[h].text[0][tlen - 1] != '\n')
                u8sFeed1(u8bIdle(less_scr), '\n');
            if (h + 1 >= nhunks || !$empty(hunks[h + 1].title))
                u8sFeed1(u8bIdle(less_scr), '\n');
        }
        // Flush per hunk (hunks can be large)
        LESSScreenFlush();
    }
    done;
}

// --- Main entry ---

ok64 LESSRun(LESShunk const *hunks, u32 nhunks) {
    sane(hunks != NULL && nhunks > 0);

    // Worker side of pipe mode: hunks already written, just close
    if (less_pipe_fd >= 0) {
        close(less_pipe_fd);
        less_pipe_fd = -1;
        return OK;
    }

    // Fallback: plain output when stdout is not a terminal
    if (!isatty(STDOUT_FILENO))
        return LESSPlain(hunks, nhunks);

    LESSstate st = {};
    st.hunks = hunks;
    st.nhunks = nhunks;

    LESSGetSize(&st);
    call(LESSBuildIndex, &st);
    call(LESSScreenInit);
    call(LESSRawEnable, &st);

    // Install SIGWINCH handler
    struct sigaction sa = {}, old_sa = {};
    sa.sa_handler = less_winch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, &old_sa);

    // Hide cursor
    less_puts("\033[?25l");

    // Alternate screen buffer
    less_puts("\033[?1049h");

    // Start at line 1: skip the first hunk title (shown in status bar)
    if (st.nlines > 1) st.scroll = 1;

    LESSRender(&st);

    b8 quit = NO;
    while (!quit) {
        if (less_resized) {
            less_resized = 0;
            LESSGetSize(&st);
            LESSRender(&st);
        }

        u8 ch = 0;
        ssize_t nr = read(STDIN_FILENO, &ch, 1);
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
            LESSRender(&st);
        } else if (ch == 'b') {
            // Page up
            if (st.scroll >= page)
                st.scroll -= page;
            else
                st.scroll = 0;
            LESSRender(&st);
        } else if (ch == 'j') {
            // Line down
            if (st.scroll + 1 < st.nlines) {
                st.scroll++;
                LESSRender(&st);
            }
        } else if (ch == 'k') {
            // Line up
            if (st.scroll > 0) {
                st.scroll--;
                LESSRender(&st);
            }
        } else if (ch == 'g') {
            st.scroll = 0;
            LESSRender(&st);
        } else if (ch == 'G') {
            if (st.nlines > page)
                st.scroll = st.nlines - page;
            else
                st.scroll = 0;
            LESSRender(&st);
        } else if (ch == ':') {
            LESSReadGoto(&st);
            LESSRender(&st);
        } else if (ch == '/') {
            LESSReadSearch(&st);
            if (st.search_len > 0) {
                u32 found = LESSSearchNext(&st, st.scroll, +1);
                if (found != UINT32_MAX) st.scroll = found;
            }
            LESSRender(&st);
        } else if (ch == 'n') {
            u32 found = LESSSearchNext(&st, st.scroll, +1);
            if (found != UINT32_MAX) {
                st.scroll = found;
                LESSRender(&st);
            }
        } else if (ch == 'N') {
            u32 found = LESSSearchNext(&st, st.scroll, -1);
            if (found != UINT32_MAX) {
                st.scroll = found;
                LESSRender(&st);
            }
        } else if (ch == 'd') {
            // Half page down
            u32 half = page / 2;
            if (st.scroll + half < st.nlines)
                st.scroll += half;
            else if (st.nlines > page)
                st.scroll = st.nlines - page;
            LESSRender(&st);
        } else if (ch == 'u') {
            // Half page up
            u32 half = page / 2;
            if (st.scroll >= half)
                st.scroll -= half;
            else
                st.scroll = 0;
            LESSRender(&st);
        } else if (ch == 033) {
            // Escape sequence: read next bytes
            u8 seq[2] = {};
            ssize_t n1 = read(STDIN_FILENO, &seq[0], 1);
            if (n1 <= 0) continue;
            if (seq[0] == '[') {
                ssize_t n2 = read(STDIN_FILENO, &seq[1], 1);
                if (n2 <= 0) continue;
                switch (seq[1]) {
                    case 'A':  // Up
                        if (st.scroll > 0) st.scroll--;
                        LESSRender(&st);
                        break;
                    case 'B':  // Down
                        if (st.scroll + 1 < st.nlines) st.scroll++;
                        LESSRender(&st);
                        break;
                    case '5':  // PgUp: \033[5~
                        (void)read(STDIN_FILENO, &seq[0], 1);  // consume '~'
                        if (st.scroll >= page)
                            st.scroll -= page;
                        else
                            st.scroll = 0;
                        LESSRender(&st);
                        break;
                    case '6':  // PgDn: \033[6~
                        (void)read(STDIN_FILENO, &seq[0], 1);  // consume '~'
                        if (st.scroll + page < st.nlines)
                            st.scroll += page;
                        else if (st.nlines > page)
                            st.scroll = st.nlines - page;
                        LESSRender(&st);
                        break;
                    case 'H':  // Home
                        st.scroll = 0;
                        LESSRender(&st);
                        break;
                    case 'F':  // End
                        if (st.nlines > page)
                            st.scroll = st.nlines - page;
                        else
                            st.scroll = 0;
                        LESSRender(&st);
                        break;
                }
            }
        }
    }

    // Restore: leave alternate screen, show cursor, reset terminal
    less_puts("\033[?1049l");
    less_puts("\033[?25h");
    LESSRawDisable(&st);
    sigaction(SIGWINCH, &old_sa, NULL);
    range32bFree(st.linesbuf);

    done;
}

// --- Pipe pager: incremental display of TLV hunks from a pipe ---

#define PIPE_RDBUF_INIT (1UL << 22)  // 4MB initial read buffer
#define PIPE_MAX_LINES  (1UL << 20)  // 1M lines max

// Count lines for hunks [from..nhunks), append to lines array.
// Returns new total nlines.
static u32 LESSExtendIndex(range32 *lines, u32 nlines,
                            LESShunk const *hunks,
                            u32 from, u32 nhunks) {
    u32 li = nlines;
    for (u32 h = from; h < nhunks; h++) {
        if (!$empty(hunks[h].title)) {
            if (li < PIPE_MAX_LINES)
                lines[li++] = (range32){h, LESS_TITLE_LINE};
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

ok64 LESSPipeRun(int pipefd) {
    sane(pipefd >= 0);

    call(LESSArenaInit);

    // Allocate growable read buffer
    Bu8 rdbuf = {};
    call(u8bMap, rdbuf, PIPE_RDBUF_INIT);

    // Allocate line index
    LESSstate st = {};
    ok64 lo = range32bAlloc(st.linesbuf, PIPE_MAX_LINES);
    if (lo != OK) {
        u8bUnMap(rdbuf);
        return NOROOM;
    }
    st.lines = st.linesbuf[0];
    st.hunks = less_hunks;
    st.nhunks = 0;

    LESSGetSize(&st);
    st.nlines = 0;

    call(LESSScreenInit);
    call(LESSRawEnable, &st);

    struct sigaction sa = {}, old_sa = {};
    sa.sa_handler = less_winch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, &old_sa);

    less_puts("\033[?25l");    // hide cursor
    less_puts("\033[?1049h");  // alt screen

    // Show initial "nothing..." while waiting for data
    u8bReset(less_scr);
    scr_goto(st.rows, 1);
    scr_puts(TTY_INVERSE TTY_ERASE_LINE " nothing..." TTY_RESET);
    LESSScreenFlush();

    b8 pipe_eof = NO;
    b8 quit = NO;
    u32 indexed_nhunks = 0;
    u32 rendered_nhunks = 0;

    while (!quit) {
        if (less_resized) {
            less_resized = 0;
            LESSGetSize(&st);
        }

        struct pollfd fds[2];
        int nfds = 0;
        fds[0].fd = STDIN_FILENO;
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
            while (!$empty(from) && less_nhunks < LESS_MAX_HUNKS) {
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
                LESShunk *hk = &less_hunks[less_nhunks];
                *hk = (LESShunk){};
                if (!$empty(h.title)) {
                    u8p tp = LESSArenaWrite(h.title[0],
                                            (size_t)$len(h.title));
                    if (tp) {
                        hk->title[0] = tp;
                        hk->title[1] = u8bIdleHead(less_arena);
                    }
                }
                if (!$empty(h.text)) {
                    u8p xp = LESSArenaWrite(h.text[0],
                                            (size_t)$len(h.text));
                    if (xp) {
                        hk->text[0] = xp;
                        hk->text[1] = u8bIdleHead(less_arena);
                    }
                }
                if (!$empty(h.toks)) {
                    u8p tkp = LESSArenaWrite(h.toks[0],
                                  (size_t)((u8cp)h.toks[1] -
                                           (u8cp)h.toks[0]));
                    if (tkp) {
                        hk->toks[0] = (u32cp)tkp;
                        hk->toks[1] = (u32cp)u8bIdleHead(less_arena);
                    }
                }
                if (!$empty(h.hili)) {
                    u8p hp = LESSArenaWrite(h.hili[0],
                                 (size_t)((u8cp)h.hili[1] -
                                          (u8cp)h.hili[0]));
                    if (hp) {
                        hk->hili[0] = (u32cp)hp;
                        hk->hili[1] = (u32cp)u8bIdleHead(less_arena);
                    }
                }
                less_nhunks++;
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
            ssize_t nr = read(STDIN_FILENO, &ch, 1);
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
                    LESSReadGoto(&st);
                    changed = YES;
                } else if (ch == '/') {
                    LESSReadSearch(&st);
                    if (st.search_len > 0) {
                        u32 found = LESSSearchNext(&st, st.scroll, +1);
                        if (found != UINT32_MAX) st.scroll = found;
                    }
                    changed = YES;
                } else if (ch == 'n') {
                    u32 found = LESSSearchNext(&st, st.scroll, +1);
                    if (found != UINT32_MAX) { st.scroll = found; changed = YES; }
                } else if (ch == 'N') {
                    u32 found = LESSSearchNext(&st, st.scroll, -1);
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
                } else if (ch == 033) {
                    u8 seq[2] = {};
                    ssize_t n1 = read(STDIN_FILENO, &seq[0], 1);
                    if (n1 > 0 && seq[0] == '[') {
                        ssize_t n2 = read(STDIN_FILENO, &seq[1], 1);
                        if (n2 > 0) {
                            switch (seq[1]) {
                            case 'A': if (st.scroll > 0) st.scroll--; changed = YES; break;
                            case 'B': if (st.scroll + 1 < st.nlines) st.scroll++; changed = YES; break;
                            case '5':
                                (void)read(STDIN_FILENO, &seq[0], 1);
                                if (st.scroll >= page) st.scroll -= page;
                                else st.scroll = 0;
                                changed = YES; break;
                            case '6':
                                (void)read(STDIN_FILENO, &seq[0], 1);
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
        if (less_nhunks > indexed_nhunks) {
            st.nlines = LESSExtendIndex(st.lines, st.nlines,
                                         less_hunks,
                                         indexed_nhunks,
                                         less_nhunks);
            st.nhunks = less_nhunks;
            indexed_nhunks = less_nhunks;
        }
        // Render when: key pressed, or new hunks extend the visible area
        b8 pipe_idle = (pr == 0) || pipe_eof;
        if (indexed_nhunks > rendered_nhunks && pipe_idle) {
            // Skip first title line on first render
            if (rendered_nhunks == 0 && st.nlines > 1) st.scroll = 1;
            u32 visible_end = st.scroll + (u32)(st.rows - 1);
            if (st.nlines <= visible_end || rendered_nhunks == 0)
                changed = YES;
            else if (st.nlines > 0) {
                u8bReset(less_scr);
                LESSStatusBar(&st);
                LESSScreenFlush();
            }
            rendered_nhunks = indexed_nhunks;
        }
        if ((changed || key_pressed) && st.nlines > 0)
            LESSRender(&st);

        // Pipe done, no results — show "nothing!" and wait for quit
        if (pipe_eof && st.nlines == 0 && !quit) {
            u8bReset(less_scr);
            scr_goto(st.rows, 1);
            scr_puts(TTY_INVERSE TTY_ERASE_LINE " nothing!" TTY_RESET);
            LESSScreenFlush();
        }
    }

    // Teardown
    less_puts("\033[?1049l");
    less_puts("\033[?25h");
    LESSRawDisable(&st);
    sigaction(SIGWINCH, &old_sa, NULL);

    range32bFree(st.linesbuf);
    u8bUnMap(rdbuf);
    LESSArenaCleanup();

    done;
}
