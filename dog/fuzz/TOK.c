#include "CT.h"
#include "GOT.h"
#include "PYT.h"
#include "JST.h"
#include "RST.h"
#include "JAT.h"
#include "CPPT.h"
#include "CST.h"
#include "HTMT.h"
#include "CSST.h"
#include "JSONT.h"
#include "SHT.h"
#include "RBT.h"
#include "HST.h"
#include "MLT.h"
#include "JLT.h"
#include "PHPT.h"
#include "AGDT.h"
#include "VERT.h"
#include "TST.h"
#include "KTT.h"
#include "SCLT.h"
#include "SWFT.h"
#include "DARTT.h"
#include "ZIGT.h"
#include "DT.h"
#include "LUAT.h"
#include "PRLT.h"
#include "RT.h"
#include "ELXT.h"
#include "ERLT.h"
#include "NIMT.h"
#include "NIXT.h"
#include "VIMT.h"
#include "YMLT.h"
#include "TOMLT.h"
#include "SQLT.h"
#include "GQLT.h"
#include "PRTT.h"
#include "HCLT.h"
#include "SCSST.h"
#include "LAXT.h"
#include "CLJT.h"
#include "CMKT.h"
#include "DKFT.h"
#include "FORT.h"
#include "FSHT.h"
#include "GLMT.h"
#include "GLST.h"
#include "MAKT.h"
#include "ODNT.h"
#include "PWST.h"
#include "SOLT.h"
#include "TYST.h"

#include "abc/TEST.h"

//  Canonical emit-tag alphabet (see dog/TOK.h): every dogenizer
//  emits tokens tagged with one of
//      D  comment / docstring (split via TOKSplitText)
//      G  string literal
//      H  preprocessor / heading line
//      L  number literal
//      N  name (symbol in heading/definition context, e.g.
//         MDTHeadingCb remaps S → N)
//      P  punctuation
//      R  keyword (reserved word)
//      S  symbol / identifier
//      W  whitespace (emitted by *onSpace callbacks)
static ok64 TOKFUZZcb(u8 tag, u8cs tok, void *ctx) {
    (void)ctx;
    must(tag == 'D' || tag == 'G' || tag == 'H' || tag == 'L' ||
         tag == 'N' || tag == 'P' || tag == 'R' || tag == 'S' ||
         tag == 'W',
         "bad tag");
    must($ok(tok), "bad tok");
    must(!$empty(tok), "empty tok");
    return OK;
}

FUZZ(u8, TOKfuzz) {
    sane(1);

    if ($len(input) > 4096) done;

    #define FUZZ_LEXER(TYPE, LEXER) do { \
        TYPE##state st = { \
            .data = {input[0], input[1]}, \
            .cb = TOKFUZZcb, \
            .ctx = NULL, \
        }; \
        (void)LEXER(&st); \
    } while(0)

    FUZZ_LEXER(CT, CTLexer);
    FUZZ_LEXER(GOT, GOTLexer);
    FUZZ_LEXER(PYT, PYTLexer);
    FUZZ_LEXER(JST, JSTLexer);
    FUZZ_LEXER(RST, RSTLexer);
    FUZZ_LEXER(JAT, JATLexer);
    FUZZ_LEXER(CPPT, CPPTLexer);
    FUZZ_LEXER(CST, CSTLexer);
    FUZZ_LEXER(HTMT, HTMTLexer);
    FUZZ_LEXER(CSST, CSSTLexer);
    FUZZ_LEXER(JSONT, JSONTLexer);
    FUZZ_LEXER(SHT, SHTLexer);
    FUZZ_LEXER(RBT, RBTLexer);
    FUZZ_LEXER(HST, HSTLexer);
    FUZZ_LEXER(MLT, MLTLexer);
    FUZZ_LEXER(JLT, JLTLexer);
    FUZZ_LEXER(PHPT, PHPTLexer);
    FUZZ_LEXER(AGDT, AGDTLexer);
    FUZZ_LEXER(VERT, VERTLexer);
    FUZZ_LEXER(TST, TSTLexer);
    FUZZ_LEXER(KTT, KTTLexer);
    FUZZ_LEXER(SCLT, SCLTLexer);
    FUZZ_LEXER(SWFT, SWFTLexer);
    FUZZ_LEXER(DARTT, DARTTLexer);
    FUZZ_LEXER(ZIGT, ZIGTLexer);
    FUZZ_LEXER(DT, DTLexer);
    FUZZ_LEXER(LUAT, LUATLexer);
    FUZZ_LEXER(PRLT, PRLTLexer);
    FUZZ_LEXER(RT, RTLexer);
    FUZZ_LEXER(ELXT, ELXTLexer);
    FUZZ_LEXER(ERLT, ERLTLexer);
    FUZZ_LEXER(NIMT, NIMTLexer);
    FUZZ_LEXER(NIXT, NIXTLexer);
    FUZZ_LEXER(VIMT, VIMTLexer);
    FUZZ_LEXER(YMLT, YMLTLexer);
    FUZZ_LEXER(TOMLT, TOMLTLexer);
    FUZZ_LEXER(SQLT, SQLTLexer);
    FUZZ_LEXER(GQLT, GQLTLexer);
    FUZZ_LEXER(PRTT, PRTTLexer);
    FUZZ_LEXER(HCLT, HCLTLexer);
    FUZZ_LEXER(SCSST, SCSSTLexer);
    FUZZ_LEXER(LAXT, LAXTLexer);
    FUZZ_LEXER(CLJT, CLJTLexer);
    FUZZ_LEXER(CMKT, CMKTLexer);
    FUZZ_LEXER(DKFT, DKFTLexer);
    FUZZ_LEXER(FORT, FORTLexer);
    FUZZ_LEXER(FSHT, FSHTLexer);
    FUZZ_LEXER(GLMT, GLMTLexer);
    FUZZ_LEXER(GLST, GLSTLexer);
    FUZZ_LEXER(MAKT, MAKTLexer);
    FUZZ_LEXER(ODNT, ODNTLexer);
    FUZZ_LEXER(PWST, PWSTLexer);
    FUZZ_LEXER(SOLT, SOLTLexer);
    FUZZ_LEXER(TYST, TYSTLexer);

    #undef FUZZ_LEXER

    done;
}
