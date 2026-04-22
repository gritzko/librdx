//  AT: per-worktree at.log.  See sniff/AT.md.
//
#include "AT.h"
#include "SNIFF.h"

#include <fcntl.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "keeper/KEEP.h"

//  <reporoot>/.sniff/at.log
static u8c *const AT_REL_S[2] = {
    (u8c *)".sniff/" SNIFF_AT_FILE,
    (u8c *)".sniff/" SNIFF_AT_FILE
        + sizeof(".sniff/" SNIFF_AT_FILE) - 1,
};

//  Fill `out_line` with the last complete line of `full` (trailing '\n'
//  stripped).  Empty slice if file has no content.
static void at_tail_line(u8cs out_line, u8cs full) {
    out_line[0] = full[0];
    out_line[1] = full[0];
    u8cp base = full[0];
    u64 n = (u64)(full[1] - base);
    if (n == 0) return;
    u64 end = n;
    while (end > 0 && base[end - 1] == '\n') end--;
    if (end == 0) return;
    u64 start = end;
    while (start > 0 && base[start - 1] != '\n') start--;
    out_line[0] = base + start;
    out_line[1] = base + end;
}

//  Parse a line: `<ron60>\t?<branch>\t?<sha>` (no trailing '\n').
static ok64 at_parse(sniff_at *out, u8cs line) {
    sane(out);
    if (u8csEmpty(line)) fail(KEEPNONE);

    a_dup(u8c, rest, line);
    //  timestamp up to first '\t'
    a_dup(u8c, scan, rest);
    if (u8csFind(scan, '\t') != OK) fail(SNIFFFAIL);
    u8cs ts_str = {rest[0], scan[0]};
    rest[0] = scan[0] + 1;
    ok64 time_val = 0;
    a_dup(u8c, ts_dup, ts_str);
    (void)RONutf8sDrain(&time_val, ts_dup);
    out->time = time_val;

    //  branch: '?' + chars + '\t'
    if (u8csEmpty(rest) || *rest[0] != '?') fail(SNIFFFAIL);
    u8csUsed(rest, 1);
    a_dup(u8c, scan2, rest);
    if (u8csFind(scan2, '\t') != OK) fail(SNIFFFAIL);
    u8cs branch_slice = {rest[0], scan2[0]};
    rest[0] = scan2[0] + 1;
    u8bReset(out->branch);
    u8bFeed(out->branch, branch_slice);

    //  sha: '?' + 40 hex
    if (u8csEmpty(rest) || *rest[0] != '?') fail(SNIFFFAIL);
    u8csUsed(rest, 1);
    if (u8csLen(rest) != 40) fail(SNIFFFAIL);
    u8bReset(out->sha);
    u8bFeed(out->sha, rest);
    done;
}

ok64 SNIFFAtRead(sniff_at *out) {
    sane(out && out->branch && out->sha && SNIFF.h);

    a_dup(u8c, root, u8bDataC(SNIFF.h->root));
    a_path(apath, root, AT_REL_S);

    u8bp map = NULL;
    ok64 o = FILEMapRO(&map, $path(apath));
    if (o != OK) fail(KEEPNONE);

    a_dup(u8c, full, u8bData(map));
    u8cs line = {};
    at_tail_line(line, full);
    if (u8csEmpty(line)) {
        u8bUnMap(map);
        fail(KEEPNONE);
    }
    o = at_parse(out, line);
    u8bUnMap(map);
    return o;
}

ok64 SNIFFAtAppend(u8cs branch, u8cs sha) {
    //  branch may be empty (detached); only its length matters.
    sane($ok(sha) && SNIFF.h);
    if (u8csLen(sha) != 40) fail(SNIFFFAIL);
    (void)branch;

    a_dup(u8c, root, u8bDataC(SNIFF.h->root));
    a_path(apath, root, AT_REL_S);

    int fd = open((char *)u8bDataHead(apath),
                  O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) fail(SNIFFFAIL);

    a_pad(u8, line, 256);
    RONutf8sFeed(u8bIdle(line), RONNow());
    u8bFeed1(line, '\t');
    u8bFeed1(line, '?');
    if (!u8csEmpty(branch)) u8bFeed(line, branch);
    u8bFeed1(line, '\t');
    u8bFeed1(line, '?');
    u8bFeed(line, sha);
    u8bFeed1(line, '\n');

    a_dup(u8c, data, u8bData(line));
    ok64 o = FILEFeed(fd, data);
    close(fd);
    if (o != OK) fail(SNIFFFAIL);
    done;
}
