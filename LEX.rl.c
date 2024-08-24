
#line 1 "LEX.rl"
#include "LEX.rl.h"



#line 110 "LEX.rl"



#line 7 "LEX.rl.c"
static const char _LEX_actions[] = {
	0, 1, 1, 2, 1, 0, 2, 1, 
	23, 2, 7, 6, 2, 7, 15, 2, 
	21, 23, 2, 22, 23, 3, 1, 7, 
	6, 3, 1, 7, 15, 3, 1, 16, 
	2, 3, 3, 13, 6, 3, 3, 13, 
	15, 3, 3, 17, 18, 3, 5, 7, 
	6, 3, 5, 7, 15, 3, 7, 6, 
	0, 3, 7, 6, 4, 3, 7, 12, 
	2, 3, 7, 12, 8, 3, 7, 12, 
	10, 3, 9, 13, 6, 3, 9, 13, 
	15, 3, 11, 13, 6, 3, 11, 13, 
	15, 3, 19, 14, 6, 3, 19, 14, 
	15, 3, 21, 20, 0, 3, 22, 20, 
	0, 4, 1, 7, 6, 0, 4, 1, 
	7, 6, 4, 4, 1, 7, 12, 2, 
	4, 1, 7, 12, 8, 4, 1, 7, 
	12, 10, 4, 3, 13, 6, 0, 4, 
	3, 13, 6, 4, 4, 3, 17, 18, 
	0, 4, 5, 7, 6, 0, 4, 5, 
	7, 6, 4, 4, 5, 7, 12, 2, 
	4, 5, 7, 12, 8, 4, 5, 7, 
	12, 10, 4, 9, 13, 6, 0, 4, 
	9, 13, 6, 4, 4, 11, 13, 6, 
	0, 4, 11, 13, 6, 4, 4, 19, 
	14, 6, 0, 4, 19, 14, 6, 4, 
	4, 21, 20, 16, 2, 4, 22, 20, 
	16, 2
};

static const short _LEX_key_offsets[] = {
	0, 0, 12, 17, 31, 52, 54, 68, 
	89, 110, 113, 116, 137, 139, 141, 157, 
	180, 183, 199, 222, 236, 243, 259, 273, 
	278, 283, 306, 329, 335, 338, 355, 378, 
	401, 416, 424, 441, 457, 479, 495, 517, 
	531, 538, 554, 556, 561, 566, 588, 611, 
	617, 623, 629, 652, 676, 683, 700, 704, 
	713, 722, 731, 742, 753, 765, 777, 788
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
	57u, 125u, 48u, 57u, 13u, 32u, 34u, 45u, 
	59u, 91u, 95u, 123u, 124u, 9u, 10u, 40u, 
	43u, 58u, 60u, 62u, 63u, 65u, 90u, 97u, 
	122u, 92u, 93u, 92u, 93u, 13u, 32u, 45u, 
	59u, 92u, 93u, 123u, 124u, 9u, 10u, 40u, 
	43u, 58u, 60u, 62u, 63u, 13u, 32u, 34u, 
	45u, 59u, 91u, 92u, 93u, 95u, 123u, 124u, 
	9u, 10u, 40u, 43u, 58u, 60u, 62u, 63u, 
	65u, 90u, 97u, 122u, 34u, 92u, 93u, 13u, 
	32u, 45u, 59u, 92u, 93u, 123u, 124u, 9u, 
	10u, 40u, 43u, 58u, 60u, 62u, 63u, 13u, 
	32u, 34u, 45u, 59u, 91u, 92u, 93u, 95u, 
	123u, 124u, 9u, 10u, 40u, 43u, 58u, 60u, 
	62u, 63u, 65u, 90u, 97u, 122u, 13u, 32u, 
	61u, 92u, 93u, 95u, 9u, 10u, 48u, 57u, 
	65u, 90u, 97u, 122u, 13u, 32u, 61u, 92u, 
	93u, 9u, 10u, 13u, 32u, 45u, 59u, 92u, 
	93u, 123u, 124u, 9u, 10u, 40u, 43u, 58u, 
	60u, 62u, 63u, 13u, 32u, 45u, 59u, 123u, 
	124u, 9u, 10u, 40u, 43u, 58u, 60u, 62u, 
	63u, 44u, 92u, 93u, 48u, 57u, 92u, 93u, 
	125u, 48u, 57u, 13u, 32u, 34u, 45u, 59u, 
	91u, 92u, 93u, 95u, 123u, 124u, 9u, 10u, 
	40u, 43u, 58u, 60u, 62u, 63u, 65u, 90u, 
	97u, 122u, 13u, 32u, 45u, 59u, 92u, 93u, 
	95u, 123u, 124u, 9u, 10u, 40u, 43u, 48u, 
	57u, 58u, 60u, 62u, 63u, 65u, 90u, 97u, 
	122u, 44u, 92u, 93u, 125u, 48u, 57u, 34u, 
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
	57u, 34u, 92u, 125u, 48u, 57u, 13u, 32u, 
	34u, 45u, 59u, 91u, 92u, 95u, 123u, 124u, 
	9u, 10u, 40u, 43u, 58u, 60u, 62u, 63u, 
	65u, 90u, 97u, 122u, 13u, 32u, 34u, 45u, 
	59u, 92u, 95u, 123u, 124u, 9u, 10u, 40u, 
	43u, 48u, 57u, 58u, 60u, 62u, 63u, 65u, 
	90u, 97u, 122u, 34u, 44u, 92u, 125u, 48u, 
	57u, 34u, 44u, 92u, 93u, 48u, 57u, 34u, 
	92u, 93u, 125u, 48u, 57u, 13u, 32u, 34u, 
	45u, 59u, 91u, 92u, 93u, 95u, 123u, 124u, 
	9u, 10u, 40u, 43u, 58u, 60u, 62u, 63u, 
	65u, 90u, 97u, 122u, 13u, 32u, 34u, 45u, 
	59u, 92u, 93u, 95u, 123u, 124u, 9u, 10u, 
	40u, 43u, 48u, 57u, 58u, 60u, 62u, 63u, 
	65u, 90u, 97u, 122u, 34u, 44u, 92u, 93u, 
	125u, 48u, 57u, 13u, 32u, 34u, 45u, 59u, 
	92u, 93u, 123u, 124u, 9u, 10u, 40u, 43u, 
	58u, 60u, 62u, 63u, 44u, 125u, 48u, 57u, 
	13u, 32u, 95u, 9u, 10u, 65u, 90u, 97u, 
	122u, 13u, 32u, 95u, 9u, 10u, 65u, 90u, 
	97u, 122u, 13u, 32u, 95u, 9u, 10u, 65u, 
	90u, 97u, 122u, 13u, 32u, 92u, 93u, 95u, 
	9u, 10u, 65u, 90u, 97u, 122u, 13u, 32u, 
	92u, 93u, 95u, 9u, 10u, 65u, 90u, 97u, 
	122u, 13u, 32u, 34u, 92u, 93u, 95u, 9u, 
	10u, 65u, 90u, 97u, 122u, 13u, 32u, 34u, 
	92u, 93u, 95u, 9u, 10u, 65u, 90u, 97u, 
	122u, 13u, 32u, 34u, 92u, 95u, 9u, 10u, 
	65u, 90u, 97u, 122u, 13u, 32u, 34u, 92u, 
	95u, 9u, 10u, 65u, 90u, 97u, 122u, 0
};

static const char _LEX_single_lengths[] = {
	0, 4, 3, 6, 9, 2, 6, 9, 
	7, 1, 1, 9, 2, 2, 8, 11, 
	3, 8, 11, 6, 5, 8, 6, 3, 
	3, 11, 9, 4, 3, 9, 11, 11, 
	7, 6, 9, 8, 10, 8, 10, 6, 
	5, 8, 2, 3, 3, 10, 9, 4, 
	4, 4, 11, 10, 5, 9, 2, 3, 
	3, 3, 5, 5, 6, 6, 5, 5
};

static const char _LEX_range_lengths[] = {
	0, 4, 1, 4, 6, 0, 4, 6, 
	7, 1, 1, 6, 0, 0, 4, 6, 
	0, 4, 6, 4, 1, 4, 4, 1, 
	1, 6, 7, 1, 0, 4, 6, 6, 
	4, 1, 4, 4, 6, 4, 6, 4, 
	1, 4, 0, 1, 1, 6, 7, 1, 
	1, 1, 6, 7, 1, 4, 1, 3, 
	3, 3, 3, 3, 3, 3, 3, 3
};

static const short _LEX_index_offsets[] = {
	0, 0, 9, 14, 25, 41, 44, 55, 
	71, 86, 89, 92, 108, 111, 114, 127, 
	145, 149, 162, 180, 191, 198, 211, 222, 
	227, 232, 250, 267, 273, 277, 291, 309, 
	327, 339, 347, 361, 374, 391, 404, 421, 
	432, 439, 452, 455, 460, 465, 482, 499, 
	505, 511, 517, 535, 553, 560, 574, 578, 
	585, 592, 599, 608, 617, 627, 637, 646
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
	1, 38, 36, 1, 39, 39, 40, 41, 
	42, 44, 43, 45, 41, 39, 41, 41, 
	41, 43, 43, 1, 47, 48, 46, 47, 
	49, 46, 50, 50, 51, 52, 47, 48, 
	53, 51, 50, 51, 51, 51, 46, 54, 
	54, 55, 56, 57, 15, 47, 48, 58, 
	59, 56, 54, 56, 56, 56, 58, 58, 
	46, 61, 62, 63, 60, 64, 64, 65, 
	66, 47, 48, 67, 65, 64, 65, 65, 
	65, 46, 68, 68, 69, 70, 71, 29, 
	47, 48, 72, 73, 70, 68, 70, 70, 
	70, 72, 72, 46, 74, 74, 76, 47, 
	48, 75, 74, 75, 75, 75, 46, 77, 
	77, 78, 47, 48, 77, 46, 79, 79, 
	80, 81, 47, 48, 82, 80, 79, 80, 
	80, 80, 46, 83, 83, 84, 85, 86, 
	84, 83, 84, 84, 84, 1, 87, 47, 
	48, 88, 46, 47, 48, 89, 87, 46, 
	90, 90, 91, 92, 93, 44, 47, 48, 
	94, 95, 92, 90, 92, 92, 92, 94, 
	94, 46, 96, 96, 97, 99, 47, 48, 
	98, 100, 97, 96, 97, 98, 97, 97, 
	98, 98, 46, 87, 47, 48, 89, 88, 
	46, 101, 62, 102, 60, 103, 103, 61, 
	104, 105, 62, 63, 106, 104, 103, 104, 
	104, 104, 60, 107, 107, 108, 109, 110, 
	112, 62, 63, 111, 113, 109, 107, 109, 
	109, 109, 111, 111, 60, 114, 114, 115, 
	116, 117, 119, 62, 63, 118, 120, 116, 
	114, 116, 116, 116, 118, 118, 60, 121, 
	121, 61, 123, 62, 63, 122, 121, 122, 
	122, 122, 60, 124, 124, 61, 125, 62, 
	63, 124, 60, 126, 126, 61, 127, 128, 
	62, 63, 129, 127, 126, 127, 127, 127, 
	60, 130, 130, 18, 131, 132, 19, 133, 
	131, 130, 131, 131, 131, 17, 134, 134, 
	135, 136, 137, 112, 19, 138, 139, 136, 
	134, 136, 136, 136, 138, 138, 17, 140, 
	140, 18, 141, 142, 19, 143, 141, 140, 
	141, 141, 141, 17, 144, 144, 145, 146, 
	147, 119, 19, 148, 149, 146, 144, 146, 
	146, 146, 148, 148, 17, 150, 150, 18, 
	152, 19, 151, 150, 151, 151, 151, 17, 
	153, 153, 18, 154, 19, 153, 17, 155, 
	155, 18, 156, 157, 19, 158, 156, 155, 
	156, 156, 156, 17, 159, 19, 17, 18, 
	160, 19, 161, 17, 18, 19, 162, 160, 
	17, 163, 163, 164, 165, 166, 168, 19, 
	167, 169, 165, 163, 165, 165, 165, 167, 
	167, 17, 170, 170, 18, 171, 173, 19, 
	172, 174, 171, 170, 171, 172, 171, 171, 
	172, 172, 17, 18, 160, 19, 162, 161, 
	17, 61, 175, 62, 63, 176, 60, 61, 
	62, 63, 177, 175, 60, 178, 178, 179, 
	180, 181, 168, 62, 63, 182, 183, 180, 
	178, 180, 180, 180, 182, 182, 60, 184, 
	184, 61, 185, 187, 62, 63, 186, 188, 
	185, 184, 185, 186, 185, 185, 186, 186, 
	60, 61, 175, 62, 63, 177, 176, 60, 
	189, 189, 61, 190, 191, 62, 63, 192, 
	190, 189, 190, 190, 190, 60, 36, 38, 
	37, 1, 193, 193, 194, 193, 194, 194, 
	1, 195, 195, 196, 195, 196, 196, 1, 
	197, 197, 198, 197, 198, 198, 1, 199, 
	199, 47, 48, 200, 199, 200, 200, 46, 
	201, 201, 47, 48, 202, 201, 202, 202, 
	46, 203, 203, 61, 62, 63, 204, 203, 
	204, 204, 60, 205, 205, 61, 62, 63, 
	206, 205, 206, 206, 60, 207, 207, 18, 
	19, 208, 207, 208, 208, 17, 209, 209, 
	18, 19, 210, 209, 210, 210, 17, 0
};

static const char _LEX_trans_targs[] = {
	2, 0, 1, 3, 2, 3, 4, 7, 
	57, 9, 4, 5, 7, 57, 8, 12, 
	9, 5, 6, 42, 4, 7, 57, 9, 
	4, 5, 7, 57, 8, 12, 9, 4, 
	7, 8, 57, 9, 10, 54, 11, 4, 
	5, 7, 57, 8, 12, 9, 12, 13, 
	22, 14, 15, 18, 58, 23, 15, 16, 
	18, 58, 26, 23, 16, 17, 28, 35, 
	15, 18, 58, 23, 15, 16, 18, 58, 
	26, 23, 20, 19, 21, 20, 21, 15, 
	18, 58, 23, 4, 7, 57, 9, 24, 
	27, 25, 15, 16, 18, 58, 26, 23, 
	15, 18, 26, 58, 23, 29, 53, 30, 
	31, 60, 48, 30, 29, 31, 60, 51, 
	16, 48, 30, 29, 31, 60, 51, 16, 
	48, 33, 32, 34, 33, 34, 30, 31, 
	60, 48, 36, 38, 62, 43, 36, 37, 
	38, 62, 46, 43, 36, 38, 62, 43, 
	36, 37, 38, 62, 46, 43, 40, 39, 
	41, 40, 41, 36, 38, 62, 43, 37, 
	44, 47, 45, 36, 37, 38, 62, 46, 
	16, 43, 36, 38, 46, 62, 43, 49, 
	52, 50, 30, 29, 31, 60, 51, 48, 
	30, 31, 51, 60, 48, 30, 31, 60, 
	48, 56, 1, 56, 1, 56, 1, 59, 
	19, 59, 19, 61, 32, 61, 32, 63, 
	39, 63, 39
};

static const unsigned char _LEX_trans_actions[] = {
	140, 0, 0, 41, 3, 1, 190, 89, 
	93, 195, 105, 125, 21, 25, 115, 120, 
	110, 0, 0, 0, 180, 81, 85, 185, 
	53, 69, 9, 12, 61, 65, 57, 130, 
	33, 0, 37, 135, 0, 0, 0, 145, 
	165, 45, 49, 155, 160, 150, 0, 0, 
	0, 0, 170, 73, 77, 175, 105, 125, 
	21, 25, 115, 110, 0, 0, 0, 0, 
	180, 81, 85, 185, 53, 69, 9, 12, 
	61, 57, 140, 0, 41, 3, 1, 190, 
	89, 93, 195, 170, 73, 77, 175, 0, 
	0, 0, 145, 165, 45, 49, 155, 150, 
	130, 33, 0, 37, 135, 0, 0, 180, 
	81, 85, 185, 105, 125, 21, 25, 115, 
	120, 110, 53, 69, 9, 12, 61, 65, 
	57, 140, 0, 41, 3, 1, 190, 89, 
	93, 195, 170, 73, 77, 175, 105, 125, 
	21, 25, 115, 110, 180, 81, 85, 185, 
	53, 69, 9, 12, 61, 57, 140, 0, 
	41, 3, 1, 190, 89, 93, 195, 0, 
	0, 0, 0, 145, 165, 45, 49, 155, 
	160, 150, 130, 33, 0, 37, 135, 0, 
	0, 0, 145, 165, 45, 49, 155, 150, 
	130, 33, 0, 37, 135, 170, 73, 77, 
	175, 101, 205, 3, 29, 97, 200, 97, 
	200, 3, 29, 97, 200, 3, 29, 97, 
	200, 3, 29
};

static const unsigned char _LEX_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 18, 
	6, 15, 15, 6, 15, 6, 15, 6
};

static const int LEX_start = 55;
static const int LEX_first_final = 55;
static const int LEX_error = 0;

static const int LEX_en_main = 55;


#line 113 "LEX.rl"

pro(LEXlexer, LEXstate* state) {
    LEXbase* lex = &(state->lex);

    a$dup(u8c, text, lex->text);
    sane($ok(text));

    int cs = lex->cs;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = pe;
    u8c *pb = p;

    u32 sp = 2;
    $u8c tok = {p, p};

    
#line 363 "LEX.rl.c"
	{
	cs = LEX_start;
	}

#line 131 "LEX.rl"
    
#line 366 "LEX.rl.c"
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
#line 10 "LEX.rl"
	{ lex->mark0[LEXSpace] = p - lex->text[0]; }
	break;
	case 1:
#line 11 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXSpace];
    tok[1] = p;
    call(LEXonSpace, tok, state); 
}
	break;
	case 2:
#line 16 "LEX.rl"
	{ lex->mark0[LEXName] = p - lex->text[0]; }
	break;
	case 3:
#line 17 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXName];
    tok[1] = p;
    call(LEXonName, tok, state); 
}
	break;
	case 4:
#line 22 "LEX.rl"
	{ lex->mark0[LEXRep] = p - lex->text[0]; }
	break;
	case 5:
#line 23 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXRep];
    tok[1] = p;
    call(LEXonRep, tok, state); 
}
	break;
	case 6:
#line 28 "LEX.rl"
	{ lex->mark0[LEXOp] = p - lex->text[0]; }
	break;
	case 7:
#line 29 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXOp];
    tok[1] = p;
    call(LEXonOp, tok, state); 
}
	break;
	case 8:
#line 34 "LEX.rl"
	{ lex->mark0[LEXClass] = p - lex->text[0]; }
	break;
	case 9:
#line 35 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXClass];
    tok[1] = p;
    call(LEXonClass, tok, state); 
}
	break;
	case 10:
#line 40 "LEX.rl"
	{ lex->mark0[LEXString] = p - lex->text[0]; }
	break;
	case 11:
#line 41 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXString];
    tok[1] = p;
    call(LEXonString, tok, state); 
}
	break;
	case 12:
#line 46 "LEX.rl"
	{ lex->mark0[LEXEntity] = p - lex->text[0]; }
	break;
	case 13:
#line 47 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXEntity];
    tok[1] = p;
    call(LEXonEntity, tok, state); 
}
	break;
	case 14:
#line 52 "LEX.rl"
	{ lex->mark0[LEXExpr] = p - lex->text[0]; }
	break;
	case 15:
#line 53 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXExpr];
    tok[1] = p;
    call(LEXonExpr, tok, state); 
}
	break;
	case 16:
#line 58 "LEX.rl"
	{ lex->mark0[LEXRuleName] = p - lex->text[0]; }
	break;
	case 17:
#line 59 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXRuleName];
    tok[1] = p;
    call(LEXonRuleName, tok, state); 
}
	break;
	case 18:
#line 64 "LEX.rl"
	{ lex->mark0[LEXEq] = p - lex->text[0]; }
	break;
	case 19:
#line 65 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXEq];
    tok[1] = p;
    call(LEXonEq, tok, state); 
}
	break;
	case 20:
#line 70 "LEX.rl"
	{ lex->mark0[LEXLine] = p - lex->text[0]; }
	break;
	case 21:
#line 71 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXLine];
    tok[1] = p;
    call(LEXonLine, tok, state); 
}
	break;
	case 22:
#line 76 "LEX.rl"
	{ lex->mark0[LEXRoot] = p - lex->text[0]; }
	break;
#line 552 "LEX.rl.c"
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
#line 11 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXSpace];
    tok[1] = p;
    call(LEXonSpace, tok, state); 
}
	break;
	case 21:
#line 71 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXLine];
    tok[1] = p;
    call(LEXonLine, tok, state); 
}
	break;
	case 22:
#line 76 "LEX.rl"
	{ lex->mark0[LEXRoot] = p - lex->text[0]; }
	break;
	case 23:
#line 77 "LEX.rl"
	{
    tok[0] = lex->text[0] + lex->mark0[LEXRoot];
    tok[1] = p;
    call(LEXonRoot, tok, state); 
}
	break;
#line 591 "LEX.rl.c"
		}
	}
	}

	_out: {}
	}

#line 132 "LEX.rl"

    test(p==text[1], LEXfail);

    test(cs >= LEX_first_final, LEXfail);

    nedo(
        lex->text[0] = p;
    );
}
