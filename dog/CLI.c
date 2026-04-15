#include "CLI.h"

#include <string.h>
#include <unistd.h>

#include "abc/PRO.h"

// Check if flag appears in the val_flags string (NUL-separated list).
// e.g. val_flags = "-g\0-C\0-r\0" (triple-NUL terminated).
static b8 cli_takes_val(char const *val_flags, u8csc flag) {
    if (val_flags == NULL) return NO;
    char const *p = val_flags;
    while (*p) {
        a_cstr(vf, p);
        if ($eq(flag, vf)) return YES;
        p += strlen(p) + 1;
    }
    return NO;
}

ok64 CLIParse(cli *c, char const *const *verb_names,
              char const *val_flags) {
    sane(c != NULL);
    *c = (cli){};
    c->tty_out = isatty(STDOUT_FILENO) ? YES : NO;

    // Repo root — copy into owned storage
    a_path(root);
    if (HOMEFind(root) == OK) {
        a_dup(u8c, rr, u8bDataC(root));
        size_t rlen = u8csLen(rr);
        if (rlen >= sizeof(c->_repo)) rlen = sizeof(c->_repo) - 1;
        memcpy(c->_repo, rr[0], rlen);
        c->_repo[rlen] = 0;
        c->repo[0] = (u8cp)c->_repo;
        c->repo[1] = (u8cp)c->_repo + rlen;
    }

    int argn = (int)$arglen;
    int ai = 1;  // skip argv[0]

    // Verb: first arg that matches a known verb name
    if (ai < argn && verb_names != NULL) {
        a$rg(a, ai);
        for (char const *const *vn = verb_names; *vn != NULL; vn++) {
            a_cstr(vs, *vn);
            if ($eq(a, vs)) {
                $mv(c->verb, a);
                ai++;
                break;
            }
        }
    }

    // Remaining args: flags or URIs
    u8cs empty_val = {(u8cp)"", (u8cp)""};  // non-NULL empty sentinel
    while (ai < argn) {
        a$rg(a, ai);
        ai++;

        if ($len(a) >= 1 && a[0][0] == '-') {
            if (c->nflags + 2 > CLI_MAX_FLAGS * 2) continue;

            // Check for --flag=value
            u8cs flag_part = {};
            u8cs val_part = {};
            $mv(flag_part, a);
            $for(u8c, ep, a) {
                if (*ep == '=') {
                    flag_part[1] = ep;
                    val_part[0] = ep + 1;
                    val_part[1] = a[1];
                    break;
                }
            }

            if (!$empty(val_part)) {
                // --flag=value
                $mv(c->flags[c->nflags], flag_part);
                c->nflags++;
                $mv(c->flags[c->nflags], val_part);
                c->nflags++;
            } else if (cli_takes_val(val_flags, a)) {
                // -f value (separate args) or -fN (attached)
                // Check for short flag with attached value: -XY where
                // -X takes a value and Y... is the value
                b8 attached = NO;
                if (a[0][1] != '-' && $len(a) > 2) {
                    // Short flag: -X is first 2 chars, rest is value
                    u8cs sf = {a[0], a[0] + 2};
                    if (cli_takes_val(val_flags, sf)) {
                        $mv(c->flags[c->nflags], sf);
                        c->nflags++;
                        u8cs sv = {a[0] + 2, a[1]};
                        $mv(c->flags[c->nflags], sv);
                        c->nflags++;
                        attached = YES;
                    }
                }
                if (!attached) {
                    $mv(c->flags[c->nflags], a);
                    c->nflags++;
                    if (ai < argn) {
                        a$rg(v, ai);
                        ai++;
                        $mv(c->flags[c->nflags], v);
                    } else {
                        $mv(c->flags[c->nflags], empty_val);
                    }
                    c->nflags++;
                }
            } else {
                // Boolean flag
                $mv(c->flags[c->nflags], a);
                c->nflags++;
                $mv(c->flags[c->nflags], empty_val);
                c->nflags++;
            }
        } else {
            // URI
            if (c->nuris >= CLI_MAX_URIS) continue;
            uri *u = &c->uris[c->nuris];
            *u = (uri){};
            $mv(u->data, a);
            URILexer(u);  // best-effort parse (consumes data)
            $mv(u->data, a);  // restore original data slice
            c->nuris++;
        }
    }
    done;
}
