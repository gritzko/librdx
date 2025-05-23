
#line 1 "CLEX.c.rl"
#include "abc/INT.h"
#include "abc/PRO.h"
#include "CLEX.h"

// action indices for the parser
#define CLEXenum 0
enum {
	CLEXToken = CLEXenum+40,
	CLEXRoot = CLEXenum+41,
};

// user functions (callbacks) for the parser
ok64 CLEXonToken ($cu8c tok, CLEXstate* state);
ok64 CLEXonRoot ($cu8c tok, CLEXstate* state);




#line 148 "CLEX.c.rl"



#line 21 "CLEX.rl.c"
static const int CLEX_start = 19;
static const int CLEX_first_final = 19;
static const int CLEX_error = 0;

static const int CLEX_en_main = 19;


#line 151 "CLEX.c.rl"

// the public API function
ok64 CLEXlexer(CLEXstate* state) {

    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = pe;
    u64 mark0[64] = {};
    ok64 o = OK;

    $u8c tok = {p, p};

    
#line 43 "CLEX.rl.c"
	{
	cs = CLEX_start;
	}

#line 168 "CLEX.c.rl"
    
#line 46 "CLEX.rl.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 19:
	switch( (*p) ) {
		case 13u: goto tr19;
		case 32u: goto tr19;
		case 33u: goto tr20;
		case 34u: goto tr21;
		case 35u: goto tr22;
		case 36u: goto tr23;
		case 37u: goto tr24;
		case 38u: goto tr25;
		case 39u: goto tr26;
		case 42u: goto tr20;
		case 43u: goto tr28;
		case 45u: goto tr29;
		case 46u: goto tr30;
		case 47u: goto tr31;
		case 48u: goto tr32;
		case 58u: goto tr34;
		case 60u: goto tr35;
		case 61u: goto tr20;
		case 62u: goto tr36;
		case 76u: goto tr37;
		case 94u: goto tr20;
		case 95u: goto tr23;
		case 124u: goto tr38;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr19;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr33;
		} else
			goto tr27;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr23;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr27;
			} else if ( (*p) >= 97u )
				goto tr23;
		} else
			goto tr27;
	} else
		goto tr27;
	goto st0;
st0:
cs = 0;
	goto _out;
tr19:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
	goto st20;
tr59:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 20; goto _out;}
    }
}
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 121 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto st20;
		case 32u: goto st20;
		case 33u: goto tr40;
		case 34u: goto tr41;
		case 35u: goto tr42;
		case 36u: goto tr43;
		case 37u: goto tr44;
		case 38u: goto tr45;
		case 39u: goto tr46;
		case 42u: goto tr40;
		case 43u: goto tr48;
		case 45u: goto tr49;
		case 46u: goto tr50;
		case 47u: goto tr51;
		case 48u: goto tr52;
		case 58u: goto tr54;
		case 60u: goto tr55;
		case 61u: goto tr40;
		case 62u: goto tr56;
		case 76u: goto tr57;
		case 94u: goto tr40;
		case 95u: goto tr43;
		case 124u: goto tr58;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto st20;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr53;
		} else
			goto tr47;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr43;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr47;
			} else if ( (*p) >= 97u )
				goto tr43;
		} else
			goto tr47;
	} else
		goto tr47;
	goto st0;
tr20:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st21;
tr40:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st21;
tr60:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 21; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st21;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
#line 192 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto st22;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr21:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st1;
tr41:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st1;
tr61:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 1; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st1;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 263 "CLEX.rl.c"
	switch( (*p) ) {
		case 10u: goto st0;
		case 34u: goto st22;
		case 92u: goto st16;
	}
	goto st1;
tr27:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st22;
tr47:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st22;
tr67:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 22; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st22;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
#line 291 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr22:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st23;
tr42:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st23;
tr62:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 23; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st23;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
#line 362 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto st22;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr23:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st24;
tr43:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st24;
tr63:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 24; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st24;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
#line 433 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto st24;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 94u: goto tr60;
		case 95u: goto st24;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 48u <= (*p) && (*p) <= 57u )
				goto st24;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto st24;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto st24;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr24:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st25;
tr44:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st25;
tr64:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 25; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st25;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
#line 502 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto st50;
		case 59u: goto tr67;
		case 60u: goto tr75;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 63u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( (*p) > 57u ) {
				if ( 61u <= (*p) && (*p) <= 62u )
					goto st22;
			} else if ( (*p) >= 49u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr25:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st26;
tr45:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st26;
tr65:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 26; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st26;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
#line 575 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto st22;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto st22;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr26:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st2;
tr46:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st2;
tr66:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 2; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 646 "CLEX.rl.c"
	switch( (*p) ) {
		case 10u: goto st0;
		case 39u: goto st0;
		case 92u: goto st4;
	}
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	switch( (*p) ) {
		case 10u: goto st0;
		case 39u: goto st22;
		case 92u: goto st4;
	}
	goto st3;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( (*p) ) {
		case 92u: goto st5;
		case 120u: goto st6;
	}
	if ( 48u <= (*p) && (*p) <= 55u )
		goto st3;
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	switch( (*p) ) {
		case 34u: goto st3;
		case 39u: goto st3;
		case 63u: goto st3;
		case 102u: goto st3;
		case 110u: goto st3;
		case 114u: goto st3;
		case 116u: goto st3;
		case 118u: goto st3;
	}
	if ( (*p) > 92u ) {
		if ( 97u <= (*p) && (*p) <= 98u )
			goto st3;
	} else if ( (*p) >= 91u )
		goto st3;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st3;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st3;
	} else
		goto st3;
	goto st0;
tr28:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st27;
tr48:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st27;
tr68:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 27; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st27;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
#line 728 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto st22;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto st22;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr29:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st28;
tr49:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st28;
tr69:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 28; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st28;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
#line 799 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto st22;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 59u: goto tr67;
		case 60u: goto tr75;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 63u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( (*p) > 57u ) {
				if ( 61u <= (*p) && (*p) <= 62u )
					goto st22;
			} else if ( (*p) >= 49u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr30:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st29;
tr50:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st29;
tr70:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 29; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st29;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
#line 872 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto st7;
		case 47u: goto tr71;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 48u <= (*p) && (*p) <= 57u )
				goto st33;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 46u )
		goto st22;
	goto st0;
tr31:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st30;
tr51:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st30;
tr71:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 30; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st30;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
#line 949 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto st31;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto st22;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
	if ( (*p) == 10u )
		goto tr59;
	goto st31;
tr32:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st32;
tr52:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st32;
tr72:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 32; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st32;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
#line 1027 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto st33;
		case 47u: goto tr71;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 69u: goto st8;
		case 76u: goto st41;
		case 85u: goto st43;
		case 88u: goto st11;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 101u: goto st8;
		case 108u: goto st46;
		case 117u: goto st43;
		case 120u: goto st12;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( (*p) > 55u ) {
				if ( 56u <= (*p) && (*p) <= 57u )
					goto st10;
			} else if ( (*p) >= 48u )
				goto st47;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 69u: goto st8;
		case 70u: goto st22;
		case 76u: goto st22;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 101u: goto st8;
		case 102u: goto st22;
		case 108u: goto st22;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 48u <= (*p) && (*p) <= 57u )
				goto st33;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr34:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st34;
tr54:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st34;
tr74:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 34; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st34;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
#line 1164 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto st22;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr33:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st35;
tr53:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st35;
tr73:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 35; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st35;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
#line 1235 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto st33;
		case 47u: goto tr71;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 69u: goto st8;
		case 76u: goto st41;
		case 85u: goto st43;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 101u: goto st8;
		case 108u: goto st46;
		case 117u: goto st43;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 48u <= (*p) && (*p) <= 57u )
				goto st35;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr35:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st36;
tr55:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st36;
tr75:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 36; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st36;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
#line 1310 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto st22;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 59u: goto tr67;
		case 60u: goto st21;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 63u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( (*p) > 57u ) {
				if ( 58u <= (*p) && (*p) <= 61u )
					goto st22;
			} else if ( (*p) >= 49u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr36:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st37;
tr56:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st37;
tr76:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 37; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st37;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
#line 1383 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto st22;
		case 62u: goto st21;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr37:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st38;
tr57:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st38;
tr77:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 38; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st38;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
#line 1454 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto st1;
		case 35u: goto tr62;
		case 36u: goto st24;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto st2;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 94u: goto tr60;
		case 95u: goto st24;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 48u <= (*p) && (*p) <= 57u )
				goto st24;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto st24;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto st24;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
tr38:
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st39;
tr58:
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st39;
tr78:
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 39; goto _out;}
    }
}
#line 25 "CLEX.c.rl"
	{ mark0[CLEXToken] = p - text[0]; }
	goto st39;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
#line 1523 "CLEX.rl.c"
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto st22;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto st22;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	switch( (*p) ) {
		case 43u: goto st9;
		case 45u: goto st9;
	}
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st40;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st40;
	goto st0;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 70u: goto st22;
		case 76u: goto st22;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 102u: goto st22;
		case 108u: goto st22;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 48u <= (*p) && (*p) <= 57u )
				goto st40;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto st42;
		case 85u: goto st22;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 117u: goto st22;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 85u: goto st22;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 117u: goto st22;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto st44;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 108u: goto st45;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto st22;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 108u: goto st22;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 85u: goto st22;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 108u: goto st42;
		case 117u: goto st22;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto st33;
		case 47u: goto tr71;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 69u: goto st8;
		case 76u: goto st41;
		case 85u: goto st43;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 101u: goto st8;
		case 108u: goto st46;
		case 117u: goto st43;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( (*p) > 55u ) {
				if ( 56u <= (*p) && (*p) <= 57u )
					goto st10;
			} else if ( (*p) >= 48u )
				goto st47;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( (*p) ) {
		case 46u: goto st33;
		case 69u: goto st8;
		case 101u: goto st8;
	}
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st10;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st48;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st48;
	} else
		goto st48;
	goto st0;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto st41;
		case 85u: goto st43;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 108u: goto st46;
		case 117u: goto st43;
		case 124u: goto tr78;
	}
	if ( (*p) < 65u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( (*p) > 57u ) {
				if ( 59u <= (*p) && (*p) <= 64u )
					goto tr67;
			} else if ( (*p) >= 48u )
				goto st48;
		} else
			goto tr67;
	} else if ( (*p) > 70u ) {
		if ( (*p) < 97u ) {
			if ( (*p) > 90u ) {
				if ( 91u <= (*p) && (*p) <= 96u )
					goto tr67;
			} else if ( (*p) >= 71u )
				goto tr63;
		} else if ( (*p) > 102u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 103u )
				goto tr63;
		} else
			goto st48;
	} else
		goto st48;
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( (*p) == 46u )
		goto st13;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st49;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st49;
	} else
		goto st49;
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st14;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st14;
	} else
		goto st14;
	goto st0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	switch( (*p) ) {
		case 80u: goto st8;
		case 112u: goto st8;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st14;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st14;
	} else
		goto st14;
	goto st0;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto tr64;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto st14;
		case 47u: goto tr71;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto st41;
		case 80u: goto st8;
		case 85u: goto st43;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 108u: goto st46;
		case 112u: goto st8;
		case 117u: goto st43;
		case 124u: goto tr78;
	}
	if ( (*p) < 65u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( (*p) > 57u ) {
				if ( 59u <= (*p) && (*p) <= 64u )
					goto tr67;
			} else if ( (*p) >= 48u )
				goto st49;
		} else
			goto tr67;
	} else if ( (*p) > 70u ) {
		if ( (*p) < 97u ) {
			if ( (*p) > 90u ) {
				if ( 91u <= (*p) && (*p) <= 96u )
					goto tr67;
			} else if ( (*p) >= 71u )
				goto tr63;
		} else if ( (*p) > 102u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 103u )
				goto tr63;
		} else
			goto st49;
	} else
		goto st49;
	goto st0;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	switch( (*p) ) {
		case 13u: goto tr59;
		case 32u: goto tr59;
		case 33u: goto tr60;
		case 34u: goto tr61;
		case 35u: goto tr62;
		case 36u: goto tr63;
		case 37u: goto st15;
		case 38u: goto tr65;
		case 39u: goto tr66;
		case 42u: goto tr60;
		case 43u: goto tr68;
		case 45u: goto tr69;
		case 46u: goto tr70;
		case 47u: goto tr71;
		case 48u: goto tr72;
		case 58u: goto tr74;
		case 60u: goto tr75;
		case 61u: goto tr60;
		case 62u: goto tr76;
		case 76u: goto tr77;
		case 94u: goto tr60;
		case 95u: goto tr63;
		case 124u: goto tr78;
	}
	if ( (*p) < 59u ) {
		if ( (*p) < 40u ) {
			if ( 9u <= (*p) && (*p) <= 10u )
				goto tr59;
		} else if ( (*p) > 44u ) {
			if ( 49u <= (*p) && (*p) <= 57u )
				goto tr73;
		} else
			goto tr67;
	} else if ( (*p) > 64u ) {
		if ( (*p) < 91u ) {
			if ( 65u <= (*p) && (*p) <= 90u )
				goto tr63;
		} else if ( (*p) > 96u ) {
			if ( (*p) > 122u ) {
				if ( 123u <= (*p) && (*p) <= 126u )
					goto tr67;
			} else if ( (*p) >= 97u )
				goto tr63;
		} else
			goto tr67;
	} else
		goto tr67;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( (*p) == 58u )
		goto st22;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	switch( (*p) ) {
		case 92u: goto st17;
		case 120u: goto st18;
	}
	if ( 48u <= (*p) && (*p) <= 55u )
		goto st1;
	goto st0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	switch( (*p) ) {
		case 34u: goto st1;
		case 39u: goto st1;
		case 63u: goto st1;
		case 102u: goto st1;
		case 110u: goto st1;
		case 114u: goto st1;
		case 116u: goto st1;
		case 118u: goto st1;
	}
	if ( (*p) > 92u ) {
		if ( 97u <= (*p) && (*p) <= 98u )
			goto st1;
	} else if ( (*p) >= 91u )
		goto st1;
	goto st0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st1;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st1;
	} else
		goto st1;
	goto st0;
	}
	_test_eof20: cs = 20; goto _test_eof; 
	_test_eof21: cs = 21; goto _test_eof; 
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof22: cs = 22; goto _test_eof; 
	_test_eof23: cs = 23; goto _test_eof; 
	_test_eof24: cs = 24; goto _test_eof; 
	_test_eof25: cs = 25; goto _test_eof; 
	_test_eof26: cs = 26; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof27: cs = 27; goto _test_eof; 
	_test_eof28: cs = 28; goto _test_eof; 
	_test_eof29: cs = 29; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof30: cs = 30; goto _test_eof; 
	_test_eof31: cs = 31; goto _test_eof; 
	_test_eof32: cs = 32; goto _test_eof; 
	_test_eof33: cs = 33; goto _test_eof; 
	_test_eof34: cs = 34; goto _test_eof; 
	_test_eof35: cs = 35; goto _test_eof; 
	_test_eof36: cs = 36; goto _test_eof; 
	_test_eof37: cs = 37; goto _test_eof; 
	_test_eof38: cs = 38; goto _test_eof; 
	_test_eof39: cs = 39; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof40: cs = 40; goto _test_eof; 
	_test_eof41: cs = 41; goto _test_eof; 
	_test_eof42: cs = 42; goto _test_eof; 
	_test_eof43: cs = 43; goto _test_eof; 
	_test_eof44: cs = 44; goto _test_eof; 
	_test_eof45: cs = 45; goto _test_eof; 
	_test_eof46: cs = 46; goto _test_eof; 
	_test_eof47: cs = 47; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof48: cs = 48; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof49: cs = 49; goto _test_eof; 
	_test_eof50: cs = 50; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof18: cs = 18; goto _test_eof; 

	_test_eof: {}
	if ( p == eof )
	{
	switch ( cs ) {
	case 20: 
#line 35 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXRoot];
    tok[1] = p;
    o = CLEXonRoot(tok, state); 
    if (o!=OK) {
        {p++; cs = 0; goto _out;}
    }
}
	break;
	case 21: 
	case 22: 
	case 23: 
	case 24: 
	case 25: 
	case 26: 
	case 27: 
	case 28: 
	case 29: 
	case 30: 
	case 31: 
	case 32: 
	case 33: 
	case 34: 
	case 35: 
	case 36: 
	case 37: 
	case 38: 
	case 39: 
	case 40: 
	case 41: 
	case 42: 
	case 43: 
	case 44: 
	case 45: 
	case 46: 
	case 47: 
	case 48: 
	case 49: 
	case 50: 
#line 26 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXToken];
    tok[1] = p;
    o = CLEXonToken(tok, state); 
    if (o!=OK) {
        {p++; cs = 0; goto _out;}
    }
}
#line 35 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXRoot];
    tok[1] = p;
    o = CLEXonRoot(tok, state); 
    if (o!=OK) {
        {p++; cs = 0; goto _out;}
    }
}
	break;
	case 19: 
#line 34 "CLEX.c.rl"
	{ mark0[CLEXRoot] = p - text[0]; }
#line 35 "CLEX.c.rl"
	{
    tok[0] = text[0] + mark0[CLEXRoot];
    tok[1] = p;
    o = CLEXonRoot(tok, state); 
    if (o!=OK) {
        {p++; cs = 0; goto _out;}
    }
}
	break;
#line 2453 "CLEX.rl.c"
	}
	}

	_out: {}
	}

#line 169 "CLEX.c.rl"

    state->text[0] = p;
    if (p!=text[1] || cs < CLEX_first_final || o!=OK) {
        return CLEXfail;
    }
    return o;
}
