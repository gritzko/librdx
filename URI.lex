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

Path = slash ( path_char+ ( slash path_char* )* )? ;

drivePath = (slash|(alpha ":" slash)) ( path_char+ ( slash path_char* )* )? ;

Scheme = (alpha ( alpha | digit | "+" | "-" | "." )*) ':' ;

dec_octet = digit{1,3};
IPv4address = dec_octet "." dec_octet "." dec_octet "." dec_octet;
IPvFuture  = "v" xdigit+ "." ( unreserved | sub_delims | ":" )+;
IPv6address = (":" | xdigit)+ IPv4address?;
IP_literal = "[" ( IPv6address | IPvFuture  ) "]";

reg_name = ( unreserved | pct_encoded | sub_delims )+;

User    = ( unreserved | pct_encoded | sub_delims | ":" | "@" )*;
Host        = IP_literal | IPv4address | reg_name ;
Port        = (pchar - ("/" | "?" | "#")){1,5} ;
authority   = "//" ( ( User "@" )? Host ( ":" Port )? ) ;

Fragment = ( pchar | "/" | "?" )* ;

Query = (pchar - "#")* ;

full_ref = Path ( "?" Query )? ( "#" Fragment )?;
relative_ref = Path ( "?" Query )? ( "#" Fragment )?;
absolute_hier_part = authority? full_ref?;
hier_part = authority? relative_ref?;

absolute_URI = Scheme? absolute_hier_part;
URI = absolute_URI | relative_ref ;

Root = URI;

