octet = (0..0xff);
char = (0..127);
upper = [A-Z];
lower = [a-z];
alpha = upper | lower;
digit = [0-9];
ctl = (0..31) | (127);
cr = (13);
lf = (10);
sp = (32);
ht = (9);
crlf = cr lf;
lws = crlf? [ \t]+;
txt = octet - ctl;
text = (lws | txt)+;
hex = [0-9a-fA-F];
qdTxt = txt - ["];
quotedPair = "\\" char;
quotedString = ["] (qdTxt|quotedPair)* ["];
sep = [()<>@,;:\\"/\[\]?={} \t];
token = (char - ctl - sep)+;

Method =   "OPTIONS" | 
            "GET" |
            "HEAD" |
            "POST" |
            "PUT" |
            "DELETE" |
            "TRACE" |
            "CONNECT";
RequestURI = [^ \t\r\n];
HTTPVersion = "HTTP/" digit "." digit;
RequestLine = Method sp RequestURI sp HTTPVersion crlf ;
FieldName = token ;
FieldValue = (text*) ;
MessageHeader =  FieldName ":" lws* FieldValue? crlf ;
RequestHead = RequestLine MessageHeader* crlf ;
Body = octet*;
Request = lws* RequestHead Body;
StatusCode = digit{3} ;
ReasonPhrase = text ;
StatusLine = HTTPVersion sp StatusCode sp ReasonPhrase crlf ;
Response = lws* StatusLine MessageHeader* crlf Body;
Message = Request | Response;

Root = Message;
