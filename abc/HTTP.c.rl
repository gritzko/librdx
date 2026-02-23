#include "abc/INT.h"
#include "abc/PRO.h"
#include "HTTP.h"

// action indices for the parser
#define HTTPenum 0
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

// user functions (callbacks) for the parser
ok64 HTTPonMethod (u8cs tok, HTTPstate* state);
ok64 HTTPonRequestURI (u8cs tok, HTTPstate* state);
ok64 HTTPonHTTPVersion (u8cs tok, HTTPstate* state);
ok64 HTTPonRequestLine (u8cs tok, HTTPstate* state);
ok64 HTTPonFieldName (u8cs tok, HTTPstate* state);
ok64 HTTPonFieldValue (u8cs tok, HTTPstate* state);
ok64 HTTPonMessageHeader (u8cs tok, HTTPstate* state);
ok64 HTTPonRequestHead (u8cs tok, HTTPstate* state);
ok64 HTTPonBody (u8cs tok, HTTPstate* state);
ok64 HTTPonRequest (u8cs tok, HTTPstate* state);
ok64 HTTPonStatusCode (u8cs tok, HTTPstate* state);
ok64 HTTPonReasonPhrase (u8cs tok, HTTPstate* state);
ok64 HTTPonStatusLine (u8cs tok, HTTPstate* state);
ok64 HTTPonResponse (u8cs tok, HTTPstate* state);
ok64 HTTPonMessage (u8cs tok, HTTPstate* state);
ok64 HTTPonRoot (u8cs tok, HTTPstate* state);



%%{

machine HTTP;

alphtype unsigned char;

# ragel actions
action HTTPMethod0 { mark0[HTTPMethod] = p - data[0]; }
action HTTPMethod1 {
    tok[0] = data[0] + mark0[HTTPMethod];
    tok[1] = p;
    o = HTTPonMethod(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPRequestURI0 { mark0[HTTPRequestURI] = p - data[0]; }
action HTTPRequestURI1 {
    tok[0] = data[0] + mark0[HTTPRequestURI];
    tok[1] = p;
    o = HTTPonRequestURI(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPHTTPVersion0 { mark0[HTTPHTTPVersion] = p - data[0]; }
action HTTPHTTPVersion1 {
    tok[0] = data[0] + mark0[HTTPHTTPVersion];
    tok[1] = p;
    o = HTTPonHTTPVersion(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPRequestLine0 { mark0[HTTPRequestLine] = p - data[0]; }
action HTTPRequestLine1 {
    tok[0] = data[0] + mark0[HTTPRequestLine];
    tok[1] = p;
    o = HTTPonRequestLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPFieldName0 { mark0[HTTPFieldName] = p - data[0]; }
action HTTPFieldName1 {
    tok[0] = data[0] + mark0[HTTPFieldName];
    tok[1] = p;
    o = HTTPonFieldName(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPFieldValue0 { mark0[HTTPFieldValue] = p - data[0]; }
action HTTPFieldValue1 {
    tok[0] = data[0] + mark0[HTTPFieldValue];
    tok[1] = p;
    o = HTTPonFieldValue(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPMessageHeader0 { mark0[HTTPMessageHeader] = p - data[0]; }
action HTTPMessageHeader1 {
    tok[0] = data[0] + mark0[HTTPMessageHeader];
    tok[1] = p;
    o = HTTPonMessageHeader(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPRequestHead0 { mark0[HTTPRequestHead] = p - data[0]; }
action HTTPRequestHead1 {
    tok[0] = data[0] + mark0[HTTPRequestHead];
    tok[1] = p;
    o = HTTPonRequestHead(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPBody0 { mark0[HTTPBody] = p - data[0]; }
action HTTPBody1 {
    tok[0] = data[0] + mark0[HTTPBody];
    tok[1] = p;
    o = HTTPonBody(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPRequest0 { mark0[HTTPRequest] = p - data[0]; }
action HTTPRequest1 {
    tok[0] = data[0] + mark0[HTTPRequest];
    tok[1] = p;
    o = HTTPonRequest(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPStatusCode0 { mark0[HTTPStatusCode] = p - data[0]; }
action HTTPStatusCode1 {
    tok[0] = data[0] + mark0[HTTPStatusCode];
    tok[1] = p;
    o = HTTPonStatusCode(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPReasonPhrase0 { mark0[HTTPReasonPhrase] = p - data[0]; }
action HTTPReasonPhrase1 {
    tok[0] = data[0] + mark0[HTTPReasonPhrase];
    tok[1] = p;
    o = HTTPonReasonPhrase(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPStatusLine0 { mark0[HTTPStatusLine] = p - data[0]; }
action HTTPStatusLine1 {
    tok[0] = data[0] + mark0[HTTPStatusLine];
    tok[1] = p;
    o = HTTPonStatusLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPResponse0 { mark0[HTTPResponse] = p - data[0]; }
action HTTPResponse1 {
    tok[0] = data[0] + mark0[HTTPResponse];
    tok[1] = p;
    o = HTTPonResponse(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPMessage0 { mark0[HTTPMessage] = p - data[0]; }
action HTTPMessage1 {
    tok[0] = data[0] + mark0[HTTPMessage];
    tok[1] = p;
    o = HTTPonMessage(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action HTTPRoot0 { mark0[HTTPRoot] = p - data[0]; }
action HTTPRoot1 {
    tok[0] = data[0] + mark0[HTTPRoot];
    tok[1] = p;
    o = HTTPonRoot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}

# ragel grammar rules
HTTPoctet = (   (0..0xff) ); # no octet callback
HTTPchar = (   (0..127) ); # no char callback
HTTPupper = (   [A-Z] ); # no upper callback
HTTPlower = (   [a-z] ); # no lower callback
HTTPalpha = (   HTTPupper  |  HTTPlower ); # no alpha callback
HTTPdigit = (   [0-9] ); # no digit callback
HTTPctl = (   (0..31)  |  (127) ); # no ctl callback
HTTPcr = (   (13) ); # no cr callback
HTTPlf = (   (10) ); # no lf callback
HTTPsp = (   (32) ); # no sp callback
HTTPht = (   (9) ); # no ht callback
HTTPcrlf = (   HTTPcr  HTTPlf ); # no crlf callback
HTTPlws = (   HTTPcrlf?  [ \t]+ ); # no lws callback
HTTPtxt = (   HTTPoctet  -  HTTPctl ); # no txt callback
HTTPtext = (   (HTTPlws  |  HTTPtxt)+ ); # no text callback
HTTPhex = (   [0-9a-fA-F] ); # no hex callback
HTTPqdTxt = (   HTTPtxt  -  ["] ); # no qdTxt callback
HTTPquotedPair = (   "\\"  HTTPchar ); # no quotedPair callback
HTTPquotedString = (   ["]  (HTTPqdTxt|HTTPquotedPair)*  ["] ); # no quotedString callback
HTTPsep = (   [()<>@,;:\\"/\[\][()<>@,;:\\"/\[\]?={} \t] ); # no sep callback
HTTPtoken = (   (HTTPchar  -  HTTPctl  -  HTTPsep)+ ); # no token callback
HTTPMethod = (       "OPTIONS"  |   
                        "GET"  | 
                        "HEAD"  | 
                        "POST"  | 
                        "PUT"  | 
                        "DELETE"  | 
                        "TRACE"  | 
                        "CONNECT" )  >HTTPMethod0 %HTTPMethod1;
HTTPRequestURI = (   [^ \t\r\n]+ )  >HTTPRequestURI0 %HTTPRequestURI1;
HTTPHTTPVersion = (   "HTTP/"  HTTPdigit  "."  HTTPdigit )  >HTTPHTTPVersion0 %HTTPHTTPVersion1;
HTTPRequestLine = (   HTTPMethod  HTTPsp  HTTPRequestURI  HTTPsp  HTTPHTTPVersion  HTTPcrlf   )  >HTTPRequestLine0 %HTTPRequestLine1;
HTTPFieldName = (   HTTPtoken   )  >HTTPFieldName0 %HTTPFieldName1;
HTTPFieldValue = (   (HTTPtext*)   )  >HTTPFieldValue0 %HTTPFieldValue1;
HTTPMessageHeader = (     HTTPFieldName  ":"  HTTPlws*  HTTPFieldValue?  HTTPcrlf   )  >HTTPMessageHeader0 %HTTPMessageHeader1;
HTTPRequestHead = (   HTTPRequestLine  HTTPMessageHeader*  HTTPcrlf   )  >HTTPRequestHead0 %HTTPRequestHead1;
HTTPBody = (   HTTPoctet* )  >HTTPBody0 %HTTPBody1;
HTTPRequest = (   HTTPlws*  HTTPRequestHead  HTTPBody )  >HTTPRequest0 %HTTPRequest1;
HTTPStatusCode = (   HTTPdigit{3}   )  >HTTPStatusCode0 %HTTPStatusCode1;
HTTPReasonPhrase = (   HTTPtext   )  >HTTPReasonPhrase0 %HTTPReasonPhrase1;
HTTPStatusLine = (   HTTPHTTPVersion  HTTPsp  HTTPStatusCode  HTTPsp  HTTPReasonPhrase  HTTPcrlf   )  >HTTPStatusLine0 %HTTPStatusLine1;
HTTPResponse = (   HTTPlws*  HTTPStatusLine  HTTPMessageHeader*  HTTPcrlf  HTTPBody )  >HTTPResponse0 %HTTPResponse1;
HTTPMessage = (   HTTPRequest  |  HTTPResponse )  >HTTPMessage0 %HTTPMessage1;
HTTPRoot = (   HTTPMessage )  >HTTPRoot0 %HTTPRoot1;

main := HTTPRoot;

}%%

%%write data;

// the public API function
ok64 HTTPLexer(HTTPstate* state) {

    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    u8c *p = (u8c*) data[0];
    u8c *pe = (u8c*) data[1];
    u8c *eof = pe;
    u64 mark0[64] = {};
    ok64 o = OK;

    u8cs tok = {p, p};

    %% write init;
    %% write exec;

    state->data[0] = p;
    if (o==OK && cs < HTTP_first_final) 
        o = HTTPBAD;
    
    return o;
}
