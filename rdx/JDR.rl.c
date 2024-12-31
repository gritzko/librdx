
#line 1 "JDR.c.rl"
#include "abc/INT.h"
#include "abc/PRO.h"
#include "JDR.h"

// action indices for the parser
#define JDRenum 0
enum {
	JDRUtf8cp1 = JDRenum+8,
	JDRUtf8cp2 = JDRenum+9,
	JDRUtf8cp3 = JDRenum+10,
	JDRUtf8cp4 = JDRenum+11,
	JDRInt = JDRenum+17,
	JDRFloat = JDRenum+18,
	JDRRef = JDRenum+19,
	JDRString = JDRenum+20,
	JDRMLString = JDRenum+21,
	JDRTerm = JDRenum+22,
	JDRStamp = JDRenum+23,
	JDROpenP = JDRenum+24,
	JDRCloseP = JDRenum+25,
	JDROpenL = JDRenum+26,
	JDRCloseL = JDRenum+27,
	JDROpenE = JDRenum+28,
	JDRCloseE = JDRenum+29,
	JDROpenX = JDRenum+30,
	JDRCloseX = JDRenum+31,
	JDRComma = JDRenum+32,
	JDRColon = JDRenum+33,
	JDROpen = JDRenum+34,
	JDRClose = JDRenum+35,
	JDRInter = JDRenum+36,
	JDRFIRST = JDRenum+38,
	JDRRoot = JDRenum+39,
};

// user functions (callbacks) for the parser
ok64 JDRonUtf8cp1 ($cu8c tok, JDRstate* state);
ok64 JDRonUtf8cp2 ($cu8c tok, JDRstate* state);
ok64 JDRonUtf8cp3 ($cu8c tok, JDRstate* state);
ok64 JDRonUtf8cp4 ($cu8c tok, JDRstate* state);
ok64 JDRonInt ($cu8c tok, JDRstate* state);
ok64 JDRonFloat ($cu8c tok, JDRstate* state);
ok64 JDRonRef ($cu8c tok, JDRstate* state);
ok64 JDRonString ($cu8c tok, JDRstate* state);
ok64 JDRonMLString ($cu8c tok, JDRstate* state);
ok64 JDRonTerm ($cu8c tok, JDRstate* state);
ok64 JDRonStamp ($cu8c tok, JDRstate* state);
ok64 JDRonOpenP ($cu8c tok, JDRstate* state);
ok64 JDRonCloseP ($cu8c tok, JDRstate* state);
ok64 JDRonOpenL ($cu8c tok, JDRstate* state);
ok64 JDRonCloseL ($cu8c tok, JDRstate* state);
ok64 JDRonOpenE ($cu8c tok, JDRstate* state);
ok64 JDRonCloseE ($cu8c tok, JDRstate* state);
ok64 JDRonOpenX ($cu8c tok, JDRstate* state);
ok64 JDRonCloseX ($cu8c tok, JDRstate* state);
ok64 JDRonComma ($cu8c tok, JDRstate* state);
ok64 JDRonColon ($cu8c tok, JDRstate* state);
ok64 JDRonOpen ($cu8c tok, JDRstate* state);
ok64 JDRonClose ($cu8c tok, JDRstate* state);
ok64 JDRonInter ($cu8c tok, JDRstate* state);
ok64 JDRonFIRST ($cu8c tok, JDRstate* state);
ok64 JDRonRoot ($cu8c tok, JDRstate* state);




#line 275 "JDR.c.rl"



#line 69 "JDR.rl.c"
static const char _JDR_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 9, 1, 11, 1, 13, 1, 
	15, 1, 17, 1, 19, 1, 20, 1, 
	21, 1, 23, 1, 25, 1, 27, 1, 
	29, 1, 31, 1, 33, 1, 35, 1, 
	37, 1, 39, 1, 41, 1, 50, 1, 
	51, 2, 1, 0, 2, 1, 2, 2, 
	1, 4, 2, 1, 6, 2, 3, 0, 
	2, 3, 2, 2, 3, 4, 2, 3, 
	6, 2, 5, 0, 2, 5, 2, 2, 
	5, 4, 2, 5, 6, 2, 7, 0, 
	2, 7, 2, 2, 7, 4, 2, 7, 
	6, 2, 9, 20, 2, 11, 20, 2, 
	13, 19, 2, 13, 20, 2, 15, 20, 
	2, 17, 20, 2, 19, 20, 2, 23, 
	20, 2, 27, 20, 2, 31, 20, 2, 
	35, 20, 2, 42, 22, 2, 42, 26, 
	2, 42, 30, 2, 42, 34, 2, 43, 
	51, 2, 44, 24, 2, 44, 28, 2, 
	44, 32, 2, 44, 36, 2, 45, 51, 
	2, 46, 38, 2, 46, 40, 2, 47, 
	51, 2, 48, 14, 2, 48, 16, 2, 
	48, 18, 2, 49, 51, 2, 50, 51, 
	3, 9, 49, 51, 3, 11, 49, 51, 
	3, 13, 19, 20, 3, 13, 49, 51, 
	3, 15, 49, 51, 3, 17, 49, 51, 
	3, 19, 49, 51, 3, 21, 43, 51, 
	3, 21, 49, 51, 3, 23, 43, 51, 
	3, 25, 45, 51, 3, 27, 43, 51, 
	3, 29, 45, 51, 3, 31, 43, 51, 
	3, 33, 45, 51, 3, 35, 43, 51, 
	3, 37, 45, 51, 3, 39, 47, 51, 
	3, 41, 47, 51, 3, 43, 42, 22, 
	3, 43, 42, 26, 3, 43, 42, 30, 
	3, 43, 42, 34, 3, 43, 44, 24, 
	3, 43, 44, 28, 3, 43, 44, 32, 
	3, 43, 44, 36, 3, 43, 46, 38, 
	3, 43, 46, 40, 3, 43, 48, 14, 
	3, 43, 48, 16, 3, 43, 48, 18, 
	3, 45, 42, 22, 3, 45, 42, 26, 
	3, 45, 42, 30, 3, 45, 42, 34, 
	3, 45, 44, 24, 3, 45, 44, 28, 
	3, 45, 44, 32, 3, 45, 44, 36, 
	3, 45, 46, 38, 3, 45, 46, 40, 
	3, 45, 48, 14, 3, 45, 48, 16, 
	3, 45, 48, 18, 3, 47, 42, 22, 
	3, 47, 42, 26, 3, 47, 42, 30, 
	3, 47, 42, 34, 3, 47, 44, 24, 
	3, 47, 44, 28, 3, 47, 44, 32, 
	3, 47, 44, 36, 3, 47, 46, 38, 
	3, 47, 46, 40, 3, 47, 48, 14, 
	3, 47, 48, 16, 3, 47, 48, 18, 
	3, 48, 10, 8, 3, 48, 12, 18, 
	3, 49, 42, 22, 3, 49, 42, 26, 
	3, 49, 42, 30, 3, 49, 42, 34, 
	3, 49, 44, 24, 3, 49, 44, 28, 
	3, 49, 44, 32, 3, 49, 44, 36, 
	3, 49, 46, 38, 3, 49, 46, 40, 
	3, 50, 42, 22, 3, 50, 42, 26, 
	3, 50, 42, 30, 3, 50, 42, 34, 
	3, 50, 44, 24, 3, 50, 44, 28, 
	3, 50, 44, 32, 3, 50, 44, 36, 
	3, 50, 46, 38, 3, 50, 46, 40, 
	3, 50, 48, 14, 3, 50, 48, 16, 
	3, 50, 48, 18, 4, 9, 49, 42, 
	22, 4, 9, 49, 42, 26, 4, 9, 
	49, 42, 30, 4, 9, 49, 42, 34, 
	4, 9, 49, 44, 24, 4, 9, 49, 
	44, 28, 4, 9, 49, 44, 32, 4, 
	9, 49, 44, 36, 4, 9, 49, 46, 
	38, 4, 9, 49, 46, 40, 4, 11, 
	49, 42, 22, 4, 11, 49, 42, 26, 
	4, 11, 49, 42, 30, 4, 11, 49, 
	42, 34, 4, 11, 49, 44, 24, 4, 
	11, 49, 44, 28, 4, 11, 49, 44, 
	32, 4, 11, 49, 44, 36, 4, 11, 
	49, 46, 38, 4, 11, 49, 46, 40, 
	4, 13, 19, 49, 51, 4, 13, 49, 
	42, 22, 4, 13, 49, 42, 26, 4, 
	13, 49, 42, 30, 4, 13, 49, 42, 
	34, 4, 13, 49, 44, 24, 4, 13, 
	49, 44, 28, 4, 13, 49, 44, 32, 
	4, 13, 49, 44, 36, 4, 13, 49, 
	46, 38, 4, 13, 49, 46, 40, 4, 
	15, 49, 42, 22, 4, 15, 49, 42, 
	26, 4, 15, 49, 42, 30, 4, 15, 
	49, 42, 34, 4, 15, 49, 44, 24, 
	4, 15, 49, 44, 28, 4, 15, 49, 
	44, 32, 4, 15, 49, 44, 36, 4, 
	15, 49, 46, 38, 4, 15, 49, 46, 
	40, 4, 17, 49, 42, 22, 4, 17, 
	49, 42, 26, 4, 17, 49, 42, 30, 
	4, 17, 49, 42, 34, 4, 17, 49, 
	44, 24, 4, 17, 49, 44, 28, 4, 
	17, 49, 44, 32, 4, 17, 49, 44, 
	36, 4, 17, 49, 46, 38, 4, 17, 
	49, 46, 40, 4, 19, 49, 42, 22, 
	4, 19, 49, 42, 26, 4, 19, 49, 
	42, 30, 4, 19, 49, 42, 34, 4, 
	19, 49, 44, 24, 4, 19, 49, 44, 
	28, 4, 19, 49, 44, 32, 4, 19, 
	49, 44, 36, 4, 19, 49, 46, 38, 
	4, 19, 49, 46, 40, 4, 21, 43, 
	42, 22, 4, 21, 43, 42, 26, 4, 
	21, 43, 42, 30, 4, 21, 43, 42, 
	34, 4, 21, 43, 44, 24, 4, 21, 
	43, 44, 28, 4, 21, 43, 44, 32, 
	4, 21, 43, 44, 36, 4, 21, 43, 
	46, 38, 4, 21, 43, 46, 40, 4, 
	21, 43, 48, 14, 4, 21, 43, 48, 
	16, 4, 21, 43, 48, 18, 4, 21, 
	49, 42, 22, 4, 21, 49, 42, 26, 
	4, 21, 49, 42, 30, 4, 21, 49, 
	42, 34, 4, 21, 49, 44, 24, 4, 
	21, 49, 44, 28, 4, 21, 49, 44, 
	32, 4, 21, 49, 44, 36, 4, 21, 
	49, 46, 38, 4, 21, 49, 46, 40, 
	4, 23, 43, 42, 22, 4, 23, 43, 
	42, 26, 4, 23, 43, 42, 30, 4, 
	23, 43, 42, 34, 4, 23, 43, 44, 
	24, 4, 23, 43, 44, 28, 4, 23, 
	43, 44, 32, 4, 23, 43, 44, 36, 
	4, 23, 43, 46, 38, 4, 23, 43, 
	46, 40, 4, 23, 43, 48, 14, 4, 
	23, 43, 48, 16, 4, 23, 43, 48, 
	18, 4, 25, 45, 42, 22, 4, 25, 
	45, 42, 26, 4, 25, 45, 42, 30, 
	4, 25, 45, 42, 34, 4, 25, 45, 
	44, 24, 4, 25, 45, 44, 28, 4, 
	25, 45, 44, 32, 4, 25, 45, 44, 
	36, 4, 25, 45, 46, 38, 4, 25, 
	45, 46, 40, 4, 25, 45, 48, 14, 
	4, 25, 45, 48, 16, 4, 25, 45, 
	48, 18, 4, 27, 43, 42, 22, 4, 
	27, 43, 42, 26, 4, 27, 43, 42, 
	30, 4, 27, 43, 42, 34, 4, 27, 
	43, 44, 24, 4, 27, 43, 44, 28, 
	4, 27, 43, 44, 32, 4, 27, 43, 
	44, 36, 4, 27, 43, 46, 38, 4, 
	27, 43, 46, 40, 4, 27, 43, 48, 
	14, 4, 27, 43, 48, 16, 4, 27, 
	43, 48, 18, 4, 29, 45, 42, 22, 
	4, 29, 45, 42, 26, 4, 29, 45, 
	42, 30, 4, 29, 45, 42, 34, 4, 
	29, 45, 44, 24, 4, 29, 45, 44, 
	28, 4, 29, 45, 44, 32, 4, 29, 
	45, 44, 36, 4, 29, 45, 46, 38, 
	4, 29, 45, 46, 40, 4, 29, 45, 
	48, 14, 4, 29, 45, 48, 16, 4, 
	29, 45, 48, 18, 4, 31, 43, 42, 
	22, 4, 31, 43, 42, 26, 4, 31, 
	43, 42, 30, 4, 31, 43, 42, 34, 
	4, 31, 43, 44, 24, 4, 31, 43, 
	44, 28, 4, 31, 43, 44, 32, 4, 
	31, 43, 44, 36, 4, 31, 43, 46, 
	38, 4, 31, 43, 46, 40, 4, 31, 
	43, 48, 14, 4, 31, 43, 48, 16, 
	4, 31, 43, 48, 18, 4, 33, 45, 
	42, 22, 4, 33, 45, 42, 26, 4, 
	33, 45, 42, 30, 4, 33, 45, 42, 
	34, 4, 33, 45, 44, 24, 4, 33, 
	45, 44, 28, 4, 33, 45, 44, 32, 
	4, 33, 45, 44, 36, 4, 33, 45, 
	46, 38, 4, 33, 45, 46, 40, 4, 
	33, 45, 48, 14, 4, 33, 45, 48, 
	16, 4, 33, 45, 48, 18, 4, 35, 
	43, 42, 22, 4, 35, 43, 42, 26, 
	4, 35, 43, 42, 30, 4, 35, 43, 
	42, 34, 4, 35, 43, 44, 24, 4, 
	35, 43, 44, 28, 4, 35, 43, 44, 
	32, 4, 35, 43, 44, 36, 4, 35, 
	43, 46, 38, 4, 35, 43, 46, 40, 
	4, 35, 43, 48, 14, 4, 35, 43, 
	48, 16, 4, 35, 43, 48, 18, 4, 
	37, 45, 42, 22, 4, 37, 45, 42, 
	26, 4, 37, 45, 42, 30, 4, 37, 
	45, 42, 34, 4, 37, 45, 44, 24, 
	4, 37, 45, 44, 28, 4, 37, 45, 
	44, 32, 4, 37, 45, 44, 36, 4, 
	37, 45, 46, 38, 4, 37, 45, 46, 
	40, 4, 37, 45, 48, 14, 4, 37, 
	45, 48, 16, 4, 37, 45, 48, 18, 
	4, 39, 47, 42, 22, 4, 39, 47, 
	42, 26, 4, 39, 47, 42, 30, 4, 
	39, 47, 42, 34, 4, 39, 47, 44, 
	24, 4, 39, 47, 44, 28, 4, 39, 
	47, 44, 32, 4, 39, 47, 44, 36, 
	4, 39, 47, 46, 38, 4, 39, 47, 
	46, 40, 4, 39, 47, 48, 14, 4, 
	39, 47, 48, 16, 4, 39, 47, 48, 
	18, 4, 41, 47, 42, 22, 4, 41, 
	47, 42, 26, 4, 41, 47, 42, 30, 
	4, 41, 47, 42, 34, 4, 41, 47, 
	44, 24, 4, 41, 47, 44, 28, 4, 
	41, 47, 44, 32, 4, 41, 47, 44, 
	36, 4, 41, 47, 46, 38, 4, 41, 
	47, 46, 40, 4, 41, 47, 48, 14, 
	4, 41, 47, 48, 16, 4, 41, 47, 
	48, 18, 4, 43, 48, 10, 8, 4, 
	43, 48, 12, 18, 4, 45, 48, 10, 
	8, 4, 45, 48, 12, 18, 4, 47, 
	48, 10, 8, 4, 47, 48, 12, 18, 
	4, 50, 48, 10, 8, 4, 50, 48, 
	12, 18, 5, 13, 19, 49, 42, 22, 
	5, 13, 19, 49, 42, 26, 5, 13, 
	19, 49, 42, 30, 5, 13, 19, 49, 
	42, 34, 5, 13, 19, 49, 44, 24, 
	5, 13, 19, 49, 44, 28, 5, 13, 
	19, 49, 44, 32, 5, 13, 19, 49, 
	44, 36, 5, 13, 19, 49, 46, 38, 
	5, 13, 19, 49, 46, 40, 5, 21, 
	43, 48, 10, 8, 5, 23, 43, 48, 
	10, 8, 5, 23, 43, 48, 12, 18, 
	5, 25, 45, 48, 10, 8, 5, 25, 
	45, 48, 12, 18, 5, 27, 43, 48, 
	10, 8, 5, 27, 43, 48, 12, 18, 
	5, 29, 45, 48, 10, 8, 5, 29, 
	45, 48, 12, 18, 5, 31, 43, 48, 
	10, 8, 5, 31, 43, 48, 12, 18, 
	5, 33, 45, 48, 10, 8, 5, 33, 
	45, 48, 12, 18, 5, 35, 43, 48, 
	10, 8, 5, 35, 43, 48, 12, 18, 
	5, 37, 45, 48, 10, 8, 5, 37, 
	45, 48, 12, 18, 5, 39, 47, 48, 
	10, 8, 5, 39, 47, 48, 12, 18, 
	5, 41, 47, 48, 10, 8, 5, 41, 
	47, 48, 12, 18, 5, 48, 10, 8, 
	12, 18, 6, 43, 48, 10, 8, 12, 
	18, 6, 45, 48, 10, 8, 12, 18, 
	6, 47, 48, 10, 8, 12, 18, 6, 
	50, 48, 10, 8, 12, 18, 7, 23, 
	43, 48, 10, 8, 12, 18, 7, 25, 
	45, 48, 10, 8, 12, 18, 7, 27, 
	43, 48, 10, 8, 12, 18, 7, 29, 
	45, 48, 10, 8, 12, 18, 7, 31, 
	43, 48, 10, 8, 12, 18, 7, 33, 
	45, 48, 10, 8, 12, 18, 7, 35, 
	43, 48, 10, 8, 12, 18, 7, 37, 
	45, 48, 10, 8, 12, 18, 7, 39, 
	47, 48, 10, 8, 12, 18, 7, 41, 
	47, 48, 10, 8, 12, 18
};

static const short _JDR_key_offsets[] = {
	0, 0, 14, 28, 31, 33, 39, 45, 
	51, 52, 53, 64, 75, 76, 77, 79, 
	90, 92, 94, 105, 107, 109, 111, 122, 
	128, 134, 136, 142, 146, 155, 161, 167, 
	173, 179, 181, 195, 197, 199, 213, 215, 
	217, 219, 233, 263, 293, 308, 323, 354, 
	385, 415, 445, 475, 505, 523, 542, 572, 
	603, 624, 655, 686, 716, 744, 765, 779, 
	810, 839, 869, 892, 922, 937, 968, 998, 
	1027, 1047, 1076, 1093, 1114, 1142
};

static const unsigned char _JDR_trans_keys[] = {
	10u, 13u, 34u, 92u, 128u, 191u, 192u, 223u, 
	224u, 239u, 240u, 247u, 248u, 255u, 10u, 13u, 
	34u, 92u, 128u, 191u, 192u, 223u, 224u, 239u, 
	240u, 247u, 248u, 255u, 48u, 49u, 57u, 48u, 
	57u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 96u, 96u, 96u, 128u, 191u, 
	192u, 223u, 224u, 239u, 240u, 247u, 248u, 255u, 
	96u, 128u, 191u, 192u, 223u, 224u, 239u, 240u, 
	247u, 248u, 255u, 96u, 96u, 128u, 191u, 96u, 
	128u, 191u, 192u, 223u, 224u, 239u, 240u, 247u, 
	248u, 255u, 128u, 191u, 128u, 191u, 96u, 128u, 
	191u, 192u, 223u, 224u, 239u, 240u, 247u, 248u, 
	255u, 128u, 191u, 128u, 191u, 128u, 191u, 96u, 
	128u, 191u, 192u, 223u, 224u, 239u, 240u, 247u, 
	248u, 255u, 48u, 57u, 65u, 70u, 97u, 102u, 
	48u, 57u, 65u, 70u, 97u, 102u, 48u, 57u, 
	48u, 57u, 65u, 70u, 97u, 102u, 43u, 45u, 
	48u, 57u, 34u, 47u, 92u, 98u, 102u, 110u, 
	114u, 116u, 117u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 128u, 191u, 10u, 13u, 34u, 
	92u, 128u, 191u, 192u, 223u, 224u, 239u, 240u, 
	247u, 248u, 255u, 128u, 191u, 128u, 191u, 10u, 
	13u, 34u, 92u, 128u, 191u, 192u, 223u, 224u, 
	239u, 240u, 247u, 248u, 255u, 128u, 191u, 128u, 
	191u, 128u, 191u, 10u, 13u, 34u, 92u, 128u, 
	191u, 192u, 223u, 224u, 239u, 240u, 247u, 248u, 
	255u, 13u, 32u, 34u, 40u, 41u, 44u, 45u, 
	48u, 58u, 60u, 62u, 91u, 93u, 95u, 96u, 
	123u, 125u, 126u, 9u, 10u, 49u, 57u, 65u, 
	70u, 71u, 90u, 97u, 102u, 103u, 122u, 13u, 
	32u, 34u, 40u, 41u, 44u, 45u, 48u, 58u, 
	60u, 62u, 91u, 93u, 95u, 96u, 123u, 125u, 
	126u, 9u, 10u, 49u, 57u, 65u, 70u, 71u, 
	90u, 97u, 102u, 103u, 122u, 13u, 32u, 40u, 
	41u, 44u, 58u, 60u, 62u, 64u, 91u, 93u, 
	123u, 125u, 9u, 10u, 13u, 32u, 40u, 41u, 
	44u, 58u, 60u, 62u, 64u, 91u, 93u, 123u, 
	125u, 9u, 10u, 13u, 32u, 34u, 40u, 41u, 
	44u, 45u, 48u, 58u, 60u, 62u, 64u, 91u, 
	93u, 95u, 96u, 123u, 125u, 126u, 9u, 10u, 
	49u, 57u, 65u, 70u, 71u, 90u, 97u, 102u, 
	103u, 122u, 13u, 32u, 34u, 40u, 41u, 44u, 
	45u, 48u, 58u, 60u, 62u, 64u, 91u, 93u, 
	95u, 96u, 123u, 125u, 126u, 9u, 10u, 49u, 
	57u, 65u, 70u, 71u, 90u, 97u, 102u, 103u, 
	122u, 13u, 32u, 34u, 40u, 41u, 44u, 45u, 
	48u, 58u, 60u, 62u, 91u, 93u, 95u, 96u, 
	123u, 125u, 126u, 9u, 10u, 49u, 57u, 65u, 
	70u, 71u, 90u, 97u, 102u, 103u, 122u, 13u, 
	32u, 34u, 40u, 41u, 44u, 45u, 48u, 58u, 
	60u, 62u, 91u, 93u, 95u, 96u, 123u, 125u, 
	126u, 9u, 10u, 49u, 57u, 65u, 70u, 71u, 
	90u, 97u, 102u, 103u, 122u, 13u, 32u, 34u, 
	40u, 41u, 44u, 45u, 48u, 58u, 60u, 62u, 
	91u, 93u, 95u, 96u, 123u, 125u, 126u, 9u, 
	10u, 49u, 57u, 65u, 70u, 71u, 90u, 97u, 
	102u, 103u, 122u, 13u, 32u, 34u, 40u, 41u, 
	44u, 45u, 48u, 58u, 60u, 62u, 91u, 93u, 
	95u, 96u, 123u, 125u, 126u, 9u, 10u, 49u, 
	57u, 65u, 70u, 71u, 90u, 97u, 102u, 103u, 
	122u, 13u, 32u, 40u, 41u, 44u, 46u, 58u, 
	60u, 62u, 64u, 69u, 91u, 93u, 101u, 123u, 
	125u, 9u, 10u, 13u, 32u, 40u, 41u, 44u, 
	58u, 60u, 62u, 64u, 69u, 91u, 93u, 101u, 
	123u, 125u, 9u, 10u, 48u, 57u, 13u, 32u, 
	34u, 40u, 41u, 44u, 45u, 48u, 58u, 60u, 
	62u, 91u, 93u, 95u, 96u, 123u, 125u, 126u, 
	9u, 10u, 49u, 57u, 65u, 70u, 71u, 90u, 
	97u, 102u, 103u, 122u, 13u, 32u, 40u, 41u, 
	44u, 45u, 46u, 58u, 60u, 62u, 64u, 69u, 
	91u, 93u, 95u, 101u, 123u, 125u, 126u, 9u, 
	10u, 48u, 57u, 65u, 70u, 71u, 90u, 97u, 
	102u, 103u, 122u, 13u, 32u, 40u, 41u, 44u, 
	58u, 60u, 62u, 64u, 91u, 93u, 123u, 125u, 
	9u, 10u, 48u, 57u, 65u, 70u, 97u, 102u, 
	13u, 32u, 34u, 40u, 41u, 44u, 45u, 48u, 
	58u, 60u, 62u, 64u, 91u, 93u, 95u, 96u, 
	123u, 125u, 126u, 9u, 10u, 49u, 57u, 65u, 
	70u, 71u, 90u, 97u, 102u, 103u, 122u, 13u, 
	32u, 40u, 41u, 44u, 45u, 46u, 58u, 60u, 
	62u, 64u, 69u, 91u, 93u, 95u, 101u, 123u, 
	125u, 126u, 9u, 10u, 48u, 57u, 65u, 70u, 
	71u, 90u, 97u, 102u, 103u, 122u, 13u, 32u, 
	34u, 40u, 41u, 44u, 45u, 48u, 58u, 60u, 
	62u, 91u, 93u, 95u, 96u, 123u, 125u, 126u, 
	9u, 10u, 49u, 57u, 65u, 70u, 71u, 90u, 
	97u, 102u, 103u, 122u, 13u, 32u, 40u, 41u, 
	44u, 45u, 58u, 60u, 62u, 64u, 91u, 93u, 
	95u, 123u, 125u, 126u, 9u, 10u, 48u, 57u, 
	65u, 70u, 71u, 90u, 97u, 102u, 103u, 122u, 
	13u, 32u, 40u, 41u, 44u, 45u, 58u, 60u, 
	62u, 91u, 93u, 123u, 125u, 9u, 10u, 48u, 
	57u, 65u, 70u, 97u, 102u, 13u, 32u, 40u, 
	41u, 44u, 58u, 60u, 62u, 91u, 93u, 123u, 
	125u, 9u, 10u, 13u, 32u, 34u, 40u, 41u, 
	44u, 45u, 48u, 58u, 60u, 62u, 64u, 91u, 
	93u, 95u, 96u, 123u, 125u, 126u, 9u, 10u, 
	49u, 57u, 65u, 70u, 71u, 90u, 97u, 102u, 
	103u, 122u, 13u, 32u, 34u, 40u, 41u, 44u, 
	45u, 58u, 60u, 62u, 91u, 93u, 95u, 96u, 
	123u, 125u, 126u, 9u, 10u, 48u, 57u, 65u, 
	70u, 71u, 90u, 97u, 102u, 103u, 122u, 13u, 
	32u, 34u, 40u, 41u, 44u, 45u, 48u, 58u, 
	60u, 62u, 91u, 93u, 95u, 96u, 123u, 125u, 
	126u, 9u, 10u, 49u, 57u, 65u, 70u, 71u, 
	90u, 97u, 102u, 103u, 122u, 13u, 32u, 40u, 
	41u, 44u, 58u, 60u, 62u, 64u, 91u, 93u, 
	95u, 123u, 125u, 126u, 9u, 10u, 48u, 57u, 
	65u, 90u, 97u, 122u, 13u, 32u, 34u, 40u, 
	41u, 44u, 45u, 48u, 58u, 60u, 62u, 91u, 
	93u, 95u, 96u, 123u, 125u, 126u, 9u, 10u, 
	49u, 57u, 65u, 70u, 71u, 90u, 97u, 102u, 
	103u, 122u, 13u, 32u, 40u, 41u, 44u, 58u, 
	60u, 62u, 64u, 91u, 93u, 123u, 125u, 9u, 
	10u, 13u, 32u, 34u, 40u, 41u, 44u, 45u, 
	48u, 58u, 60u, 62u, 64u, 91u, 93u, 95u, 
	96u, 123u, 125u, 126u, 9u, 10u, 49u, 57u, 
	65u, 70u, 71u, 90u, 97u, 102u, 103u, 122u, 
	13u, 32u, 34u, 40u, 41u, 44u, 45u, 48u, 
	58u, 60u, 62u, 91u, 93u, 95u, 96u, 123u, 
	125u, 126u, 9u, 10u, 49u, 57u, 65u, 70u, 
	71u, 90u, 97u, 102u, 103u, 122u, 13u, 32u, 
	34u, 40u, 41u, 44u, 45u, 58u, 60u, 62u, 
	91u, 93u, 95u, 96u, 123u, 125u, 126u, 9u, 
	10u, 48u, 57u, 65u, 70u, 71u, 90u, 97u, 
	102u, 103u, 122u, 13u, 32u, 40u, 41u, 44u, 
	58u, 60u, 62u, 91u, 93u, 123u, 125u, 9u, 
	10u, 48u, 57u, 65u, 70u, 97u, 102u, 13u, 
	32u, 40u, 41u, 43u, 44u, 45u, 58u, 60u, 
	62u, 64u, 91u, 93u, 95u, 123u, 125u, 126u, 
	9u, 10u, 48u, 57u, 65u, 70u, 71u, 90u, 
	97u, 102u, 103u, 122u, 13u, 32u, 40u, 41u, 
	44u, 58u, 60u, 62u, 64u, 91u, 93u, 123u, 
	125u, 9u, 10u, 48u, 57u, 13u, 32u, 40u, 
	41u, 44u, 58u, 60u, 62u, 64u, 91u, 93u, 
	123u, 125u, 9u, 10u, 48u, 57u, 65u, 70u, 
	97u, 102u, 13u, 32u, 40u, 41u, 44u, 45u, 
	58u, 60u, 62u, 64u, 91u, 93u, 95u, 123u, 
	125u, 126u, 9u, 10u, 48u, 57u, 65u, 70u, 
	71u, 90u, 97u, 102u, 103u, 122u, 13u, 32u, 
	40u, 41u, 44u, 46u, 58u, 60u, 62u, 64u, 
	69u, 91u, 93u, 101u, 123u, 125u, 9u, 10u, 
	48u, 57u, 0
};

static const char _JDR_single_lengths[] = {
	0, 4, 4, 1, 0, 0, 0, 0, 
	1, 1, 1, 1, 1, 1, 0, 1, 
	0, 0, 1, 0, 0, 0, 1, 0, 
	0, 0, 0, 2, 9, 0, 0, 0, 
	0, 0, 4, 0, 0, 4, 0, 0, 
	0, 4, 18, 18, 13, 13, 19, 19, 
	18, 18, 18, 18, 16, 15, 18, 19, 
	13, 19, 19, 18, 16, 13, 12, 19, 
	17, 18, 15, 18, 13, 19, 18, 17, 
	12, 17, 13, 13, 16, 16
};

static const char _JDR_range_lengths[] = {
	0, 5, 5, 1, 1, 3, 3, 3, 
	0, 0, 5, 5, 0, 0, 1, 5, 
	1, 1, 5, 1, 1, 1, 5, 3, 
	3, 1, 3, 1, 0, 3, 3, 3, 
	3, 1, 5, 1, 1, 5, 1, 1, 
	1, 5, 6, 6, 1, 1, 6, 6, 
	6, 6, 6, 6, 1, 2, 6, 6, 
	4, 6, 6, 6, 6, 4, 1, 6, 
	6, 6, 4, 6, 1, 6, 6, 6, 
	4, 6, 2, 4, 6, 2
};

static const short _JDR_index_offsets[] = {
	0, 0, 10, 20, 23, 25, 29, 33, 
	37, 39, 41, 48, 55, 57, 59, 61, 
	68, 70, 72, 79, 81, 83, 85, 92, 
	96, 100, 102, 106, 110, 120, 124, 128, 
	132, 136, 138, 148, 150, 152, 162, 164, 
	166, 168, 178, 203, 228, 243, 258, 284, 
	310, 335, 360, 385, 410, 428, 446, 471, 
	497, 515, 541, 567, 592, 615, 633, 647, 
	673, 697, 722, 742, 767, 782, 808, 833, 
	857, 874, 898, 914, 932, 955
};

static const char _JDR_trans_targs[] = {
	0, 0, 44, 28, 0, 33, 35, 38, 
	0, 2, 0, 0, 44, 28, 0, 33, 
	35, 38, 0, 2, 52, 77, 0, 53, 
	0, 56, 56, 56, 0, 61, 61, 61, 
	0, 64, 64, 64, 0, 9, 0, 10, 
	0, 12, 0, 14, 16, 19, 0, 11, 
	12, 0, 14, 16, 19, 0, 11, 13, 
	0, 68, 0, 15, 0, 12, 0, 14, 
	16, 19, 0, 11, 17, 0, 18, 0, 
	12, 0, 14, 16, 19, 0, 11, 20, 
	0, 21, 0, 22, 0, 12, 0, 14, 
	16, 19, 0, 11, 71, 71, 71, 0, 
	72, 72, 72, 0, 74, 0, 75, 56, 
	56, 0, 25, 25, 74, 0, 1, 1, 
	1, 1, 1, 1, 1, 1, 29, 0, 
	30, 30, 30, 0, 31, 31, 31, 0, 
	32, 32, 32, 0, 1, 1, 1, 0, 
	34, 0, 0, 0, 44, 28, 0, 33, 
	35, 38, 0, 2, 36, 0, 37, 0, 
	0, 0, 44, 28, 0, 33, 35, 38, 
	0, 2, 39, 0, 40, 0, 41, 0, 
	0, 0, 44, 28, 0, 33, 35, 38, 
	0, 2, 43, 43, 1, 46, 48, 50, 
	3, 55, 54, 57, 59, 63, 67, 66, 
	8, 69, 70, 66, 43, 58, 60, 66, 
	60, 66, 0, 43, 43, 1, 46, 48, 
	50, 3, 55, 54, 57, 59, 63, 67, 
	66, 8, 69, 70, 66, 43, 58, 60, 
	66, 60, 66, 0, 45, 45, 46, 48, 
	50, 54, 57, 59, 6, 63, 67, 69, 
	70, 45, 0, 45, 45, 46, 48, 50, 
	54, 57, 59, 6, 63, 67, 69, 70, 
	45, 0, 47, 47, 1, 46, 48, 50, 
	3, 55, 54, 57, 59, 7, 63, 67, 
	66, 8, 69, 70, 66, 47, 58, 60, 
	66, 60, 66, 0, 47, 47, 1, 46, 
	48, 50, 3, 55, 54, 57, 59, 7, 
	63, 67, 66, 8, 69, 70, 66, 47, 
	58, 60, 66, 60, 66, 0, 49, 49, 
	1, 46, 48, 50, 3, 55, 54, 57, 
	59, 63, 67, 66, 8, 69, 70, 66, 
	49, 58, 60, 66, 60, 66, 0, 49, 
	49, 1, 46, 48, 50, 3, 55, 54, 
	57, 59, 63, 67, 66, 8, 69, 70, 
	66, 49, 58, 60, 66, 60, 66, 0, 
	51, 51, 1, 46, 48, 50, 3, 55, 
	54, 57, 59, 63, 67, 66, 8, 69, 
	70, 66, 51, 58, 60, 66, 60, 66, 
	0, 51, 51, 1, 46, 48, 50, 3, 
	55, 54, 57, 59, 63, 67, 66, 8, 
	69, 70, 66, 51, 58, 60, 66, 60, 
	66, 0, 45, 45, 46, 48, 50, 4, 
	54, 57, 59, 6, 27, 63, 67, 27, 
	69, 70, 45, 0, 45, 45, 46, 48, 
	50, 54, 57, 59, 6, 27, 63, 67, 
	27, 69, 70, 45, 53, 0, 51, 51, 
	1, 46, 48, 50, 3, 55, 54, 57, 
	59, 63, 67, 66, 8, 69, 70, 66, 
	51, 58, 60, 66, 60, 66, 0, 45, 
	45, 46, 48, 50, 5, 4, 54, 57, 
	59, 6, 73, 63, 67, 66, 73, 69, 
	70, 66, 45, 60, 60, 66, 60, 66, 
	0, 45, 45, 46, 48, 50, 54, 57, 
	59, 6, 63, 67, 69, 70, 45, 56, 
	56, 56, 0, 47, 47, 1, 46, 48, 
	50, 3, 55, 54, 57, 59, 7, 63, 
	67, 66, 8, 69, 70, 66, 47, 58, 
	60, 66, 60, 66, 0, 45, 45, 46, 
	48, 50, 5, 4, 54, 57, 59, 6, 
	73, 63, 67, 66, 73, 69, 70, 66, 
	45, 58, 60, 66, 60, 66, 0, 49, 
	49, 1, 46, 48, 50, 3, 55, 54, 
	57, 59, 63, 67, 66, 8, 69, 70, 
	66, 49, 58, 60, 66, 60, 66, 0, 
	45, 45, 46, 48, 50, 5, 54, 57, 
	59, 6, 63, 67, 66, 69, 70, 66, 
	45, 60, 60, 66, 60, 66, 0, 62, 
	62, 46, 48, 50, 24, 54, 57, 59, 
	63, 67, 69, 70, 62, 61, 61, 61, 
	0, 62, 62, 46, 48, 50, 54, 57, 
	59, 63, 67, 69, 70, 62, 0, 47, 
	47, 1, 46, 48, 50, 3, 55, 54, 
	57, 59, 7, 63, 67, 66, 8, 69, 
	70, 66, 47, 58, 60, 66, 60, 66, 
	0, 65, 65, 1, 46, 48, 50, 23, 
	54, 57, 59, 63, 67, 66, 8, 69, 
	70, 66, 65, 64, 64, 66, 64, 66, 
	0, 65, 65, 1, 46, 48, 50, 3, 
	55, 54, 57, 59, 63, 67, 66, 8, 
	69, 70, 66, 65, 58, 60, 66, 60, 
	66, 0, 45, 45, 46, 48, 50, 54, 
	57, 59, 6, 63, 67, 66, 69, 70, 
	66, 45, 66, 66, 66, 0, 49, 49, 
	1, 46, 48, 50, 3, 55, 54, 57, 
	59, 63, 67, 66, 8, 69, 70, 66, 
	49, 58, 60, 66, 60, 66, 0, 45, 
	45, 46, 48, 50, 54, 57, 59, 6, 
	63, 67, 69, 70, 45, 0, 47, 47, 
	1, 46, 48, 50, 3, 55, 54, 57, 
	59, 7, 63, 67, 66, 8, 69, 70, 
	66, 47, 58, 60, 66, 60, 66, 0, 
	49, 49, 1, 46, 48, 50, 3, 55, 
	54, 57, 59, 63, 67, 66, 8, 69, 
	70, 66, 49, 58, 60, 66, 60, 66, 
	0, 65, 65, 1, 46, 48, 50, 3, 
	54, 57, 59, 63, 67, 66, 8, 69, 
	70, 66, 65, 71, 71, 66, 71, 66, 
	0, 62, 62, 46, 48, 50, 54, 57, 
	59, 63, 67, 69, 70, 62, 72, 72, 
	72, 0, 45, 45, 46, 48, 25, 50, 
	26, 54, 57, 59, 6, 63, 67, 66, 
	69, 70, 66, 45, 76, 60, 66, 60, 
	66, 0, 45, 45, 46, 48, 50, 54, 
	57, 59, 6, 63, 67, 69, 70, 45, 
	74, 0, 45, 45, 46, 48, 50, 54, 
	57, 59, 6, 63, 67, 69, 70, 45, 
	75, 56, 56, 0, 45, 45, 46, 48, 
	50, 5, 54, 57, 59, 6, 63, 67, 
	66, 69, 70, 66, 45, 76, 60, 66, 
	60, 66, 0, 45, 45, 46, 48, 50, 
	4, 54, 57, 59, 6, 27, 63, 67, 
	27, 69, 70, 45, 77, 0, 0
};

static const short _JDR_trans_actions[] = {
	0, 0, 0, 0, 0, 5, 9, 13, 
	0, 1, 0, 0, 3, 3, 0, 60, 
	63, 66, 0, 57, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 5, 9, 13, 0, 1, 
	3, 0, 60, 63, 66, 0, 57, 0, 
	0, 0, 0, 0, 0, 7, 0, 72, 
	75, 78, 0, 69, 0, 0, 0, 0, 
	11, 0, 84, 87, 90, 0, 81, 0, 
	0, 0, 0, 0, 0, 15, 0, 96, 
	99, 102, 0, 93, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 7, 7, 0, 72, 
	75, 78, 0, 69, 0, 0, 0, 0, 
	0, 0, 11, 11, 0, 84, 87, 90, 
	0, 81, 0, 0, 0, 0, 0, 0, 
	0, 0, 15, 15, 0, 96, 99, 102, 
	0, 93, 53, 53, 512, 484, 500, 504, 
	1624, 1847, 508, 472, 488, 476, 492, 520, 
	516, 480, 496, 520, 53, 1847, 1629, 520, 
	1629, 520, 0, 0, 0, 177, 147, 162, 
	168, 424, 1820, 171, 138, 153, 141, 156, 
	183, 180, 144, 159, 183, 0, 1820, 428, 
	183, 428, 183, 0, 23, 23, 694, 714, 
	719, 724, 679, 699, 117, 684, 704, 689, 
	709, 23, 0, 0, 0, 444, 460, 464, 
	468, 432, 448, 29, 436, 452, 440, 456, 
	0, 0, 45, 45, 1384, 1349, 1369, 1374, 
	1772, 1902, 1379, 1334, 1354, 135, 1339, 1359, 
	1394, 1389, 1344, 1364, 1394, 45, 1902, 1778, 
	1394, 1778, 1394, 0, 0, 0, 308, 280, 
	296, 300, 1594, 1826, 304, 268, 284, 29, 
	272, 288, 316, 312, 276, 292, 316, 0, 
	1826, 1599, 316, 1599, 316, 0, 47, 47, 
	1449, 1414, 1434, 1439, 1784, 1910, 1444, 1399, 
	1419, 1404, 1424, 1459, 1454, 1409, 1429, 1459, 
	47, 1910, 1790, 1459, 1790, 1459, 0, 0, 
	0, 360, 332, 348, 352, 1604, 1833, 356, 
	320, 336, 324, 340, 368, 364, 328, 344, 
	368, 0, 1833, 1609, 368, 1609, 368, 0, 
	49, 49, 1514, 1479, 1499, 1504, 1796, 1918, 
	1509, 1464, 1484, 1469, 1489, 1524, 1519, 1474, 
	1494, 1524, 49, 1918, 1802, 1524, 1802, 1524, 
	0, 0, 0, 412, 384, 400, 404, 1614, 
	1840, 408, 372, 388, 376, 392, 420, 416, 
	380, 396, 420, 0, 1840, 1619, 420, 1619, 
	420, 0, 17, 17, 539, 559, 564, 0, 
	569, 524, 544, 105, 0, 529, 549, 0, 
	534, 554, 17, 0, 19, 19, 589, 609, 
	614, 619, 574, 594, 108, 0, 579, 599, 
	0, 584, 604, 19, 0, 0, 51, 51, 
	1579, 1544, 1564, 1569, 1808, 1926, 1574, 1529, 
	1549, 1534, 1554, 1589, 1584, 1539, 1559, 1589, 
	51, 1926, 1814, 1589, 1814, 1589, 0, 17, 
	17, 539, 559, 564, 0, 0, 569, 524, 
	544, 105, 0, 529, 549, 0, 0, 534, 
	554, 0, 17, 0, 0, 0, 0, 0, 
	0, 21, 21, 644, 664, 669, 674, 629, 
	649, 114, 634, 654, 639, 659, 21, 0, 
	0, 0, 0, 33, 33, 994, 959, 979, 
	984, 1700, 1854, 989, 944, 964, 126, 949, 
	969, 1004, 999, 954, 974, 1004, 33, 1854, 
	1706, 1004, 1706, 1004, 0, 17, 17, 539, 
	559, 564, 0, 0, 569, 524, 544, 105, 
	0, 529, 549, 0, 0, 534, 554, 0, 
	17, 0, 0, 0, 0, 0, 0, 35, 
	35, 1059, 1024, 1044, 1049, 1712, 1862, 1054, 
	1009, 1029, 1014, 1034, 1069, 1064, 1019, 1039, 
	1069, 35, 1862, 1718, 1069, 1718, 1069, 0, 
	111, 111, 1652, 1676, 1682, 0, 1688, 1634, 
	1658, 200, 1640, 1664, 0, 1646, 1670, 0, 
	111, 0, 0, 0, 0, 0, 0, 31, 
	31, 909, 929, 934, 0, 939, 894, 914, 
	899, 919, 904, 924, 31, 0, 0, 0, 
	0, 0, 0, 444, 460, 464, 468, 432, 
	448, 436, 452, 440, 456, 0, 0, 37, 
	37, 1124, 1089, 1109, 1114, 1724, 1870, 1119, 
	1074, 1094, 129, 1079, 1099, 1134, 1129, 1084, 
	1104, 1134, 37, 1870, 1730, 1134, 1730, 1134, 
	0, 31, 31, 879, 844, 864, 869, 0, 
	874, 829, 849, 834, 854, 889, 884, 839, 
	859, 889, 31, 0, 0, 889, 0, 889, 
	0, 0, 0, 308, 280, 296, 300, 1594, 
	1826, 304, 268, 284, 272, 288, 316, 312, 
	276, 292, 316, 0, 1826, 1599, 316, 1599, 
	316, 0, 27, 27, 794, 814, 819, 824, 
	779, 799, 123, 784, 804, 0, 789, 809, 
	0, 27, 0, 0, 0, 0, 39, 39, 
	1189, 1154, 1174, 1179, 1736, 1878, 1184, 1139, 
	1159, 1144, 1164, 1199, 1194, 1149, 1169, 1199, 
	39, 1878, 1742, 1199, 1742, 1199, 0, 25, 
	25, 744, 764, 769, 774, 729, 749, 120, 
	734, 754, 739, 759, 25, 0, 41, 41, 
	1254, 1219, 1239, 1244, 1748, 1886, 1249, 1204, 
	1224, 132, 1209, 1229, 1264, 1259, 1214, 1234, 
	1264, 41, 1886, 1754, 1264, 1754, 1264, 0, 
	43, 43, 1319, 1284, 1304, 1309, 1760, 1894, 
	1314, 1269, 1289, 1274, 1294, 1329, 1324, 1279, 
	1299, 1329, 43, 1894, 1766, 1329, 1766, 1329, 
	0, 31, 31, 879, 844, 864, 869, 1694, 
	874, 829, 849, 834, 854, 889, 884, 839, 
	859, 889, 31, 0, 0, 889, 0, 889, 
	0, 31, 31, 909, 929, 934, 939, 894, 
	914, 899, 919, 904, 924, 31, 0, 0, 
	0, 0, 111, 111, 1652, 1676, 0, 1682, 
	0, 1688, 1634, 1658, 200, 1640, 1664, 0, 
	1646, 1670, 0, 111, 0, 0, 0, 0, 
	0, 0, 19, 19, 589, 609, 614, 619, 
	574, 594, 108, 579, 599, 584, 604, 19, 
	0, 0, 19, 19, 589, 609, 614, 619, 
	574, 594, 108, 579, 599, 584, 604, 19, 
	0, 0, 0, 0, 19, 19, 589, 609, 
	614, 0, 619, 574, 594, 108, 579, 599, 
	0, 584, 604, 0, 19, 0, 0, 0, 
	0, 0, 0, 17, 17, 539, 559, 564, 
	0, 569, 524, 544, 105, 0, 529, 549, 
	0, 534, 554, 17, 0, 0, 0
};

static const short _JDR_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 189, 55, 208, 186, 252, 150, 
	256, 165, 260, 174, 192, 196, 264, 192, 
	204, 228, 192, 232, 624, 224, 186, 236, 
	220, 150, 216, 240, 212, 244, 248, 220, 
	224, 624, 196, 196, 196, 192
};

static const int JDR_start = 42;
static const int JDR_first_final = 42;
static const int JDR_error = 0;

static const int JDR_en_main = 42;


#line 278 "JDR.c.rl"

// the public API function
pro(JDRlexer, JDRstate* state) {

    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = pe;
    u64 mark0[64] = {};

    $u8c tok = {p, p};

    
#line 799 "JDR.rl.c"
	{
	cs = JDR_start;
	}

#line 294 "JDR.c.rl"
    
#line 802 "JDR.rl.c"
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
	_keys = _JDR_trans_keys + _JDR_key_offsets[cs];
	_trans = _JDR_index_offsets[cs];

	_klen = _JDR_single_lengths[cs];
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

	_klen = _JDR_range_lengths[cs];
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
	cs = _JDR_trans_targs[_trans];

	if ( _JDR_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _JDR_actions + _JDR_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 73 "JDR.c.rl"
	{ mark0[JDRUtf8cp1] = p - text[0]; }
	break;
	case 1:
#line 74 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRUtf8cp1];
    tok[1] = p;
    call(JDRonUtf8cp1, tok, state); 
}
	break;
	case 2:
#line 79 "JDR.c.rl"
	{ mark0[JDRUtf8cp2] = p - text[0]; }
	break;
	case 3:
#line 80 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRUtf8cp2];
    tok[1] = p;
    call(JDRonUtf8cp2, tok, state); 
}
	break;
	case 4:
#line 85 "JDR.c.rl"
	{ mark0[JDRUtf8cp3] = p - text[0]; }
	break;
	case 5:
#line 86 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRUtf8cp3];
    tok[1] = p;
    call(JDRonUtf8cp3, tok, state); 
}
	break;
	case 6:
#line 91 "JDR.c.rl"
	{ mark0[JDRUtf8cp4] = p - text[0]; }
	break;
	case 7:
#line 92 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRUtf8cp4];
    tok[1] = p;
    call(JDRonUtf8cp4, tok, state); 
}
	break;
	case 8:
#line 97 "JDR.c.rl"
	{ mark0[JDRInt] = p - text[0]; }
	break;
	case 9:
#line 98 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRInt];
    tok[1] = p;
    call(JDRonInt, tok, state); 
}
	break;
	case 10:
#line 103 "JDR.c.rl"
	{ mark0[JDRFloat] = p - text[0]; }
	break;
	case 11:
#line 104 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRFloat];
    tok[1] = p;
    call(JDRonFloat, tok, state); 
}
	break;
	case 12:
#line 109 "JDR.c.rl"
	{ mark0[JDRRef] = p - text[0]; }
	break;
	case 13:
#line 110 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRRef];
    tok[1] = p;
    call(JDRonRef, tok, state); 
}
	break;
	case 14:
#line 115 "JDR.c.rl"
	{ mark0[JDRString] = p - text[0]; }
	break;
	case 15:
#line 116 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRString];
    tok[1] = p;
    call(JDRonString, tok, state); 
}
	break;
	case 16:
#line 121 "JDR.c.rl"
	{ mark0[JDRMLString] = p - text[0]; }
	break;
	case 17:
#line 122 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRMLString];
    tok[1] = p;
    call(JDRonMLString, tok, state); 
}
	break;
	case 18:
#line 127 "JDR.c.rl"
	{ mark0[JDRTerm] = p - text[0]; }
	break;
	case 19:
#line 128 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRTerm];
    tok[1] = p;
    call(JDRonTerm, tok, state); 
}
	break;
	case 20:
#line 133 "JDR.c.rl"
	{ mark0[JDRStamp] = p - text[0]; }
	break;
	case 21:
#line 134 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRStamp];
    tok[1] = p;
    call(JDRonStamp, tok, state); 
}
	break;
	case 22:
#line 139 "JDR.c.rl"
	{ mark0[JDROpenP] = p - text[0]; }
	break;
	case 23:
#line 140 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDROpenP];
    tok[1] = p;
    call(JDRonOpenP, tok, state); 
}
	break;
	case 24:
#line 145 "JDR.c.rl"
	{ mark0[JDRCloseP] = p - text[0]; }
	break;
	case 25:
#line 146 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRCloseP];
    tok[1] = p;
    call(JDRonCloseP, tok, state); 
}
	break;
	case 26:
#line 151 "JDR.c.rl"
	{ mark0[JDROpenL] = p - text[0]; }
	break;
	case 27:
#line 152 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDROpenL];
    tok[1] = p;
    call(JDRonOpenL, tok, state); 
}
	break;
	case 28:
#line 157 "JDR.c.rl"
	{ mark0[JDRCloseL] = p - text[0]; }
	break;
	case 29:
#line 158 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRCloseL];
    tok[1] = p;
    call(JDRonCloseL, tok, state); 
}
	break;
	case 30:
#line 163 "JDR.c.rl"
	{ mark0[JDROpenE] = p - text[0]; }
	break;
	case 31:
#line 164 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDROpenE];
    tok[1] = p;
    call(JDRonOpenE, tok, state); 
}
	break;
	case 32:
#line 169 "JDR.c.rl"
	{ mark0[JDRCloseE] = p - text[0]; }
	break;
	case 33:
#line 170 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRCloseE];
    tok[1] = p;
    call(JDRonCloseE, tok, state); 
}
	break;
	case 34:
#line 175 "JDR.c.rl"
	{ mark0[JDROpenX] = p - text[0]; }
	break;
	case 35:
#line 176 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDROpenX];
    tok[1] = p;
    call(JDRonOpenX, tok, state); 
}
	break;
	case 36:
#line 181 "JDR.c.rl"
	{ mark0[JDRCloseX] = p - text[0]; }
	break;
	case 37:
#line 182 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRCloseX];
    tok[1] = p;
    call(JDRonCloseX, tok, state); 
}
	break;
	case 38:
#line 187 "JDR.c.rl"
	{ mark0[JDRComma] = p - text[0]; }
	break;
	case 39:
#line 188 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRComma];
    tok[1] = p;
    call(JDRonComma, tok, state); 
}
	break;
	case 40:
#line 193 "JDR.c.rl"
	{ mark0[JDRColon] = p - text[0]; }
	break;
	case 41:
#line 194 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRColon];
    tok[1] = p;
    call(JDRonColon, tok, state); 
}
	break;
	case 42:
#line 199 "JDR.c.rl"
	{ mark0[JDROpen] = p - text[0]; }
	break;
	case 43:
#line 200 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDROpen];
    tok[1] = p;
    call(JDRonOpen, tok, state); 
}
	break;
	case 44:
#line 205 "JDR.c.rl"
	{ mark0[JDRClose] = p - text[0]; }
	break;
	case 45:
#line 206 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRClose];
    tok[1] = p;
    call(JDRonClose, tok, state); 
}
	break;
	case 46:
#line 211 "JDR.c.rl"
	{ mark0[JDRInter] = p - text[0]; }
	break;
	case 47:
#line 212 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRInter];
    tok[1] = p;
    call(JDRonInter, tok, state); 
}
	break;
	case 48:
#line 217 "JDR.c.rl"
	{ mark0[JDRFIRST] = p - text[0]; }
	break;
	case 49:
#line 218 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRFIRST];
    tok[1] = p;
    call(JDRonFIRST, tok, state); 
}
	break;
	case 50:
#line 223 "JDR.c.rl"
	{ mark0[JDRRoot] = p - text[0]; }
	break;
#line 1127 "JDR.rl.c"
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
	const char *__acts = _JDR_actions + _JDR_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 9:
#line 98 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRInt];
    tok[1] = p;
    call(JDRonInt, tok, state); 
}
	break;
	case 11:
#line 104 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRFloat];
    tok[1] = p;
    call(JDRonFloat, tok, state); 
}
	break;
	case 13:
#line 110 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRRef];
    tok[1] = p;
    call(JDRonRef, tok, state); 
}
	break;
	case 15:
#line 116 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRString];
    tok[1] = p;
    call(JDRonString, tok, state); 
}
	break;
	case 17:
#line 122 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRMLString];
    tok[1] = p;
    call(JDRonMLString, tok, state); 
}
	break;
	case 19:
#line 128 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRTerm];
    tok[1] = p;
    call(JDRonTerm, tok, state); 
}
	break;
	case 21:
#line 134 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRStamp];
    tok[1] = p;
    call(JDRonStamp, tok, state); 
}
	break;
	case 23:
#line 140 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDROpenP];
    tok[1] = p;
    call(JDRonOpenP, tok, state); 
}
	break;
	case 25:
#line 146 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRCloseP];
    tok[1] = p;
    call(JDRonCloseP, tok, state); 
}
	break;
	case 27:
#line 152 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDROpenL];
    tok[1] = p;
    call(JDRonOpenL, tok, state); 
}
	break;
	case 29:
#line 158 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRCloseL];
    tok[1] = p;
    call(JDRonCloseL, tok, state); 
}
	break;
	case 31:
#line 164 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDROpenE];
    tok[1] = p;
    call(JDRonOpenE, tok, state); 
}
	break;
	case 33:
#line 170 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRCloseE];
    tok[1] = p;
    call(JDRonCloseE, tok, state); 
}
	break;
	case 35:
#line 176 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDROpenX];
    tok[1] = p;
    call(JDRonOpenX, tok, state); 
}
	break;
	case 37:
#line 182 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRCloseX];
    tok[1] = p;
    call(JDRonCloseX, tok, state); 
}
	break;
	case 39:
#line 188 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRComma];
    tok[1] = p;
    call(JDRonComma, tok, state); 
}
	break;
	case 41:
#line 194 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRColon];
    tok[1] = p;
    call(JDRonColon, tok, state); 
}
	break;
	case 43:
#line 200 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDROpen];
    tok[1] = p;
    call(JDRonOpen, tok, state); 
}
	break;
	case 45:
#line 206 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRClose];
    tok[1] = p;
    call(JDRonClose, tok, state); 
}
	break;
	case 47:
#line 212 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRInter];
    tok[1] = p;
    call(JDRonInter, tok, state); 
}
	break;
	case 49:
#line 218 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRFIRST];
    tok[1] = p;
    call(JDRonFIRST, tok, state); 
}
	break;
	case 50:
#line 223 "JDR.c.rl"
	{ mark0[JDRRoot] = p - text[0]; }
	break;
	case 51:
#line 224 "JDR.c.rl"
	{
    tok[0] = text[0] + mark0[JDRRoot];
    tok[1] = p;
    call(JDRonRoot, tok, state); 
}
	break;
#line 1299 "JDR.rl.c"
		}
	}
	}

	_out: {}
	}

#line 295 "JDR.c.rl"

    state->text[0] = p;
    if (p!=text[1] || cs < JDR_first_final) {
        return JDRfail;
    }
    done;
}
