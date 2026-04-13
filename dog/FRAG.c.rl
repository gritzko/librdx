#include "FRAG.h"
#include <string.h>

%%{
    machine frag;

    action mark       { mark = p; }
    action ident_end  { f->type = FRAG_IDENT; f->body[0] = mark; f->body[1] = p; }
    action line_start { nval = 0; }
    action line_digit { nval = nval * 10 + (*p - '0'); }
    action line_end   { if (f->line == 0) f->line = nval; else f->line_end = nval; }
    action colon_line { f->line = nval; nval = 0; }
    action range_end  { f->line_end = nval; }
    action ext_start  { ext_mark = p; }
    action ext_end    {
        if (f->nexts < FRAG_MAX_EXTS) {
            f->exts[f->nexts][0] = ext_mark;
            f->exts[f->nexts][1] = p;
            f->nexts++;
        }
    }
    action spot_start { mark = p; }
    action spot_end   { f->type = FRAG_SPOT; f->body[0] = mark; f->body[1] = p; }
    action pcre_start { mark = p; }
    action pcre_end   { f->type = FRAG_PCRE; f->body[0] = mark; f->body[1] = p; }

    number = ( digit >line_start digit* ) $line_digit ;

    line_or_range = number %line_end ( '-' number %range_end )? ;

    ident = ( [A-Za-z_] [A-Za-z0-9_]* ) >mark %ident_end ;

    line_spec = ':' number %colon_line ( '-' number %range_end )? ;

    ext = '.' ( [A-Za-z0-9]+ >ext_start %ext_end ) ;

    ext_spec = ext+ ;

    # Spot body: everything except unescaped '.  Backslash escapes.
    spot_char = ( [^'\\] | '\\' any ) ;
    spot_body = spot_char* ;

    # Pcre body: everything except unescaped /.  Backslash escapes.
    pcre_char = ( [^/\\] | '\\' any ) ;
    pcre_body = pcre_char* ;

    # Line-only fragment (starts with digit)
    frag_line = line_or_range ext_spec? ;

    # Identifier fragment (symbol/grep)
    frag_ident = ident line_spec? ext_spec? ;

    # Structural search (spot)
    frag_spot = "'" ( spot_body >spot_start %spot_end ) "'"? ext_spec? ;

    # Regex search (must close with /)
    frag_pcre = "/" ( pcre_body >pcre_start %pcre_end ) "/" ext_spec? ;

    main := ( frag_line | frag_ident | frag_spot | frag_pcre ) ;
}%%

%% write data nofinal noerror;

ok64 FRAGu8sDrain(u8cs input, fragp f) {
    if (f == NULL) return FRAGBAD;
    memset(f, 0, sizeof(frag));
    if (input[0] == NULL || input[0] >= input[1]) return OK;

    u8cp p = input[0];
    u8cp pe = input[1];
    u8cp eof = pe;
    u8cp mark = NULL;
    u8cp ext_mark = NULL;
    u32 nval = 0;
    int cs = 0;

    %% write init;
    %% write exec;

    if (cs < %%{ write first_final; }%%) {
        // If parser didn't reach a final state but we got a type,
        // it's a tolerant parse (e.g., unclosed spot quote).
        if (f->type == FRAG_SPOT && f->body[0] != NULL) {
            // Unclosed spot: body extends to end of input
            f->body[1] = pe;
            return OK;
        }
        return FRAGFAIL;
    }

    // Line-only: digit-started fragment, type not set by ident/spot/pcre
    if (f->type == FRAG_NONE && f->line > 0)
        f->type = FRAG_LINE;

    return OK;
}
