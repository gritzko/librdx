#include "BASON.h"

#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

// bason: convert between JSON, BASON binary, and BASON text dump.
// Usage: bason <input> [output]
// Input format auto-detected (JSON if starts with '{' or '[', else BASON).
// Output format determined by extension: .json .bason .txt
// No output arg: JSON input → BASON to stdout, BASON input → text to stdout.

#define BASON_BUF_LEN (4 * 1024 * 1024)

static b8 basonIsJSON(u8csc data) {
    u8cp p = data[0];
    while (p < data[1] && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        ++p;
    if (p >= data[1]) return NO;
    return *p == '{' || *p == '[';
}

// 0=bason, 1=json, 2=txt
static u8 basonOutFmt(u8csc name) {
    size_t n = $len(name);
    if (n >= 5 && memcmp(name[1] - 5, ".json", 5) == 0) return 1;
    if (n >= 6 && memcmp(name[1] - 6, ".bason", 6) == 0) return 0;
    if (n >= 4 && memcmp(name[1] - 4, ".txt", 4) == 0) return 2;
    return 0;  // default: binary bason
}

ok64 basoncli() {
    sane(1);
    if ($arglen < 2 || $arglen > 3) {
        a_cstr(u1, "Usage: bason <input> [output]\n");
        a_cstr(u2, "Output ext: .json .bason .txt\n");
        FILEerr(u1);
        FILEerr(u2);
        fail(BADARG);
    }

    call(FILEInit);

    a$rg(arg, 1);
    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, arg);
    u8bFeed1(path, 0);
    u8bShed1(path);

    u8bp mapped = NULL;
    call(FILEMapRO, &mapped, PATHu8cgIn(path));
    u8cp i0 = u8bDataHead(mapped), i1 = u8bIdleHead(mapped);
    u8cs indata = {i0, i1};

    // Determine output format
    u8 ofmt;
    if ($arglen == 3) {
        a$rg(oarg, 2);
        ofmt = basonOutFmt(oarg);
    } else {
        ofmt = basonIsJSON(indata) ? 0 : 2;  // json→bason, bason→txt
    }

    // Parse JSON input to BASON if needed
    b8 is_json = basonIsJSON(indata);
    u8b bson = {};
    u8cs bdata;
    if (is_json) {
        call(u8bMap, bson, BASON_BUF_LEN);
        u64 _idx[256];
        u64b idx = {_idx, _idx, _idx, _idx + 256};
        call(BASONParseJSON, bson, idx, indata);
        u8cp b0 = u8bDataHead(bson), b1 = u8bIdleHead(bson);
        bdata[0] = b0;
        bdata[1] = b1;
    } else {
        $mv(bdata, indata);
    }

    // Produce output
    u8b outbuf = {};
    if (ofmt == 0) {
        // binary BASON
        if ($arglen == 3) {
            a$rg(oarg, 2);
            a_pad(u8, opath, FILE_PATH_MAX_LEN);
            call(u8bFeed, opath, oarg);
            u8bFeed1(opath, 0);
            u8bShed1(opath);
            int ofd;
            call(FILECreate, &ofd, PATHu8cgIn(opath));
            if ($len(bdata) > 0)
                call(FILEFeedall, ofd, bdata);
            close(ofd);
        } else {
            call(FILEFeedall, STDOUT_FILENO, bdata);
        }
    } else {
        // text or json export
        call(u8bMap, outbuf, BASON_BUF_LEN);
        u64 _stk[256];
        u64b stk = {_stk, _stk, _stk, _stk + 256};
        if (ofmt == 1) {
            call(BASONExportJSON, u8bIdle(outbuf), stk, bdata);
        } else {
            call(BASONExportText, u8bIdle(outbuf), stk, bdata);
        }
        u8cp o0 = u8bDataHead(outbuf), o1 = u8bIdleHead(outbuf);
        u8cs out = {o0, o1};
        if ($arglen == 3) {
            a$rg(oarg, 2);
            a_pad(u8, opath, FILE_PATH_MAX_LEN);
            call(u8bFeed, opath, oarg);
            u8bFeed1(opath, 0);
            u8bShed1(opath);
            int ofd;
            call(FILECreate, &ofd, PATHu8cgIn(opath));
            call(FILEFeedall, ofd, out);
            close(ofd);
        } else {
            call(FILEFeedall, STDOUT_FILENO, out);
            if (ofmt == 1) {
                u8cs nl = $u8str("\n");
                call(FILEFeedall, STDOUT_FILENO, nl);
            }
        }
        u8bUnMap(outbuf);
    }

    if (is_json) u8bUnMap(bson);
    call(FILEUnMap, mapped);
    done;
}

MAIN(basoncli);
