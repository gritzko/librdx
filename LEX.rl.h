#include "PRO.h"
#include "INT.h"
#include "LEX.h"
#define LEXmaxnest 1024

enum {
	LEX = 0,
	LEXSpace = LEX+1,
	LEXName = LEX+2,
	LEXRep = LEX+3,
	LEXOp = LEX+4,
	LEXClass = LEX+5,
	LEXString = LEX+6,
	LEXEntity = LEX+7,
	LEXExpr = LEX+8,
	LEXRuleName = LEX+9,
	LEXEq = LEX+10,
	LEXLine = LEX+11,
	LEXRoot = LEX+12,
};
ok64 LEXonSpace ($cu8c tok, LEXstate* state);
ok64 LEXonName ($cu8c tok, LEXstate* state);
ok64 LEXonRep ($cu8c tok, LEXstate* state);
ok64 LEXonOp ($cu8c tok, LEXstate* state);
ok64 LEXonClass ($cu8c tok, LEXstate* state);
ok64 LEXonString ($cu8c tok, LEXstate* state);
ok64 LEXonEntity ($cu8c tok, LEXstate* state);
ok64 LEXonExpr ($cu8c tok, LEXstate* state);
ok64 LEXonRuleName ($cu8c tok, LEXstate* state);
ok64 LEXonEq ($cu8c tok, LEXstate* state);
ok64 LEXonLine ($cu8c tok, LEXstate* state);
ok64 LEXonRoot ($cu8c tok, LEXstate* state);


