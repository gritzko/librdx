// RXLITS: walk a regex pattern, calling a callback for each
// literal character and on each meta-boundary "flush".  Used by
// CAPOPcreGrep to extract trigram-indexable substrings.
#include "RXLITS.h"

%%{
    machine rxlits;
    alphtype unsigned char;

    action emit_here  { (void)cb(ctx, *p, NO); }
    action emit_esc   { (void)cb(ctx, *p, NO); }  # *p is the escaped char
    action flush      { (void)cb(ctx, 0, YES); }

    # \d \D \w \W \s \S — character-class shorthands break runs.
    classesc = '\\' [dDwWsS];

    # Any other escape: the escaped char itself is a literal.
    litesc   = '\\' (any - [dDwWsS]) @emit_esc;

    # Regex meta-characters that always break a literal run.
    metac    = [*+?|.()$^{}];

    # [class]: skip to closing bracket; meta-boundary itself.
    cclass   = '[' '^'? ']'? ([^\]\\] | '\\' any)* ']';

    # A plain literal byte: anything not above.
    plain    = (any - ([\\\[*+?|.()$^{}])) @emit_here;

    main := (
        classesc %flush
      | cclass   %flush
      | metac    %flush
      | litesc
      | plain
    )*;
}%%

%% write data nofinal noerror;

void RXLITSu8sDrain(u8csc pattern, rxlits_cb cb, void *ctx) {
    if (pattern[0] == NULL || pattern[0] >= pattern[1] || cb == NULL) {
        if (cb) (void)cb(ctx, 0, YES);
        return;
    }
    u8c *p = pattern[0];
    u8c *pe = pattern[1];
    u8c *eof = pe;
    int cs = 0;
    %% write init;
    %% write exec;
    (void)cs;  // tolerate trailing partial token
    (void)cb(ctx, 0, YES);  // final flush
}
