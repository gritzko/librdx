//
// tokjoin - Token-level 3-way file merge CLI
//
// Usage: tokjoin base ours theirs [-o output]
// Git merge driver: tokjoin %O %A %B -o %A
//

#include "JOIN.h"

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"

static u8 _buf0[1 << 24];
static u8 _buf1[1 << 24];
static u8 _buf2[1 << 24];

static ok64 join_read(u8cs *data, u8p buf, u64 bufsz, u8csc path_arg) {
    sane(data != NULL && buf != NULL);
    a_pad(u8, path, 4096);
    call(u8sFeed, path_idle, path_arg);
    PATHu8bTerm(path);
    u8s idle = {buf, buf + bufsz};
    u8p start = idle[0];
    int fd;
    call(FILEOpen, &fd, PATHu8cgIn(path), O_RDONLY);
    call(FILEdrainall, idle, fd);
    call(FILEClose, &fd);
    (*data)[0] = start;
    (*data)[1] = idle[0];
    done;
}

static ok64 join_ext(u8cs *ext, u8csc path) {
    sane(ext != NULL);
    u8cp scan = path[1];
    while (scan > path[0]) {
        --scan;
        if (*scan == '.') {
            (*ext)[0] = (u8c *)(scan + 1);
            (*ext)[1] = (u8c *)path[1];
            done;
        }
        if (*scan == '/') break;
    }
    fail(TOKBAD);
}

static ok64 join_write(u8csc data, u8csc path_arg) {
    sane(!$empty(data));
    a_pad(u8, path, 4096);
    call(u8sFeed, path_idle, path_arg);
    PATHu8bTerm(path);
    int fd = open((char *)path[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) fail(FILEFAIL);
    call(FILEFeedall, fd, data);
    call(FILEClose, &fd);
    done;
}

ok64 tokjoin() {
    sane($arglen >= 4);
    a$rg(base_arg, 1);
    a$rg(ours_arg, 2);
    a$rg(theirs_arg, 3);

    // Optional -o output_path
    u8cs out_path = {NULL, NULL};
    for (u64 i = 4; i + 1 < (u64)$arglen; i++) {
        a$rg(flag, i);
        if ($len(flag) == 2 && flag[0][0] == '-' && flag[0][1] == 'o') {
            a$rg(opath, i + 1);
            $set(out_path, opath);
            break;
        }
    }

    // Detect extension from ours path
    u8cs ext = {};
    call(join_ext, &ext, ours_arg);

    // Read three files
    u8cs base_data = {}, ours_data = {}, theirs_data = {};
    call(join_read, &base_data, _buf0, sizeof(_buf0), base_arg);
    call(join_read, &ours_data, _buf1, sizeof(_buf1), ours_arg);
    call(join_read, &theirs_data, _buf2, sizeof(_buf2), theirs_arg);

    // Tokenize
    JOINfile base = {}, ours = {}, theirs = {};
    call(JOINTokenize, &base, base_data, ext);
    call(JOINTokenize, &ours, ours_data, ext);
    call(JOINTokenize, &theirs, theirs_data, ext);

    // Merge
    u64 outsz = $len(ours_data) + $len(theirs_data) + 4096;
    u8 *out[4] = {};
    call(u8bAlloc, out, outsz);
    call(JOINMerge, out, &base, &ours, &theirs);

    // Output
    u8csc result = {out[1], out[2]};
    if (out_path[0] != NULL) {
        call(join_write, result, out_path);
    } else {
        call(FILEFeedall, STDOUT_FILENO, result);
    }

    JOINFree(&base);
    JOINFree(&ours);
    JOINFree(&theirs);
    u8bFree(out);
    done;
}

MAIN(tokjoin);
