//
// DEF01 — data-driven definition marking tests
//
// Reads source files from tok/test/data/, tokenizes + DEFMark,
// compares N-tagged identifiers against .defs expected output.
//

#include "DEF.h"
#include "TOK.h"

#include "abc/PRO.h"
#include "abc/TEST.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef TEST_DATA_DIR
#error "TEST_DATA_DIR must be defined"
#endif

static u8 g_srcbuf[1 << 20];
static u32 g_tokbuf[1 << 16];
static u8 g_outbuf[1 << 16];
static u8 g_expbuf[1 << 16];

typedef struct {
    u8c *base;
    u32 ntoks;
} DEF01ctx;

static ok64 def01_cb(u8 tag, u8cs tok, void *vctx) {
    sane(vctx != NULL);
    DEF01ctx *c = vctx;
    if (c->ntoks >= (1u << 16)) return DEFFAIL;
    u32 end = (u32)(tok[1] - c->base);
    g_tokbuf[c->ntoks++] = tok32Pack(tag,end);
    done;
}

static ok64 read_file(const char *path, u8 *buf, u64 bufsz, u64 *out_len) {
    sane(path != NULL);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return DEFFAIL;
    struct stat st;
    if (fstat(fd, &st) < 0) { close(fd); return DEFFAIL; }
    u64 sz = (u64)st.st_size;
    if (sz > bufsz) { close(fd); return DEFFAIL; }
    u64 rd = 0;
    while (rd < sz) {
        ssize_t n = read(fd, buf + rd, sz - rd);
        if (n <= 0) break;
        rd += (u64)n;
    }
    close(fd);
    *out_len = rd;
    done;
}

typedef struct {
    const char *file;
    const char *ext;
} DEF01case;

static const DEF01case CASES[] = {
    {"test.c",     "c"},
    {"test.go",    "go"},
    {"test.py",    "py"},
    {"test.rs",    "rs"},
    {"test.js",    "js"},
    {"test.ts",    "ts"},
    {"test.java",  "java"},
    {"test.cs",    "cs"},
    {"test.kt",    "kt"},
    {"test.swift", "swift"},
    {"test.dart",  "dart"},
    {"test.zig",   "zig"},
    {NULL, NULL},
};

ok64 test_def01() {
    sane(1);

    for (const DEF01case *tc = CASES; tc->file; tc++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", TEST_DATA_DIR, tc->file);
        char defpath[512];
        snprintf(defpath, sizeof(defpath), "%s/%s.defs", TEST_DATA_DIR, tc->file);

        // read source
        u64 srclen = 0;
        ok64 o = read_file(path, g_srcbuf, sizeof(g_srcbuf), &srclen);
        if (o != OK) {
            fprintf(stderr, "SKIP %s: cannot read\n", tc->file);
            continue;
        }
        u8csc src = {g_srcbuf, g_srcbuf + srclen};
        u8csc ext = u8scstr(tc->ext);

        // tokenize
        DEF01ctx ctx = {.base = src[0], .ntoks = 0};
        TOKstate st = {.data = {src[0], src[1]}, .cb = def01_cb, .ctx = &ctx};
        call(TOKLexer, &st, ext);

        // DEFMark
        u32 *ts[2] = {g_tokbuf, g_tokbuf + ctx.ntoks};
        call(DEFMark, ts, src, ext);

        // collect N-tagged names
        a_dup(u32c, tsc, ts);
        u8s out = {g_outbuf, g_outbuf + sizeof(g_outbuf)};
        for (u32 i = 0; i < ctx.ntoks; i++) {
            u8 tag = tok32Tag(g_tokbuf[i]);
            if (tag != 'N') continue;
            u8cs val;
            tok32Val(val, tsc, src[0], i);
            call(u8sFeed, out, val);
            call(u8sFeed1, out, '\n');
        }
        u64 got_len = out[0] - g_outbuf;

        // read expected .defs
        u64 exp_len = 0;
        o = read_file(defpath, g_expbuf, sizeof(g_expbuf), &exp_len);
        if (o != OK) {
            fprintf(stderr, "FAIL %s: no .defs file\n", tc->file);
            fail(TESTFAIL);
        }

        if (got_len != exp_len ||
            memcmp(g_outbuf, g_expbuf, got_len) != 0) {
            fprintf(stderr, "FAIL %s:\n  got:\n%.*s  want:\n%.*s",
                    tc->file, (int)got_len, g_outbuf,
                    (int)exp_len, g_expbuf);
            fail(TESTFAIL);
        }
    }
    done;
}

TEST(test_def01)
