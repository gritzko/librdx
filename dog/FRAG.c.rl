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

    pct = '%' [0-9a-fA-F]{2} ;
    ident = ( [A-Za-z_] ([A-Za-z0-9_] | pct)* ) >mark %ident_end ;

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

    # Structural search (spot) — optional line_spec after closing quote
    frag_spot = "'" ( spot_body >spot_start %spot_end ) "'"? line_spec? ext_spec? ;

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

// Is `c` legal in a URI fragment?
// Printable ASCII (0x20-0x7E) except '#' (fragment delimiter) and '%' (pct prefix).
static const u8 FRAG_CHAR[256] = {
    // 0x00-0x1F: control chars — illegal
    [0x20] = 1,  // space (unwise, but legal per URI.lex)
    [0x21] = 1,  // !
    [0x22] = 1,  // "
    // 0x23 '#' — fragment delimiter, must escape
    [0x24] = 1,  // $
    // 0x25 '%' — pct-encoded prefix, must escape
    [0x26] = 1,  // &
    [0x27] = 1,  // '
    [0x28] = 1,  // (
    [0x29] = 1,  // )
    [0x2A] = 1,  // *
    [0x2B] = 1,  // +
    [0x2C] = 1,  // ,
    [0x2D] = 1,  // -
    [0x2E] = 1,  // .
    [0x2F] = 1,  // /
    ['0'] = 1, ['1'] = 1, ['2'] = 1, ['3'] = 1, ['4'] = 1,
    ['5'] = 1, ['6'] = 1, ['7'] = 1, ['8'] = 1, ['9'] = 1,
    [0x3A] = 1,  // :
    [0x3B] = 1,  // ;
    [0x3C] = 1,  // <
    [0x3D] = 1,  // =
    [0x3E] = 1,  // >
    [0x3F] = 1,  // ?
    [0x40] = 1,  // @
    ['A'] = 1, ['B'] = 1, ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1,
    ['G'] = 1, ['H'] = 1, ['I'] = 1, ['J'] = 1, ['K'] = 1, ['L'] = 1,
    ['M'] = 1, ['N'] = 1, ['O'] = 1, ['P'] = 1, ['Q'] = 1, ['R'] = 1,
    ['S'] = 1, ['T'] = 1, ['U'] = 1, ['V'] = 1, ['W'] = 1, ['X'] = 1,
    ['Y'] = 1, ['Z'] = 1,
    [0x5B] = 1,  // [
    [0x5C] = 1,  // backslash
    [0x5D] = 1,  // ]
    [0x5E] = 1,  // ^
    [0x5F] = 1,  // _
    [0x60] = 1,  // `
    ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1,
    ['g'] = 1, ['h'] = 1, ['i'] = 1, ['j'] = 1, ['k'] = 1, ['l'] = 1,
    ['m'] = 1, ['n'] = 1, ['o'] = 1, ['p'] = 1, ['q'] = 1, ['r'] = 1,
    ['s'] = 1, ['t'] = 1, ['u'] = 1, ['v'] = 1, ['w'] = 1, ['x'] = 1,
    ['y'] = 1, ['z'] = 1,
    [0x7B] = 1,  // {
    [0x7C] = 1,  // |
    [0x7D] = 1,  // }
    [0x7E] = 1,  // ~
    // 0x7F DEL — control, illegal
    // 0x80-0xFF — non-ASCII, illegal
};

con u8c FRAG_HEX[16] = "0123456789ABCDEF";

ok64 FRAGu8sEsc(u8s into, u8cs raw) {
    if (into[0] == NULL || into[0] >= into[1]) return FRAGFAIL;
    if (raw[0] == NULL || raw[0] >= raw[1]) return OK;
    u8cp p = raw[0];
    u8cp end = raw[1];
    while (p < end) {
        u8 c = *p++;
        if (FRAG_CHAR[c]) {
            if (into[0] >= into[1]) return FRAGFAIL;
            *into[0]++ = c;
        } else {
            if (into[0] + 3 > into[1]) return FRAGFAIL;
            *into[0]++ = '%';
            *into[0]++ = FRAG_HEX[(c >> 4) & 0xF];
            *into[0]++ = FRAG_HEX[c & 0xF];
        }
    }
    return OK;
}

static int frag_hexval(u8 c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

ok64 FRAGu8sUnesc(u8s into, u8cs esc) {
    if (into[0] == NULL || into[0] >= into[1]) return FRAGFAIL;
    if (esc[0] == NULL || esc[0] >= esc[1]) return OK;
    u8cp p = esc[0];
    u8cp end = esc[1];
    while (p < end) {
        if (into[0] >= into[1]) return FRAGFAIL;
        u8 c = *p++;
        if (c == '%' && p + 2 <= end) {
            int hi = frag_hexval(p[0]);
            int lo = frag_hexval(p[1]);
            if (hi >= 0 && lo >= 0) {
                *into[0]++ = (u8)((hi << 4) | lo);
                p += 2;
                continue;
            }
        }
        *into[0]++ = c;
    }
    return OK;
}
