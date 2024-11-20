#include "abc/INT.h"
#include "abc/PRO.h"
#include "URI.h"

enum {
	URIPath = URIenum+14,
	URIScheme = URIenum+16,
	URIIPv4address = URIenum+18,
	URIIPvFuture = URIenum+19,
	URIIPv6address = URIenum+20,
	URIIP_literal = URIenum+21,
	URIUser = URIenum+23,
	URIHost = URIenum+24,
	URIPort = URIenum+25,
	URIFragment = URIenum+27,
	URIQuery = URIenum+28,
	URIURI = URIenum+34,
	URIRoot = URIenum+35,
};
ok64 URIonPath ($cu8c tok, URIstate* state);
ok64 URIonScheme ($cu8c tok, URIstate* state);
ok64 URIonIPv4address ($cu8c tok, URIstate* state);
ok64 URIonIPvFuture ($cu8c tok, URIstate* state);
ok64 URIonIPv6address ($cu8c tok, URIstate* state);
ok64 URIonIP_literal ($cu8c tok, URIstate* state);
ok64 URIonUser ($cu8c tok, URIstate* state);
ok64 URIonHost ($cu8c tok, URIstate* state);
ok64 URIonPort ($cu8c tok, URIstate* state);
ok64 URIonFragment ($cu8c tok, URIstate* state);
ok64 URIonQuery ($cu8c tok, URIstate* state);
ok64 URIonURI ($cu8c tok, URIstate* state);
ok64 URIonRoot ($cu8c tok, URIstate* state);


