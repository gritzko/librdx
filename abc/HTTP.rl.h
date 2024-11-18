#include "PRO.h"
#include "INT.h"
#include "HTTP.h"

enum {
	HTTPMethod = HTTPenum+22,
	HTTPRequestURI = HTTPenum+23,
	HTTPHTTPVersion = HTTPenum+24,
	HTTPRequestLine = HTTPenum+25,
	HTTPFieldName = HTTPenum+26,
	HTTPFieldValue = HTTPenum+27,
	HTTPMessageHeader = HTTPenum+28,
	HTTPRequestHead = HTTPenum+29,
	HTTPBody = HTTPenum+30,
	HTTPRequest = HTTPenum+31,
	HTTPStatusCode = HTTPenum+32,
	HTTPReasonPhrase = HTTPenum+33,
	HTTPStatusLine = HTTPenum+34,
	HTTPResponse = HTTPenum+35,
	HTTPMessage = HTTPenum+36,
	HTTPRoot = HTTPenum+37,
};
ok64 HTTPonMethod ($cu8c tok, HTTPstate* state);
ok64 HTTPonRequestURI ($cu8c tok, HTTPstate* state);
ok64 HTTPonHTTPVersion ($cu8c tok, HTTPstate* state);
ok64 HTTPonRequestLine ($cu8c tok, HTTPstate* state);
ok64 HTTPonFieldName ($cu8c tok, HTTPstate* state);
ok64 HTTPonFieldValue ($cu8c tok, HTTPstate* state);
ok64 HTTPonMessageHeader ($cu8c tok, HTTPstate* state);
ok64 HTTPonRequestHead ($cu8c tok, HTTPstate* state);
ok64 HTTPonBody ($cu8c tok, HTTPstate* state);
ok64 HTTPonRequest ($cu8c tok, HTTPstate* state);
ok64 HTTPonStatusCode ($cu8c tok, HTTPstate* state);
ok64 HTTPonReasonPhrase ($cu8c tok, HTTPstate* state);
ok64 HTTPonStatusLine ($cu8c tok, HTTPstate* state);
ok64 HTTPonResponse ($cu8c tok, HTTPstate* state);
ok64 HTTPonMessage ($cu8c tok, HTTPstate* state);
ok64 HTTPonRoot ($cu8c tok, HTTPstate* state);


