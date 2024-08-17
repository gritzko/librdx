#include "PRO.h"
#include "INT.h"
#include "LEX.h"

enum {
	LEXSpace = LEXenum+1,
	LEXName = LEXenum+2,
	LEXRep = LEXenum+3,
	LEXOp = LEXenum+4,
	LEXClass = LEXenum+5,
	LEXString = LEXenum+6,
	LEXEntity = LEXenum+7,
	LEXExpr = LEXenum+8,
	LEXRuleName = LEXenum+9,
	LEXEq = LEXenum+10,
	LEXLine = LEXenum+11,
	LEXRoot = LEXenum+12,
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


