NL = "\n";
ws = [\r\t ] | NL;
hex = [0-9a-fA-Z];
ron64 = [0-9A-Za-z_~];
dec = [0-9];

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

id128 = ron64+ ("-" ron64+)?;

Int = [\-]? ( [0] | [1-9] dec* );
Float = (   Int
            ("." dec+)?
            ([eE] [\-+]? dec+ )?  ) -Int;
Term = ((ron64 - dec) ron64*) -Int -Float;
Ref = id128 -Float -Int -Term;
String = ["] utf8esc* ["];
MLString = "`" (utf8cp - [`])* "`";

Stamp = "@" id128;
NoStamp = "";

Comma = [,;];
Colon = [:];

Open = [(\[{<] ws* (Stamp | NoStamp);
Close = [)\]}>];
FIRST = ( Float | Int | Ref | String | MLString | Term ) ws* ( Stamp | NoStamp );
Inter = Comma | Colon | Open | Close | ws+;

Root = Inter* ( FIRST Inter+ )* ;
