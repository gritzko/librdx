#include "QURY.h"
#include <string.h>

%%{
    machine qury;

    action body_start  { body_mark = p; }
    action body_end    { out->body[0] = body_mark; out->body[1] = p; }
    action anc_tilde   { out->anc_type = '~'; }
    action anc_caret   { out->anc_type = '^'; }
    action anc_digit   { out->ancestry = out->ancestry * 10 + (*p - '0'); }

    atom     = alnum | [_\-] ;
    seg      = atom+ ('.' atom+)* ;
    pathbody = seg ('/' seg)* ;

    ancestry = ( '~' @anc_tilde | '^' @anc_caret ) ( digit @anc_digit )* ;

    spec = ( pathbody >body_start %body_end ) ancestry? ;

    main := spec ;
}%%

%% write data nofinal noerror;

static b8 qury_is_sha(u8cs s) {
    if ($len(s) < QURY_MIN_SHA) return NO;
    $for(u8c, p, s) {
        u8 c = *p;
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'))
            continue;
        return NO;
    }
    return YES;
}

ok64 QURYu8sDrain(u8cs input, qrefp out) {
    if (out == NULL) return QURYBAD;
    memset(out, 0, sizeof(qref));
    if (input[0] == NULL || input[0] >= input[1]) return OK;

    // Find the end of this spec (up to '&' or end of input)
    u8cp specend = input[0];
    while (specend < input[1] && *specend != '&') specend++;

    u8cp p = input[0];
    u8cp pe = specend;
    u8cp eof = pe;
    u8cp body_mark = NULL;
    int cs = 0;

    %% write init;
    %% write exec;

    // Advance input past spec and separator
    input[0] = (specend < input[1]) ? specend + 1 : specend;

    if (cs < %%{ write first_final; }%%)
        return QURYFAIL;

    out->type = qury_is_sha(out->body) ? QURY_SHA : QURY_REF;
    return OK;
}
