#include "HTTP.h"

#define HTTPfeed u8css_feed1(state->parsed, (u8c**)tok);

ok64 HTTPonMethod($cu8c tok, HTTPstate* state) {
    HTTPfeed;
    return OK;
}
ok64 HTTPonRequestURI($cu8c tok, HTTPstate* state) {
    HTTPfeed;
    return OK;
}
ok64 HTTPonHTTPVersion($cu8c tok, HTTPstate* state) { return OK; }
ok64 HTTPonRequestLine($cu8c tok, HTTPstate* state) { return OK; }
ok64 HTTPonFieldName($cu8c tok, HTTPstate* state) {
    HTTPfeed;
    return OK;
}
ok64 HTTPonFieldValue($cu8c tok, HTTPstate* state) {
    HTTPfeed;
    return OK;
}
ok64 HTTPonMessageHeader($cu8c tok, HTTPstate* state) { return OK; }
ok64 HTTPonRequestHead($cu8c tok, HTTPstate* state) { return OK; }
ok64 HTTPonBody($cu8c tok, HTTPstate* state) { return OK; }
ok64 HTTPonRequest($cu8c tok, HTTPstate* state) { return OK; }
ok64 HTTPonStatusCode($cu8c tok, HTTPstate* state) {
    HTTPfeed;
    return OK;
}
ok64 HTTPonReasonPhrase($cu8c tok, HTTPstate* state) {
    HTTPfeed;
    return OK;
}
ok64 HTTPonStatusLine($cu8c tok, HTTPstate* state) { return OK; }
ok64 HTTPonResponse($cu8c tok, HTTPstate* state) { return OK; }
ok64 HTTPonMessage($cu8c tok, HTTPstate* state) { return OK; }
ok64 HTTPonRoot($cu8c tok, HTTPstate* state) { return OK; }

ok64 HTTPfind(u8c$ value, $cu8c key, u8css parsed) {
    for (size_t i = 2; i < $len(parsed); ++i) {
        if ($eq($at(parsed, i), key)) {
            if (i + 1 < $len(parsed)) {
                $mv(value, $at(parsed, i + 1));
            } else {
                value[0] = value[1] = nil;
            }
            return OK;
        }
    }
    return HTTPnone;
}

#undef HTTPfeed
