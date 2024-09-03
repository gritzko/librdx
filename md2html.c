#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "FILE.h"
#include "INT.h"
#include "MARK.h"
#include "MMAP.h"

a$strc(ext, ".lex");

#define X(M, name) M##u8##name
#include "MMAPx.h"
#undef X
#define X(M, name) M##u64##name
#include "MMAPx.h"
#undef X
#define X(M, name) M##u8cp##name
#include "MMAPx.h"
#undef X

$u8c header_template;
$u8c footer_template;

ABC_INIT;

#define a$strf(name, len, tmpl, ...) \
    aBpad(u8, name, len);            \
    $u8c __##name = $u8str(tmpl);    \
    $feedf(Bu8idle(name), __##name, __VA_ARGS__);

pro(md2html, $u8c mod) {
    sane($ok(mod) && !$empty(mod) && $len(mod) <= 1000);
    a$strf(name, 1024, "$s.md", mod);
    int fd = 0;
    call(FILEopen, &fd, Bu8cdata(name), O_RDONLY);
    Bu8 txtbuf = {};
    call(FILEmap, (void **)txtbuf, fd, PROT_READ, 0);

    Bu8 fmtbuf = {};
    call(MMAPu8open, fmtbuf, Blen(txtbuf));
    Bu8cp linebuf = {};
    call(MMAPu8cpopen, linebuf, Blen(txtbuf));
    Bu64 divbuf = {};
    call(MMAPu64open, divbuf, Blen(txtbuf));
    Bu64 pbuf = {};
    call(MMAPu64open, pbuf, Blen(txtbuf));
    Bu8 intobuf = {};
    call(MMAPu8open, intobuf, roundup(Blen(txtbuf) * 8, PAGESIZE));

    MARKstate state = {};
    state.divB = (u64B)divbuf;
    state.lineB = (u8cpB)linebuf;
    state.pB = (u64B)pbuf;
    $mv(state.text, Bu8cdata(txtbuf));
    $mv(state.fmt, Bu8idle(fmtbuf));

    call(MARKlexer, &state);
    call(MARKMARQ, &state);
    call(MARKHTML, Bu8idle(intobuf), &state);

    int hfd = 0;
    a$strf(htmlname, 1024, "$s.html", mod);
    call(FILEcreate, &hfd, Bu8cdata(htmlname));
    call(FILEfeedall, hfd, header_template);
    call(FILEfeedall, hfd, Bu8cdata(intobuf));
    call(FILEfeedall, hfd, footer_template);

    nedo(FILEclose(hfd), FILEclose(fd);
         MMAPu8close(fmtbuf), MMAPu8close(intobuf), MMAPu8cpclose(linebuf),
         MMAPu64close(divbuf););
}

int main(int argn, char **args) {
    if (argn != 2) {
        fprintf(stderr, "Usage: md2html MOD\n");
        return -1;
    }
    a$strc(name, args[1]);
    ok64 o = md2html(name);
    if (o != OK)
        trace("%s<%s at %s:%i\n", PROindent, ok64str(o), __func__, __LINE__);

    return o;
}

$u8c header_template = $u8str(
    "<html><head>\n"
    "<link rel='stylesheet' href='style.css' type='text/css'>\n"
    "</head><body>\n");

$u8c footer_template = $u8str("</body></html>\n");
