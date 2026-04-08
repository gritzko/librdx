#include "HUNK.h"

#include <string.h>

#include "abc/PRO.h"
#include "dog/TOK.h"

ok64 HUNKu8sFeed(u8s into, HUNKhunk const *hk) {
    sane(u8sOK(into) && hk != NULL);
    u8s inner = {};
    call(TLVu8sStart, into, inner, HUNK_TLV);
    if (!$empty(hk->title))
        call(TLVu8sFeed, inner, HUNK_TLV_TTL, hk->title);
    if (!$empty(hk->text))
        call(TLVu8sFeed, inner, HUNK_TLV_TXT, hk->text);
    if (!$empty(hk->toks)) {
        u8cs tkb = {(u8cp)hk->toks[0], (u8cp)hk->toks[1]};
        call(TLVu8sFeed, inner, HUNK_TLV_TOK, tkb);
    }
    if (!$empty(hk->hili)) {
        u8cs hib = {(u8cp)hk->hili[0], (u8cp)hk->hili[1]};
        call(TLVu8sFeed, inner, HUNK_TLV_HILI, hib);
    }
    call(TLVu8sEnd, into, inner, HUNK_TLV);
    done;
}

ok64 HUNKu8sDrain(u8cs from, HUNKhunk *hk) {
    sane($ok(from) && hk != NULL);
    u8 t = 0;
    u8cs body = {};
    call(TLVu8sDrain, from, &t, body);
    test(t == HUNK_TLV, TLVBADTYPE);
    *hk = (HUNKhunk){};
    while (!$empty(body)) {
        u8 st = 0;
        u8cs val = {};
        call(TLVu8sDrain, body, &st, val);
        switch (st) {
        case HUNK_TLV_TTL:
            $mv(hk->title, val);
            break;
        case HUNK_TLV_TXT:
            $mv(hk->text, val);
            break;
        case HUNK_TLV_TOK:
            hk->toks[0] = (tok32c *)val[0];
            hk->toks[1] = (tok32c *)val[1];
            break;
        case HUNK_TLV_HILI:
            hk->hili[0] = (tok32c *)val[0];
            hk->hili[1] = (tok32c *)val[1];
            break;
        default:
            break;
        }
    }
    done;
}

// For a byte at offset `pos`, return the active hili tag (or 0).
static u8 hunk_hili_at(HUNKhunk const *hk, u32 pos) {
    if ($empty(hk->hili)) return 0;
    int n = (int)$len(hk->hili);
    int i = 0;
    while (i < n && tok32Offset(hk->hili[0][i]) <= pos) i++;
    if (i >= n) return 0;
    return tok32Tag(hk->hili[0][i]);
}

ok64 HUNKu8sFeedText(u8s into, HUNKhunk const *hk) {
    sane(u8sOK(into) && hk != NULL);
    if (!$empty(hk->title)) {
        u8sFeed(into, hk->title);
        u8sFeed1(into, '\n');
    }
    u32 tlen = (u32)$len(hk->text);
    b8 has_hili = !$empty(hk->hili);

    if (!has_hili) {
        // Plain hunk (grep / search / cat): emit text verbatim.
        if (tlen > 0) {
            u8cs t = {hk->text[0], hk->text[1]};
            u8sFeed(into, t);
            if (hk->text[0][tlen - 1] != '\n') u8sFeed1(into, '\n');
        }
        u8sFeed1(into, '\n');
        done;
    }

    // Diff hunk: per-line '+' / '-' / ' ' prefix.
    u32 i = 0;
    while (i < tlen) {
        u32 e = i;
        while (e < tlen && hk->text[0][e] != '\n') e++;
        u8 prefix = ' ';
        for (u32 k = i; k < e; k++) {
            u8 t = hunk_hili_at(hk, k);
            if (t == 'I') { prefix = '+'; break; }
            if (t == 'D') { prefix = '-'; break; }
        }
        u8sFeed1(into, prefix);
        u8cs line = {hk->text[0] + i, hk->text[0] + e};
        u8sFeed(into, line);
        u8sFeed1(into, '\n');
        i = (e < tlen) ? e + 1 : e;
    }
    u8sFeed1(into, '\n');
    done;
}

void HUNKu32sClip(Bu8 arena, u32cs out, u32cs toks, u32 lo, u32 hi) {
    out[0] = NULL;
    out[1] = NULL;
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
    u32gp g = u32aOpen(arena);
    for (int i = first; i < last; i++) {
        u8 tag = tok32Tag(toks[0][i]);
        u32 off = tok32Offset(toks[0][i]);
        if (off > hi) off = hi;
        u32 rebased = off - lo;
        u32gFeed1(g, tok32Pack(tag, rebased));
    }
    u32aClose(arena, out);
}

// --- Source tokenizer wrapper around dog/TOK lexer ---

typedef struct {
    u32bp toks;
    u32   off;
} HUNKTokCtx;

static ok64 HUNKTokCB(u8 tag, u8cs tok, void *ctx) {
    sane(ctx != NULL);
    HUNKTokCtx *c = (HUNKTokCtx *)ctx;
    u32 end = c->off + (u32)$len(tok);
    u32 packed = tok32Pack(tag, end);
    call(u32bFeed1, c->toks, packed);
    c->off = end;
    return OK;
}

ok64 HUNKu32bTokenize(u32bp toks, u8csc source, u8csc ext) {
    sane(toks != NULL && $ok(source));
    if ($empty(source)) done;

    HUNKTokCtx ctx = {.toks = toks, .off = 0};

    u8cs ext_nodot = {};
    if (!$empty(ext) && ext[0][0] == '.') {
        ext_nodot[0] = ext[0] + 1;
        ext_nodot[1] = ext[1];
    } else {
        $mv(ext_nodot, ext);
    }

    TOKstate ts = {
        .data = {source[0], source[1]},
        .cb = HUNKTokCB,
        .ctx = &ctx,
    };
    call(TOKLexer, &ts, ext_nodot);
    done;
}

void HUNKu8sExt(u8cs out, u8cp path, size_t len) {
    out[0] = NULL;
    out[1] = NULL;
    for (size_t i = len; i > 0; i--) {
        if (path[i - 1] == '/') break;
        if (path[i - 1] == '.') {
            out[0] = path + i - 1;
            out[1] = path + len;
            break;
        }
    }
}

ok64 HUNKu8sFormatTitle(u8s into, char const *filepath, char const *funcname) {
    sane(into[0] != NULL);
    u8p start = into[0];
    if (filepath && funcname && funcname[0]) {
        call(u8sPrintf, into, "--- %s :: %s ---", filepath, funcname);
    } else if (filepath) {
        call(u8sPrintf, into, "--- %s ---", filepath);
    } else if (funcname && funcname[0]) {
        call(u8sPrintf, into, "--- %s ---", funcname);
    } else {
        done;
    }

    int hlen = (int)(into[0] - start);
    if (hlen > HUNK_TITLE_MAX && filepath && funcname && funcname[0]) {
        size_t plen = strlen(filepath);
        int budget = HUNK_TITLE_MAX - 12 - (int)plen;
        if (budget < 1) budget = 1;
        into[0] = start;
        call(u8sPrintf, into, "--- %s :: %.*s ---",
             filepath, budget, funcname);
        hlen = (int)(into[0] - start);
    }
    if (hlen > HUNK_TITLE_MAX && filepath) {
        char const *p = filepath + strlen(filepath);
        int budget = HUNK_TITLE_MAX - 12 - 3;
        if (funcname && funcname[0]) {
            size_t flen = strlen(funcname);
            if (flen > 20) flen = 20;
            budget -= (int)flen + 4;
        }
        if (budget < 8) budget = 8;
        while (p > filepath && (int)(filepath + strlen(filepath) - p) < budget)
            p--;
        into[0] = start;
        if (funcname && funcname[0]) {
            int favail = HUNK_TITLE_MAX - 12 - 3 -
                         (int)(filepath + strlen(filepath) - p);
            if (favail < 1) favail = 1;
            call(u8sPrintf, into, "--- ...%s :: %.*s ---",
                 p, favail, funcname);
        } else {
            call(u8sPrintf, into, "--- ...%s ---", p);
        }
    }
    done;
}
