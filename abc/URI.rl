#include "URI.rl.h"


%%{

machine URI;

alphtype unsigned char;

action URIPath0 { mark0[URIPath] = p - text[0]; }
action URIPath1 {
    tok[0] = text[0] + mark0[URIPath];
    tok[1] = p;
    call(URIonPath, tok, state); 
}
action URIScheme0 { mark0[URIScheme] = p - text[0]; }
action URIScheme1 {
    tok[0] = text[0] + mark0[URIScheme];
    tok[1] = p;
    call(URIonScheme, tok, state); 
}
action URIIPv4address0 { mark0[URIIPv4address] = p - text[0]; }
action URIIPv4address1 {
    tok[0] = text[0] + mark0[URIIPv4address];
    tok[1] = p;
    call(URIonIPv4address, tok, state); 
}
action URIIPvFuture0 { mark0[URIIPvFuture] = p - text[0]; }
action URIIPvFuture1 {
    tok[0] = text[0] + mark0[URIIPvFuture];
    tok[1] = p;
    call(URIonIPvFuture, tok, state); 
}
action URIIPv6address0 { mark0[URIIPv6address] = p - text[0]; }
action URIIPv6address1 {
    tok[0] = text[0] + mark0[URIIPv6address];
    tok[1] = p;
    call(URIonIPv6address, tok, state); 
}
action URIIP_literal0 { mark0[URIIP_literal] = p - text[0]; }
action URIIP_literal1 {
    tok[0] = text[0] + mark0[URIIP_literal];
    tok[1] = p;
    call(URIonIP_literal, tok, state); 
}
action URIUser0 { mark0[URIUser] = p - text[0]; }
action URIUser1 {
    tok[0] = text[0] + mark0[URIUser];
    tok[1] = p;
    call(URIonUser, tok, state); 
}
action URIHost0 { mark0[URIHost] = p - text[0]; }
action URIHost1 {
    tok[0] = text[0] + mark0[URIHost];
    tok[1] = p;
    call(URIonHost, tok, state); 
}
action URIPort0 { mark0[URIPort] = p - text[0]; }
action URIPort1 {
    tok[0] = text[0] + mark0[URIPort];
    tok[1] = p;
    call(URIonPort, tok, state); 
}
action URIFragment0 { mark0[URIFragment] = p - text[0]; }
action URIFragment1 {
    tok[0] = text[0] + mark0[URIFragment];
    tok[1] = p;
    call(URIonFragment, tok, state); 
}
action URIQuery0 { mark0[URIQuery] = p - text[0]; }
action URIQuery1 {
    tok[0] = text[0] + mark0[URIQuery];
    tok[1] = p;
    call(URIonQuery, tok, state); 
}
action URIURI0 { mark0[URIURI] = p - text[0]; }
action URIURI1 {
    tok[0] = text[0] + mark0[URIURI];
    tok[1] = p;
    call(URIonURI, tok, state); 
}
action URIRoot0 { mark0[URIRoot] = p - text[0]; }
action URIRoot1 {
    tok[0] = text[0] + mark0[URIRoot];
    tok[1] = p;
    call(URIonRoot, tok, state); 
}

URIalpha  = (   [a-zA-Z] );

URIdigit  = (   [0-9] );

URIxdigit  = (   [0-9a-fA-F] );


URIpct_encoded  = (   "%"  URIxdigit  URIxdigit );


URIgen_delims   = (   ":"  |  "/"  |  "?"  |  "#"  |  "["  |  "]"  |  "@" );


URIsub_delims   = (   "!"  |  "$"  |  "&"  |  "'"  |  "("  |  ")"  |  "*"  |  "+"  |  ","  |  ";"  |  "=" );


URIreserved     = (   URIgen_delims  |  URIsub_delims );


URIunreserved   = (   URIalpha  |  URIdigit  |  "-"  |  "."  |  "_"  |  "~" );


URIdelims       = (   "<"  |  ">"  |  "%"  |    "#"  |  '"' );


URIunwise       = (   " "  |  "{"  |  "}"  |  "|"  |  "\\"  |  "^"  |  "["  |  "]"  |  "`" );


URIpchar  = (   URIunreserved  |  URIpct_encoded  |  URIsub_delims  |  ":"  |  "@"  |  URIdelims  |  URIunwise );


URIslash  = (   "/"  |  "\\" );


URIpath_char  = (   URIpchar  -  ("?"  |  "#") );


URIPath  = (   URIslash  (  URIpath_char+  (  URIslash  URIpath_char*  )*  )?   )  >URIPath0 %URIPath1;


URIdrivePath  = (   (URIslash|(URIalpha  ":"  URIslash))  (  URIpath_char+  (  URIslash  URIpath_char*  )*  )?   );


URIScheme  = (   (URIalpha  (  URIalpha  |  URIdigit  |  "+"  |  "-"  |  "."  )*)  ':'   )  >URIScheme0 %URIScheme1;


URIdec_octet  = (   URIdigit{1,3} );

URIIPv4address  = (   URIdec_octet  "."  URIdec_octet  "."  URIdec_octet  "."  URIdec_octet )  >URIIPv4address0 %URIIPv4address1;

URIIPvFuture   = (   "v"  URIxdigit+  "."  (  URIunreserved  |  URIsub_delims  |  ":"  )+ )  >URIIPvFuture0 %URIIPvFuture1;

URIIPv6address  = (   (":"  |  URIxdigit)+  URIIPv4address? )  >URIIPv6address0 %URIIPv6address1;

URIIP_literal  = (   "["  (  URIIPv6address  |  URIIPvFuture    )  "]" )  >URIIP_literal0 %URIIP_literal1;


URIreg_name  = (   (  URIunreserved  |  URIpct_encoded  |  URIsub_delims  )+ );


URIUser     = (   (  URIunreserved  |  URIpct_encoded  |  URIsub_delims  |  ":"  |  "@"  )* )  >URIUser0 %URIUser1;

URIHost         = (   URIIP_literal  |  URIIPv4address  |  URIreg_name   )  >URIHost0 %URIHost1;

URIPort         = (   (URIpchar  -  ("/"  |  "?"  |  "#")){1,5}   )  >URIPort0 %URIPort1;

URIauthority    = (   "//"  (  (  URIUser  "@"  )?  URIHost  (  ":"  URIPort  )?  )   );


URIFragment  = (   (  URIpchar  |  "/"  |  "?"  )*   )  >URIFragment0 %URIFragment1;


URIQuery  = (   (URIpchar  -  "#")*   )  >URIQuery0 %URIQuery1;


URIfull_ref  = (   URIPath  (  "?"  URIQuery  )?  (  "#"  URIFragment  )? );

URIrelative_ref  = (   URIPath  (  "?"  URIQuery  )?  (  "#"  URIFragment  )? );

URIabsolute_hier_part  = (   URIauthority?  URIfull_ref? );

URIhier_part  = (   URIauthority?  URIrelative_ref? );


URIabsolute_URI  = (   URIScheme?  URIabsolute_hier_part );

URIURI  = (   URIabsolute_URI  |  URIrelative_ref   )  >URIURI0 %URIURI1;


URIRoot  = (   URIURI )  >URIRoot0 %URIRoot1;


main := URIRoot;

}%%

%%write data;

pro(URIlexer, URIstate* state) {

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
    if (p!=text[1] || cs < URI_first_final) {
        return URIfail;
    }
    done;
}
