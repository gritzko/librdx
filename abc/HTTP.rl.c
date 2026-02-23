
#line 1 "HTTP.c.rl"
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




#line 246 "HTTP.c.rl"



#line 49 "HTTP.rl.c"
static const char _HTTP_actions[] = {
	0, 1, 1, 1, 2, 1, 3, 1, 
	4, 1, 5, 1, 7, 1, 9, 1, 
	10, 1, 11, 1, 13, 1, 16, 1, 
	20, 1, 21, 1, 22, 1, 23, 1, 
	25, 2, 10, 11, 2, 15, 16, 3, 
	7, 12, 8, 3, 13, 12, 8, 3, 
	14, 6, 0, 3, 25, 12, 8, 4, 
	17, 19, 29, 31, 4, 17, 27, 29, 
	31, 4, 30, 28, 18, 26, 5, 14, 
	6, 0, 24, 4, 5, 16, 17, 27, 
	29, 31, 6, 15, 16, 17, 19, 29, 
	31, 6, 30, 28, 18, 14, 6, 0, 
	9, 30, 28, 18, 14, 6, 0, 26, 
	24, 4
};

static const short _HTTP_key_offsets[] = {
	0, 0, 10, 20, 21, 23, 24, 25, 
	26, 27, 28, 29, 30, 34, 38, 39, 
	40, 41, 42, 43, 45, 46, 48, 49, 
	50, 66, 67, 83, 89, 90, 108, 114, 
	115, 133, 134, 135, 136, 137, 138, 139, 
	141, 142, 143, 144, 145, 146, 148, 149, 
	151, 152, 154, 156, 158, 159, 165, 171, 
	172, 190, 191, 207, 213, 214, 232, 238, 
	239, 257, 258, 260, 261, 262, 263, 264, 
	265, 266, 268, 269, 270, 271, 272, 272, 
	272, 272
};

static const unsigned char _HTTP_trans_keys[] = {
	9u, 13u, 32u, 67u, 68u, 71u, 72u, 79u, 
	80u, 84u, 9u, 13u, 32u, 67u, 68u, 71u, 
	72u, 79u, 80u, 84u, 10u, 9u, 32u, 79u, 
	78u, 78u, 69u, 67u, 84u, 32u, 13u, 32u, 
	9u, 10u, 13u, 32u, 9u, 10u, 72u, 84u, 
	84u, 80u, 47u, 48u, 57u, 46u, 48u, 57u, 
	13u, 10u, 13u, 33u, 124u, 126u, 35u, 39u, 
	42u, 43u, 45u, 46u, 48u, 57u, 65u, 90u, 
	94u, 122u, 10u, 33u, 58u, 124u, 126u, 35u, 
	39u, 42u, 43u, 45u, 46u, 48u, 57u, 65u, 
	90u, 94u, 122u, 9u, 13u, 32u, 127u, 0u, 
	31u, 10u, 9u, 13u, 32u, 33u, 124u, 126u, 
	35u, 39u, 42u, 43u, 45u, 46u, 48u, 57u, 
	65u, 90u, 94u, 122u, 13u, 127u, 0u, 8u, 
	10u, 31u, 10u, 9u, 13u, 32u, 33u, 124u, 
	126u, 35u, 39u, 42u, 43u, 45u, 46u, 48u, 
	57u, 65u, 90u, 94u, 122u, 69u, 76u, 69u, 
	84u, 69u, 69u, 69u, 84u, 65u, 68u, 84u, 
	80u, 47u, 48u, 57u, 46u, 48u, 57u, 32u, 
	48u, 57u, 48u, 57u, 48u, 57u, 32u, 13u, 
	127u, 0u, 8u, 10u, 31u, 13u, 127u, 0u, 
	8u, 10u, 31u, 10u, 9u, 13u, 32u, 33u, 
	124u, 126u, 35u, 39u, 42u, 43u, 45u, 46u, 
	48u, 57u, 65u, 90u, 94u, 122u, 10u, 33u, 
	58u, 124u, 126u, 35u, 39u, 42u, 43u, 45u, 
	46u, 48u, 57u, 65u, 90u, 94u, 122u, 9u, 
	13u, 32u, 127u, 0u, 31u, 10u, 9u, 13u, 
	32u, 33u, 124u, 126u, 35u, 39u, 42u, 43u, 
	45u, 46u, 48u, 57u, 65u, 90u, 94u, 122u, 
	13u, 127u, 0u, 8u, 10u, 31u, 10u, 9u, 
	13u, 32u, 33u, 124u, 126u, 35u, 39u, 42u, 
	43u, 45u, 46u, 48u, 57u, 65u, 90u, 94u, 
	122u, 10u, 9u, 32u, 80u, 84u, 73u, 79u, 
	78u, 83u, 79u, 85u, 83u, 82u, 65u, 67u, 
	0
};

static const char _HTTP_single_lengths[] = {
	0, 10, 10, 1, 2, 1, 1, 1, 
	1, 1, 1, 1, 2, 2, 1, 1, 
	1, 1, 1, 0, 1, 0, 1, 1, 
	4, 1, 4, 4, 1, 6, 2, 1, 
	6, 1, 1, 1, 1, 1, 1, 2, 
	1, 1, 1, 1, 1, 0, 1, 0, 
	1, 0, 0, 0, 1, 2, 2, 1, 
	6, 1, 4, 4, 1, 6, 2, 1, 
	6, 1, 2, 1, 1, 1, 1, 1, 
	1, 2, 1, 1, 1, 1, 0, 0, 
	0, 0
};

static const char _HTTP_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 1, 1, 0, 0, 
	0, 0, 0, 1, 0, 1, 0, 0, 
	6, 0, 6, 1, 0, 6, 2, 0, 
	6, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 0, 1, 
	0, 1, 1, 1, 0, 2, 2, 0, 
	6, 0, 6, 1, 0, 6, 2, 0, 
	6, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const short _HTTP_index_offsets[] = {
	0, 0, 11, 22, 24, 27, 29, 31, 
	33, 35, 37, 39, 41, 45, 49, 51, 
	53, 55, 57, 59, 61, 63, 65, 67, 
	69, 80, 82, 93, 99, 101, 114, 119, 
	121, 134, 136, 138, 140, 142, 144, 146, 
	149, 151, 153, 155, 157, 159, 161, 163, 
	165, 167, 169, 171, 173, 175, 180, 185, 
	187, 200, 202, 213, 219, 221, 234, 239, 
	241, 254, 256, 259, 261, 263, 265, 267, 
	269, 271, 274, 276, 278, 280, 282, 283, 
	284, 285
};

static const char _HTTP_indicies[] = {
	0, 2, 0, 3, 4, 5, 6, 7, 
	8, 9, 1, 10, 11, 10, 12, 13, 
	14, 15, 16, 17, 18, 1, 19, 1, 
	10, 10, 1, 20, 1, 21, 1, 22, 
	1, 23, 1, 24, 1, 25, 1, 26, 
	1, 1, 1, 1, 27, 1, 29, 1, 
	28, 30, 1, 31, 1, 32, 1, 33, 
	1, 34, 1, 35, 1, 36, 1, 37, 
	1, 38, 1, 39, 1, 40, 41, 41, 
	41, 41, 41, 41, 41, 41, 41, 1, 
	42, 1, 43, 44, 43, 43, 43, 43, 
	43, 43, 43, 43, 1, 45, 46, 45, 
	1, 1, 47, 48, 1, 49, 50, 49, 
	51, 51, 51, 51, 51, 51, 51, 51, 
	51, 1, 53, 1, 1, 1, 52, 54, 
	1, 52, 50, 52, 51, 51, 51, 51, 
	51, 51, 51, 51, 51, 1, 55, 1, 
	56, 1, 57, 1, 58, 1, 25, 1, 
	24, 1, 59, 60, 1, 61, 1, 25, 
	1, 62, 1, 63, 1, 64, 1, 65, 
	1, 66, 1, 67, 1, 68, 1, 69, 
	1, 70, 1, 71, 1, 72, 1, 74, 
	1, 1, 1, 73, 76, 1, 1, 1, 
	75, 77, 1, 75, 78, 75, 79, 79, 
	79, 79, 79, 79, 79, 79, 79, 1, 
	80, 1, 81, 82, 81, 81, 81, 81, 
	81, 81, 81, 81, 1, 83, 84, 83, 
	1, 1, 85, 86, 1, 87, 88, 87, 
	89, 89, 89, 89, 89, 89, 89, 89, 
	89, 1, 91, 1, 1, 1, 90, 92, 
	1, 90, 88, 90, 89, 89, 89, 89, 
	89, 89, 89, 89, 89, 1, 93, 1, 
	75, 75, 1, 94, 1, 95, 1, 96, 
	1, 97, 1, 98, 1, 25, 1, 99, 
	24, 1, 24, 1, 100, 1, 101, 1, 
	58, 1, 102, 103, 104, 105, 0
};

static const char _HTTP_trans_targs[] = {
	2, 0, 3, 5, 33, 38, 39, 67, 
	73, 75, 2, 3, 5, 33, 38, 39, 
	67, 73, 75, 4, 6, 7, 8, 9, 
	10, 11, 12, 13, 13, 14, 15, 16, 
	17, 18, 19, 20, 21, 22, 23, 24, 
	25, 26, 78, 26, 27, 27, 28, 30, 
	29, 27, 25, 26, 30, 31, 32, 34, 
	35, 36, 37, 40, 42, 41, 43, 44, 
	45, 46, 47, 48, 49, 50, 51, 52, 
	53, 54, 65, 54, 55, 56, 57, 58, 
	80, 58, 59, 59, 60, 62, 61, 59, 
	57, 58, 62, 63, 64, 66, 68, 69, 
	70, 71, 72, 74, 76, 77, 79, 79, 
	81, 81
};

static const char _HTTP_trans_actions[] = {
	65, 0, 65, 89, 89, 89, 96, 89, 
	89, 89, 0, 0, 47, 47, 47, 70, 
	47, 47, 47, 0, 0, 0, 0, 0, 
	0, 0, 1, 3, 0, 5, 7, 0, 
	0, 0, 0, 0, 0, 0, 9, 0, 
	11, 39, 0, 0, 13, 15, 33, 15, 
	0, 0, 19, 43, 0, 17, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 9, 23, 0, 0, 
	25, 27, 27, 0, 29, 0, 31, 51, 
	0, 0, 13, 15, 33, 15, 0, 0, 
	19, 43, 0, 17, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 36, 0, 
	21, 0
};

static const char _HTTP_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 82, 55, 
	76, 60
};

static const int HTTP_start = 1;
static const int HTTP_first_final = 78;
static const int HTTP_error = 0;

static const int HTTP_en_main = 1;


#line 249 "HTTP.c.rl"

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

    
#line 269 "HTTP.rl.c"
	{
	cs = HTTP_start;
	}

#line 266 "HTTP.c.rl"
    
#line 272 "HTTP.rl.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _HTTP_trans_keys + _HTTP_key_offsets[cs];
	_trans = _HTTP_index_offsets[cs];

	_klen = _HTTP_single_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _HTTP_range_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _HTTP_indicies[_trans];
	cs = _HTTP_trans_targs[_trans];

	if ( _HTTP_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _HTTP_actions + _HTTP_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 53 "HTTP.c.rl"
	{ mark0[HTTPMethod] = p - data[0]; }
	break;
	case 1:
#line 54 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPMethod];
    tok[1] = p;
    o = HTTPonMethod(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 2:
#line 62 "HTTP.c.rl"
	{ mark0[HTTPRequestURI] = p - data[0]; }
	break;
	case 3:
#line 63 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPRequestURI];
    tok[1] = p;
    o = HTTPonRequestURI(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 4:
#line 71 "HTTP.c.rl"
	{ mark0[HTTPHTTPVersion] = p - data[0]; }
	break;
	case 5:
#line 72 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPHTTPVersion];
    tok[1] = p;
    o = HTTPonHTTPVersion(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 6:
#line 80 "HTTP.c.rl"
	{ mark0[HTTPRequestLine] = p - data[0]; }
	break;
	case 7:
#line 81 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPRequestLine];
    tok[1] = p;
    o = HTTPonRequestLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 8:
#line 89 "HTTP.c.rl"
	{ mark0[HTTPFieldName] = p - data[0]; }
	break;
	case 9:
#line 90 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPFieldName];
    tok[1] = p;
    o = HTTPonFieldName(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 10:
#line 98 "HTTP.c.rl"
	{ mark0[HTTPFieldValue] = p - data[0]; }
	break;
	case 11:
#line 99 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPFieldValue];
    tok[1] = p;
    o = HTTPonFieldValue(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 12:
#line 107 "HTTP.c.rl"
	{ mark0[HTTPMessageHeader] = p - data[0]; }
	break;
	case 13:
#line 108 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPMessageHeader];
    tok[1] = p;
    o = HTTPonMessageHeader(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 14:
#line 116 "HTTP.c.rl"
	{ mark0[HTTPRequestHead] = p - data[0]; }
	break;
	case 15:
#line 117 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPRequestHead];
    tok[1] = p;
    o = HTTPonRequestHead(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 16:
#line 125 "HTTP.c.rl"
	{ mark0[HTTPBody] = p - data[0]; }
	break;
	case 18:
#line 134 "HTTP.c.rl"
	{ mark0[HTTPRequest] = p - data[0]; }
	break;
	case 20:
#line 143 "HTTP.c.rl"
	{ mark0[HTTPStatusCode] = p - data[0]; }
	break;
	case 21:
#line 144 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPStatusCode];
    tok[1] = p;
    o = HTTPonStatusCode(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 22:
#line 152 "HTTP.c.rl"
	{ mark0[HTTPReasonPhrase] = p - data[0]; }
	break;
	case 23:
#line 153 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPReasonPhrase];
    tok[1] = p;
    o = HTTPonReasonPhrase(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 24:
#line 161 "HTTP.c.rl"
	{ mark0[HTTPStatusLine] = p - data[0]; }
	break;
	case 25:
#line 162 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPStatusLine];
    tok[1] = p;
    o = HTTPonStatusLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 26:
#line 170 "HTTP.c.rl"
	{ mark0[HTTPResponse] = p - data[0]; }
	break;
	case 28:
#line 179 "HTTP.c.rl"
	{ mark0[HTTPMessage] = p - data[0]; }
	break;
	case 30:
#line 188 "HTTP.c.rl"
	{ mark0[HTTPRoot] = p - data[0]; }
	break;
#line 503 "HTTP.rl.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _HTTP_actions + _HTTP_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 15:
#line 117 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPRequestHead];
    tok[1] = p;
    o = HTTPonRequestHead(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 16:
#line 125 "HTTP.c.rl"
	{ mark0[HTTPBody] = p - data[0]; }
	break;
	case 17:
#line 126 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPBody];
    tok[1] = p;
    o = HTTPonBody(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 19:
#line 135 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPRequest];
    tok[1] = p;
    o = HTTPonRequest(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 27:
#line 171 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPResponse];
    tok[1] = p;
    o = HTTPonResponse(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 29:
#line 180 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPMessage];
    tok[1] = p;
    o = HTTPonMessage(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 31:
#line 189 "HTTP.c.rl"
	{
    tok[0] = data[0] + mark0[HTTPRoot];
    tok[1] = p;
    o = HTTPonRoot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
#line 581 "HTTP.rl.c"
		}
	}
	}

	_out: {}
	}

#line 267 "HTTP.c.rl"

    state->data[0] = p;
    if (o==OK && cs < HTTP_first_final) 
        o = HTTPBAD;
    
    return o;
}
