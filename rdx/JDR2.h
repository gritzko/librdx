#ifndef ABC_RDX2_H
#define ABC_RDX2_H
#include "RDX2.h"
#include "abc/UTF8.h"

con ok64 JDRbad = 0x4cd6e6968;

fun b8 RDXisWS(u8 c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

ok64 RDXutf8sRender(utf8s jdr, rdxb reader, u64 style);
ok64 RDXutf8sParse(utf8cs jdr, u8b builder, utf8s err);

fun ok64 RDXutf8sFeed(utf8s jdr, u8cs rdx_data) {
    a_pad(rdx, pad, RDX_MAX_NESTING);
    rdxbInit(pad, rdx_data);
    return RDXutf8sRender(jdr, pad, 0);
}
fun ok64 RDXutf8sDrain(utf8cs jdr, u8s rdx_idle) {
    u8p builder[4] = {rdx_idle[0], rdx_idle[0], rdx_idle[0], rdx_idle[1]};
    ok64 o = RDXutf8sParse(jdr, builder, NULL);
    if (o == OK) rdx_idle[0] = builder[2];
    return o;
}

ok64 RDXutf8sFeed1(utf8s elem, rdxcp rdx);
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

#endif
