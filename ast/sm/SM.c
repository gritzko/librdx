#include "SM.h"
#include "SMI.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/RON.h"

// BASON type tags for StrictMark blocks (vowels = containers)
#define SM_DOC   'A'
#define SM_HEAD  'E'
#define SM_PARA  'A'
#define SM_QUOTE 'I'
#define SM_ULIST 'U'
#define SM_OLIST 'U'
#define SM_ITEM  'A'
#define SM_CODE  'I'
#define SM_DIV   'A'
#define SM_TEXT  'S'
#define SM_NAME  'F'

// Map div markup to BASON container type
fun u8 SMDivType(u8 div) {
    switch (div) {
        case 'H': return SM_HEAD;
        case 'Q': return SM_QUOTE;
        case 'U': return SM_ULIST;
        case 'R': return SM_OLIST;
        case 'C': return SM_CODE;
        case 'I': return SM_DIV;
        default:  return SM_PARA;
    }
}

// Get prevk slice for current key level
fun void SMGetPrevK(SMstate* st, u8cs prevk) {
    SMkeyframe* kf = &st->kframes[st->klevel];
    if (kf->klen == 0) {
        prevk[0] = NULL;
        prevk[1] = NULL;
    } else {
        prevk[0] = (u8cp)kf->kb;
        prevk[1] = (u8cp)kf->kb + kf->klen;
    }
}

// Save key after increment
fun void SMSaveKey(SMstate* st, u8cp kstart, u8cp kend) {
    SMkeyframe* kf = &st->kframes[st->klevel];
    kf->klen = (u8)(kend - kstart);
    memcpy(kf->kb, kstart, kf->klen);
}

// Emit an auto-incrementing key and feed a leaf
static ok64 SMFeedLeaf(SMstate* st, u8 type, u8csc val) {
    sane(st != NULL);
    u8 kb[11];
    u8cs prevk = {};
    SMGetPrevK(st, prevk);
    u8s ki = {kb, kb + sizeof(kb)};
    call(BASONFeedInfInc, ki, prevk);
    u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
    SMSaveKey(st, (u8cp)kb, (u8cp)ki[0]);
    call(BASONFeed, st->idx, st->buf, type, ck, val);
    done;
}

// Emit an auto-incrementing key and open a container
static ok64 SMFeedInto(SMstate* st, u8 type) {
    sane(st != NULL);
    u8 kb[11];
    u8cs prevk = {};
    SMGetPrevK(st, prevk);
    u8s ki = {kb, kb + sizeof(kb)};
    call(BASONFeedInfInc, ki, prevk);
    u8cs ck = {(u8cp)kb, (u8cp)ki[0]};
    SMSaveKey(st, (u8cp)kb, (u8cp)ki[0]);
    call(BASONFeedInto, st->idx, st->buf, type, ck);
    // Push new key level for children
    st->klevel++;
    test(st->klevel < SM_MAXDEPTH, SMFAIL);
    st->kframes[st->klevel].klen = 0;
    done;
}

// Close a container
static ok64 SMFeedOuto(SMstate* st) {
    sane(st != NULL);
    call(BASONFeedOuto, st->idx, st->buf);
    // Pop key level
    if (st->klevel > 0) st->klevel--;
    done;
}

// Close containers that diverge from current divstack
static ok64 SMCloseContainers(SMstate* st, int keep) {
    sane(st != NULL);
    while (st->prevdepth > keep) {
        st->prevdepth--;
        call(SMFeedOuto, st);
    }
    done;
}

// Open containers for current divstack starting at given depth
static ok64 SMOpenContainers(SMstate* st, int from) {
    sane(st != NULL);
    for (int i = from; i < st->depth; i++) {
        u8 type = SMDivType(st->divstack[i]);
        call(SMFeedInto, st, type);
    }
    done;
}

// --- Lexer callbacks ---

ok64 SMonHLine(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'H';
    return OK;
}

ok64 SMonIndent(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'I';
    return OK;
}

ok64 SMonOList(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'R';
    return OK;
}

ok64 SMonUList(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'U';
    return OK;
}

ok64 SMonH1(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'H';
    return OK;
}

ok64 SMonH2(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'H';
    return OK;
}

ok64 SMonH3(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'H';
    return OK;
}

ok64 SMonH4(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'H';
    return OK;
}

ok64 SMonH(u8cs tok, SMstate* state) {
    return OK;
}

ok64 SMonQuote(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'Q';
    return OK;
}

ok64 SMonCode(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'C';
    return OK;
}

ok64 SMonTodo(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'U';
    return OK;
}

ok64 SMonLink(u8cs tok, SMstate* state) {
    state->divstack[state->depth++] = 'I';
    return OK;
}

ok64 SMonDiv(u8cs tok, SMstate* state) {
    return OK;
}

// --- Inline markup (SMI) callbacks ---

static u8 SMILeafType(SMIstate *st) {
    if (st->mode & SMI_CODE)   return 'G';
    if (st->link_phase == 1)   return 'T';  // display text: link color
    if (st->link_phase == 3)   return 'P';  // ref tag: gray
    if (st->mode & SMI_BOLD)   return 'V';
    if (st->mode & SMI_STRIKE) return 'J';
    if (st->mode & SMI_ITALIC) return 'W';
    return st->base;
}

// Open: space/SOL before, non-space after
static b8 smi_can_open(u8cs tok, SMIstate *st) {
    b8 sp_b = (tok[0] == st->data_start) ||
              tok[0][-1] == ' ' || tok[0][-1] == '\t';
    b8 sp_a = (tok[1] >= st->data_end) ||
              tok[1][0] == ' ' || tok[1][0] == '\t';
    return sp_b && !sp_a;
}

// Close: non-space before, space/EOL after
static b8 smi_can_close(u8cs tok, SMIstate *st) {
    b8 sp_b = (tok[0] == st->data_start) ||
              tok[0][-1] == ' ' || tok[0][-1] == '\t';
    b8 sp_a = (tok[1] >= st->data_end) ||
              tok[1][0] == ' ' || tok[1][0] == '\t';
    return !sp_b && sp_a;
}

// Stars callback: handle *, **, ***, etc.
// Length 1 = italic toggle, length 2 = bold toggle, other = literal
ok64 SMIonStars(u8cs tok, SMIstate* st) {
    sane(st != NULL);
    size_t len = tok[1] - tok[0];
    if (!(st->mode & SMI_CODE)) {
        if (len == 2) {
            // Bold toggle
            if (!(st->mode & SMI_BOLD) && smi_can_open(tok, st)) {
                st->mode |= SMI_BOLD;
                call(SMFeedLeaf, st->sm, 'P', tok);
                done;
            }
            if ((st->mode & SMI_BOLD) && smi_can_close(tok, st)) {
                st->mode &= ~SMI_BOLD;
                call(SMFeedLeaf, st->sm, 'P', tok);
                done;
            }
        } else if (len == 1) {
            // Italic toggle
            if (!(st->mode & SMI_ITALIC) && smi_can_open(tok, st)) {
                st->mode |= SMI_ITALIC;
                call(SMFeedLeaf, st->sm, 'P', tok);
                done;
            }
            if ((st->mode & SMI_ITALIC) && smi_can_close(tok, st)) {
                st->mode &= ~SMI_ITALIC;
                call(SMFeedLeaf, st->sm, 'P', tok);
                done;
            }
        }
    }
    // Literal: emit as current leaf type
    call(SMFeedLeaf, st->sm, SMILeafType(st), tok);
    done;
}

// Tildes callback: handle ~, ~~, etc.
// Length 2 = strikethrough toggle, other = literal
ok64 SMIonTildes(u8cs tok, SMIstate* st) {
    sane(st != NULL);
    size_t len = tok[1] - tok[0];
    if (!(st->mode & SMI_CODE)) {
        if (len == 2) {
            if (!(st->mode & SMI_STRIKE) && smi_can_open(tok, st)) {
                st->mode |= SMI_STRIKE;
                call(SMFeedLeaf, st->sm, 'P', tok);
                done;
            }
            if ((st->mode & SMI_STRIKE) && smi_can_close(tok, st)) {
                st->mode &= ~SMI_STRIKE;
                call(SMFeedLeaf, st->sm, 'P', tok);
                done;
            }
        }
    }
    call(SMFeedLeaf, st->sm, SMILeafType(st), tok);
    done;
}

ok64 SMIonCode(u8cs tok, SMIstate* st) {
    sane(st != NULL);
    // Code toggles unconditionally (no boundary rules)
    if (st->mode & SMI_CODE) {
        st->mode &= ~SMI_CODE;
    } else {
        st->mode |= SMI_CODE;
    }
    call(SMFeedLeaf, st->sm, 'P', tok);
    done;
}

ok64 SMIonWord(u8cs tok, SMIstate* st) {
    sane(st != NULL);
    call(SMFeedLeaf, st->sm, SMILeafType(st), tok);
    done;
}

ok64 SMIonSpace(u8cs tok, SMIstate* st) {
    sane(st != NULL);
    call(SMFeedLeaf, st->sm, SMILeafType(st), tok);
    done;
}

ok64 SMIonOpen(u8cs tok, SMIstate* st) {
    sane(st != NULL);
    if (st->mode & SMI_CODE) {
        call(SMFeedLeaf, st->sm, 'G', tok);
        done;
    }
    if (st->link_phase == 0) {
        // Start display text: [
        st->link_phase = 1;
        call(SMFeedLeaf, st->sm, 'P', tok);
        done;
    }
    if (st->link_phase == 2) {
        // Start ref tag: ][
        st->link_phase = 3;
        call(SMFeedLeaf, st->sm, 'P', tok);
        done;
    }
    // Unexpected [ — emit literal, reset
    st->link_phase = 0;
    call(SMFeedLeaf, st->sm, SMILeafType(st), tok);
    done;
}

ok64 SMIonClose(u8cs tok, SMIstate* st) {
    sane(st != NULL);
    if (st->mode & SMI_CODE) {
        call(SMFeedLeaf, st->sm, 'G', tok);
        done;
    }
    if (st->link_phase == 1) {
        // End display text: ]
        st->link_phase = 2;
        call(SMFeedLeaf, st->sm, 'P', tok);
        done;
    }
    if (st->link_phase == 3) {
        // End ref tag: ]
        st->link_phase = 0;
        call(SMFeedLeaf, st->sm, 'P', tok);
        done;
    }
    // Unexpected ] — emit literal
    call(SMFeedLeaf, st->sm, SMILeafType(st), tok);
    done;
}

ok64 SMIonRoot(u8cs tok, SMIstate* st) {
    return OK;
}

ok64 SMonLine(u8cs tok, SMstate* st) {
    sane(st != NULL);

    // Find where div markup ends and content begins
    u8cp linestart = tok[0];
    u8cp lineend = tok[1];
    int divchars = st->depth * 4;
    u8cp content = linestart;
    if (content + divchars <= lineend) {
        content = linestart + divchars;
    } else {
        content = lineend;
    }

    // Check for blank line (only whitespace/newline after div markup)
    b8 blank = YES;
    for (u8cp q = content; q < lineend; q++) {
        if (*q != ' ' && *q != '\t' && *q != '\n' && *q != '\r') {
            blank = NO;
            break;
        }
    }

    // Implicit paragraph: non-blank line with no div markup
    if (st->depth == 0 && !blank && !st->incode) {
        st->divstack[st->depth++] = 'P';
    }

    // Compare divstack with previous to manage containers
    int keep = 0;
    int mind = st->depth < st->prevdepth ? st->depth : st->prevdepth;
    for (int i = 0; i < mind; i++) {
        if (st->divstack[i] == st->prevdiv[i])
            keep = i + 1;
        else
            break;
    }

    // Handle code blocks: inside code, emit raw lines
    if (st->incode) {
        if (st->depth > 0 && st->divstack[st->depth - 1] == 'C') {
            // Code-closing fence
            st->incode = NO;
            u8cs val = {linestart, lineend};
            call(SMFeedLeaf, st, 'P', val);
            call(SMCloseContainers, st, 0);
            st->prevdepth = 0;
            memset(st->prevdiv, 0, sizeof(st->prevdiv));
        } else {
            // Raw line inside code block
            u8cs val = {linestart, lineend};
            call(SMFeedLeaf, st, 'G', val);
        }
        st->depth = 0;
        done;
    }

    // Opening a code block
    if (st->depth > 0 && st->divstack[st->depth - 1] == 'C') {
        call(SMCloseContainers, st, keep);
        call(SMOpenContainers, st, keep);
        // Emit the opening fence line as punctuation
        u8cs val = {linestart, lineend};
        call(SMFeedLeaf, st, 'P', val);
        st->incode = YES;
        st->inline_mode = 0;
        memcpy(st->prevdiv, st->divstack, sizeof(st->prevdiv));
        st->prevdepth = st->depth;
        st->depth = 0;
        done;
    }

    // Blank line: emit separator between blocks
    if (blank) {
        if (st->prevdepth > 0) {
            call(SMCloseContainers, st, 0);
            st->prevdepth = 0;
            memset(st->prevdiv, 0, sizeof(st->prevdiv));
        }
        st->inline_mode = 0;
        u8cs val = {linestart, lineend};
        call(SMFeedLeaf, st, SM_TEXT, val);
        st->depth = 0;
        done;
    }

    // Normal content line
    call(SMCloseContainers, st, keep);
    call(SMOpenContainers, st, keep);

    // Emit div markup as 'P' (punctuation) token
    if (divchars > 0 && linestart + divchars <= lineend) {
        u8cs divval = {linestart, linestart + divchars};
        call(SMFeedLeaf, st, 'P', divval);
    }

    // Determine leaf type from context
    u8 leaf = SM_TEXT;
    if (st->depth > 0 && st->divstack[st->depth - 1] == 'H') {
        leaf = SM_NAME;
    } else {
        for (int i = 0; i < st->depth; i++) {
            if (st->divstack[i] == 'Q') { leaf = 'W'; break; }
        }
    }

    // Tokenize content (after div markup, before newline)
    u8cp nlpos = lineend;
    if (nlpos > linestart && *(nlpos - 1) == '\n') nlpos--;

    if (content < nlpos) {
        SMIstate smi = {};
        smi.data[0] = content;
        smi.data[1] = nlpos;
        smi.data_start = content;
        smi.data_end = nlpos;
        smi.sm = st;
        smi.mode = st->inline_mode;
        smi.base = leaf;
        call(SMILexer, &smi);
        st->inline_mode = smi.mode;
    }

    // Emit trailing newline as separate token
    if (lineend > linestart && *(lineend - 1) == '\n') {
        u8cs nl = {lineend - 1, lineend};
        call(SMFeedLeaf, st, SM_TEXT, nl);
    }

    // Save current div state as previous
    memcpy(st->prevdiv, st->divstack, sizeof(st->prevdiv));
    st->prevdepth = st->depth;
    st->depth = 0;
    done;
}

ok64 SMonRoot(u8cs tok, SMstate* state) {
    return OK;
}

// --- Main entry point ---

ok64 SMParse(u8bp buf, u64bp idx, u8csc source) {
    sane(buf != NULL);

    SMstate st = {};
    st.data[0] = source[0];
    st.data[1] = source[1];
    st.buf = buf;
    st.idx = idx;

    // Open document root
    u8cs nokey = {(u8cp)"", (u8cp)""};
    call(BASONFeedInto, idx, buf, SM_DOC, nokey);

    // Push key level for root children
    st.klevel = 0;
    st.kframes[0].klen = 0;

    call(SMLexer, &st);

    // Close any remaining open containers
    call(SMCloseContainers, &st, 0);

    // Close document root
    call(BASONFeedOuto, idx, buf);
    done;
}
