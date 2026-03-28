
/* #line 1 "CT.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "CT.h"

// user functions (callbacks) for the parser
ok64 CTonComment (u8cs tok, CTstate* state);
ok64 CTonString (u8cs tok, CTstate* state);
ok64 CTonNumber (u8cs tok, CTstate* state);
ok64 CTonPreproc (u8cs tok, CTstate* state);
ok64 CTonWord (u8cs tok, CTstate* state);
ok64 CTonPunct (u8cs tok, CTstate* state);
ok64 CTonSpace (u8cs tok, CTstate* state);


/* #line 151 "CT.c.rl" */



/* #line 17 "CT.rl.c" */
static const char _CT_actions[] = {
	0, 1, 2, 1, 3, 1, 15, 1, 
	16, 1, 17, 1, 18, 1, 19, 1, 
	20, 1, 21, 1, 22, 1, 23, 1, 
	24, 1, 25, 1, 26, 1, 27, 1, 
	28, 1, 29, 1, 30, 1, 31, 1, 
	32, 1, 33, 1, 34, 1, 35, 1, 
	36, 1, 37, 1, 38, 1, 39, 1, 
	40, 1, 41, 1, 42, 1, 43, 1, 
	44, 1, 45, 1, 46, 1, 47, 1, 
	48, 1, 49, 1, 50, 2, 0, 1, 
	2, 3, 4, 2, 3, 5, 2, 3, 
	6, 2, 3, 7, 2, 3, 8, 2, 
	3, 9, 2, 3, 10, 2, 3, 11, 
	2, 3, 12, 2, 3, 13, 2, 3, 
	14
};

static const short _CT_key_offsets[] = {
	0, 0, 2, 19, 25, 31, 37, 43, 
	49, 55, 61, 67, 82, 84, 101, 107, 
	113, 119, 125, 131, 137, 143, 149, 150, 
	152, 156, 158, 159, 161, 163, 165, 169, 
	171, 175, 177, 179, 185, 193, 202, 208, 
	212, 214, 245, 248, 249, 264, 272, 281, 
	290, 299, 308, 317, 328, 338, 347, 356, 
	365, 374, 383, 392, 402, 412, 421, 430, 
	439, 448, 457, 466, 475, 484, 493, 502, 
	511, 520, 529, 538, 547, 556, 565, 574, 
	582, 584, 586, 589, 592, 603, 612, 615, 
	616, 632, 642, 652, 663, 672, 681, 685, 
	687, 689, 691, 703, 707, 709, 711, 713, 
	720, 724, 726, 728, 730, 744, 753, 757, 
	759, 761, 763, 765, 767, 777, 788
};

static const unsigned char _CT_trans_keys[] = {
	34u, 92u, 34u, 39u, 63u, 85u, 92u, 110u, 
	114u, 117u, 120u, 48u, 55u, 97u, 98u, 101u, 
	102u, 116u, 118u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 9u, 32u, 36u, 95u, 100u, 
	101u, 105u, 108u, 112u, 117u, 119u, 65u, 90u, 
	97u, 122u, 39u, 92u, 34u, 39u, 63u, 85u, 
	92u, 110u, 114u, 117u, 120u, 48u, 55u, 97u, 
	98u, 101u, 102u, 116u, 118u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 46u, 48u, 57u, 
	43u, 45u, 48u, 57u, 48u, 57u, 42u, 42u, 
	47u, 48u, 57u, 48u, 57u, 43u, 45u, 48u, 
	57u, 48u, 57u, 43u, 45u, 48u, 57u, 48u, 
	57u, 48u, 49u, 48u, 57u, 65u, 70u, 97u, 
	102u, 80u, 112u, 48u, 57u, 65u, 70u, 97u, 
	102u, 39u, 80u, 112u, 48u, 57u, 65u, 70u, 
	97u, 102u, 48u, 57u, 65u, 70u, 97u, 102u, 
	43u, 45u, 48u, 57u, 48u, 57u, 32u, 34u, 
	35u, 36u, 38u, 39u, 42u, 43u, 45u, 46u, 
	47u, 48u, 60u, 61u, 62u, 76u, 85u, 94u, 
	95u, 117u, 124u, 9u, 13u, 33u, 37u, 49u, 
	57u, 65u, 90u, 97u, 122u, 32u, 9u, 13u, 
	61u, 9u, 32u, 36u, 95u, 100u, 101u, 105u, 
	108u, 112u, 117u, 119u, 65u, 90u, 97u, 122u, 
	36u, 95u, 48u, 57u, 65u, 90u, 97u, 122u, 
	36u, 95u, 101u, 48u, 57u, 65u, 90u, 97u, 
	122u, 36u, 95u, 102u, 48u, 57u, 65u, 90u, 
	97u, 122u, 36u, 95u, 105u, 48u, 57u, 65u, 
	90u, 97u, 122u, 36u, 95u, 110u, 48u, 57u, 
	65u, 90u, 97u, 122u, 36u, 95u, 101u, 48u, 
	57u, 65u, 90u, 97u, 122u, 36u, 95u, 108u, 
	110u, 114u, 48u, 57u, 65u, 90u, 97u, 122u, 
	36u, 95u, 105u, 115u, 48u, 57u, 65u, 90u, 
	97u, 122u, 36u, 95u, 102u, 48u, 57u, 65u, 
	90u, 97u, 122u, 36u, 95u, 100u, 48u, 57u, 
	65u, 90u, 97u, 122u, 36u, 95u, 105u, 48u, 
	57u, 65u, 90u, 97u, 122u, 36u, 95u, 114u, 
	48u, 57u, 65u, 90u, 97u, 122u, 36u, 95u, 
	111u, 48u, 57u, 65u, 90u, 97u, 122u, 36u, 
	95u, 114u, 48u, 57u, 65u, 90u, 97u, 122u, 
	36u, 95u, 102u, 110u, 48u, 57u, 65u, 90u, 
	97u, 122u, 36u, 95u, 100u, 110u, 48u, 57u, 
	65u, 90u, 97u, 122u, 36u, 95u, 101u, 48u, 
	57u, 65u, 90u, 97u, 122u, 36u, 95u, 100u, 
	48u, 57u, 65u, 90u, 97u, 122u, 36u, 95u, 
	99u, 48u, 57u, 65u, 90u, 97u, 122u, 36u, 
	95u, 108u, 48u, 57u, 65u, 90u, 97u, 122u, 
	36u, 95u, 117u, 48u, 57u, 65u, 90u, 97u, 
	122u, 36u, 95u, 100u, 48u, 57u, 65u, 90u, 
	97u, 122u, 36u, 95u, 114u, 48u, 57u, 65u, 
	90u, 97u, 122u, 36u, 95u, 97u, 48u, 57u, 
	65u, 90u, 98u, 122u, 36u, 95u, 103u, 48u, 
	57u, 65u, 90u, 97u, 122u, 36u, 95u, 109u, 
	48u, 57u, 65u, 90u, 97u, 122u, 36u, 95u, 
	97u, 48u, 57u, 65u, 90u, 98u, 122u, 36u, 
	95u, 110u, 48u, 57u, 65u, 90u, 97u, 122u, 
	36u, 95u, 97u, 48u, 57u, 65u, 90u, 98u, 
	122u, 36u, 95u, 114u, 48u, 57u, 65u, 90u, 
	97u, 122u, 36u, 95u, 110u, 48u, 57u, 65u, 
	90u, 97u, 122u, 36u, 95u, 105u, 48u, 57u, 
	65u, 90u, 97u, 122u, 36u, 95u, 110u, 48u, 
	57u, 65u, 90u, 97u, 122u, 36u, 95u, 103u, 
	48u, 57u, 65u, 90u, 97u, 122u, 36u, 95u, 
	48u, 57u, 65u, 90u, 97u, 122u, 38u, 61u, 
	43u, 61u, 45u, 61u, 62u, 46u, 48u, 57u, 
	39u, 69u, 76u, 101u, 108u, 48u, 57u, 68u, 
	70u, 100u, 102u, 39u, 68u, 70u, 76u, 100u, 
	102u, 108u, 48u, 57u, 42u, 47u, 61u, 10u, 
	39u, 46u, 66u, 69u, 76u, 85u, 88u, 98u, 
	101u, 108u, 117u, 120u, 48u, 55u, 56u, 57u, 
	39u, 46u, 69u, 76u, 85u, 101u, 108u, 117u, 
	48u, 57u, 69u, 76u, 101u, 108u, 48u, 57u, 
	68u, 70u, 100u, 102u, 39u, 69u, 76u, 101u, 
	108u, 48u, 57u, 68u, 70u, 100u, 102u, 39u, 
	68u, 70u, 76u, 100u, 102u, 108u, 48u, 57u, 
	39u, 68u, 70u, 76u, 100u, 102u, 108u, 48u, 
	57u, 76u, 85u, 108u, 117u, 85u, 117u, 76u, 
	108u, 76u, 108u, 39u, 46u, 69u, 76u, 85u, 
	101u, 108u, 117u, 48u, 55u, 56u, 57u, 76u, 
	85u, 108u, 117u, 85u, 117u, 76u, 108u, 76u, 
	108u, 39u, 76u, 85u, 108u, 117u, 48u, 49u, 
	76u, 85u, 108u, 117u, 85u, 117u, 76u, 108u, 
	76u, 108u, 39u, 46u, 76u, 80u, 85u, 108u, 
	112u, 117u, 48u, 57u, 65u, 70u, 97u, 102u, 
	39u, 68u, 70u, 76u, 100u, 102u, 108u, 48u, 
	57u, 76u, 85u, 108u, 117u, 85u, 117u, 76u, 
	108u, 76u, 108u, 60u, 61u, 61u, 62u, 34u, 
	36u, 39u, 95u, 48u, 57u, 65u, 90u, 97u, 
	122u, 34u, 36u, 39u, 56u, 95u, 48u, 57u, 
	65u, 90u, 97u, 122u, 61u, 124u, 0
};

static const char _CT_single_lengths[] = {
	0, 2, 9, 0, 0, 0, 0, 0, 
	0, 0, 0, 11, 2, 9, 0, 0, 
	0, 0, 0, 0, 0, 0, 1, 0, 
	2, 0, 1, 2, 0, 0, 2, 0, 
	2, 0, 0, 0, 2, 3, 0, 2, 
	0, 21, 1, 1, 11, 2, 3, 3, 
	3, 3, 3, 5, 4, 3, 3, 3, 
	3, 3, 3, 4, 4, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 2, 
	2, 2, 1, 1, 5, 7, 3, 1, 
	12, 8, 4, 5, 7, 7, 4, 2, 
	2, 2, 8, 4, 2, 2, 2, 5, 
	4, 2, 2, 2, 8, 7, 4, 2, 
	2, 2, 2, 2, 4, 5, 2
};

static const char _CT_range_lengths[] = {
	0, 0, 4, 3, 3, 3, 3, 3, 
	3, 3, 3, 2, 0, 4, 3, 3, 
	3, 3, 3, 3, 3, 3, 0, 1, 
	1, 1, 0, 0, 1, 1, 1, 1, 
	1, 1, 1, 3, 3, 3, 3, 1, 
	1, 5, 1, 0, 2, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	0, 0, 1, 1, 3, 1, 0, 0, 
	2, 1, 3, 3, 1, 1, 0, 0, 
	0, 0, 2, 0, 0, 0, 0, 1, 
	0, 0, 0, 0, 3, 1, 0, 0, 
	0, 0, 0, 0, 3, 3, 0
};

static const short _CT_index_offsets[] = {
	0, 0, 3, 17, 21, 25, 29, 33, 
	37, 41, 45, 49, 63, 66, 80, 84, 
	88, 92, 96, 100, 104, 108, 112, 114, 
	116, 120, 122, 124, 127, 129, 131, 135, 
	137, 141, 143, 145, 149, 155, 162, 166, 
	170, 172, 199, 202, 204, 218, 224, 231, 
	238, 245, 252, 259, 268, 276, 283, 290, 
	297, 304, 311, 318, 326, 334, 341, 348, 
	355, 362, 369, 376, 383, 390, 397, 404, 
	411, 418, 425, 432, 439, 446, 453, 460, 
	466, 469, 472, 475, 478, 487, 496, 500, 
	502, 517, 527, 535, 544, 553, 562, 567, 
	570, 573, 576, 587, 592, 595, 598, 601, 
	608, 613, 616, 619, 622, 634, 643, 648, 
	651, 654, 657, 660, 663, 671, 680
};

static const unsigned char _CT_indicies[] = {
	2, 3, 1, 1, 1, 1, 4, 1, 
	1, 1, 5, 6, 1, 1, 1, 1, 
	0, 7, 7, 7, 0, 8, 8, 8, 
	0, 9, 9, 9, 0, 5, 5, 5, 
	0, 10, 10, 10, 0, 11, 11, 11, 
	0, 6, 6, 6, 0, 1, 1, 1, 
	0, 13, 13, 14, 14, 15, 16, 17, 
	18, 19, 20, 21, 14, 14, 12, 23, 
	24, 22, 22, 22, 22, 25, 22, 22, 
	22, 26, 27, 22, 22, 22, 22, 0, 
	28, 28, 28, 0, 29, 29, 29, 0, 
	30, 30, 30, 0, 26, 26, 26, 0, 
	31, 31, 31, 0, 32, 32, 32, 0, 
	27, 27, 27, 0, 22, 22, 22, 0, 
	34, 33, 36, 35, 37, 37, 38, 35, 
	38, 35, 41, 40, 41, 42, 40, 43, 
	0, 45, 44, 46, 46, 47, 44, 47, 
	44, 48, 48, 49, 0, 49, 0, 50, 
	0, 51, 51, 51, 0, 54, 54, 53, 
	53, 53, 52, 55, 54, 54, 53, 53, 
	53, 52, 53, 53, 53, 52, 56, 56, 
	57, 52, 57, 0, 59, 1, 61, 62, 
	63, 22, 60, 64, 65, 66, 67, 68, 
	69, 60, 70, 71, 71, 60, 62, 72, 
	73, 59, 60, 43, 62, 62, 58, 59, 
	59, 74, 34, 0, 13, 13, 14, 14, 
	15, 16, 17, 18, 19, 20, 21, 14, 
	14, 75, 14, 14, 14, 14, 14, 0, 
	14, 14, 77, 14, 14, 14, 76, 14, 
	14, 18, 14, 14, 14, 76, 14, 14, 
	78, 14, 14, 14, 76, 14, 14, 79, 
	14, 14, 14, 76, 14, 14, 80, 14, 
	14, 14, 76, 14, 14, 81, 82, 83, 
	14, 14, 14, 76, 14, 14, 84, 79, 
	14, 14, 14, 76, 14, 14, 80, 14, 
	14, 14, 76, 14, 14, 85, 14, 14, 
	14, 76, 14, 14, 84, 14, 14, 14, 
	76, 14, 14, 86, 14, 14, 14, 76, 
	14, 14, 87, 14, 14, 14, 76, 14, 
	14, 80, 14, 14, 14, 76, 14, 14, 
	88, 89, 14, 14, 14, 76, 14, 14, 
	91, 92, 14, 14, 14, 90, 14, 14, 
	84, 14, 14, 14, 76, 14, 14, 91, 
	14, 14, 14, 76, 14, 14, 93, 14, 
	14, 14, 76, 14, 14, 94, 14, 14, 
	14, 76, 14, 14, 95, 14, 14, 14, 
	76, 14, 14, 79, 14, 14, 14, 76, 
	14, 14, 96, 14, 14, 14, 76, 14, 
	14, 97, 14, 14, 14, 76, 14, 14, 
	98, 14, 14, 14, 76, 14, 14, 99, 
	14, 14, 14, 76, 14, 14, 80, 14, 
	14, 14, 76, 14, 14, 92, 14, 14, 
	14, 76, 14, 14, 100, 14, 14, 14, 
	76, 14, 14, 101, 14, 14, 14, 76, 
	14, 14, 102, 14, 14, 14, 76, 14, 
	14, 103, 14, 14, 14, 76, 14, 14, 
	104, 14, 14, 14, 76, 14, 14, 80, 
	14, 14, 14, 76, 62, 62, 62, 62, 
	62, 105, 34, 34, 106, 34, 34, 106, 
	34, 34, 106, 108, 36, 107, 110, 112, 
	111, 112, 111, 36, 111, 111, 109, 37, 
	111, 111, 111, 111, 111, 111, 38, 109, 
	40, 113, 34, 106, 114, 113, 116, 117, 
	119, 120, 121, 122, 123, 119, 120, 121, 
	122, 123, 118, 43, 115, 116, 117, 120, 
	121, 122, 120, 121, 122, 43, 115, 126, 
	125, 126, 125, 45, 125, 125, 124, 127, 
	126, 125, 126, 125, 45, 125, 125, 124, 
	46, 125, 125, 125, 125, 125, 125, 47, 
	124, 48, 129, 129, 129, 129, 129, 129, 
	49, 128, 130, 131, 130, 131, 115, 131, 
	131, 115, 132, 132, 115, 131, 131, 115, 
	116, 117, 120, 134, 135, 120, 134, 135, 
	118, 43, 133, 136, 137, 136, 137, 133, 
	137, 137, 133, 138, 138, 133, 137, 137, 
	133, 119, 140, 141, 140, 141, 50, 139, 
	142, 143, 142, 143, 139, 143, 143, 139, 
	144, 144, 139, 143, 143, 139, 123, 146, 
	147, 54, 148, 147, 54, 148, 51, 51, 
	51, 145, 56, 150, 150, 150, 150, 150, 
	150, 57, 149, 151, 152, 151, 152, 145, 
	152, 152, 145, 153, 153, 145, 152, 152, 
	145, 154, 34, 106, 34, 154, 106, 1, 
	62, 22, 62, 62, 62, 62, 105, 1, 
	62, 22, 71, 62, 62, 62, 62, 105, 
	34, 34, 106, 0
};

static const char _CT_trans_targs[] = {
	41, 1, 41, 2, 3, 7, 10, 4, 
	5, 6, 8, 9, 41, 11, 45, 46, 
	51, 59, 48, 67, 72, 73, 12, 41, 
	13, 14, 18, 21, 15, 16, 17, 19, 
	20, 41, 41, 41, 84, 25, 85, 41, 
	26, 27, 41, 89, 41, 91, 31, 92, 
	33, 93, 103, 108, 41, 37, 39, 38, 
	40, 109, 41, 42, 43, 44, 79, 80, 
	81, 82, 83, 86, 88, 114, 115, 116, 
	117, 118, 41, 41, 41, 47, 49, 50, 
	45, 52, 54, 56, 53, 55, 57, 58, 
	60, 63, 41, 61, 62, 64, 65, 66, 
	68, 69, 70, 71, 74, 75, 76, 77, 
	78, 41, 41, 41, 22, 41, 23, 41, 
	24, 87, 41, 41, 28, 90, 98, 34, 
	32, 94, 96, 35, 41, 41, 30, 29, 
	41, 41, 95, 41, 97, 41, 99, 101, 
	100, 41, 102, 41, 104, 106, 105, 41, 
	107, 41, 36, 110, 112, 41, 41, 111, 
	41, 113, 43
};

static const char _CT_trans_actions[] = {
	75, 0, 7, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 69, 0, 101, 0, 
	0, 0, 0, 0, 0, 0, 0, 9, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 73, 27, 67, 3, 0, 3, 71, 
	0, 0, 5, 95, 65, 3, 0, 3, 
	0, 92, 86, 83, 63, 0, 0, 0, 
	0, 80, 29, 0, 110, 3, 0, 0, 
	0, 0, 3, 3, 95, 0, 0, 104, 
	104, 0, 61, 53, 51, 0, 0, 0, 
	98, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 49, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 55, 57, 59, 0, 43, 0, 21, 
	0, 0, 31, 47, 0, 3, 89, 0, 
	0, 0, 0, 0, 41, 19, 0, 0, 
	45, 23, 0, 25, 0, 39, 0, 0, 
	0, 17, 0, 37, 0, 0, 0, 15, 
	0, 35, 0, 0, 0, 33, 11, 0, 
	13, 0, 107
};

static const char _CT_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 77, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0
};

static const char _CT_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0
};

static const short _CT_eof_trans[] = {
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 13, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 34, 36, 
	36, 36, 40, 40, 1, 45, 45, 45, 
	1, 1, 1, 1, 53, 53, 53, 53, 
	1, 0, 75, 1, 76, 1, 77, 77, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 91, 77, 77, 77, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 77, 77, 106, 
	107, 107, 107, 108, 110, 110, 107, 115, 
	116, 116, 125, 125, 125, 129, 116, 116, 
	116, 116, 134, 134, 134, 134, 134, 140, 
	140, 140, 140, 140, 146, 150, 146, 146, 
	146, 146, 107, 107, 106, 106, 107
};

static const int CT_start = 41;
static const int CT_first_final = 41;
static const int CT_error = 0;

static const int CT_en_main = 41;


/* #line 154 "CT.c.rl" */

// the public API function
ok64 CTLexer(CTstate* state) {

    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    int act = 0;
    u8c *p = (u8c*) data[0];
    u8c *pe = (u8c*) data[1];
    u8c *eof = pe;
    u8c *ts = NULL;
    u8c *te = NULL;
    ok64 o = OK;

    u8cs tok = {p, p};

    
/* #line 422 "CT.rl.c" */
	{
	cs = CT_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 173 "CT.c.rl" */
    
/* #line 428 "CT.rl.c" */
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
	_acts = _CT_actions + _CT_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 2:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 447 "CT.rl.c" */
		}
	}

	_keys = _CT_trans_keys + _CT_key_offsets[cs];
	_trans = _CT_index_offsets[cs];

	_klen = _CT_single_lengths[cs];
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

	_klen = _CT_range_lengths[cs];
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
	_trans = _CT_indicies[_trans];
_eof_trans:
	cs = _CT_trans_targs[_trans];

	if ( _CT_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _CT_actions + _CT_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 3:
/* #line 1 "NONE" */
	{te = p+1;}
	break;
	case 4:
/* #line 50 "CT.c.rl" */
	{act = 5;}
	break;
	case 5:
/* #line 50 "CT.c.rl" */
	{act = 6;}
	break;
	case 6:
/* #line 50 "CT.c.rl" */
	{act = 7;}
	break;
	case 7:
/* #line 50 "CT.c.rl" */
	{act = 8;}
	break;
	case 8:
/* #line 50 "CT.c.rl" */
	{act = 11;}
	break;
	case 9:
/* #line 50 "CT.c.rl" */
	{act = 12;}
	break;
	case 10:
/* #line 56 "CT.c.rl" */
	{act = 13;}
	break;
	case 11:
/* #line 56 "CT.c.rl" */
	{act = 14;}
	break;
	case 12:
/* #line 62 "CT.c.rl" */
	{act = 16;}
	break;
	case 13:
/* #line 68 "CT.c.rl" */
	{act = 17;}
	break;
	case 14:
/* #line 68 "CT.c.rl" */
	{act = 18;}
	break;
	case 15:
/* #line 38 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 16:
/* #line 44 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 17:
/* #line 44 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 50 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 19:
/* #line 50 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 20:
/* #line 50 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 21:
/* #line 50 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 22:
/* #line 50 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 23:
/* #line 50 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 24:
/* #line 50 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 25:
/* #line 50 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 26:
/* #line 68 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 27:
/* #line 68 "CT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 28:
/* #line 38 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 29:
/* #line 50 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 30:
/* #line 50 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 31:
/* #line 50 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 32:
/* #line 50 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 33:
/* #line 50 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 34:
/* #line 50 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 35:
/* #line 50 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 36:
/* #line 50 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 37:
/* #line 56 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPreproc(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 38:
/* #line 56 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPreproc(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 39:
/* #line 68 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 40:
/* #line 62 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 41:
/* #line 68 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 42:
/* #line 68 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 43:
/* #line 74 "CT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 44:
/* #line 50 "CT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 45:
/* #line 50 "CT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 46:
/* #line 50 "CT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 47:
/* #line 68 "CT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 48:
/* #line 68 "CT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 49:
/* #line 68 "CT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 50:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 0:
	{{cs = 0;goto _again;}}
	break;
	case 5:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 6:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 7:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 8:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 11:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 12:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 13:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPreproc(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 14:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPreproc(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 16:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 17:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 18:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	}
	}
	break;
/* #line 924 "CT.rl.c" */
		}
	}

_again:
	_acts = _CT_actions + _CT_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
	case 1:
/* #line 1 "NONE" */
	{act = 0;}
	break;
/* #line 938 "CT.rl.c" */
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _CT_eof_trans[cs] > 0 ) {
		_trans = _CT_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 174 "CT.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < CT_first_final)
        o = CTBAD;

    return o;
}
