alpha = [a-zA-Z];
digit = [0-9];
xdigit = [0-9a-fA-F];

pct_encoded = "%" xdigit xdigit;

gen_delims  = ":" | "/" | "?" | "#" | "[" | "]" | "@";

sub_delims  = "!" | "$" | "&" | "'" | "(" | ")" | "*" | "+" | "," | ";" | "=";

reserved    = gen_delims | sub_delims;

unreserved  = alpha | digit | "-" | "." | "_" | "~";

delims      = "<" | ">" | "%" |  "#" | '"';

unwise      = " " | "{" | "}" | "|" | "\\" | "^" | "[" | "]" | "`";

pchar = unreserved | pct_encoded | sub_delims | ":" | "@" | delims | unwise;

slash = "/" | "\\";

path_char = pchar - ("?" | "#");

Segment = path_char*;
Segment_nz = path_char+;
segment_nz_nc = (path_char - ":")+;

Path = slash ( Segment_nz ( slash Segment )* )? ;
PathNoscheme = segment_nz_nc ( slash Segment )*;

drivePath = (slash|(alpha ":" slash)) ( path_char+ ( slash path_char* )* )? ;

Scheme = (alpha ( alpha | digit | "+" | "-" | "." )*) ':' ;

dec_octet = digit{1,3};
IPv4address = dec_octet "." dec_octet "." dec_octet "." dec_octet;
IPvFuture  = "v" xdigit+ "." ( unreserved | sub_delims | ":" )+;
IPv6address = (":" | xdigit)+ IPv4address?;
IP_literal = "[" ( IPv6address | IPvFuture  ) "]";

reg_name = ( unreserved | pct_encoded | sub_delims )*;

User    = ( unreserved | pct_encoded | sub_delims | ":" | "@" )*;
Host        = IP_literal | IPv4address | reg_name ;
Port        = (pchar - ("/" | "?" | "#")){1,5} ;
Authority   = "//" ( ( User "@" )? Host ( ":" Port )? ) ;

Fragment = ( pchar | "/" | "?" )* ;

Query = (pchar - "#")* ;

PathRootless = Segment_nz ( slash Segment )*;

hier_part = Authority Path? | Path | PathRootless;

absolute_URI = Scheme hier_part? ("?" Query)? ("#" Fragment)?;
relative_ref = ( Authority Path? | ( Path | PathNoscheme )? ) ( "?" Query )? ( "#" Fragment )?;
URI = absolute_URI | relative_ref ;

Root = URI;

