NL = "\n";
ws = [\r\t ] | NL;
hex = [0-9a-fA-F];

utf8cont =  (0x80..0xbf);
utf8lead1 = (0x00..0x7f);
utf8lead2 = (0xc0..0xdf);
utf8lead3 = (0xe0..0xef);
utf8lead4 = (0xf0..0xf7);

Utf8cp1 =  utf8lead1;
Utf8cp2 =  utf8lead2 utf8cont;
Utf8cp3 =  utf8lead3 utf8cont utf8cont;
Utf8cp4 =  utf8lead4 utf8cont utf8cont utf8cont;

utf8cp = Utf8cp1 | Utf8cp2 | Utf8cp3 | Utf8cp4;

esc = [\\] ["\\/bfnrt];
hexEsc =  "\\u" hex{4};
utf8esc = (utf8cp - ["\\\r\n]) | esc | hexEsc;

id128 = [0-9a-fA-F]+ ("-" [0-9a-fA-F]+)?;

Int = [\-]? ( [0] | [1-9] [0-9]* );
Float = ( [\-]? ( [0] | [1-9] [0-9]* )
            ("." [0-9]+)?
            ([eE] [\-+]? [0-9]+ )? ) - Int;
Term = [a-zA-Z0-9_~]+ -Int -Float;
Ref = id128 -Float -Int -Term;
String = ["] utf8esc* ["];
MLString = "`" (utf8cp - [`])* "`";

Stamp = "@" id128;
NoStamp = "";

OpenP = "<";
CloseP = ">";
OpenL = "[";
CloseL = "]";
OpenE = "{";
CloseE = "}";
OpenX = "(";
CloseX = ")";

Comma = ",";
Colon = ":";

Open = (OpenP | OpenL | OpenE | OpenX) ws* (Stamp ws* | NoStamp);
Close = (CloseP | CloseL | CloseE | CloseX) ws*;
Inter = (Comma | Colon) ws*;

delim = Open | Close | Inter;

FIRST = ( Float | Int | Ref | String | MLString | Term ) ws* ( Stamp ws* | NoStamp);

Root = ( ws | FIRST | delim )** ;
