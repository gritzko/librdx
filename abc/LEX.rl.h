#include "abc/INT.h"
#include "abc/PRO.h"
#include "LEX.h"

enum {
	LEXSpace = LEXenum+1,
	LEXName = LEXenum+4,
	LEXRep = LEXenum+5,
	LEXOp = LEXenum+6,
	LEXClass = LEXenum+7,
	LEXRange = LEXenum+8,
	LEXString = LEXenum+9,
	LEXQString = LEXenum+10,
	LEXEntity = LEXenum+11,
	LEXExpr = LEXenum+12,
	LEXRuleName = LEXenum+13,
	LEXEq = LEXenum+14,
	LEXLine = LEXenum+15,
	LEXRoot = LEXenum+16,
};
ok64 LEXonSpace ($cu8c tok, LEXstate* state);
ok64 LEXonName ($cu8c tok, LEXstate* state);
ok64 LEXonRep ($cu8c tok, LEXstate* state);
ok64 LEXonOp ($cu8c tok, LEXstate* state);
ok64 LEXonClass ($cu8c tok, LEXstate* state);
ok64 LEXonRange ($cu8c tok, LEXstate* state);
ok64 LEXonString ($cu8c tok, LEXstate* state);
ok64 LEXonQString ($cu8c tok, LEXstate* state);
ok64 LEXonEntity ($cu8c tok, LEXstate* state);
ok64 LEXonExpr ($cu8c tok, LEXstate* state);
ok64 LEXonRuleName ($cu8c tok, LEXstate* state);
ok64 LEXonEq ($cu8c tok, LEXstate* state);
ok64 LEXonLine ($cu8c tok, LEXstate* state);
ok64 LEXonRoot ($cu8c tok, LEXstate* state);


