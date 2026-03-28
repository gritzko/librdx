
/* #line 1 "RST.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "RST.h"

ok64 RSTonComment (u8cs tok, RSTstate* state);
ok64 RSTonString (u8cs tok, RSTstate* state);
ok64 RSTonNumber (u8cs tok, RSTstate* state);
ok64 RSTonAttr (u8cs tok, RSTstate* state);
ok64 RSTonWord (u8cs tok, RSTstate* state);
ok64 RSTonPunct (u8cs tok, RSTstate* state);
ok64 RSTonSpace (u8cs tok, RSTstate* state);


/* #line 145 "RST.c.rl" */



/* #line 16 "RST.rl.c" */
static const char _RST_actions[] = {
	0, 1, 2, 1, 3, 1, 12, 1, 
	13, 1, 14, 1, 15, 1, 16, 1, 
	17, 1, 18, 1, 19, 1, 20, 1, 
	21, 1, 22, 1, 23, 1, 24, 1, 
	25, 1, 26, 1, 27, 1, 28, 1, 
	29, 1, 30, 1, 31, 1, 32, 1, 
	33, 1, 34, 1, 35, 1, 36, 1, 
	37, 1, 38, 1, 39, 1, 40, 1, 
	41, 1, 42, 1, 43, 1, 44, 1, 
	45, 1, 46, 1, 47, 1, 48, 1, 
	49, 1, 50, 1, 51, 2, 0, 1, 
	2, 3, 4, 2, 3, 5, 2, 3, 
	6, 2, 3, 7, 2, 3, 8, 2, 
	3, 9, 2, 3, 10, 2, 3, 11
	
};

static const short _RST_key_offsets[] = {
	0, 0, 2, 17, 18, 24, 31, 38, 
	45, 52, 59, 60, 66, 72, 73, 74, 
	79, 86, 87, 102, 103, 109, 116, 123, 
	130, 137, 144, 145, 151, 157, 158, 160, 
	164, 166, 171, 173, 174, 175, 176, 177, 
	178, 179, 181, 185, 187, 192, 194, 195, 
	196, 197, 198, 199, 200, 202, 207, 209, 
	210, 211, 212, 213, 214, 215, 217, 222, 
	224, 225, 226, 227, 228, 229, 230, 232, 
	237, 239, 240, 241, 242, 243, 244, 245, 
	251, 256, 258, 259, 260, 261, 262, 263, 
	264, 266, 281, 282, 288, 295, 302, 309, 
	316, 323, 324, 330, 336, 338, 339, 341, 
	342, 343, 345, 346, 347, 348, 349, 350, 
	351, 352, 353, 384, 387, 388, 395, 397, 
	405, 412, 414, 415, 418, 421, 422, 423, 
	424, 439, 446, 454, 460, 469, 475, 481, 
	487, 497, 508, 509, 511, 513, 520, 525, 
	535, 544
};

static const unsigned char _RST_trans_keys[] = {
	34u, 92u, 10u, 34u, 39u, 48u, 92u, 110u, 
	114u, 117u, 120u, 97u, 98u, 101u, 102u, 116u, 
	118u, 123u, 48u, 57u, 65u, 70u, 97u, 102u, 
	125u, 48u, 57u, 65u, 70u, 97u, 102u, 125u, 
	48u, 57u, 65u, 70u, 97u, 102u, 125u, 48u, 
	57u, 65u, 70u, 97u, 102u, 125u, 48u, 57u, 
	65u, 70u, 97u, 102u, 125u, 48u, 57u, 65u, 
	70u, 97u, 102u, 125u, 48u, 57u, 65u, 70u, 
	97u, 102u, 48u, 57u, 65u, 70u, 97u, 102u, 
	33u, 91u, 95u, 65u, 90u, 97u, 122u, 39u, 
	92u, 95u, 65u, 90u, 97u, 122u, 39u, 10u, 
	34u, 39u, 48u, 92u, 110u, 114u, 117u, 120u, 
	97u, 98u, 101u, 102u, 116u, 118u, 123u, 48u, 
	57u, 65u, 70u, 97u, 102u, 125u, 48u, 57u, 
	65u, 70u, 97u, 102u, 125u, 48u, 57u, 65u, 
	70u, 97u, 102u, 125u, 48u, 57u, 65u, 70u, 
	97u, 102u, 125u, 48u, 57u, 65u, 70u, 97u, 
	102u, 125u, 48u, 57u, 65u, 70u, 97u, 102u, 
	125u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 42u, 42u, 47u, 
	43u, 45u, 48u, 57u, 48u, 57u, 49u, 51u, 
	54u, 56u, 115u, 50u, 54u, 56u, 50u, 52u, 
	105u, 122u, 101u, 48u, 57u, 43u, 45u, 48u, 
	57u, 48u, 57u, 49u, 51u, 54u, 56u, 115u, 
	50u, 54u, 56u, 50u, 52u, 105u, 122u, 101u, 
	48u, 57u, 49u, 51u, 54u, 56u, 115u, 50u, 
	54u, 56u, 50u, 52u, 105u, 122u, 101u, 48u, 
	49u, 49u, 51u, 54u, 56u, 115u, 50u, 54u, 
	56u, 50u, 52u, 105u, 122u, 101u, 48u, 55u, 
	49u, 51u, 54u, 56u, 115u, 50u, 54u, 56u, 
	50u, 52u, 105u, 122u, 101u, 48u, 57u, 65u, 
	70u, 97u, 102u, 49u, 51u, 54u, 56u, 115u, 
	50u, 54u, 56u, 50u, 52u, 105u, 122u, 101u, 
	34u, 92u, 10u, 34u, 39u, 48u, 92u, 110u, 
	114u, 117u, 120u, 97u, 98u, 101u, 102u, 116u, 
	118u, 123u, 48u, 57u, 65u, 70u, 97u, 102u, 
	125u, 48u, 57u, 65u, 70u, 97u, 102u, 125u, 
	48u, 57u, 65u, 70u, 97u, 102u, 125u, 48u, 
	57u, 65u, 70u, 97u, 102u, 125u, 48u, 57u, 
	65u, 70u, 97u, 102u, 125u, 48u, 57u, 65u, 
	70u, 97u, 102u, 125u, 48u, 57u, 65u, 70u, 
	97u, 102u, 48u, 57u, 65u, 70u, 97u, 102u, 
	39u, 92u, 34u, 34u, 35u, 34u, 35u, 34u, 
	35u, 34u, 35u, 35u, 34u, 34u, 35u, 35u, 
	35u, 32u, 33u, 34u, 35u, 37u, 38u, 39u, 
	45u, 46u, 47u, 48u, 58u, 60u, 61u, 62u, 
	91u, 94u, 95u, 98u, 114u, 124u, 9u, 13u, 
	42u, 43u, 49u, 57u, 65u, 90u, 97u, 122u, 
	32u, 9u, 13u, 61u, 95u, 48u, 57u, 65u, 
	90u, 97u, 122u, 38u, 61u, 39u, 95u, 48u, 
	57u, 65u, 90u, 97u, 122u, 95u, 48u, 57u, 
	65u, 90u, 97u, 122u, 61u, 62u, 46u, 42u, 
	47u, 61u, 10u, 33u, 47u, 10u, 10u, 10u, 
	46u, 66u, 69u, 79u, 88u, 95u, 98u, 101u, 
	102u, 105u, 111u, 117u, 120u, 48u, 57u, 69u, 
	101u, 102u, 105u, 117u, 48u, 57u, 69u, 95u, 
	101u, 102u, 105u, 117u, 48u, 57u, 95u, 102u, 
	105u, 117u, 48u, 57u, 46u, 69u, 95u, 101u, 
	102u, 105u, 117u, 48u, 57u, 95u, 102u, 105u, 
	117u, 48u, 57u, 95u, 102u, 105u, 117u, 48u, 
	49u, 95u, 102u, 105u, 117u, 48u, 55u, 95u, 
	102u, 105u, 117u, 48u, 57u, 65u, 70u, 97u, 
	101u, 95u, 102u, 105u, 115u, 117u, 48u, 57u, 
	65u, 70u, 97u, 101u, 58u, 60u, 61u, 61u, 
	62u, 95u, 48u, 57u, 65u, 90u, 97u, 122u, 
	95u, 65u, 90u, 97u, 122u, 34u, 39u, 95u, 
	114u, 48u, 57u, 65u, 90u, 97u, 122u, 34u, 
	35u, 95u, 48u, 57u, 65u, 90u, 97u, 122u, 
	61u, 124u, 0
};

static const char _RST_single_lengths[] = {
	0, 2, 9, 1, 0, 1, 1, 1, 
	1, 1, 1, 0, 0, 1, 1, 1, 
	3, 1, 9, 1, 0, 1, 1, 1, 
	1, 1, 1, 0, 0, 1, 2, 2, 
	0, 5, 2, 1, 1, 1, 1, 1, 
	1, 0, 2, 0, 5, 2, 1, 1, 
	1, 1, 1, 1, 0, 5, 2, 1, 
	1, 1, 1, 1, 1, 0, 5, 2, 
	1, 1, 1, 1, 1, 1, 0, 5, 
	2, 1, 1, 1, 1, 1, 1, 0, 
	5, 2, 1, 1, 1, 1, 1, 1, 
	2, 9, 1, 0, 1, 1, 1, 1, 
	1, 1, 0, 0, 2, 1, 2, 1, 
	1, 2, 1, 1, 1, 1, 1, 1, 
	1, 1, 21, 1, 1, 1, 2, 2, 
	1, 0, 1, 3, 3, 1, 1, 1, 
	13, 5, 6, 4, 7, 4, 4, 4, 
	4, 5, 1, 2, 2, 1, 1, 4, 
	3, 2
};

static const char _RST_range_lengths[] = {
	0, 0, 3, 0, 3, 3, 3, 3, 
	3, 3, 0, 3, 3, 0, 0, 2, 
	2, 0, 3, 0, 3, 3, 3, 3, 
	3, 3, 0, 3, 3, 0, 0, 1, 
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 1, 1, 0, 0, 0, 0, 
	0, 0, 0, 0, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 0, 0, 
	0, 0, 0, 0, 0, 0, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 3, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 3, 0, 3, 3, 3, 3, 3, 
	3, 0, 3, 3, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 5, 1, 0, 3, 0, 3, 
	3, 1, 0, 0, 0, 0, 0, 0, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	3, 3, 0, 0, 0, 3, 2, 3, 
	3, 0
};

static const short _RST_index_offsets[] = {
	0, 0, 3, 16, 18, 22, 27, 32, 
	37, 42, 47, 49, 53, 57, 59, 61, 
	65, 71, 73, 86, 88, 92, 97, 102, 
	107, 112, 117, 119, 123, 127, 129, 132, 
	136, 138, 144, 147, 149, 151, 153, 155, 
	157, 159, 161, 165, 167, 173, 176, 178, 
	180, 182, 184, 186, 188, 190, 196, 199, 
	201, 203, 205, 207, 209, 211, 213, 219, 
	222, 224, 226, 228, 230, 232, 234, 236, 
	242, 245, 247, 249, 251, 253, 255, 257, 
	261, 267, 270, 272, 274, 276, 278, 280, 
	282, 285, 298, 300, 304, 309, 314, 319, 
	324, 329, 331, 335, 339, 342, 344, 347, 
	349, 351, 354, 356, 358, 360, 362, 364, 
	366, 368, 370, 397, 400, 402, 407, 410, 
	416, 421, 423, 425, 429, 433, 435, 437, 
	439, 454, 461, 469, 475, 484, 490, 496, 
	502, 510, 519, 521, 524, 527, 532, 536, 
	544, 551
};

static const unsigned char _RST_indicies[] = {
	1, 2, 0, 0, 0, 0, 0, 0, 
	0, 0, 4, 5, 0, 0, 0, 3, 
	6, 3, 7, 7, 7, 3, 0, 8, 
	8, 8, 3, 0, 9, 9, 9, 3, 
	0, 10, 10, 10, 3, 0, 11, 11, 
	11, 3, 0, 12, 12, 12, 3, 0, 
	3, 13, 13, 13, 3, 0, 0, 0, 
	3, 14, 3, 15, 3, 16, 16, 16, 
	3, 3, 19, 18, 18, 18, 17, 21, 
	20, 17, 17, 17, 17, 17, 17, 17, 
	22, 23, 17, 17, 17, 20, 24, 20, 
	25, 25, 25, 20, 17, 26, 26, 26, 
	20, 17, 27, 27, 27, 20, 17, 28, 
	28, 28, 20, 17, 29, 29, 29, 20, 
	17, 30, 30, 30, 20, 17, 20, 31, 
	31, 31, 20, 17, 17, 17, 20, 34, 
	33, 34, 35, 33, 37, 37, 38, 36, 
	38, 36, 39, 40, 41, 42, 43, 36, 
	44, 42, 36, 42, 36, 42, 36, 42, 
	36, 45, 36, 46, 36, 42, 36, 47, 
	36, 49, 49, 50, 48, 50, 20, 52, 
	53, 54, 55, 56, 51, 57, 55, 51, 
	55, 51, 55, 51, 55, 51, 58, 51, 
	59, 51, 55, 51, 60, 48, 61, 62, 
	63, 64, 65, 48, 66, 64, 48, 64, 
	48, 64, 48, 64, 48, 67, 48, 68, 
	48, 64, 48, 69, 20, 71, 72, 73, 
	74, 75, 70, 76, 74, 70, 74, 70, 
	74, 70, 74, 70, 77, 70, 78, 70, 
	74, 70, 79, 20, 81, 82, 83, 84, 
	85, 80, 86, 84, 80, 84, 80, 84, 
	80, 84, 80, 87, 80, 88, 80, 84, 
	80, 89, 89, 89, 20, 91, 92, 93, 
	94, 95, 90, 96, 94, 90, 94, 90, 
	94, 90, 94, 90, 97, 90, 98, 90, 
	94, 90, 101, 102, 100, 100, 100, 100, 
	100, 100, 100, 100, 103, 104, 100, 100, 
	100, 99, 105, 99, 106, 106, 106, 99, 
	100, 107, 107, 107, 99, 100, 108, 108, 
	108, 99, 100, 109, 109, 109, 99, 100, 
	110, 110, 110, 99, 100, 111, 111, 111, 
	99, 100, 99, 112, 112, 112, 99, 100, 
	100, 100, 99, 99, 19, 17, 114, 113, 
	115, 116, 99, 117, 115, 118, 115, 119, 
	120, 99, 121, 119, 122, 119, 123, 119, 
	124, 99, 125, 124, 126, 124, 127, 124, 
	128, 124, 130, 131, 0, 132, 131, 133, 
	134, 135, 136, 137, 138, 139, 140, 135, 
	141, 143, 131, 142, 144, 145, 146, 130, 
	131, 60, 142, 142, 129, 130, 130, 147, 
	148, 20, 16, 16, 16, 16, 149, 148, 
	148, 150, 21, 152, 152, 152, 152, 151, 
	152, 152, 152, 152, 151, 148, 150, 154, 
	153, 33, 155, 148, 150, 156, 158, 159, 
	157, 156, 157, 160, 158, 161, 159, 163, 
	164, 165, 166, 167, 168, 164, 165, 169, 
	169, 166, 169, 167, 60, 162, 171, 171, 
	172, 172, 172, 47, 170, 171, 173, 171, 
	172, 172, 172, 47, 170, 37, 172, 172, 
	172, 38, 170, 163, 165, 168, 165, 169, 
	169, 169, 60, 162, 49, 175, 175, 175, 
	50, 174, 164, 177, 177, 177, 69, 176, 
	166, 179, 179, 179, 79, 178, 167, 181, 
	182, 182, 89, 89, 89, 180, 167, 181, 
	182, 95, 182, 89, 89, 89, 180, 148, 
	150, 154, 148, 150, 148, 154, 150, 142, 
	142, 142, 142, 183, 16, 16, 16, 150, 
	100, 184, 142, 145, 142, 142, 142, 183, 
	113, 185, 142, 142, 142, 142, 183, 148, 
	148, 150, 0
};

static const unsigned char _RST_trans_targs[] = {
	1, 114, 2, 0, 3, 11, 4, 5, 
	6, 7, 8, 9, 10, 12, 14, 15, 
	117, 17, 119, 18, 114, 114, 19, 27, 
	20, 21, 22, 23, 24, 25, 26, 28, 
	114, 29, 30, 114, 114, 32, 131, 34, 
	36, 37, 114, 38, 35, 39, 40, 130, 
	114, 43, 133, 114, 45, 47, 48, 114, 
	49, 46, 50, 51, 132, 54, 56, 57, 
	114, 58, 55, 59, 60, 134, 114, 63, 
	65, 66, 114, 67, 64, 68, 69, 135, 
	114, 72, 74, 75, 114, 76, 73, 77, 
	78, 136, 114, 81, 83, 84, 114, 85, 
	82, 86, 87, 114, 88, 114, 89, 90, 
	98, 91, 92, 93, 94, 95, 96, 97, 
	99, 101, 114, 103, 105, 104, 114, 106, 
	109, 107, 108, 114, 110, 111, 112, 113, 
	114, 114, 115, 116, 13, 118, 16, 121, 
	122, 123, 128, 138, 139, 140, 141, 142, 
	143, 144, 145, 114, 114, 114, 114, 114, 
	120, 114, 116, 124, 114, 125, 126, 127, 
	114, 114, 114, 129, 61, 42, 70, 79, 
	52, 53, 114, 31, 33, 41, 114, 44, 
	114, 62, 114, 71, 114, 137, 80, 114, 
	100, 102
};

static const char _RST_trans_actions[] = {
	0, 17, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 83, 19, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	81, 0, 0, 5, 73, 0, 3, 0, 
	0, 0, 27, 0, 0, 0, 0, 3, 
	77, 0, 97, 75, 0, 0, 0, 29, 
	0, 0, 0, 0, 100, 0, 0, 0, 
	31, 0, 0, 0, 0, 94, 71, 0, 
	0, 0, 25, 0, 0, 0, 0, 91, 
	69, 0, 0, 0, 23, 0, 0, 0, 
	0, 88, 67, 0, 0, 0, 21, 0, 
	0, 0, 0, 79, 0, 15, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 13, 0, 0, 0, 11, 0, 
	0, 0, 0, 9, 0, 0, 0, 0, 
	7, 35, 0, 109, 0, 0, 0, 0, 
	0, 3, 100, 0, 0, 0, 0, 0, 
	103, 3, 0, 65, 33, 57, 61, 43, 
	0, 63, 106, 0, 41, 0, 0, 0, 
	39, 37, 55, 3, 0, 0, 0, 0, 
	0, 0, 51, 0, 0, 0, 53, 0, 
	49, 0, 47, 0, 45, 88, 0, 59, 
	0, 0
};

static const char _RST_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 85, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const char _RST_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 1, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const short _RST_eof_trans[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 33, 33, 37, 
	37, 37, 37, 37, 37, 37, 37, 37, 
	37, 37, 49, 21, 52, 52, 52, 52, 
	52, 52, 52, 52, 49, 49, 49, 49, 
	49, 49, 49, 49, 49, 21, 71, 71, 
	71, 71, 71, 71, 71, 71, 21, 81, 
	81, 81, 81, 81, 81, 81, 81, 21, 
	91, 91, 91, 91, 91, 91, 91, 91, 
	100, 100, 100, 100, 100, 100, 100, 100, 
	100, 100, 100, 100, 100, 100, 100, 100, 
	100, 100, 100, 100, 100, 100, 100, 100, 
	100, 100, 0, 148, 21, 150, 151, 152, 
	152, 151, 154, 151, 157, 157, 161, 162, 
	163, 171, 171, 171, 163, 175, 177, 179, 
	181, 181, 151, 151, 151, 184, 151, 184, 
	184, 151
};

static const int RST_start = 114;
static const int RST_first_final = 114;
static const int RST_error = 0;

static const int RST_en_main = 114;


/* #line 148 "RST.c.rl" */

ok64 RSTLexer(RSTstate* state) {

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

    
/* #line 410 "RST.rl.c" */
	{
	cs = RST_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 166 "RST.c.rl" */
    
/* #line 416 "RST.rl.c" */
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
	_acts = _RST_actions + _RST_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 2:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 435 "RST.rl.c" */
		}
	}

	_keys = _RST_trans_keys + _RST_key_offsets[cs];
	_trans = _RST_index_offsets[cs];

	_klen = _RST_single_lengths[cs];
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

	_klen = _RST_range_lengths[cs];
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
	_trans = _RST_indicies[_trans];
_eof_trans:
	cs = _RST_trans_targs[_trans];

	if ( _RST_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _RST_actions + _RST_trans_actions[_trans];
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
/* #line 44 "RST.c.rl" */
	{act = 13;}
	break;
	case 5:
/* #line 44 "RST.c.rl" */
	{act = 14;}
	break;
	case 6:
/* #line 44 "RST.c.rl" */
	{act = 15;}
	break;
	case 7:
/* #line 44 "RST.c.rl" */
	{act = 17;}
	break;
	case 8:
/* #line 44 "RST.c.rl" */
	{act = 18;}
	break;
	case 9:
/* #line 56 "RST.c.rl" */
	{act = 20;}
	break;
	case 10:
/* #line 62 "RST.c.rl" */
	{act = 21;}
	break;
	case 11:
/* #line 62 "RST.c.rl" */
	{act = 22;}
	break;
	case 12:
/* #line 32 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 13:
/* #line 38 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 14:
/* #line 38 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 15:
/* #line 38 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 16:
/* #line 38 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 17:
/* #line 38 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 38 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 19:
/* #line 38 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 20:
/* #line 44 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 21:
/* #line 44 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 22:
/* #line 44 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 23:
/* #line 44 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 24:
/* #line 44 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 25:
/* #line 44 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 26:
/* #line 62 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 27:
/* #line 62 "RST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 28:
/* #line 32 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 29:
/* #line 32 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 30:
/* #line 32 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 31:
/* #line 56 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 32:
/* #line 44 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 33:
/* #line 44 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 34:
/* #line 44 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 35:
/* #line 44 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 36:
/* #line 44 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 37:
/* #line 44 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 38:
/* #line 50 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonAttr(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 39:
/* #line 56 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 40:
/* #line 62 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 41:
/* #line 62 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 42:
/* #line 68 "RST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 43:
/* #line 44 "RST.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 44:
/* #line 44 "RST.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 45:
/* #line 44 "RST.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 46:
/* #line 44 "RST.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 47:
/* #line 44 "RST.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 48:
/* #line 44 "RST.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 49:
/* #line 56 "RST.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 50:
/* #line 62 "RST.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 51:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 0:
	{{cs = 0;goto _again;}}
	break;
	case 13:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 14:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 15:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 17:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 18:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 20:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 21:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 22:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = RSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	}
	}
	break;
/* #line 911 "RST.rl.c" */
		}
	}

_again:
	_acts = _RST_actions + _RST_to_state_actions[cs];
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
/* #line 925 "RST.rl.c" */
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _RST_eof_trans[cs] > 0 ) {
		_trans = _RST_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 167 "RST.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < RST_first_final)
        o = RSTBAD;

    return o;
}
