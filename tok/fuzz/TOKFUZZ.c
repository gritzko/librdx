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

#include "abc/TEST.h"

static ok64 TOKFUZZcb(u8 tag, u8cs tok, void *ctx) {
    must(tag == 'D' || tag == 'G' || tag == 'L' || tag == 'H' ||
         tag == 'R' || tag == 'S' || tag == 'P', "bad tag");
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

    #undef FUZZ_LEXER

    done;
}
