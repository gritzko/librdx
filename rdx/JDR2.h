#ifndef ABC_RDX2_H
#define ABC_RDX2_H
#include "RDX2.h"
#include "abc/UTF8.h"

con ok64 JDRbad = 0x4cd6e6968;

ok64 RDXutf8sFeedInStyle(utf8s jdr, rdxb reader, u64 style);
fun ok64 RDXutf8bFeedInStyle(utf8s jdr, rdxb reader, u64 style) {
    return RDXutf8sFeedInStyle(Butf8idle(jdr), reader, style);
}
ok64 RDXutf8sFeed(utf8s jdr, rdxb reader);

ok64 RDXutf8sDrain(utf8cs jdr, u8b builder);

ok64 RDXutf8sFeedID(utf8s elem, rdxcp rdx);
ok64 RDXutf8sFeed1(utf8s elem, rdxcp rdx);
ok64 RDXutf8sFeedF(utf8s elem, rdxcp rdx);
ok64 RDXutf8sFeedI(utf8s elem, rdxcp rdx);
ok64 RDXutf8sFeedR(utf8s elem, rdxcp rdx);
ok64 RDXutf8sFeedS(utf8s elem, rdxcp rdx);  // esc etc
ok64 RDXutf8sFeedT(utf8s elem, rdxcp rdx);

ok64 RDXutf8sDrainID(utf8csc elem, rdxp rdx);
ok64 RDXutf8sDrainF(utf8csc elem, rdxp rdx);
ok64 RDXutf8sDrainI(utf8csc elem, rdxp rdx);
ok64 RDXutf8sDrainR(utf8csc elem, rdxp rdx);
ok64 RDXutf8sDrainS(utf8csc elem, rdxp rdx);
ok64 RDXutf8sDrainT(utf8csc elem, rdxp rdx);

#endif
