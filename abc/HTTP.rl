#include "HTTP.rl.h"


%%{

machine HTTP;

alphtype unsigned char;

action HTTPMethod0 { mark0[HTTPMethod] = p - text[0]; }
action HTTPMethod1 {
    tok[0] = text[0] + mark0[HTTPMethod];
    tok[1] = p;
    call(HTTPonMethod, tok, state); 
}
action HTTPRequestURI0 { mark0[HTTPRequestURI] = p - text[0]; }
action HTTPRequestURI1 {
    tok[0] = text[0] + mark0[HTTPRequestURI];
    tok[1] = p;
    call(HTTPonRequestURI, tok, state); 
}
action HTTPHTTPVersion0 { mark0[HTTPHTTPVersion] = p - text[0]; }
action HTTPHTTPVersion1 {
    tok[0] = text[0] + mark0[HTTPHTTPVersion];
    tok[1] = p;
    call(HTTPonHTTPVersion, tok, state); 
}
action HTTPRequestLine0 { mark0[HTTPRequestLine] = p - text[0]; }
action HTTPRequestLine1 {
    tok[0] = text[0] + mark0[HTTPRequestLine];
    tok[1] = p;
    call(HTTPonRequestLine, tok, state); 
}
action HTTPFieldName0 { mark0[HTTPFieldName] = p - text[0]; }
action HTTPFieldName1 {
    tok[0] = text[0] + mark0[HTTPFieldName];
    tok[1] = p;
    call(HTTPonFieldName, tok, state); 
}
action HTTPFieldValue0 { mark0[HTTPFieldValue] = p - text[0]; }
action HTTPFieldValue1 {
    tok[0] = text[0] + mark0[HTTPFieldValue];
    tok[1] = p;
    call(HTTPonFieldValue, tok, state); 
}
action HTTPMessageHeader0 { mark0[HTTPMessageHeader] = p - text[0]; }
action HTTPMessageHeader1 {
    tok[0] = text[0] + mark0[HTTPMessageHeader];
    tok[1] = p;
    call(HTTPonMessageHeader, tok, state); 
}
action HTTPRequestHead0 { mark0[HTTPRequestHead] = p - text[0]; }
action HTTPRequestHead1 {
    tok[0] = text[0] + mark0[HTTPRequestHead];
    tok[1] = p;
    call(HTTPonRequestHead, tok, state); 
}
action HTTPBody0 { mark0[HTTPBody] = p - text[0]; }
action HTTPBody1 {
    tok[0] = text[0] + mark0[HTTPBody];
    tok[1] = p;
    call(HTTPonBody, tok, state); 
}
action HTTPRequest0 { mark0[HTTPRequest] = p - text[0]; }
action HTTPRequest1 {
    tok[0] = text[0] + mark0[HTTPRequest];
    tok[1] = p;
    call(HTTPonRequest, tok, state); 
}
action HTTPStatusCode0 { mark0[HTTPStatusCode] = p - text[0]; }
action HTTPStatusCode1 {
    tok[0] = text[0] + mark0[HTTPStatusCode];
    tok[1] = p;
    call(HTTPonStatusCode, tok, state); 
}
action HTTPReasonPhrase0 { mark0[HTTPReasonPhrase] = p - text[0]; }
action HTTPReasonPhrase1 {
    tok[0] = text[0] + mark0[HTTPReasonPhrase];
    tok[1] = p;
    call(HTTPonReasonPhrase, tok, state); 
}
action HTTPStatusLine0 { mark0[HTTPStatusLine] = p - text[0]; }
action HTTPStatusLine1 {
    tok[0] = text[0] + mark0[HTTPStatusLine];
    tok[1] = p;
    call(HTTPonStatusLine, tok, state); 
}
action HTTPResponse0 { mark0[HTTPResponse] = p - text[0]; }
action HTTPResponse1 {
    tok[0] = text[0] + mark0[HTTPResponse];
    tok[1] = p;
    call(HTTPonResponse, tok, state); 
}
action HTTPMessage0 { mark0[HTTPMessage] = p - text[0]; }
action HTTPMessage1 {
    tok[0] = text[0] + mark0[HTTPMessage];
    tok[1] = p;
    call(HTTPonMessage, tok, state); 
}
action HTTPRoot0 { mark0[HTTPRoot] = p - text[0]; }
action HTTPRoot1 {
    tok[0] = text[0] + mark0[HTTPRoot];
    tok[1] = p;
    call(HTTPonRoot, tok, state); 
}

HTTPoctet  = (   (0..0xff) );

HTTPchar  = (   (0..127) );

HTTPupper  = (   [A-Z] );

HTTPlower  = (   [a-z] );

HTTPalpha  = (   HTTPupper  |  HTTPlower );

HTTPdigit  = (   [0-9] );

HTTPctl  = (   (0..31)  |  (127) );

HTTPcr  = (   (13) );

HTTPlf  = (   (10) );

HTTPsp  = (   (32) );

HTTPht  = (   (9) );

HTTPcrlf  = (   HTTPcr  HTTPlf );

HTTPlws  = (   HTTPcrlf?  [ \t]+ );

HTTPtxt  = (   HTTPoctet  -  HTTPctl );

HTTPtext  = (   (HTTPlws  |  HTTPtxt)+ );

HTTPhex  = (   [0-9a-fA-F] );

HTTPqdTxt  = (   HTTPtxt  -  ["] );

HTTPquotedPair  = (   "\\"  HTTPchar );

HTTPquotedString  = (   ["]  (HTTPqdTxt|HTTPquotedPair)*  ["] );

HTTPsep  = (   [()<>@,;:\\"/\[\][()<>@,;:\\"/\[\]?={} \t] );

HTTPtoken  = (   (HTTPchar  -  HTTPctl  -  HTTPsep)+ );


HTTPMethod  = (       "OPTIONS"  |  

                        "GET"  |

                        "HEAD"  |

                        "POST"  |

                        "PUT"  |

                        "DELETE"  |

                        "TRACE"  |

                        "CONNECT" )  >HTTPMethod0 %HTTPMethod1;

HTTPRequestURI  = (   [^ \t\r\n] )  >HTTPRequestURI0 %HTTPRequestURI1;

HTTPHTTPVersion  = (   "HTTP/"  HTTPdigit  "."  HTTPdigit )  >HTTPHTTPVersion0 %HTTPHTTPVersion1;

HTTPRequestLine  = (   HTTPMethod  HTTPsp  HTTPRequestURI  HTTPsp  HTTPHTTPVersion  HTTPcrlf   )  >HTTPRequestLine0 %HTTPRequestLine1;

HTTPFieldName  = (   HTTPtoken   )  >HTTPFieldName0 %HTTPFieldName1;

HTTPFieldValue  = (   (HTTPtext*)   )  >HTTPFieldValue0 %HTTPFieldValue1;

HTTPMessageHeader  = (     HTTPFieldName  ":"  HTTPlws*  HTTPFieldValue?  HTTPcrlf   )  >HTTPMessageHeader0 %HTTPMessageHeader1;

HTTPRequestHead  = (   HTTPRequestLine  HTTPMessageHeader*  HTTPcrlf   )  >HTTPRequestHead0 %HTTPRequestHead1;

HTTPBody  = (   HTTPoctet* )  >HTTPBody0 %HTTPBody1;

HTTPRequest  = (   HTTPlws*  HTTPRequestHead  HTTPBody )  >HTTPRequest0 %HTTPRequest1;

HTTPStatusCode  = (   HTTPdigit{3}   )  >HTTPStatusCode0 %HTTPStatusCode1;

HTTPReasonPhrase  = (   HTTPtext   )  >HTTPReasonPhrase0 %HTTPReasonPhrase1;

HTTPStatusLine  = (   HTTPHTTPVersion  HTTPsp  HTTPStatusCode  HTTPsp  HTTPReasonPhrase  HTTPcrlf   )  >HTTPStatusLine0 %HTTPStatusLine1;

HTTPResponse  = (   HTTPlws*  HTTPStatusLine  HTTPMessageHeader*  HTTPcrlf  HTTPBody )  >HTTPResponse0 %HTTPResponse1;

HTTPMessage  = (   HTTPRequest  |  HTTPResponse )  >HTTPMessage0 %HTTPMessage1;


HTTPRoot  = (   HTTPMessage )  >HTTPRoot0 %HTTPRoot1;

main := HTTPRoot;

}%%

%%write data;

pro(HTTPlexer, HTTPstate* state) {

    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = 0;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = pe;
    u8c *pb = p;
    u64 mark0[64] = {};

    u32 sp = 2;
    $u8c tok = {p, p};

    %% write init;
    %% write exec;

    state->text[0] = p;
    if (p!=text[1] || cs < HTTP_first_final) {
        return HTTPfail;
    }
    done;
}
