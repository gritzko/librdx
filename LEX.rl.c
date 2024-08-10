
#line 1 "LEX.rl"
#include "PRO.h"
#include "LEX.h"

enum {
	LEX = 0,
	LEXspace = LEX+1,
	LEXname = LEX+2,
	LEXop = LEX+4,
	LEXclass = LEX+5,
	LEXstring = LEX+6,
	LEXentity = LEX+7,
	LEXexpr = LEX+8,
	LEXrulename = LEX+9,
	LEXeq = LEX+10,
	LEXline = LEX+11,
	LEXroot = LEX+12,
};

#define LEXmaxnest 1024

fun ok64 popfails(u32* stack, u32* sp, u32 type) {
    while (*sp && stack[*sp]!=type) *sp -= 2;
    return *sp ? OK : LEXfail;
}

#define lexpush(t) { \
    if (sp>=LEXmaxnest) fail(LEXfail); \
    stack[++sp] = p - pb; \
    stack[++sp] = t; \
}
#define lexpop(t)  \
    if (stack[sp]!=t) call(popfails, stack, &sp, t); \
    tok[0] = *(text)+stack[sp-1]; \
    tok[1] = p; \
    sp -= 2;

ok64 _LEXspace ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXname ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXop ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXclass ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXstring ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXentity ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXexpr ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXrulename ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXeq ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXline ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXroot ($cu8c text, $cu8c tok, LEXstate* state);



#line 106 "LEX.rl"



#line 53 "LEX.rl.c"
static const char _LEX_actions[] = {
	0, 1, 0, 1, 1, 2, 1, 0, 
	2, 5, 4, 2, 5, 13, 2, 19, 
	21, 2, 20, 21, 3, 1, 5, 4, 
	3, 1, 5, 13, 3, 1, 19, 21, 
	3, 3, 11, 4, 3, 3, 11, 13, 
	3, 3, 15, 16, 3, 5, 4, 0, 
	3, 5, 10, 2, 3, 5, 10, 6, 
	3, 5, 10, 8, 3, 7, 11, 4, 
	3, 7, 11, 13, 3, 9, 11, 4, 
	3, 9, 11, 13, 3, 17, 12, 4, 
	3, 17, 12, 13, 4, 1, 5, 4, 
	0, 4, 1, 5, 10, 2, 4, 1, 
	5, 10, 6, 4, 1, 5, 10, 8, 
	4, 3, 11, 4, 0, 4, 3, 15, 
	16, 0, 4, 7, 11, 4, 0, 4, 
	9, 11, 4, 0, 4, 17, 12, 4, 
	0, 4, 19, 18, 14, 2, 4, 20, 
	18, 14, 2, 5, 1, 19, 18, 14, 
	2
};

static const short _LEX_key_offsets[] = {
	0, 0, 12, 17, 31, 52, 54, 68, 
	89, 110, 113, 116, 120, 122, 124, 140, 
	163, 166, 182, 205, 219, 226, 242, 256, 
	261, 266, 272, 295, 298, 315, 338, 361, 
	376, 384, 401, 417, 439, 455, 477, 491, 
	498, 514, 516, 521, 526, 532, 555, 561, 
	567, 574, 598, 615, 620, 629, 638, 649, 
	660, 672, 684, 695
};

static const unsigned char _LEX_trans_keys[] = {
	13u, 32u, 61u, 95u, 9u, 10u, 48u, 57u, 
	65u, 90u, 97u, 122u, 13u, 32u, 61u, 9u, 
	10u, 13u, 32u, 45u, 59u, 123u, 124u, 9u, 
	10u, 40u, 43u, 58u, 60u, 62u, 63u, 13u, 
	32u, 34u, 45u, 59u, 91u, 95u, 123u, 124u, 
	9u, 10u, 40u, 43u, 58u, 60u, 62u, 63u, 
	65u, 90u, 97u, 122u, 34u, 92u, 13u, 32u, 
	45u, 59u, 123u, 124u, 9u, 10u, 40u, 43u, 
	58u, 60u, 62u, 63u, 13u, 32u, 34u, 45u, 
	59u, 91u, 95u, 123u, 124u, 9u, 10u, 40u, 
	43u, 58u, 60u, 62u, 63u, 65u, 90u, 97u, 
	122u, 13u, 32u, 45u, 59u, 95u, 123u, 124u, 
	9u, 10u, 40u, 43u, 48u, 57u, 58u, 60u, 
	62u, 63u, 65u, 90u, 97u, 122u, 44u, 48u, 
	57u, 125u, 48u, 57u, 44u, 125u, 48u, 57u, 
	92u, 93u, 92u, 93u, 13u, 32u, 45u, 59u, 
	92u, 93u, 123u, 124u, 9u, 10u, 40u, 43u, 
	58u, 60u, 62u, 63u, 13u, 32u, 34u, 45u, 
	59u, 91u, 92u, 93u, 95u, 123u, 124u, 9u, 
	10u, 40u, 43u, 58u, 60u, 62u, 63u, 65u, 
	90u, 97u, 122u, 34u, 92u, 93u, 13u, 32u, 
	45u, 59u, 92u, 93u, 123u, 124u, 9u, 10u, 
	40u, 43u, 58u, 60u, 62u, 63u, 13u, 32u, 
	34u, 45u, 59u, 91u, 92u, 93u, 95u, 123u, 
	124u, 9u, 10u, 40u, 43u, 58u, 60u, 62u, 
	63u, 65u, 90u, 97u, 122u, 13u, 32u, 61u, 
	92u, 93u, 95u, 9u, 10u, 48u, 57u, 65u, 
	90u, 97u, 122u, 13u, 32u, 61u, 92u, 93u, 
	9u, 10u, 13u, 32u, 45u, 59u, 92u, 93u, 
	123u, 124u, 9u, 10u, 40u, 43u, 58u, 60u, 
	62u, 63u, 13u, 32u, 45u, 59u, 123u, 124u, 
	9u, 10u, 40u, 43u, 58u, 60u, 62u, 63u, 
	44u, 92u, 93u, 48u, 57u, 92u, 93u, 125u, 
	48u, 57u, 44u, 92u, 93u, 125u, 48u, 57u, 
	13u, 32u, 45u, 59u, 92u, 93u, 95u, 123u, 
	124u, 9u, 10u, 40u, 43u, 48u, 57u, 58u, 
	60u, 62u, 63u, 65u, 90u, 97u, 122u, 34u, 
	92u, 93u, 13u, 32u, 34u, 45u, 59u, 92u, 
	93u, 123u, 124u, 9u, 10u, 40u, 43u, 58u, 
	60u, 62u, 63u, 13u, 32u, 34u, 45u, 59u, 
	91u, 92u, 93u, 95u, 123u, 124u, 9u, 10u, 
	40u, 43u, 58u, 60u, 62u, 63u, 65u, 90u, 
	97u, 122u, 13u, 32u, 34u, 45u, 59u, 91u, 
	92u, 93u, 95u, 123u, 124u, 9u, 10u, 40u, 
	43u, 58u, 60u, 62u, 63u, 65u, 90u, 97u, 
	122u, 13u, 32u, 34u, 61u, 92u, 93u, 95u, 
	9u, 10u, 48u, 57u, 65u, 90u, 97u, 122u, 
	13u, 32u, 34u, 61u, 92u, 93u, 9u, 10u, 
	13u, 32u, 34u, 45u, 59u, 92u, 93u, 123u, 
	124u, 9u, 10u, 40u, 43u, 58u, 60u, 62u, 
	63u, 13u, 32u, 34u, 45u, 59u, 92u, 123u, 
	124u, 9u, 10u, 40u, 43u, 58u, 60u, 62u, 
	63u, 13u, 32u, 34u, 45u, 59u, 91u, 92u, 
	95u, 123u, 124u, 9u, 10u, 40u, 43u, 58u, 
	60u, 62u, 63u, 65u, 90u, 97u, 122u, 13u, 
	32u, 34u, 45u, 59u, 92u, 123u, 124u, 9u, 
	10u, 40u, 43u, 58u, 60u, 62u, 63u, 13u, 
	32u, 34u, 45u, 59u, 91u, 92u, 95u, 123u, 
	124u, 9u, 10u, 40u, 43u, 58u, 60u, 62u, 
	63u, 65u, 90u, 97u, 122u, 13u, 32u, 34u, 
	61u, 92u, 95u, 9u, 10u, 48u, 57u, 65u, 
	90u, 97u, 122u, 13u, 32u, 34u, 61u, 92u, 
	9u, 10u, 13u, 32u, 34u, 45u, 59u, 92u, 
	123u, 124u, 9u, 10u, 40u, 43u, 58u, 60u, 
	62u, 63u, 34u, 92u, 34u, 44u, 92u, 48u, 
	57u, 34u, 92u, 125u, 48u, 57u, 34u, 44u, 
	92u, 125u, 48u, 57u, 13u, 32u, 34u, 45u, 
	59u, 92u, 95u, 123u, 124u, 9u, 10u, 40u, 
	43u, 48u, 57u, 58u, 60u, 62u, 63u, 65u, 
	90u, 97u, 122u, 34u, 44u, 92u, 93u, 48u, 
	57u, 34u, 92u, 93u, 125u, 48u, 57u, 34u, 
	44u, 92u, 93u, 125u, 48u, 57u, 13u, 32u, 
	34u, 45u, 59u, 92u, 93u, 95u, 123u, 124u, 
	9u, 10u, 40u, 43u, 48u, 57u, 58u, 60u, 
	62u, 63u, 65u, 90u, 97u, 122u, 13u, 32u, 
	34u, 45u, 59u, 92u, 93u, 123u, 124u, 9u, 
	10u, 40u, 43u, 58u, 60u, 62u, 63u, 95u, 
	65u, 90u, 97u, 122u, 13u, 32u, 95u, 9u, 
	10u, 65u, 90u, 97u, 122u, 13u, 32u, 95u, 
	9u, 10u, 65u, 90u, 97u, 122u, 13u, 32u, 
	92u, 93u, 95u, 9u, 10u, 65u, 90u, 97u, 
	122u, 13u, 32u, 92u, 93u, 95u, 9u, 10u, 
	65u, 90u, 97u, 122u, 13u, 32u, 34u, 92u, 
	93u, 95u, 9u, 10u, 65u, 90u, 97u, 122u, 
	13u, 32u, 34u, 92u, 93u, 95u, 9u, 10u, 
	65u, 90u, 97u, 122u, 13u, 32u, 34u, 92u, 
	95u, 9u, 10u, 65u, 90u, 97u, 122u, 13u, 
	32u, 34u, 92u, 95u, 9u, 10u, 65u, 90u, 
	97u, 122u, 0
};

static const char _LEX_single_lengths[] = {
	0, 4, 3, 6, 9, 2, 6, 9, 
	7, 1, 1, 2, 2, 2, 8, 11, 
	3, 8, 11, 6, 5, 8, 6, 3, 
	3, 4, 9, 3, 9, 11, 11, 7, 
	6, 9, 8, 10, 8, 10, 6, 5, 
	8, 2, 3, 3, 4, 9, 4, 4, 
	5, 10, 9, 1, 3, 3, 5, 5, 
	6, 6, 5, 5
};

static const char _LEX_range_lengths[] = {
	0, 4, 1, 4, 6, 0, 4, 6, 
	7, 1, 1, 1, 0, 0, 4, 6, 
	0, 4, 6, 4, 1, 4, 4, 1, 
	1, 1, 7, 0, 4, 6, 6, 4, 
	1, 4, 4, 6, 4, 6, 4, 1, 
	4, 0, 1, 1, 1, 7, 1, 1, 
	1, 7, 4, 2, 3, 3, 3, 3, 
	3, 3, 3, 3
};

static const short _LEX_index_offsets[] = {
	0, 0, 9, 14, 25, 41, 44, 55, 
	71, 86, 89, 92, 96, 99, 102, 115, 
	133, 137, 150, 168, 179, 186, 199, 210, 
	215, 220, 226, 243, 247, 261, 279, 297, 
	309, 317, 331, 344, 361, 374, 391, 402, 
	409, 422, 425, 430, 435, 441, 458, 464, 
	470, 477, 495, 509, 513, 520, 527, 536, 
	545, 555, 565, 574
};

static const unsigned char _LEX_indicies[] = {
	0, 0, 3, 2, 0, 2, 2, 2, 
	1, 4, 4, 5, 4, 1, 6, 6, 
	7, 8, 9, 7, 6, 7, 7, 7, 
	1, 10, 10, 11, 12, 13, 15, 14, 
	16, 12, 10, 12, 12, 12, 14, 14, 
	1, 18, 19, 17, 20, 20, 21, 22, 
	23, 21, 20, 21, 21, 21, 1, 24, 
	24, 25, 26, 27, 29, 28, 30, 26, 
	24, 26, 26, 26, 28, 28, 1, 31, 
	31, 32, 34, 33, 35, 32, 31, 32, 
	33, 32, 32, 33, 33, 1, 36, 37, 
	1, 38, 36, 1, 36, 38, 37, 1, 
	40, 41, 39, 40, 42, 39, 43, 43, 
	44, 45, 40, 41, 46, 44, 43, 44, 
	44, 44, 39, 47, 47, 48, 49, 50, 
	15, 40, 41, 51, 52, 49, 47, 49, 
	49, 49, 51, 51, 39, 54, 55, 56, 
	53, 57, 57, 58, 59, 40, 41, 60, 
	58, 57, 58, 58, 58, 39, 61, 61, 
	62, 63, 64, 29, 40, 41, 65, 66, 
	63, 61, 63, 63, 63, 65, 65, 39, 
	67, 67, 69, 40, 41, 68, 67, 68, 
	68, 68, 39, 70, 70, 71, 40, 41, 
	70, 39, 72, 72, 73, 74, 40, 41, 
	75, 73, 72, 73, 73, 73, 39, 76, 
	76, 77, 78, 79, 77, 76, 77, 77, 
	77, 1, 80, 40, 41, 81, 39, 40, 
	41, 82, 80, 39, 80, 40, 41, 82, 
	81, 39, 83, 83, 84, 86, 40, 41, 
	85, 87, 84, 83, 84, 85, 84, 84, 
	85, 85, 39, 88, 55, 89, 53, 90, 
	90, 54, 91, 92, 55, 56, 93, 91, 
	90, 91, 91, 91, 53, 94, 94, 95, 
	96, 97, 99, 55, 56, 98, 100, 96, 
	94, 96, 96, 96, 98, 98, 53, 101, 
	101, 102, 103, 104, 106, 55, 56, 105, 
	107, 103, 101, 103, 103, 103, 105, 105, 
	53, 108, 108, 54, 110, 55, 56, 109, 
	108, 109, 109, 109, 53, 111, 111, 54, 
	112, 55, 56, 111, 53, 113, 113, 54, 
	114, 115, 55, 56, 116, 114, 113, 114, 
	114, 114, 53, 117, 117, 18, 118, 119, 
	19, 120, 118, 117, 118, 118, 118, 17, 
	121, 121, 122, 123, 124, 99, 19, 125, 
	126, 123, 121, 123, 123, 123, 125, 125, 
	17, 127, 127, 18, 128, 129, 19, 130, 
	128, 127, 128, 128, 128, 17, 131, 131, 
	132, 133, 134, 106, 19, 135, 136, 133, 
	131, 133, 133, 133, 135, 135, 17, 137, 
	137, 18, 139, 19, 138, 137, 138, 138, 
	138, 17, 140, 140, 18, 141, 19, 140, 
	17, 142, 142, 18, 143, 144, 19, 145, 
	143, 142, 143, 143, 143, 17, 146, 19, 
	17, 18, 147, 19, 148, 17, 18, 19, 
	149, 147, 17, 18, 147, 19, 149, 148, 
	17, 150, 150, 18, 151, 153, 19, 152, 
	154, 151, 150, 151, 152, 151, 151, 152, 
	152, 17, 54, 155, 55, 56, 156, 53, 
	54, 55, 56, 157, 155, 53, 54, 155, 
	55, 56, 157, 156, 53, 158, 158, 54, 
	159, 161, 55, 56, 160, 162, 159, 158, 
	159, 160, 159, 159, 160, 160, 53, 163, 
	163, 54, 164, 165, 55, 56, 166, 164, 
	163, 164, 164, 164, 53, 167, 167, 167, 
	1, 168, 168, 169, 168, 169, 169, 1, 
	170, 170, 171, 170, 171, 171, 1, 172, 
	172, 40, 41, 173, 172, 173, 173, 39, 
	174, 174, 40, 41, 175, 174, 175, 175, 
	39, 176, 176, 54, 55, 56, 177, 176, 
	177, 177, 53, 178, 178, 54, 55, 56, 
	179, 178, 179, 179, 53, 180, 180, 18, 
	19, 181, 180, 181, 181, 17, 182, 182, 
	18, 19, 183, 182, 183, 183, 17, 0
};

static const char _LEX_trans_targs[] = {
	2, 0, 1, 3, 2, 3, 4, 7, 
	52, 9, 4, 5, 7, 52, 8, 12, 
	9, 5, 6, 41, 4, 7, 52, 9, 
	4, 5, 7, 52, 8, 12, 9, 4, 
	7, 8, 52, 9, 10, 11, 7, 12, 
	13, 22, 14, 15, 18, 54, 23, 15, 
	16, 18, 54, 26, 23, 16, 17, 27, 
	34, 15, 18, 54, 23, 15, 16, 18, 
	54, 26, 23, 20, 19, 21, 20, 21, 
	15, 18, 54, 23, 4, 7, 52, 9, 
	24, 25, 18, 15, 18, 26, 54, 23, 
	28, 50, 29, 30, 56, 46, 29, 28, 
	30, 56, 49, 16, 46, 29, 28, 30, 
	56, 49, 16, 46, 32, 31, 33, 32, 
	33, 29, 30, 56, 46, 35, 37, 58, 
	42, 35, 36, 37, 58, 45, 42, 35, 
	37, 58, 42, 35, 36, 37, 58, 45, 
	42, 39, 38, 40, 39, 40, 35, 37, 
	58, 42, 36, 43, 44, 37, 35, 37, 
	45, 58, 42, 47, 48, 30, 29, 30, 
	49, 56, 46, 29, 30, 56, 46, 1, 
	53, 1, 53, 1, 55, 19, 55, 19, 
	57, 31, 57, 31, 59, 38, 59, 38
};

static const unsigned char _LEX_trans_actions[] = {
	109, 0, 0, 40, 5, 3, 124, 76, 
	80, 76, 84, 99, 20, 24, 89, 94, 
	20, 0, 0, 0, 119, 68, 72, 68, 
	44, 56, 8, 11, 48, 52, 8, 104, 
	32, 0, 36, 32, 0, 0, 0, 0, 
	0, 0, 0, 114, 60, 64, 60, 84, 
	99, 20, 24, 89, 20, 0, 0, 0, 
	0, 119, 68, 72, 68, 44, 56, 8, 
	11, 48, 8, 109, 0, 40, 5, 3, 
	124, 76, 80, 76, 114, 60, 64, 60, 
	0, 0, 0, 104, 32, 0, 36, 32, 
	0, 0, 119, 68, 72, 68, 84, 99, 
	20, 24, 89, 94, 20, 44, 56, 8, 
	11, 48, 52, 8, 109, 0, 40, 5, 
	3, 124, 76, 80, 76, 114, 60, 64, 
	60, 84, 99, 20, 24, 89, 20, 119, 
	68, 72, 68, 44, 56, 8, 11, 48, 
	8, 109, 0, 40, 5, 3, 124, 76, 
	80, 76, 0, 0, 0, 0, 104, 32, 
	0, 36, 32, 0, 0, 0, 104, 32, 
	0, 36, 32, 114, 60, 64, 60, 134, 
	1, 129, 5, 139, 1, 129, 5, 139, 
	1, 129, 5, 139, 1, 129, 5, 139
};

static const unsigned char _LEX_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 17, 14, 28, 14, 28, 
	14, 28, 14, 28
};

static const int LEX_start = 51;
static const int LEX_first_final = 51;
static const int LEX_error = 0;

static const int LEX_en_main = 51;


#line 109 "LEX.rl"

pro(LEXlexer, LEXstate* state) {
    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = state->cs;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = state->tbc ? NULL : pe;
    u8c *pb = p;

    u32 stack[LEXmaxnest] = {0, LEX};
    u32 sp = 2;
    $u8c tok = {p, p};

    
#line 372 "LEX.rl.c"
	{
	cs = LEX_start;
	}

#line 126 "LEX.rl"
    
#line 375 "LEX.rl.c"
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
	_keys = _LEX_trans_keys + _LEX_key_offsets[cs];
	_trans = _LEX_index_offsets[cs];

	_klen = _LEX_single_lengths[cs];
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

	_klen = _LEX_range_lengths[cs];
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
	_trans = _LEX_indicies[_trans];
	cs = _LEX_trans_targs[_trans];

	if ( _LEX_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _LEX_actions + _LEX_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 56 "LEX.rl"
	{ lexpush(LEXspace); }
	break;
	case 1:
#line 57 "LEX.rl"
	{ lexpop(LEXspace); call(_LEXspace, text, tok, state); }
	break;
	case 2:
#line 58 "LEX.rl"
	{ lexpush(LEXname); }
	break;
	case 3:
#line 59 "LEX.rl"
	{ lexpop(LEXname); call(_LEXname, text, tok, state); }
	break;
	case 4:
#line 60 "LEX.rl"
	{ lexpush(LEXop); }
	break;
	case 5:
#line 61 "LEX.rl"
	{ lexpop(LEXop); call(_LEXop, text, tok, state); }
	break;
	case 6:
#line 62 "LEX.rl"
	{ lexpush(LEXclass); }
	break;
	case 7:
#line 63 "LEX.rl"
	{ lexpop(LEXclass); call(_LEXclass, text, tok, state); }
	break;
	case 8:
#line 64 "LEX.rl"
	{ lexpush(LEXstring); }
	break;
	case 9:
#line 65 "LEX.rl"
	{ lexpop(LEXstring); call(_LEXstring, text, tok, state); }
	break;
	case 10:
#line 66 "LEX.rl"
	{ lexpush(LEXentity); }
	break;
	case 11:
#line 67 "LEX.rl"
	{ lexpop(LEXentity); call(_LEXentity, text, tok, state); }
	break;
	case 12:
#line 68 "LEX.rl"
	{ lexpush(LEXexpr); }
	break;
	case 13:
#line 69 "LEX.rl"
	{ lexpop(LEXexpr); call(_LEXexpr, text, tok, state); }
	break;
	case 14:
#line 70 "LEX.rl"
	{ lexpush(LEXrulename); }
	break;
	case 15:
#line 71 "LEX.rl"
	{ lexpop(LEXrulename); call(_LEXrulename, text, tok, state); }
	break;
	case 16:
#line 72 "LEX.rl"
	{ lexpush(LEXeq); }
	break;
	case 17:
#line 73 "LEX.rl"
	{ lexpop(LEXeq); call(_LEXeq, text, tok, state); }
	break;
	case 18:
#line 74 "LEX.rl"
	{ lexpush(LEXline); }
	break;
	case 19:
#line 75 "LEX.rl"
	{ lexpop(LEXline); call(_LEXline, text, tok, state); }
	break;
	case 20:
#line 76 "LEX.rl"
	{ lexpush(LEXroot); }
	break;
#line 511 "LEX.rl.c"
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
	const char *__acts = _LEX_actions + _LEX_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
#line 57 "LEX.rl"
	{ lexpop(LEXspace); call(_LEXspace, text, tok, state); }
	break;
	case 19:
#line 75 "LEX.rl"
	{ lexpop(LEXline); call(_LEXline, text, tok, state); }
	break;
	case 20:
#line 76 "LEX.rl"
	{ lexpush(LEXroot); }
	break;
	case 21:
#line 77 "LEX.rl"
	{ lexpop(LEXroot); call(_LEXroot, text, tok, state); }
	break;
#line 538 "LEX.rl.c"
		}
	}
	}

	_out: {}
	}

#line 127 "LEX.rl"

    test(p==text[1], LEXfail);

    if (state->tbc) {
        test(cs != LEX_error, LEXfail);
        state->cs = cs;
    } else {
        test(cs >= LEX_first_final, LEXfail);
    }

    nedo(
        state->text[0] = p;
    );
}


