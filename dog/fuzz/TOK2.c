#include "TOK.h"

#include "abc/TEST.h"

//  See dog/fuzz/TOK.c for the canonical tag alphabet.
//  D G H L N P R S W.
static ok64 TOK2FUZZcb(u8 tag, u8cs tok, void *ctx) {
    (void)ctx;
    must(tag == 'D' || tag == 'G' || tag == 'H' || tag == 'L' ||
         tag == 'N' || tag == 'P' || tag == 'R' || tag == 'S' ||
         tag == 'W',
         "bad tag");
    must($ok(tok), "bad tok");
    must(!$empty(tok), "empty tok");
    return OK;
}

FUZZ(u8, TOK2fuzz) {
    sane(1);

    if ($len(input) < 2) done;
    if ($len(input) > 4096) done;

    // First bytes up to \n (max 5) are the extension
    u8c *p = input[0];
    u8c *end = input[1];
    u8c *nl = p;
    while (nl < end && *nl != '\n' && nl - p < 5) ++nl;
    if (nl >= end || *nl != '\n') done;

    u8csc ext = {p, nl};
    u8c *body = nl + 1;
    if (body >= end) done;

    TOKstate state = {
        .data = {body, end},
        .cb = TOK2FUZZcb,
        .ctx = NULL,
    };
    (void)TOKLexer(&state, ext);

    done;
}
