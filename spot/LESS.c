#include "LESS.h"

#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
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
    ((u8 **)less_arena)[2] = p + len;
    return p;
}

// Reserve len bytes in the arena (zeroed), return pointer
u8p LESSArenaAlloc(size_t len) {
    if (u8bIdleLen(less_arena) < len) return NULL;
    u8p p = u8bIdleHead(less_arena);
    memset(p, 0, len);
    ((u8 **)less_arena)[2] = p + len;
    return p;
}

// Defer file+toks cleanup until after LESSRun
void LESSDefer(u8bp mapped, Bu32 toks) {
    if (less_nmaps >= LESS_MAX_MAPS) return;
    less_maps[less_nmaps] = mapped;
    memcpy(less_toks[less_nmaps], toks, sizeof(Bu32));
    less_nmaps++;
}

// --- LESSHunkEmit: serialize + write to pipe in fork mode ---

void LESSHunkEmit(void) {
    u32 idx = less_nhunks++;
    if (less_pipe_fd < 0) return;  // sync mode: nothing more to do

    LESShunk *hk = &less_hunks[idx];
    HUNKhunk h = {};
    $mv(h.title, hk->title);
    $mv(h.text, hk->text);
    $mv(h.hili, hk->lits);

    a_pad(u8, buf, 1 << 16);  // 64KB serialize buffer
    if (HUNKu8sFeed(u8bIdle(buf), &h) != OK) {
        less_nhunks--;
        return;
    }

    u8cp p = buf[1];
    u8cp e = buf[2];
    while (p < e) {
        ssize_t w = write(less_pipe_fd, p, (size_t)(e - p));
        if (w <= 0) break;
        p += w;
    }

    less_nhunks--;  // recycle slot: data is in the pipe now
}

// 256-color ink violet for hunk titles
#define LESS_TITLE_COLOR "\033[38;5;56m"

// Tag-index-to-ANSI-color mapping.
// lits stores tag - 'A' (0-25), so add 'A' back to recover the character.
// Returns fg color in lower bits; sets *bold = YES for definitions.
static int LESSTagColor(u8 tag_idx, b8 *bold) {
    *bold = NO;
    switch (tag_idx + 'A') {
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
// A "line" is a region ending at '\n' or at end of hunk text.

typedef struct {
    u32 hunk;   // hunk index
    u32 off;    // byte offset within hunk->text
} LESSline;

// A title separator is stored as hunk index with off=UINT32_MAX
#define LESS_TITLE_LINE UINT32_MAX

typedef struct {
    LESShunk const *hunks;
    u32 nhunks;
    LESSline *lines;   // line index array (malloc'd)
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
        u8cp p = st->hunks[h].text[0];
        u8cp e = st->hunks[h].text[1];
        if (p == e) continue;
        total++;  // at least one line
        while (p < e) {
            if (*p == '\n') total++;
            p++;
        }
        // If text ends with '\n', we overcounted (the '\n' at end doesn't start a new visible line)
        if (e > st->hunks[h].text[0] && *(e - 1) == '\n') total--;
    }

    st->lines = (LESSline *)malloc(total * sizeof(LESSline));
    test(st->lines != NULL, NOROOM);

    u32 li = 0;
    for (u32 h = 0; h < st->nhunks; h++) {
        // Title separator line
        if (!$empty(st->hunks[h].title)) {
            st->lines[li++] = (LESSline){h, LESS_TITLE_LINE};
        }
        u8cp base = st->hunks[h].text[0];
        u8cp e = st->hunks[h].text[1];
        if (base == e) continue;
        u32 lstart = 0;
        u32 tlen = (u32)(e - base);
        st->lines[li++] = (LESSline){h, 0};
        for (u32 i = 0; i < tlen; i++) {
            if (base[i] == '\n' && i + 1 < tlen) {
                st->lines[li++] = (LESSline){h, i + 1};
            }
        }
    }
    st->nlines = li;
    done;
}

// --- Rendering ---

// Write a positioned cursor escape: \033[row;colH
static void less_goto(int row, int col) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "\033[%d;%dH", row, col);
    (void)write(STDOUT_FILENO, buf, (size_t)n);
}

// Write raw bytes
static void less_write(char const *s, size_t n) {
    (void)write(STDOUT_FILENO, s, n);
}

static void less_puts(char const *s) {
    less_write(s, strlen(s));
}

// Emit one byte with appropriate ANSI color based on lits
static void less_emit_byte(u8 ch, u8 lit, b8 in_search, LESSstate *st) {
    (void)st;
    u8 tag_idx = lit & LESS_TAG;
    b8 is_ins = (lit & LESS_INS) != 0;
    b8 is_del = (lit & LESS_DEL) != 0;

    // Build escape: fg from tag, bg from INS/DEL or search
    char esc[64];
    int elen = 0;
    b8 bold = NO;
    int fg = LESSTagColor(tag_idx, &bold);
    if (fg != 0 || bold || is_ins || is_del || in_search) {
        elen = snprintf(esc, sizeof(esc), "\033[");
        if (bold) { elen += snprintf(esc + elen, sizeof(esc) - (size_t)elen, "1"); }
        if (fg != 0) {
            if (bold) esc[elen++] = ';';
            elen += snprintf(esc + elen, sizeof(esc) - (size_t)elen, "%d", fg);
        }
        if (is_ins) {
            if (fg != 0 || bold) esc[elen++] = ';';
            elen += snprintf(esc + elen, sizeof(esc) - (size_t)elen, "48;5;157");
        } else if (is_del) {
            if (fg != 0 || bold) esc[elen++] = ';';
            elen += snprintf(esc + elen, sizeof(esc) - (size_t)elen, "48;5;217");
        } else if (in_search) {
            if (fg != 0 || bold) esc[elen++] = ';';
            elen += snprintf(esc + elen, sizeof(esc) - (size_t)elen, "7");
        }
        esc[elen++] = 'm';
        less_write(esc, (size_t)elen);
        less_write((char *)&ch, 1);
        less_puts(TTY_RESET);
    } else {
        less_write((char *)&ch, 1);
    }
}

// Check if search pattern matches at position pos in the given text
static b8 less_search_at(LESSstate *st, u8cp text, u32 textlen, u32 pos) {
    if (st->search_len == 0) return NO;
    if (pos + st->search_len > textlen) return NO;
    return memcmp(text + pos, st->search, st->search_len) == 0 ? YES : NO;
}

static void LESSRender(LESSstate *st) {
    less_puts(TTY_CUR_HOME);

    u32 visible = (u32)(st->rows - 1);  // last line is status bar
    // Clamp scroll so content fills the screen when possible
    if (st->nlines > visible) {
        if (st->scroll > st->nlines - visible)
            st->scroll = st->nlines - visible;
    } else {
        st->scroll = 0;
    }
    u32 end = st->scroll + visible;
    if (end > st->nlines) end = st->nlines;

    for (u32 vi = st->scroll; vi < end; vi++) {
        u32 row = vi - st->scroll + 1;
        less_goto((int)row, 1);
        less_puts(TTY_ERASE_LINE);

        LESSline *ln = &st->lines[vi];
        LESShunk const *hk = &st->hunks[ln->hunk];

        if (ln->off == LESS_TITLE_LINE) {
            less_puts(LESS_TITLE_COLOR);
            u32 tlen = (u32)$len(hk->title);
            u32 w = tlen < st->cols ? tlen : st->cols;
            less_write((char const *)hk->title[0], w);
            less_puts(TTY_RESET);
            continue;
        }

        // Regular line: find extent (up to next '\n' or end of text)
        u32 textlen = (u32)$len(hk->text);
        u32 off = ln->off;
        u32 line_end = off;
        while (line_end < textlen && hk->text[0][line_end] != '\n')
            line_end++;

        u32 w = line_end - off;
        if (w > st->cols) w = st->cols;

        // Emit bytes with coloring
        u8cp text = hk->text[0];
        u8cp lits = $empty(hk->lits) ? NULL : hk->lits[0];
        u32 lits_len = $empty(hk->lits) ? 0 : (u32)$len(hk->lits);

        // Track search matches within this line
        for (u32 j = 0; j < w; j++) {
            u32 pos = off + j;
            u8 ch = text[pos];
            u8 lit = (lits != NULL && pos < lits_len) ? lits[pos] : 0;
            b8 in_search = less_search_at(st, text, textlen, pos);
            less_emit_byte(ch, lit, in_search, st);
        }
    }

    // Clear any leftover lines below content
    for (u32 row = end - st->scroll + 1; row < st->rows; row++) {
        less_goto((int)row, 1);
        less_puts(TTY_ERASE_LINE);
    }

    // Status bar on last line
    less_goto(st->rows, 1);
    less_puts(TTY_INVERSE);
    less_puts(TTY_ERASE_LINE);

    // Find current hunk title for status
    char status[512];
    u32 cur_hunk = 0;
    if (st->scroll < st->nlines)
        cur_hunk = st->lines[st->scroll].hunk;

    // Count logical line from start of this hunk
    u32 hunk_line = 0;
    for (u32 i = 0; i < st->scroll && i < st->nlines; i++) {
        if (st->lines[i].hunk == cur_hunk &&
            st->lines[i].off != LESS_TITLE_LINE)
            hunk_line++;
    }

    LESShunk const *ch = &st->hunks[cur_hunk];
    int slen;
    if (st->search_len > 0) {
        slen = snprintf(status, sizeof(status),
                        " %.*s  line %u/%u  [/%.*s]",
                        (int)$len(ch->title), (char *)ch->title[0],
                        st->scroll + 1, st->nlines,
                        (int)st->search_len, st->search);
    } else {
        slen = snprintf(status, sizeof(status),
                        " %.*s  line %u/%u",
                        (int)$len(ch->title), (char *)ch->title[0],
                        st->scroll + 1, st->nlines);
    }
    if (slen < 0) slen = 0;
    u32 sw = (u32)slen < st->cols ? (u32)slen : st->cols;
    less_write(status, sw);
    for (u32 j = sw; j < st->cols; j++)
        less_write(" ", 1);
    less_puts(TTY_RESET);
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
        LESSline *ln = &st->lines[i];
        if (ln->off == LESS_TITLE_LINE) continue;
        LESShunk const *hk = &st->hunks[ln->hunk];
        u32 textlen = (u32)$len(hk->text);
        u32 off = ln->off;
        u32 line_end = off;
        while (line_end < textlen && hk->text[0][line_end] != '\n')
            line_end++;
        // Search within this line
        u32 w = line_end - off;
        if (w >= st->search_len) {
            u32 limit = w - st->search_len + 1;
            for (u32 j = 0; j < limit; j++) {
                if (memcmp(hk->text[0] + off + j, st->search,
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
        if (st->search_len > 0)
            less_write(st->search, st->search_len);
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

// --- Fallback: plain output (when piped) ---

static ok64 LESSPlain(LESShunk const *hunks, u32 nhunks) {
    sane(hunks != NULL);
    for (u32 h = 0; h < nhunks; h++) {
        if (!$empty(hunks[h].title)) {
            if (CAPO_COLOR)
                fprintf(stdout, LESS_TITLE_COLOR "%.*s" TTY_RESET "\n",
                        (int)$len(hunks[h].title), (char *)hunks[h].title[0]);
            else
                fprintf(stdout, "%.*s\n",
                        (int)$len(hunks[h].title), (char *)hunks[h].title[0]);
        }
        if (!$empty(hunks[h].text)) {
            u8cp text = hunks[h].text[0];
            u8cp lits = $empty(hunks[h].lits) ? NULL : hunks[h].lits[0];
            u32 tlen = (u32)$len(hunks[h].text);
            u32 llen = $empty(hunks[h].lits) ? 0 : (u32)$len(hunks[h].lits);
            u32 i = 0;
            int prev_fg = 0;
            int prev_bg = 0;
            b8 prev_bold = NO;
            while (i < tlen) {
                u8 lit = (lits != NULL && i < llen) ? lits[i] : 0;
                b8 bold = NO;
                int fg = LESSTagColor(lit & LESS_TAG, &bold);
                int bg = 0;
                if (lit & LESS_INS) bg = 157;
                else if (lit & LESS_DEL) bg = 217;
                if (fg != prev_fg || bg != prev_bg || bold != prev_bold) {
                    if (prev_fg != 0 || prev_bg != 0 || prev_bold)
                        fputs(TTY_RESET, stdout);
                    if (bold && fg != 0 && bg != 0)
                        fprintf(stdout, "\033[1;%d;48;5;%dm", fg, bg);
                    else if (bold && fg != 0)
                        fprintf(stdout, "\033[1;%dm", fg);
                    else if (bold && bg != 0)
                        fprintf(stdout, "\033[1;48;5;%dm", bg);
                    else if (bold)
                        fputs(TTY_BOLD, stdout);
                    else if (fg != 0 && bg != 0)
                        fprintf(stdout, "\033[%d;48;5;%dm", fg, bg);
                    else if (fg != 0)
                        fprintf(stdout, "\033[%dm", fg);
                    else if (bg != 0)
                        fprintf(stdout, "\033[48;5;%dm", bg);
                    prev_fg = fg;
                    prev_bg = bg;
                    prev_bold = bold;
                }
                fputc(text[i], stdout);
                if (text[i] == '\n' && (prev_fg != 0 || prev_bg != 0 || prev_bold)) {
                    fputs(TTY_RESET, stdout);
                    prev_fg = 0;
                    prev_bg = 0;
                    prev_bold = NO;
                }
                i++;
            }
            if (prev_fg != 0 || prev_bg != 0 || prev_bold)
                fputs(TTY_RESET, stdout);
            // Trailing newline if text doesn't end with one
            if (tlen > 0 && text[tlen - 1] != '\n')
                fputc('\n', stdout);
            // Blank line between titled sections
            if (h + 1 >= nhunks || !$empty(hunks[h + 1].title))
                fputc('\n', stdout);
        }
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
    free(st.lines);

    done;
}

// --- Pipe pager: incremental display of TLV hunks from a pipe ---

#define PIPE_RDBUF_SIZE (1UL << 22)  // 4MB read buffer
#define PIPE_MAX_LINES  (1UL << 20)  // 1M lines max

// Count lines for hunks [from..nhunks), append to lines array.
// Returns new total nlines.
static u32 LESSExtendIndex(LESSline *lines, u32 nlines,
                            LESShunk const *hunks,
                            u32 from, u32 nhunks) {
    u32 li = nlines;
    for (u32 h = from; h < nhunks; h++) {
        if (!$empty(hunks[h].title)) {
            if (li < PIPE_MAX_LINES)
                lines[li++] = (LESSline){h, LESS_TITLE_LINE};
        }
        u8cp base = hunks[h].text[0];
        u8cp e = hunks[h].text[1];
        if (base == e) continue;
        u32 tlen = (u32)(e - base);
        if (li < PIPE_MAX_LINES)
            lines[li++] = (LESSline){h, 0};
        for (u32 i = 0; i < tlen; i++) {
            if (base[i] == '\n' && i + 1 < tlen) {
                if (li < PIPE_MAX_LINES)
                    lines[li++] = (LESSline){h, i + 1};
            }
        }
    }
    return li;
}

ok64 LESSPipeRun(int pipefd) {
    sane(pipefd >= 0);

    call(LESSArenaInit);

    // Allocate read buffer (mmap anonymous)
    u8p rdbuf = (u8p)mmap(NULL, PIPE_RDBUF_SIZE,
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    test(rdbuf != MAP_FAILED, NOROOM);
    u32 rdlen = 0;  // bytes accumulated in rdbuf

    // Allocate line index
    LESSline *lines = (LESSline *)malloc(PIPE_MAX_LINES * sizeof(LESSline));
    if (lines == NULL) {
        munmap(rdbuf, PIPE_RDBUF_SIZE);
        return NOROOM;
    }

    LESSstate st = {};
    st.hunks = less_hunks;
    st.nhunks = 0;

    LESSGetSize(&st);
    st.lines = lines;
    st.nlines = 0;

    call(LESSRawEnable, &st);

    struct sigaction sa = {}, old_sa = {};
    sa.sa_handler = less_winch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, &old_sa);

    less_puts("\033[?25l");    // hide cursor
    less_puts("\033[?1049h");  // alt screen

    b8 pipe_eof = NO;
    b8 quit = NO;
    u32 indexed_nhunks = 0;

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

        int pr = poll(fds, (nfds_t)nfds, 100);
        (void)pr;

        b8 changed = NO;

        // Read from pipe
        if (!pipe_eof && (fds[1].revents & (POLLIN | POLLHUP))) {
            u32 space = PIPE_RDBUF_SIZE - rdlen;
            if (space > 0) {
                ssize_t nr = read(pipefd, rdbuf + rdlen, space);
                if (nr > 0) {
                    rdlen += (u32)nr;
                } else if (nr == 0) {
                    pipe_eof = YES;
                }
            }

            // Drain complete TLV records
            u8cs from = {rdbuf, rdbuf + rdlen};
            while (!$empty(from) && less_nhunks < LESS_MAX_HUNKS) {
                u8cs save = {from[0], from[1]};
                HUNKhunk h = {};
                ok64 o = HUNKu8sDrain(from, &h);
                if (o != OK) {
                    // Incomplete record — restore and wait for more
                    $mv(from, save);
                    break;
                }
                // Copy fields into arena
                LESShunk *hk = &less_hunks[less_nhunks];
                memset(hk, 0, sizeof(*hk));
                if (!$empty(h.title)) {
                    u8p tp = LESSArenaWrite(h.title[0],
                                            (size_t)$len(h.title));
                    if (tp) {
                        hk->title[0] = tp;
                        hk->title[1] = tp + $len(h.title);
                    }
                }
                if (!$empty(h.text)) {
                    u8p xp = LESSArenaWrite(h.text[0],
                                            (size_t)$len(h.text));
                    if (xp) {
                        hk->text[0] = xp;
                        hk->text[1] = xp + $len(h.text);
                    }
                }
                if (!$empty(h.hili)) {
                    u8p lp = LESSArenaWrite(h.hili[0],
                                            (size_t)$len(h.hili));
                    if (lp) {
                        hk->lits[0] = lp;
                        hk->lits[1] = lp + $len(h.hili);
                    }
                }
                less_nhunks++;
                changed = YES;
            }

            // Shift remaining bytes to front
            u32 consumed = (u32)(from[0] - rdbuf);
            if (consumed > 0 && consumed < rdlen) {
                memmove(rdbuf, rdbuf + consumed, rdlen - consumed);
                rdlen -= consumed;
            } else if (consumed >= rdlen) {
                rdlen = 0;
            }
        }

        // Extend line index for new hunks
        if (less_nhunks > indexed_nhunks) {
            st.nlines = LESSExtendIndex(lines, st.nlines,
                                         less_hunks,
                                         indexed_nhunks,
                                         less_nhunks);
            st.nhunks = less_nhunks;
            indexed_nhunks = less_nhunks;
            changed = YES;
        }

        // Handle keyboard input
        if (fds[0].revents & POLLIN) {
            u8 ch = 0;
            ssize_t nr = read(STDIN_FILENO, &ch, 1);
            if (nr > 0) {
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

        if (changed && st.nlines > 0)
            LESSRender(&st);
    }

    // Teardown
    less_puts("\033[?1049l");
    less_puts("\033[?25h");
    LESSRawDisable(&st);
    sigaction(SIGWINCH, &old_sa, NULL);

    free(lines);
    munmap(rdbuf, PIPE_RDBUF_SIZE);
    LESSArenaCleanup();

    done;
}
