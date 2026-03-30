//
// NFApcre2 — run pcre2 testinput1 against NFA.h
//
// Parses pcre2test format:
//   /pattern/flags
//       subject line
//   output: " 0: match" or "No match"
//
// Skips patterns with unsupported features.
//

#include "NFA.h"
#include "PRO.h"
#include "TEST.h"

#include <ctype.h>

// Check if a pattern uses only features we support:
// literals, . | * + ? () \ escape
// We skip: ^ $ [ ] { } \d \w \s \b etc, flags like /i /m /s /x
b8 nfa_supported(u8cs pat) {
    u8c *p = pat[0];
    u8c *pe = pat[1];
    int depth = 0;
    while (p < pe) {
        u8 c = *p++;
        if (c == '\\') {
            if (p >= pe) return NO;
            u8 esc = *p++;
            // support \d \D \w \W \s \S and common escapes
            if (esc == 'd' || esc == 'D' || esc == 'w' || esc == 'W' ||
                esc == 's' || esc == 'S' || esc == 'n' || esc == 't' ||
                esc == 'r' || esc == 'f' || esc == 'a' || esc == 'e')
                continue;
            // \b word boundary — not yet implemented
            if (esc == 'b' || esc == 'B') return NO;
            // \Q...\E literal quoting — not implemented
            if (esc == 'Q' || esc == 'E') return NO;
            // \c control character — not implemented
            if (esc == 'c') return NO;
            // reject other alpha/digit escapes (\A \Z \z \G \1-\9 etc)
            if (isalpha(esc) || isdigit(esc)) return NO;
        } else if (c == '[') {
            // skip to closing ]
            if (p < pe && *p == '^') p++;
            if (p < pe && *p == ']') p++;
            while (p < pe && *p != ']') {
                // reject POSIX classes [[:...]]
                if (*p == '[' && p + 1 < pe && *(p + 1) == ':') return NO;
                // reject \Q \E \c inside classes
                if (*p == '\\' && p + 1 < pe) {
                    u8 e = *(p + 1);
                    if (e == 'Q' || e == 'E' || e == 'c') return NO;
                    // \b (backspace) and \1-\9 backreferences in classes
                    if (e == 'b' || e == 'B') return NO;
                    if (e >= '1' && e <= '9') return NO;
                }
                p++;
            }
            if (p >= pe) return NO;
            p++; // skip ]
            continue;
        } else if (c == '{') {
            // skip counted repetition
            while (p < pe && *p != '}') p++;
            if (p >= pe) return NO;
            p++;
            continue;
        } else if (c == '(') {
            // reject (?...) special groups
            if (p < pe && *p == '?') return NO;
            depth++;
        } else if (c == ')') {
            depth--;
            if (depth < 0) return NO;
        }
    }
    return depth == 0;
}

// Unescape pcre2test subject line (handles \t \n \r \xHH etc.)
// Returns length of unescaped data written into out.
u64 unescape_subject(u8 *out, u64 maxlen, u8c *s, u64 slen) {
    u64 oi = 0;
    for (u64 i = 0; i < slen && oi < maxlen; i++) {
        if (s[i] == '\\' && i + 1 < slen) {
            switch (s[i + 1]) {
                case 't':
                    out[oi++] = '\t';
                    i++;
                    break;
                case 'n':
                    out[oi++] = '\n';
                    i++;
                    break;
                case 'r':
                    out[oi++] = '\r';
                    i++;
                    break;
                case '\\':
                    out[oi++] = '\\';
                    i++;
                    break;
                case 'x':
                    if (i + 2 < slen && s[i + 2] == '{') {
                        // \x{HH} form
                        i += 3;
                        u32 val = 0;
                        while (i < slen && s[i] != '}') {
                            val = val * 16;
                            if (s[i] >= '0' && s[i] <= '9')
                                val += s[i] - '0';
                            else if (s[i] >= 'a' && s[i] <= 'f')
                                val += s[i] - 'a' + 10;
                            else if (s[i] >= 'A' && s[i] <= 'F')
                                val += s[i] - 'A' + 10;
                            i++;
                        }
                        // skip }
                        if (val < 256) out[oi++] = (u8)val;
                    } else if (i + 3 < slen && isxdigit(s[i + 2]) &&
                               isxdigit(s[i + 3])) {
                        char hex[3] = {s[i + 2], s[i + 3], 0};
                        out[oi++] = (u8)strtol(hex, NULL, 16);
                        i += 3;
                    } else {
                        out[oi++] = s[i];
                    }
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7': {
                    // octal escape
                    u8 val = s[i + 1] - '0';
                    i++;
                    if (i + 1 < slen && s[i + 1] >= '0' && s[i + 1] <= '7') {
                        val = val * 8 + (s[++i] - '0');
                        if (i + 1 < slen && s[i + 1] >= '0' &&
                            s[i + 1] <= '7')
                            val = val * 8 + (s[++i] - '0');
                    }
                    out[oi++] = val;
                    break;
                }
                default:
                    out[oi++] = s[i + 1]; // \X -> X for unrecognized escapes
                    i++;
                    break;
            }
        } else {
            out[oi++] = s[i];
        }
    }
    return oi;
}

ok64 run_pcre2_tests(u8cs filename) {
    sane(1);
    char fname[256];
    snprintf(fname, sizeof(fname), "%.*s", (int)$len(filename), filename[0]);
    FILE *fp = fopen(fname, "r");
    if (!fp) {
        printf("cannot open %s: %s\n", fname, strerror(errno));
        fail(NFABADSYN);
    }

    char line[4096];
    u8 patbuf[2048];
    u16 patlen = 0;
    b8 have_pattern = NO;
    b8 pattern_supported = NO;
    b8 expect_no_match = NO;

    nfau8 nfabuf[4096];
    u32 pbuf[8192];
    nfau8cs nfa = {};
    u32 wbuf[4096 * 3];
    u32 *ws[2] = {wbuf, wbuf + sizeof(wbuf) / sizeof(wbuf[0])};

    u64 total = 0, tested = 0, passed = 0, skipped = 0, errors = 0;

    while (fgets(line, sizeof(line), fp)) {
        u64 len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = 0;

        // skip comments and directives
        if (line[0] == '#' || len == 0) {
            expect_no_match = NO;
            continue;
        }

        // Check for \= Expect no match
        if (strncmp(line, "\\= Expect no match", 18) == 0) {
            expect_no_match = YES;
            continue;
        }
        // skip other \= directives
        if (line[0] == '\\' && len > 1 && line[1] == '=') continue;

        // Pattern line: /pattern/flags
        if (line[0] == '/') {
            // find closing /
            char *end = strrchr(line + 1, '/');
            if (!end) {
                have_pattern = NO;
                continue;
            }
            patlen = (u16)(end - (line + 1));
            memcpy(patbuf, line + 1, patlen);

            // check flags after closing /
            char *flags = end + 1;
            b8 has_flags = NO;
            for (char *f = flags; *f; f++) {
                if (*f != ' ' && *f != '\n' && *f != '\r') {
                    has_flags = YES;
                    break;
                }
            }

            u8cs pat = {patbuf, patbuf + patlen};
            pattern_supported = !has_flags && nfa_supported(pat);

            if (pattern_supported) {
                nfau8g prog = {nfabuf, nfabuf + 4096, nfabuf};
                u32 *pws[2] = {pbuf, pbuf + 8192};
                ok64 o = NFAu8Compile(prog, pat, pws);
                if (o != OK) {
                    pattern_supported = NO;
                }
                nfa[0] = prog[2];
                nfa[1] = prog[0];
            }

            have_pattern = YES;
            expect_no_match = NO;
            continue;
        }

        // Subject line: starts with spaces
        if (have_pattern && (line[0] == ' ' || line[0] == '\t')) {
            total++;
            if (!pattern_supported) {
                skipped++;
                continue;
            }

            // trim leading and trailing whitespace
            char *subj = line;
            while (*subj == ' ' || *subj == '\t') subj++;
            u64 subjlen = strlen(subj);
            while (subjlen > 0 &&
                   (subj[subjlen - 1] == ' ' || subj[subjlen - 1] == '\t'))
                subjlen--;

            // skip whitespace-only lines
            if (subjlen == 0) {
                total--;
                continue;
            }

            // trailing \ means "no newline" in pcre2test — strip it
            if (subjlen > 0 && subj[subjlen - 1] == '\\') subjlen--;

            // unescape
            u8 ubuf[4096];
            u64 ulen =
                unescape_subject(ubuf, sizeof(ubuf), (u8 *)subj, subjlen);
            // Strip trailing \n for matching (pcre2test convention)
            if (ulen > 0 && ubuf[ulen - 1] == '\n') ulen--;
            u8cs text = {ubuf, ubuf + ulen};

            b8 matched = NFAu8Search(nfa, text, ws);

            tested++;
            if (expect_no_match) {
                if (!matched)
                    passed++;
                else {
                    errors++;
                    fprintf(stdout,
                            "FAIL: /%.*s/ should NOT match \"%.*s\"\n",
                            patlen, patbuf, (int)ulen, ubuf);
                }
            } else {
                if (matched)
                    passed++;
                else {
                    errors++;
                    fprintf(stdout, "FAIL: /%.*s/ should match \"%.*s\"\n",
                            patlen, patbuf, (int)ulen, ubuf);
                }
            }
        }
    }

    fclose(fp);

    fprintf(stdout,
            "pcre2 test: total=%" PRIu64 " tested=%" PRIu64 " passed=%" PRIu64
            " skipped=%" PRIu64 " errors=%" PRIu64 "\n",
            total, tested, passed, skipped, errors);

    want(errors == 0);
    want(tested > 0);
    done;
}

ok64 all_tests() {
    sane(1);
    u8 f1s[] = "pcre2-pcre2-10.47/testdata/testinput1";
    u8cs f1 = {f1s, f1s + sizeof(f1s) - 1};
    call(run_pcre2_tests, f1);
    done;
}

TEST(all_tests)
