#include "abc/INT.h"
#include "abc/PRO.h"
#include "URI.h"

// action indices for the parser
#define URIenum 0
enum {
	URISegment = URIenum+14,
	URISegment_nz = URIenum+15,
	URIPath = URIenum+17,
	URIPathNoscheme = URIenum+18,
	URIScheme = URIenum+20,
	URIIPv4address = URIenum+22,
	URIIPvFuture = URIenum+23,
	URIIPv6address = URIenum+24,
	URIIP_literal = URIenum+25,
	URIUser = URIenum+27,
	URIHost = URIenum+28,
	URIPort = URIenum+29,
	URIAuthority = URIenum+30,
	URIFragment = URIenum+31,
	URIQuery = URIenum+32,
	URIPathRootless = URIenum+33,
	URIURI = URIenum+37,
	URIRoot = URIenum+38,
};

// user functions (callbacks) for the parser
ok64 URIonSegment (u8cs tok, URIstate* state);
ok64 URIonSegment_nz (u8cs tok, URIstate* state);
ok64 URIonPath (u8cs tok, URIstate* state);
ok64 URIonPathNoscheme (u8cs tok, URIstate* state);
ok64 URIonScheme (u8cs tok, URIstate* state);
ok64 URIonIPv4address (u8cs tok, URIstate* state);
ok64 URIonIPvFuture (u8cs tok, URIstate* state);
ok64 URIonIPv6address (u8cs tok, URIstate* state);
ok64 URIonIP_literal (u8cs tok, URIstate* state);
ok64 URIonUser (u8cs tok, URIstate* state);
ok64 URIonHost (u8cs tok, URIstate* state);
ok64 URIonPort (u8cs tok, URIstate* state);
ok64 URIonAuthority (u8cs tok, URIstate* state);
ok64 URIonFragment (u8cs tok, URIstate* state);
ok64 URIonQuery (u8cs tok, URIstate* state);
ok64 URIonPathRootless (u8cs tok, URIstate* state);
ok64 URIonURI (u8cs tok, URIstate* state);
ok64 URIonRoot (u8cs tok, URIstate* state);



%%{

machine URI;

alphtype unsigned char;

# ragel actions
action URISegment0 { mark0[URISegment] = p - data[0]; }
action URISegment1 {
    tok[0] = data[0] + mark0[URISegment];
    tok[1] = p;
    o = URIonSegment(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URISegment_nz0 { mark0[URISegment_nz] = p - data[0]; }
action URISegment_nz1 {
    tok[0] = data[0] + mark0[URISegment_nz];
    tok[1] = p;
    o = URIonSegment_nz(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIPath0 { mark0[URIPath] = p - data[0]; }
action URIPath1 {
    tok[0] = data[0] + mark0[URIPath];
    tok[1] = p;
    o = URIonPath(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIPathNoscheme0 { mark0[URIPathNoscheme] = p - data[0]; }
action URIPathNoscheme1 {
    tok[0] = data[0] + mark0[URIPathNoscheme];
    tok[1] = p;
    o = URIonPathNoscheme(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIScheme0 { mark0[URIScheme] = p - data[0]; }
action URIScheme1 {
    tok[0] = data[0] + mark0[URIScheme];
    tok[1] = p;
    o = URIonScheme(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIIPv4address0 { mark0[URIIPv4address] = p - data[0]; }
action URIIPv4address1 {
    tok[0] = data[0] + mark0[URIIPv4address];
    tok[1] = p;
    o = URIonIPv4address(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIIPvFuture0 { mark0[URIIPvFuture] = p - data[0]; }
action URIIPvFuture1 {
    tok[0] = data[0] + mark0[URIIPvFuture];
    tok[1] = p;
    o = URIonIPvFuture(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIIPv6address0 { mark0[URIIPv6address] = p - data[0]; }
action URIIPv6address1 {
    tok[0] = data[0] + mark0[URIIPv6address];
    tok[1] = p;
    o = URIonIPv6address(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIIP_literal0 { mark0[URIIP_literal] = p - data[0]; }
action URIIP_literal1 {
    tok[0] = data[0] + mark0[URIIP_literal];
    tok[1] = p;
    o = URIonIP_literal(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIUser0 { mark0[URIUser] = p - data[0]; }
action URIUser1 {
    tok[0] = data[0] + mark0[URIUser];
    tok[1] = p;
    o = URIonUser(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIHost0 { mark0[URIHost] = p - data[0]; }
action URIHost1 {
    tok[0] = data[0] + mark0[URIHost];
    tok[1] = p;
    o = URIonHost(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIPort0 { mark0[URIPort] = p - data[0]; }
action URIPort1 {
    tok[0] = data[0] + mark0[URIPort];
    tok[1] = p;
    o = URIonPort(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIAuthority0 { mark0[URIAuthority] = p - data[0]; }
action URIAuthority1 {
    tok[0] = data[0] + mark0[URIAuthority];
    tok[1] = p;
    o = URIonAuthority(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIFragment0 { mark0[URIFragment] = p - data[0]; }
action URIFragment1 {
    tok[0] = data[0] + mark0[URIFragment];
    tok[1] = p;
    o = URIonFragment(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIQuery0 { mark0[URIQuery] = p - data[0]; }
action URIQuery1 {
    tok[0] = data[0] + mark0[URIQuery];
    tok[1] = p;
    o = URIonQuery(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIPathRootless0 { mark0[URIPathRootless] = p - data[0]; }
action URIPathRootless1 {
    tok[0] = data[0] + mark0[URIPathRootless];
    tok[1] = p;
    o = URIonPathRootless(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIURI0 { mark0[URIURI] = p - data[0]; }
action URIURI1 {
    tok[0] = data[0] + mark0[URIURI];
    tok[1] = p;
    o = URIonURI(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
action URIRoot0 { mark0[URIRoot] = p - data[0]; }
action URIRoot1 {
    tok[0] = data[0] + mark0[URIRoot];
    tok[1] = p;
    o = URIonRoot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}

# ragel grammar rules
URIalpha = (   [a-zA-Z] ); # no alpha callback
URIdigit = (   [0-9] ); # no digit callback
URIxdigit = (   [0-9a-fA-F] ); # no xdigit callback
URIpct_encoded = (   "%"  URIxdigit  URIxdigit ); # no pct_encoded callback
URIgen_delims = (   ":"  |  "/"  |  "?"  |  "#"  |  "["  |  "]"  |  "@" ); # no gen_delims callback
URIsub_delims = (   "!"  |  "$"  |  "&"  |  "'"  |  "("  |  ")"  |  "*"  |  "+"  |  ","  |  ";"  |  "=" ); # no sub_delims callback
URIreserved = (   URIgen_delims  |  URIsub_delims ); # no reserved callback
URIunreserved = (   URIalpha  |  URIdigit  |  "-"  |  "."  |  "_"  |  "~" ); # no unreserved callback
URIdelims = (   "<"  |  ">"  |  "%"  |    "#"  |  '"' ); # no delims callback
URIunwise = (   " "  |  "{"  |  "}"  |  "|"  |  "\\"  |  "^"  |  "["  |  "]"  |  "`" ); # no unwise callback
URIpchar = (   URIunreserved  |  URIpct_encoded  |  URIsub_delims  |  ":"  |  "@"  |  URIdelims  |  URIunwise ); # no pchar callback
URIslash = (   "/"  |  "\\" ); # no slash callback
URIpath_char = (   URIpchar  -  ("?"  |  "#") ); # no path_char callback
URISegment = (   URIpath_char* )  >URISegment0 %URISegment1;
URISegment_nz = (   URIpath_char+ )  >URISegment_nz0 %URISegment_nz1;
URIsegment_nz_nc = (   (URIpath_char  -  ":")+ ); # no segment_nz_nc callback
URIPath = (   URIslash  (  URISegment_nz  (  URIslash  URISegment  )*  )?   )  >URIPath0 %URIPath1;
URIPathNoscheme = (   URIsegment_nz_nc  (  URIslash  URISegment  )* )  >URIPathNoscheme0 %URIPathNoscheme1;
URIdrivePath = (   (URIslash|(URIalpha  ":"  URIslash))  (  URIpath_char+  (  URIslash  URIpath_char*  )*  )?   ); # no drivePath callback
URIScheme = (   (URIalpha  (  URIalpha  |  URIdigit  |  "+"  |  "-"  |  "."  )*)  ':'   )  >URIScheme0 %URIScheme1;
URIdec_octet = (   URIdigit{1,3} ); # no dec_octet callback
URIIPv4address = (   URIdec_octet  "."  URIdec_octet  "."  URIdec_octet  "."  URIdec_octet )  >URIIPv4address0 %URIIPv4address1;
URIIPvFuture = (   "v"  URIxdigit+  "."  (  URIunreserved  |  URIsub_delims  |  ":"  )+ )  >URIIPvFuture0 %URIIPvFuture1;
URIIPv6address = (   (":"  |  URIxdigit)+  URIIPv4address? )  >URIIPv6address0 %URIIPv6address1;
URIIP_literal = (   "["  (  URIIPv6address  |  URIIPvFuture    )  "]" )  >URIIP_literal0 %URIIP_literal1;
URIreg_name = (   (  URIunreserved  |  URIpct_encoded  |  URIsub_delims  )* ); # no reg_name callback
URIUser = (   (  URIunreserved  |  URIpct_encoded  |  URIsub_delims  |  ":"  |  "@"  )* )  >URIUser0 %URIUser1;
URIHost = (   URIIP_literal  |  URIIPv4address  |  URIreg_name   )  >URIHost0 %URIHost1;
URIPort = (   (URIpchar  -  ("/"  |  "?"  |  "#")){1,5}   )  >URIPort0 %URIPort1;
URIAuthority = (   "//"  (  (  URIUser  "@"  )?  URIHost  (  ":"  URIPort  )?  )   )  >URIAuthority0 %URIAuthority1;
URIFragment = (   (  URIpchar  |  "/"  |  "?"  )*   )  >URIFragment0 %URIFragment1;
URIQuery = (   (URIpchar  -  "#")*   )  >URIQuery0 %URIQuery1;
URIPathRootless = (   URISegment_nz  (  URIslash  URISegment  )* )  >URIPathRootless0 %URIPathRootless1;
URIhier_part = (   URIAuthority  URIPath?  |  URIPath  |  URIPathRootless ); # no hier_part callback
URIabsolute_URI = (   URIScheme  URIhier_part?  ("?"  URIQuery)?  ("#"  URIFragment)? ); # no absolute_URI callback
URIrelative_ref = (   (  URIAuthority  URIPath?  |  (  URIPath  |  URIPathNoscheme  )?  )  (  "?"  URIQuery  )?  (  "#"  URIFragment  )? ); # no relative_ref callback
URIURI = (   URIabsolute_URI  |  URIrelative_ref   )  >URIURI0 %URIURI1;
URIRoot = (   URIURI )  >URIRoot0 %URIRoot1;

main := URIRoot;

}%%

%%write data;

// the public API function
ok64 URILexer(URIstate* state) {

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
    if (o==OK && cs < URI_first_final) 
        o = URIBAD;
    
    return o;
}
