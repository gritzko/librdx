
#line 1 "RDXJ.rl"
#include "RDXJ.rl.h"



#line 196 "RDXJ.rl"



#line 7 "RDXJ.rl.c"
static const char _RDXJ_actions[] = {
	0, 1, 0, 1, 1, 1, 3, 1, 
	5, 1, 16, 1, 17, 1, 18, 1, 
	19, 1, 20, 1, 21, 1, 22, 1, 
	23, 1, 24, 1, 25, 1, 26, 1, 
	27, 1, 28, 1, 30, 1, 31, 1, 
	32, 1, 33, 1, 35, 1, 36, 1, 
	37, 2, 2, 4, 2, 7, 28, 2, 
	7, 35, 2, 9, 28, 2, 9, 35, 
	2, 11, 28, 2, 11, 35, 2, 13, 
	28, 2, 13, 35, 2, 15, 28, 2, 
	15, 35, 2, 17, 16, 2, 17, 18, 
	2, 17, 20, 2, 17, 22, 2, 17, 
	24, 2, 17, 26, 2, 17, 30, 2, 
	17, 32, 2, 17, 37, 2, 19, 16, 
	2, 19, 18, 2, 19, 20, 2, 19, 
	22, 2, 19, 24, 2, 19, 26, 2, 
	19, 30, 2, 19, 32, 2, 19, 37, 
	2, 21, 16, 2, 21, 18, 2, 21, 
	20, 2, 21, 22, 2, 21, 24, 2, 
	21, 26, 2, 21, 30, 2, 21, 32, 
	2, 21, 37, 2, 23, 16, 2, 23, 
	18, 2, 23, 20, 2, 23, 22, 2, 
	23, 24, 2, 23, 26, 2, 23, 30, 
	2, 23, 32, 2, 23, 37, 2, 25, 
	16, 2, 25, 18, 2, 25, 20, 2, 
	25, 22, 2, 25, 24, 2, 25, 26, 
	2, 25, 30, 2, 25, 32, 2, 25, 
	37, 2, 27, 16, 2, 27, 18, 2, 
	27, 20, 2, 27, 22, 2, 27, 24, 
	2, 27, 26, 2, 27, 30, 2, 27, 
	32, 2, 27, 37, 2, 29, 35, 2, 
	31, 16, 2, 31, 18, 2, 31, 20, 
	2, 31, 22, 2, 31, 24, 2, 31, 
	26, 2, 31, 30, 2, 31, 32, 2, 
	31, 37, 2, 33, 16, 2, 33, 18, 
	2, 33, 20, 2, 33, 22, 2, 33, 
	24, 2, 33, 26, 2, 33, 30, 2, 
	33, 32, 2, 33, 37, 2, 34, 12, 
	2, 34, 14, 2, 35, 16, 2, 35, 
	18, 2, 35, 20, 2, 35, 22, 2, 
	35, 24, 2, 35, 26, 2, 35, 30, 
	2, 35, 32, 2, 35, 37, 2, 36, 
	16, 2, 36, 18, 2, 36, 20, 2, 
	36, 22, 2, 36, 24, 2, 36, 26, 
	2, 36, 30, 2, 36, 32, 2, 36, 
	37, 3, 7, 35, 16, 3, 7, 35, 
	18, 3, 7, 35, 20, 3, 7, 35, 
	22, 3, 7, 35, 24, 3, 7, 35, 
	26, 3, 7, 35, 30, 3, 7, 35, 
	32, 3, 7, 35, 37, 3, 9, 35, 
	16, 3, 9, 35, 18, 3, 9, 35, 
	20, 3, 9, 35, 22, 3, 9, 35, 
	24, 3, 9, 35, 26, 3, 9, 35, 
	30, 3, 9, 35, 32, 3, 9, 35, 
	37, 3, 11, 35, 16, 3, 11, 35, 
	18, 3, 11, 35, 20, 3, 11, 35, 
	22, 3, 11, 35, 24, 3, 11, 35, 
	26, 3, 11, 35, 30, 3, 11, 35, 
	32, 3, 11, 35, 37, 3, 13, 35, 
	16, 3, 13, 35, 18, 3, 13, 35, 
	20, 3, 13, 35, 22, 3, 13, 35, 
	24, 3, 13, 35, 26, 3, 13, 35, 
	30, 3, 13, 35, 32, 3, 13, 35, 
	37, 3, 15, 35, 16, 3, 15, 35, 
	18, 3, 15, 35, 20, 3, 15, 35, 
	22, 3, 15, 35, 24, 3, 15, 35, 
	26, 3, 15, 35, 30, 3, 15, 35, 
	32, 3, 15, 35, 37, 3, 17, 34, 
	12, 3, 17, 34, 14, 3, 19, 34, 
	12, 3, 19, 34, 14, 3, 21, 34, 
	12, 3, 21, 34, 14, 3, 23, 34, 
	12, 3, 23, 34, 14, 3, 25, 34, 
	12, 3, 25, 34, 14, 3, 27, 34, 
	12, 3, 27, 34, 14, 3, 29, 35, 
	16, 3, 29, 35, 18, 3, 29, 35, 
	20, 3, 29, 35, 22, 3, 29, 35, 
	24, 3, 29, 35, 26, 3, 29, 35, 
	30, 3, 29, 35, 32, 3, 29, 35, 
	37, 3, 31, 34, 12, 3, 31, 34, 
	14, 3, 33, 34, 12, 3, 33, 34, 
	14, 3, 34, 8, 6, 3, 34, 10, 
	14, 3, 36, 34, 12, 3, 36, 34, 
	14, 4, 17, 34, 8, 6, 4, 17, 
	34, 10, 14, 4, 19, 34, 8, 6, 
	4, 19, 34, 10, 14, 4, 21, 34, 
	8, 6, 4, 21, 34, 10, 14, 4, 
	23, 34, 8, 6, 4, 23, 34, 10, 
	14, 4, 25, 34, 8, 6, 4, 25, 
	34, 10, 14, 4, 27, 34, 8, 6, 
	4, 27, 34, 10, 14, 4, 31, 34, 
	8, 6, 4, 31, 34, 10, 14, 4, 
	33, 34, 8, 6, 4, 33, 34, 10, 
	14, 4, 34, 8, 6, 10, 4, 36, 
	34, 8, 6, 4, 36, 34, 10, 14, 
	5, 17, 34, 8, 6, 10, 5, 19, 
	34, 8, 6, 10, 5, 21, 34, 8, 
	6, 10, 5, 23, 34, 8, 6, 10, 
	5, 25, 34, 8, 6, 10, 5, 27, 
	34, 8, 6, 10, 5, 31, 34, 8, 
	6, 10, 5, 33, 34, 8, 6, 10, 
	5, 36, 34, 8, 6, 10
};

static const short _RDXJ_key_offsets[] = {
	0, 0, 4, 8, 11, 13, 19, 25, 
	32, 38, 45, 53, 55, 61, 65, 74, 
	75, 81, 87, 93, 99, 100, 125, 150, 
	163, 176, 201, 226, 251, 267, 284, 309, 
	332, 351, 369, 381, 406, 429, 444, 469, 
	494, 514, 539, 564, 583, 603
};

static const unsigned char _RDXJ_trans_keys[] = {
	34u, 92u, 0u, 31u, 34u, 92u, 0u, 31u, 
	48u, 49u, 57u, 48u, 57u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 45u, 48u, 57u, 65u, 70u, 97u, 102u, 
	48u, 57u, 65u, 70u, 97u, 102u, 45u, 48u, 
	57u, 65u, 70u, 97u, 102u, 43u, 45u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 48u, 
	57u, 65u, 70u, 97u, 102u, 43u, 45u, 48u, 
	57u, 34u, 47u, 92u, 98u, 102u, 110u, 114u, 
	116u, 117u, 34u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 34u, 13u, 32u, 34u, 40u, 
	41u, 44u, 45u, 48u, 58u, 91u, 93u, 123u, 
	125u, 9u, 10u, 49u, 57u, 65u, 70u, 71u, 
	90u, 97u, 102u, 103u, 122u, 13u, 32u, 34u, 
	40u, 41u, 44u, 45u, 48u, 58u, 91u, 93u, 
	123u, 125u, 9u, 10u, 49u, 57u, 65u, 70u, 
	71u, 90u, 97u, 102u, 103u, 122u, 13u, 32u, 
	40u, 41u, 44u, 58u, 64u, 91u, 93u, 123u, 
	125u, 9u, 10u, 13u, 32u, 40u, 41u, 44u, 
	58u, 64u, 91u, 93u, 123u, 125u, 9u, 10u, 
	13u, 32u, 34u, 40u, 41u, 44u, 45u, 48u, 
	58u, 91u, 93u, 123u, 125u, 9u, 10u, 49u, 
	57u, 65u, 70u, 71u, 90u, 97u, 102u, 103u, 
	122u, 13u, 32u, 34u, 40u, 41u, 44u, 45u, 
	48u, 58u, 91u, 93u, 123u, 125u, 9u, 10u, 
	49u, 57u, 65u, 70u, 71u, 90u, 97u, 102u, 
	103u, 122u, 13u, 32u, 34u, 40u, 41u, 44u, 
	45u, 48u, 58u, 91u, 93u, 123u, 125u, 9u, 
	10u, 49u, 57u, 65u, 70u, 71u, 90u, 97u, 
	102u, 103u, 122u, 13u, 32u, 40u, 41u, 44u, 
	46u, 58u, 64u, 69u, 91u, 93u, 101u, 123u, 
	125u, 9u, 10u, 13u, 32u, 40u, 41u, 44u, 
	58u, 64u, 69u, 91u, 93u, 101u, 123u, 125u, 
	9u, 10u, 48u, 57u, 13u, 32u, 34u, 40u, 
	41u, 44u, 45u, 48u, 58u, 91u, 93u, 123u, 
	125u, 9u, 10u, 49u, 57u, 65u, 70u, 71u, 
	90u, 97u, 102u, 103u, 122u, 13u, 32u, 40u, 
	41u, 44u, 45u, 46u, 58u, 64u, 69u, 91u, 
	93u, 101u, 123u, 125u, 9u, 10u, 48u, 57u, 
	65u, 70u, 97u, 102u, 13u, 32u, 40u, 41u, 
	44u, 58u, 64u, 91u, 93u, 123u, 125u, 9u, 
	10u, 48u, 57u, 65u, 70u, 97u, 102u, 13u, 
	32u, 40u, 41u, 44u, 58u, 91u, 93u, 123u, 
	125u, 9u, 10u, 48u, 57u, 65u, 70u, 97u, 
	102u, 13u, 32u, 40u, 41u, 44u, 58u, 91u, 
	93u, 123u, 125u, 9u, 10u, 13u, 32u, 34u, 
	40u, 41u, 44u, 45u, 48u, 58u, 91u, 93u, 
	123u, 125u, 9u, 10u, 49u, 57u, 65u, 70u, 
	71u, 90u, 97u, 102u, 103u, 122u, 13u, 32u, 
	40u, 41u, 44u, 45u, 46u, 58u, 64u, 69u, 
	91u, 93u, 101u, 123u, 125u, 9u, 10u, 48u, 
	57u, 65u, 70u, 97u, 102u, 13u, 32u, 40u, 
	41u, 44u, 58u, 64u, 91u, 93u, 123u, 125u, 
	9u, 10u, 48u, 57u, 13u, 32u, 34u, 40u, 
	41u, 44u, 45u, 48u, 58u, 91u, 93u, 123u, 
	125u, 9u, 10u, 49u, 57u, 65u, 70u, 71u, 
	90u, 97u, 102u, 103u, 122u, 13u, 32u, 40u, 
	41u, 44u, 45u, 58u, 64u, 91u, 93u, 95u, 
	123u, 125u, 9u, 10u, 48u, 57u, 65u, 70u, 
	71u, 90u, 97u, 102u, 103u, 122u, 13u, 32u, 
	40u, 41u, 44u, 58u, 64u, 91u, 93u, 95u, 
	123u, 125u, 9u, 10u, 48u, 57u, 65u, 90u, 
	97u, 122u, 13u, 32u, 34u, 40u, 41u, 44u, 
	45u, 48u, 58u, 91u, 93u, 123u, 125u, 9u, 
	10u, 49u, 57u, 65u, 70u, 71u, 90u, 97u, 
	102u, 103u, 122u, 13u, 32u, 34u, 40u, 41u, 
	44u, 45u, 48u, 58u, 91u, 93u, 123u, 125u, 
	9u, 10u, 49u, 57u, 65u, 70u, 71u, 90u, 
	97u, 102u, 103u, 122u, 13u, 32u, 40u, 41u, 
	44u, 58u, 64u, 91u, 93u, 123u, 125u, 9u, 
	10u, 48u, 57u, 65u, 70u, 97u, 102u, 13u, 
	32u, 40u, 41u, 44u, 45u, 58u, 64u, 91u, 
	93u, 123u, 125u, 9u, 10u, 48u, 57u, 65u, 
	70u, 97u, 102u, 13u, 32u, 40u, 41u, 44u, 
	46u, 58u, 64u, 69u, 91u, 93u, 101u, 123u, 
	125u, 9u, 10u, 48u, 57u, 0
};

static const char _RDXJ_single_lengths[] = {
	0, 2, 2, 1, 0, 0, 0, 1, 
	0, 1, 2, 0, 0, 2, 9, 1, 
	0, 0, 0, 0, 1, 13, 13, 11, 
	11, 13, 13, 13, 14, 13, 13, 15, 
	11, 10, 10, 13, 15, 11, 13, 13, 
	12, 13, 13, 11, 12, 14
};

static const char _RDXJ_range_lengths[] = {
	0, 1, 1, 1, 1, 3, 3, 3, 
	3, 3, 3, 1, 3, 1, 0, 0, 
	3, 3, 3, 3, 0, 6, 6, 1, 
	1, 6, 6, 6, 1, 2, 6, 4, 
	4, 4, 1, 6, 4, 2, 6, 6, 
	4, 6, 6, 4, 4, 2
};

static const short _RDXJ_index_offsets[] = {
	0, 0, 4, 8, 11, 13, 17, 21, 
	26, 30, 35, 41, 43, 47, 51, 61, 
	63, 67, 71, 75, 79, 81, 101, 121, 
	134, 147, 167, 187, 207, 223, 239, 259, 
	279, 295, 310, 322, 342, 362, 376, 396, 
	416, 433, 453, 473, 489, 506
};

static const unsigned char _RDXJ_indicies[] = {
	1, 2, 1, 0, 4, 1, 1, 3, 
	5, 6, 1, 7, 1, 8, 8, 8, 
	1, 9, 9, 9, 1, 10, 9, 9, 
	9, 1, 11, 11, 11, 1, 12, 13, 
	13, 13, 1, 14, 15, 16, 13, 13, 
	1, 17, 1, 18, 8, 8, 1, 14, 
	14, 17, 1, 19, 19, 19, 19, 19, 
	19, 19, 19, 20, 1, 21, 1, 22, 
	22, 22, 1, 23, 23, 23, 1, 24, 
	24, 24, 1, 25, 25, 25, 1, 26, 
	1, 27, 27, 28, 29, 30, 31, 32, 
	33, 35, 38, 39, 40, 41, 27, 34, 
	36, 37, 36, 37, 1, 42, 42, 43, 
	44, 45, 46, 47, 48, 50, 53, 54, 
	55, 56, 42, 49, 51, 52, 51, 52, 
	1, 57, 57, 58, 59, 60, 61, 62, 
	63, 64, 65, 66, 57, 1, 67, 67, 
	68, 69, 70, 71, 72, 73, 74, 75, 
	76, 67, 1, 77, 77, 78, 79, 80, 
	81, 82, 83, 85, 88, 89, 90, 91, 
	77, 84, 86, 87, 86, 87, 1, 92, 
	92, 93, 94, 95, 96, 97, 98, 100, 
	103, 104, 105, 106, 92, 99, 101, 102, 
	101, 102, 1, 107, 107, 108, 109, 110, 
	111, 112, 113, 115, 118, 119, 120, 121, 
	107, 114, 116, 117, 116, 117, 1, 122, 
	122, 123, 124, 125, 126, 127, 128, 129, 
	130, 131, 129, 132, 133, 122, 1, 134, 
	134, 135, 136, 137, 138, 139, 129, 140, 
	141, 129, 142, 143, 134, 7, 1, 144, 
	144, 145, 146, 147, 148, 149, 150, 152, 
	155, 156, 157, 158, 144, 151, 153, 154, 
	153, 154, 1, 122, 122, 123, 124, 125, 
	12, 126, 127, 128, 159, 130, 131, 159, 
	132, 133, 122, 13, 13, 13, 1, 160, 
	160, 161, 162, 163, 164, 165, 166, 167, 
	168, 169, 160, 8, 8, 8, 1, 170, 
	170, 171, 172, 173, 174, 175, 176, 177, 
	178, 170, 11, 11, 11, 1, 179, 179, 
	44, 45, 46, 50, 53, 54, 55, 56, 
	179, 1, 180, 180, 181, 182, 183, 184, 
	185, 186, 188, 191, 192, 193, 194, 180, 
	187, 189, 190, 189, 190, 1, 122, 122, 
	123, 124, 125, 12, 126, 127, 128, 159, 
	130, 131, 159, 132, 133, 122, 195, 13, 
	13, 1, 134, 134, 135, 136, 137, 138, 
	139, 140, 141, 142, 143, 134, 17, 1, 
	196, 196, 197, 198, 199, 200, 201, 202, 
	204, 207, 208, 209, 210, 196, 203, 205, 
	206, 205, 206, 1, 211, 211, 212, 213, 
	214, 12, 216, 217, 219, 220, 218, 221, 
	222, 211, 215, 215, 218, 215, 218, 1, 
	211, 211, 212, 213, 214, 216, 217, 219, 
	220, 218, 221, 222, 211, 218, 218, 218, 
	1, 223, 223, 224, 225, 226, 227, 228, 
	229, 231, 234, 235, 236, 237, 223, 230, 
	232, 233, 232, 233, 1, 238, 238, 239, 
	240, 241, 242, 243, 244, 246, 249, 250, 
	251, 252, 238, 245, 247, 248, 247, 248, 
	1, 134, 134, 135, 136, 137, 138, 139, 
	140, 141, 142, 143, 134, 18, 8, 8, 
	1, 134, 134, 135, 136, 137, 12, 138, 
	139, 140, 141, 142, 143, 134, 16, 13, 
	13, 1, 122, 122, 123, 124, 125, 126, 
	127, 128, 129, 130, 131, 129, 132, 133, 
	122, 6, 1, 0
};

static const char _RDXJ_trans_targs[] = {
	2, 0, 14, 2, 23, 28, 45, 29, 
	32, 7, 8, 33, 5, 9, 11, 12, 
	44, 37, 43, 15, 16, 23, 17, 18, 
	19, 20, 23, 22, 1, 25, 26, 27, 
	3, 31, 36, 30, 39, 40, 35, 38, 
	41, 42, 22, 1, 25, 26, 27, 3, 
	31, 36, 30, 39, 40, 35, 38, 41, 
	42, 24, 25, 26, 27, 30, 6, 35, 
	38, 41, 42, 24, 25, 26, 27, 30, 
	6, 35, 38, 41, 42, 22, 1, 25, 
	26, 27, 3, 31, 36, 30, 39, 40, 
	35, 38, 41, 42, 22, 1, 25, 26, 
	27, 3, 31, 36, 30, 39, 40, 35, 
	38, 41, 42, 22, 1, 25, 26, 27, 
	3, 31, 36, 30, 39, 40, 35, 38, 
	41, 42, 24, 25, 26, 27, 4, 30, 
	6, 13, 35, 38, 41, 42, 24, 25, 
	26, 27, 30, 6, 35, 38, 41, 42, 
	22, 1, 25, 26, 27, 3, 31, 36, 
	30, 39, 40, 35, 38, 41, 42, 10, 
	24, 25, 26, 27, 30, 6, 35, 38, 
	41, 42, 34, 25, 26, 27, 30, 35, 
	38, 41, 42, 34, 22, 1, 25, 26, 
	27, 3, 31, 36, 30, 39, 40, 35, 
	38, 41, 42, 36, 22, 1, 25, 26, 
	27, 3, 31, 36, 30, 39, 40, 35, 
	38, 41, 42, 24, 25, 26, 27, 39, 
	30, 6, 40, 35, 38, 41, 42, 22, 
	1, 25, 26, 27, 3, 31, 36, 30, 
	39, 40, 35, 38, 41, 42, 22, 1, 
	25, 26, 27, 3, 31, 36, 30, 39, 
	40, 35, 38, 41, 42
};

static const short _RDXJ_trans_actions[] = {
	1, 0, 49, 0, 3, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 5, 0, 0, 
	0, 0, 7, 45, 649, 346, 349, 352, 
	742, 800, 800, 355, 747, 653, 340, 343, 
	334, 337, 0, 301, 25, 29, 35, 641, 
	737, 737, 39, 645, 304, 17, 21, 9, 
	13, 73, 485, 489, 493, 497, 70, 477, 
	481, 469, 473, 43, 319, 322, 325, 328, 
	33, 313, 316, 307, 310, 27, 573, 202, 
	205, 208, 697, 776, 776, 211, 702, 577, 
	196, 199, 190, 193, 31, 581, 229, 232, 
	235, 707, 782, 782, 238, 712, 585, 223, 
	226, 217, 220, 37, 625, 259, 262, 265, 
	717, 788, 788, 268, 722, 629, 253, 256, 
	247, 250, 55, 377, 381, 385, 0, 389, 
	52, 0, 369, 373, 361, 365, 61, 413, 
	417, 421, 425, 58, 405, 409, 397, 401, 
	41, 633, 286, 289, 292, 727, 794, 794, 
	295, 732, 637, 280, 283, 274, 277, 0, 
	67, 449, 453, 457, 461, 64, 441, 445, 
	433, 437, 244, 605, 609, 613, 617, 597, 
	601, 589, 593, 0, 19, 557, 148, 151, 
	154, 677, 764, 764, 157, 682, 561, 142, 
	145, 136, 139, 0, 23, 565, 175, 178, 
	181, 687, 770, 770, 184, 692, 569, 169, 
	172, 163, 166, 79, 521, 525, 529, 0, 
	533, 76, 0, 513, 517, 505, 509, 11, 
	541, 94, 97, 100, 657, 752, 752, 103, 
	662, 545, 88, 91, 82, 85, 15, 549, 
	121, 124, 127, 667, 758, 758, 130, 672, 
	553, 115, 118, 109, 112
};

static const short _RDXJ_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 358, 47, 501, 
	331, 214, 241, 271, 393, 429, 298, 393, 
	465, 621, 47, 160, 393, 429, 187, 537, 
	537, 106, 133, 429, 429, 393
};

static const int RDXJ_start = 21;
static const int RDXJ_first_final = 21;
static const int RDXJ_error = 0;

static const int RDXJ_en_main = 21;


#line 199 "RDXJ.rl"

pro(RDXJlexer, RDXJstate* state) {

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

    
#line 399 "RDXJ.rl.c"
	{
	cs = RDXJ_start;
	}

#line 217 "RDXJ.rl"
    
#line 402 "RDXJ.rl.c"
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
	_keys = _RDXJ_trans_keys + _RDXJ_key_offsets[cs];
	_trans = _RDXJ_index_offsets[cs];

	_klen = _RDXJ_single_lengths[cs];
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

	_klen = _RDXJ_range_lengths[cs];
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
	_trans = _RDXJ_indicies[_trans];
	cs = _RDXJ_trans_targs[_trans];

	if ( _RDXJ_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _RDXJ_actions + _RDXJ_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 10 "RDXJ.rl"
	{ mark0[RDXJUTF8] = p - text[0]; }
	break;
	case 1:
#line 11 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJUTF8];
    tok[1] = p;
    call(RDXJonUTF8, tok, state); 
}
	break;
	case 2:
#line 16 "RDXJ.rl"
	{ mark0[RDXJEsc] = p - text[0]; }
	break;
	case 3:
#line 17 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJEsc];
    tok[1] = p;
    call(RDXJonEsc, tok, state); 
}
	break;
	case 4:
#line 22 "RDXJ.rl"
	{ mark0[RDXJHexEsc] = p - text[0]; }
	break;
	case 5:
#line 23 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJHexEsc];
    tok[1] = p;
    call(RDXJonHexEsc, tok, state); 
}
	break;
	case 6:
#line 28 "RDXJ.rl"
	{ mark0[RDXJInt] = p - text[0]; }
	break;
	case 7:
#line 29 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJInt];
    tok[1] = p;
    call(RDXJonInt, tok, state); 
}
	break;
	case 8:
#line 34 "RDXJ.rl"
	{ mark0[RDXJFloat] = p - text[0]; }
	break;
	case 9:
#line 35 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJFloat];
    tok[1] = p;
    call(RDXJonFloat, tok, state); 
}
	break;
	case 10:
#line 40 "RDXJ.rl"
	{ mark0[RDXJRef] = p - text[0]; }
	break;
	case 11:
#line 41 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJRef];
    tok[1] = p;
    call(RDXJonRef, tok, state); 
}
	break;
	case 12:
#line 46 "RDXJ.rl"
	{ mark0[RDXJString] = p - text[0]; }
	break;
	case 13:
#line 47 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJString];
    tok[1] = p;
    call(RDXJonString, tok, state); 
}
	break;
	case 14:
#line 52 "RDXJ.rl"
	{ mark0[RDXJTerm] = p - text[0]; }
	break;
	case 15:
#line 53 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJTerm];
    tok[1] = p;
    call(RDXJonTerm, tok, state); 
}
	break;
	case 16:
#line 58 "RDXJ.rl"
	{ mark0[RDXJOpenObject] = p - text[0]; }
	break;
	case 17:
#line 59 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenObject];
    tok[1] = p;
    call(RDXJonOpenObject, tok, state); 
}
	break;
	case 18:
#line 64 "RDXJ.rl"
	{ mark0[RDXJCloseObject] = p - text[0]; }
	break;
	case 19:
#line 65 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseObject];
    tok[1] = p;
    call(RDXJonCloseObject, tok, state); 
}
	break;
	case 20:
#line 70 "RDXJ.rl"
	{ mark0[RDXJOpenArray] = p - text[0]; }
	break;
	case 21:
#line 71 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenArray];
    tok[1] = p;
    call(RDXJonOpenArray, tok, state); 
}
	break;
	case 22:
#line 76 "RDXJ.rl"
	{ mark0[RDXJCloseArray] = p - text[0]; }
	break;
	case 23:
#line 77 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseArray];
    tok[1] = p;
    call(RDXJonCloseArray, tok, state); 
}
	break;
	case 24:
#line 82 "RDXJ.rl"
	{ mark0[RDXJOpenVector] = p - text[0]; }
	break;
	case 25:
#line 83 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenVector];
    tok[1] = p;
    call(RDXJonOpenVector, tok, state); 
}
	break;
	case 26:
#line 88 "RDXJ.rl"
	{ mark0[RDXJCloseVector] = p - text[0]; }
	break;
	case 27:
#line 89 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseVector];
    tok[1] = p;
    call(RDXJonCloseVector, tok, state); 
}
	break;
	case 28:
#line 94 "RDXJ.rl"
	{ mark0[RDXJStamp] = p - text[0]; }
	break;
	case 29:
#line 95 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJStamp];
    tok[1] = p;
    call(RDXJonStamp, tok, state); 
}
	break;
	case 30:
#line 100 "RDXJ.rl"
	{ mark0[RDXJComma] = p - text[0]; }
	break;
	case 31:
#line 101 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJComma];
    tok[1] = p;
    call(RDXJonComma, tok, state); 
}
	break;
	case 32:
#line 106 "RDXJ.rl"
	{ mark0[RDXJColon] = p - text[0]; }
	break;
	case 33:
#line 107 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJColon];
    tok[1] = p;
    call(RDXJonColon, tok, state); 
}
	break;
	case 34:
#line 112 "RDXJ.rl"
	{ mark0[RDXJFIRST] = p - text[0]; }
	break;
	case 35:
#line 113 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJFIRST];
    tok[1] = p;
    call(RDXJonFIRST, tok, state); 
}
	break;
	case 36:
#line 118 "RDXJ.rl"
	{ mark0[RDXJRoot] = p - text[0]; }
	break;
#line 658 "RDXJ.rl.c"
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
	const char *__acts = _RDXJ_actions + _RDXJ_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 7:
#line 29 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJInt];
    tok[1] = p;
    call(RDXJonInt, tok, state); 
}
	break;
	case 9:
#line 35 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJFloat];
    tok[1] = p;
    call(RDXJonFloat, tok, state); 
}
	break;
	case 11:
#line 41 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJRef];
    tok[1] = p;
    call(RDXJonRef, tok, state); 
}
	break;
	case 13:
#line 47 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJString];
    tok[1] = p;
    call(RDXJonString, tok, state); 
}
	break;
	case 15:
#line 53 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJTerm];
    tok[1] = p;
    call(RDXJonTerm, tok, state); 
}
	break;
	case 17:
#line 59 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenObject];
    tok[1] = p;
    call(RDXJonOpenObject, tok, state); 
}
	break;
	case 19:
#line 65 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseObject];
    tok[1] = p;
    call(RDXJonCloseObject, tok, state); 
}
	break;
	case 21:
#line 71 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenArray];
    tok[1] = p;
    call(RDXJonOpenArray, tok, state); 
}
	break;
	case 23:
#line 77 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseArray];
    tok[1] = p;
    call(RDXJonCloseArray, tok, state); 
}
	break;
	case 25:
#line 83 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenVector];
    tok[1] = p;
    call(RDXJonOpenVector, tok, state); 
}
	break;
	case 27:
#line 89 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseVector];
    tok[1] = p;
    call(RDXJonCloseVector, tok, state); 
}
	break;
	case 29:
#line 95 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJStamp];
    tok[1] = p;
    call(RDXJonStamp, tok, state); 
}
	break;
	case 31:
#line 101 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJComma];
    tok[1] = p;
    call(RDXJonComma, tok, state); 
}
	break;
	case 33:
#line 107 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJColon];
    tok[1] = p;
    call(RDXJonColon, tok, state); 
}
	break;
	case 35:
#line 113 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJFIRST];
    tok[1] = p;
    call(RDXJonFIRST, tok, state); 
}
	break;
	case 36:
#line 118 "RDXJ.rl"
	{ mark0[RDXJRoot] = p - text[0]; }
	break;
	case 37:
#line 119 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJRoot];
    tok[1] = p;
    call(RDXJonRoot, tok, state); 
}
	break;
#line 788 "RDXJ.rl.c"
		}
	}
	}

	_out: {}
	}

#line 218 "RDXJ.rl"

    if (p!=text[1] || cs < RDXJ_first_final) {
        state->text[0] = p;
        fail(RDXJfail);
    }
    done;
}
