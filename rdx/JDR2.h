#ifndef ABC_RDX2_H
#define ABC_RDX2_H
#include "RDX2.h"
#include "abc/UTF8.h"

typedef enum {
    RDX_STYLE_SEP_SPACE = 1,
    RDX_STYLE_SEP_COMMA = 2,
    RDX_STYLE_SEP_COLON = 4,
    RDX_STYLE_SEP_NLINE = 8,
    RDX_STYLE_SEP = 1 + 2 + 4 + 8,
    RDX_STYLE_INDENT_TAB = 16,
    RDX_STYLE_INDENT_SPACE = 32,
    RDX_STYLE_INDENT = 16 + 32,
    RDX_STYLE_SEP_TRAIL = 64,
} RDXStyle;

con ok64 JDRbad = 0x4cd6e6968;

con u64 RDX_STYLE_DEFAULT = RDX_STYLE_SEP_COMMA;

fun b8 RDXisWS(u8 c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

ok64 RDXutf8sRender(utf8s jdr, rdxb reader, u64 style);
ok64 RDXutf8sParse(utf8cs jdr, u8b builder, utf8s err);

fun ok64 RDXutf8sFeed(utf8s jdr, rdxb reader) {
    return RDXutf8sRender(jdr, reader, RDX_STYLE_DEFAULT);
}
fun ok64 RDXutf8sDrain(utf8cs jdr, u8b builder) {
    return RDXutf8sParse(jdr, builder, NULL);
}

fun ok64 RDXutf8sFeedRaw(utf8s jdr, u8cs rdx_data) {
    a_pad(rdx, pad, RDX_MAX_NESTING);
    rdxbInit(pad, rdx_data);
    return RDXutf8sRender(jdr, pad, RDX_STYLE_DEFAULT);
}
fun ok64 RDXutf8sDrainRaw(utf8cs jdr, u8s rdx_idle) {
    u8p builder[4] = {rdx_idle[0], rdx_idle[0], rdx_idle[0], rdx_idle[1]};
    ok64 o = RDXutf8sParse(jdr, builder, NULL);
    if (o == OK) rdx_idle[0] = builder[2];
    return o;
}

ok64 RDXutf8sFeedFIRST(utf8s elem, rdxcp rdx);
ok64 RDXutf8sFeedF(utf8s elem, rdxcp rdx);
ok64 RDXutf8sFeedI(utf8s elem, rdxcp rdx);
ok64 RDXutf8sFeedR(utf8s elem, rdxcp rdx);
// the reason to use rdxcp in the signature: encoding may vary (utf8, json etc)
ok64 RDXutf8sFeedS(utf8s elem, rdxcp rdx);
ok64 RDXutf8sFeedT(utf8s elem, rdxcp rdx);

ok64 RDXutf8sDrainF(utf8csc elem, rdxp rdx);
ok64 RDXutf8sDrainI(utf8csc elem, rdxp rdx);
ok64 RDXutf8sDrainR(utf8csc elem, rdxp rdx);
ok64 RDXutf8sDrainS(utf8csc elem, rdxp rdx);
ok64 RDXutf8sDrainT(utf8csc elem, rdxp rdx);

typedef struct {
    u8cs text;
    u8bp builder;
    rdx cur;
    u64 flags;
} JDRstate;

#endif
