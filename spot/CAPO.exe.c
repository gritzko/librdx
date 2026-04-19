//  SPOTExec — run a parsed CLI against an open spot state.
//  Same effect as invoking `spot ...` as a separate process.
//
#include "CAPO.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/CLI.h"
#include "dog/DOG.h"
#include "dog/FRAG.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"
#include "dog/SHA1.h"
#include "spot/CAPOi.h"
#include "spot/LESS.h"

// --- Verb / flag tables ---

char const *const SPOT_CLI_VERBS[] = {
    "get", "status", "help", NULL
};

//  Spot val-flags: -g -s -r -p -C --grep --spot --replace --pcre --context
//  Indexing now happens via SPOTUpdate (keeper's UNPK emit hook), not the
//  CLI — there are no --fork / --index / --hook flags any more.
char const SPOT_CLI_VAL_FLAGS[] =
    "-g\0-s\0-r\0-p\0-C\0"
    "--grep\0--spot\0--replace\0--pcre\0--context\0";

// --- Helpers ---

static void spot_usage(void) {
    fprintf(stderr,
        "Usage: spot [--flags] [URI...]\n"
        "\n"
        "  spot status                        index stack summary\n"
        "  spot -s \"pattern\" .ext             code snippet search\n"
        "  spot -s \"pat\" -r \"repl\" .ext       code snippet search + replace\n"
        "  spot -g \"text\" [.ext]              grep (substring)\n"
        "  spot -p \"regex\" [.ext]             regex grep\n"
        "  spot '#pattern.ext'                URI-style search\n"
        "\n"
        "Patterns: single-letter placeholders (a-z match one token/group,\n"
        "A-Z match multiple tokens). Two spaces = skip gap.\n"
        "\n"
        "Indexing is driven by keeper (pack ingest) — no spot CLI flags.\n"
        "Diff/merge tools live in graf — see `graf --help`.\n"
    );
}

static b8 argIsExt(u8csc a) {
    if ($len(a) < 2 || a[0][0] != '.') return NO;
    return CAPOKnownExt(a);
}

// --- Entry ---

ok64 SPOTExec(spotp dog, cli *c) {
    sane(dog && c);

    if (getenv("SPOT_COLOR")) { dog->color = YES; CAPO_COLOR = YES; }

    a_dup(u8c, reporoot, u8bDataC(dog->h->root));

    u8cs v = {};

    if (CLIHas(c, "-h") || CLIHas(c, "--help")) {
        spot_usage(); done;
    }

    a_cstr(v_get, "get");
    a_cstr(v_status_verb, "status");
    a_cstr(v_help_verb, "help");

    if ($eq(c->verb, v_help_verb)) { spot_usage(); done; }
    if ($eq(c->verb, v_status_verb)) {
        a_path(capodir);
        CAPOResolveDir(capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));
        u64cs runs[CAPO_MAX_LEVELS] = {};
        u64css stack = {runs, runs};
        u8bp mmaps[CAPO_MAX_LEVELS] = {};
        u32 nidxfiles = 0;
        CAPOStackOpen(stack, mmaps, &nidxfiles, dirslice);
        u64 total = 0;
        for (u32 i = 0; i < nidxfiles; i++)
            total += (u64)$len(runs[i]);
        CAPOStackClose(mmaps, nidxfiles);
        fprintf(stderr, "spot: %u index files, %llu entries\n",
                nidxfiles, (unsigned long long)total);
        done;
    }
    //  `spot get` — invoked by `be` after sniff/keeper checkouts and
    //  commits.  Indexing is reactive (keeper's UNPK emit hook feeds
    //  SPOTUpdate per pack object), so there's nothing to do here.
    //  The dispatcher just needs a non-error exit.
    if ($eq(c->verb, v_get)) done;

    b8 do_status = CLIHas(c, "--status");
    b8 force_tlv = CLIHas(c, "-t") || CLIHas(c, "--tlv");

    u32 grep_ctx = 3;
    CLIFlag(v, c, "-C");
    if (!$empty(v)) grep_ctx = (u32)atoi((char *)v[0]);
    CLIFlag(v, c, "--context");
    if (!$empty(v)) grep_ctx = (u32)atoi((char *)v[0]);

    u8cs spot_ndl = {}, spot_rep = {}, grep_ndl = {}, pcre_ndl = {};
    CLIFlag(v, c, "-s");
    if (!$empty(v)) { $mv(spot_ndl, v); }
    CLIFlag(v, c, "--spot");
    if (!$empty(v)) { $mv(spot_ndl, v); }
    CLIFlag(v, c, "-r");
    if (!$empty(v)) { $mv(spot_rep, v); }
    CLIFlag(v, c, "--replace");
    if (!$empty(v)) { $mv(spot_rep, v); }
    CLIFlag(v, c, "-g");
    if (!$empty(v)) { $mv(grep_ndl, v); }
    CLIFlag(v, c, "--grep");
    if (!$empty(v)) { $mv(grep_ndl, v); }
    CLIFlag(v, c, "-p");
    if (!$empty(v)) { $mv(pcre_ndl, v); }
    CLIFlag(v, c, "--pcre");
    if (!$empty(v)) { $mv(pcre_ndl, v); }

    u8cs trail[16] = {};
    int ntrail = 0;
    for (u32 ui = 0; ui < c->nuris && ntrail < 16; ui++) {
        uri *u = &c->uris[ui];
        if ($empty(spot_ndl) && $empty(grep_ndl) && $empty(pcre_ndl) &&
            !$empty(u->fragment)) {
            frag fr = {};
            if (FRAGu8sDrain(u->fragment, &fr) == OK) {
                if (fr.type == FRAG_SPOT && !$empty(fr.body)) {
                    $mv(spot_ndl, fr.body);
                } else if (fr.type == FRAG_PCRE && !$empty(fr.body)) {
                    $mv(pcre_ndl, fr.body);
                } else if (fr.type == FRAG_IDENT && !$empty(fr.body)) {
                    $mv(grep_ndl, fr.body);
                }
                for (u8 ei = 0; ei < fr.nexts && ntrail < 16; ei++) {
                    if (!$empty(fr.exts[ei]) && fr.exts[ei][0] > u->data[0]) {
                        trail[ntrail][0] = fr.exts[ei][0] - 1;
                        trail[ntrail][1] = fr.exts[ei][1];
                        ntrail++;
                    }
                }
            }
            if (!$empty(u->path)) {
                u8cs gpath = {};
                $mv(gpath, u->path);
                if (!$empty(gpath) && *gpath[0] == '/') {
                    u8csFed(gpath, 1);
                }
                if (!$empty(gpath) && ntrail < 16) {
                    $mv(trail[ntrail], gpath);
                    ntrail++;
                }
            }
        } else {
            u8cs dat = {};
            if (!$empty(u->path)) {
                $mv(dat, u->path);
            } else if (!$empty(u->data)) {
                $mv(dat, u->data);
            }
            if (!$empty(dat) && ntrail < 16) {
                $mv(trail[ntrail], dat);
                ntrail++;
            }
        }
    }

    if (!$empty(spot_rep) && $empty(spot_ndl)) {
        fprintf(stderr, "spot: --replace requires --spot\n");
        return FAILSANITY;
    }

    pid_t bro_pid = -1;
    b8 produces_hunks =
        (!$empty(grep_ndl) || !$empty(pcre_ndl) || !$empty(spot_ndl)) &&
        $empty(spot_rep);
    if (produces_hunks) {
        if (force_tlv) {
            spot_out_fd = STDOUT_FILENO;
            spot_emit   = HUNKu8sFeed;
            signal(SIGPIPE, SIG_IGN);
        } else if (c->tty_out) {
            a_path(bropath);
            a$rg(a0, 0);
            a_cstr(bro_name, "bro");
            HOMEResolveSibling(NULL, bropath, bro_name, a0);
            u8cs bargs[] = {u8slit("bro")};
            u8css bargv = {bargs, bargs + 1};
            int wfd = -1;
            call(FILESpawn, $path(bropath), bargv, &wfd, NULL, &bro_pid);
            dog->out_fd = wfd;
            dog->emit   = HUNKu8sFeed;
            spot_out_fd = dog->out_fd;
            spot_emit   = dog->emit;
            signal(SIGPIPE, SIG_IGN);
        } else {
            dog->out_fd = STDOUT_FILENO;
            dog->emit   = HUNKu8sFeedText;
            spot_out_fd = dog->out_fd;
            spot_emit   = dog->emit;
        }
    }

    ok64 ret = OK;

    if (do_status) {
        a_path(capodir);
        vcall("resolve_dir", CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));
        u64cs runs[CAPO_MAX_LEVELS] = {};
        u64css stack = {runs, runs};
        u8bp mmaps[CAPO_MAX_LEVELS] = {};
        u32 nidxfiles = 0;
        vcall("stack_open", CAPOStackOpen, stack, mmaps, &nidxfiles, dirslice);
        u64 total = 0;
        for (u32 i = 0; i < nidxfiles; i++)
            total += (u64)$len(runs[i]);
        CAPOStackClose(mmaps, nidxfiles);
        fprintf(stderr, "spot: %u index files, %llu entries\n",
                nidxfiles, (unsigned long long)total);
    } else if (!$empty(grep_ndl)) {
        u8cs ext = {};
        u8cs gfiles[16] = {};
        int gnf = 0;
        for (int i = 0; i < ntrail; i++) {
            if (argIsExt(trail[i])) {
                $mv(ext, trail[i]);
            } else if (gnf < 16) {
                $mv(gfiles[gnf], trail[i]);
                gnf++;
            }
        }
        if ($empty(ext) && gnf > 0) {
            u8cs pe = {};
            PATHu8sExt(pe, gfiles[0]);
            if (!$empty(pe)) {
                ext[0] = pe[0] - 1;
                ext[1] = pe[1];
            }
        }
        a_dup(u8c, ndl, grep_ndl);
        u8css gf = {gfiles, gfiles + gnf};
        ret = CAPOGrep(ndl, ext, reporoot, grep_ctx, gf);
    } else if (!$empty(pcre_ndl)) {
        u8cs ext = {};
        u8cs gfiles[16] = {};
        int gnf = 0;
        for (int i = 0; i < ntrail; i++) {
            if (argIsExt(trail[i])) {
                $mv(ext, trail[i]);
            } else if (gnf < 16) {
                $mv(gfiles[gnf], trail[i]);
                gnf++;
            }
        }
        if ($empty(ext) && gnf > 0) {
            u8cs pe = {};
            PATHu8sExt(pe, gfiles[0]);
            if (!$empty(pe)) {
                ext[0] = pe[0] - 1;
                ext[1] = pe[1];
            }
        }
        a_dup(u8c, ndl, pcre_ndl);
        u8css gf = {gfiles, gfiles + gnf};
        ret = CAPOPcreGrep(ndl, ext, reporoot, grep_ctx, gf);
    } else if (!$empty(spot_ndl)) {
        u8cs ext = {};
        u8cs sfiles[16] = {};
        int snf = 0;
        for (int i = 0; i < ntrail; i++) {
            if (argIsExt(trail[i])) {
                $mv(ext, trail[i]);
            } else if (snf < 16) {
                $mv(sfiles[snf], trail[i]);
                snf++;
            }
        }
        if ($empty(ext) && snf > 0) {
            u8cs pe = {};
            PATHu8sExt(pe, sfiles[0]);
            if (!$empty(pe)) {
                ext[0] = pe[0] - 1;
                ext[1] = pe[1];
            }
        }
        if ($empty(ext)) {
            fprintf(stderr, "spot: --spot requires a .ext argument\n");
            ret = FAILSANITY;
        } else {
            a_dup(u8c, ndl, spot_ndl);
            a_dup(u8c, rep, spot_rep);
            u8css sf = {sfiles, sfiles + snf};
            ret = CAPOSpot(ndl, rep, ext, reporoot, sf);
        }
    } else if (c->nuris > 0) {
        fprintf(stderr, "spot: file display moved to bro\n");
        ret = FAILSANITY;
    } else {
        spot_usage();
    }

    // Cleanup bro pipe (globals)
    if (spot_out_fd >= 0 && spot_out_fd != STDOUT_FILENO) {
        close(spot_out_fd);
        spot_out_fd = -1;
    }
    if (bro_pid > 0) {
        int rc = 0;
        FILEReap(bro_pid, &rc);
        if (rc == 127)
            fprintf(stderr, "spot: bro pager not found\n");
    }

    return ret;
}

// --- Update: feed a single git object into spot's index ---
//
// Driven by keeper's UNPK emit hook (UNPK.h unpk_emit_fn) — one call
// per resolved object, in commit-tree-blob order.
//   COMMIT: compute the git object SHA-1 ("commit N\0body") and append
//           it to .dogs/spot/COMMIT (rule 7 of DOG.md).
//   TREE / TAG: no-op; UNPK has already resolved tree → path bindings
//           and delivered each blob with its live path.
//   BLOB: pick tokenizer from `path`'s extension, tokenize, append
//           trigram + symbol postings to the in-memory scratch; flush
//           to a new `.idx` run when the run fills up.
// Read-only opens silently drop all updates.
ok64 SPOTUpdate(spotp s, u8 obj_type, u8cs blob, u8csc path) {
    sane(s);
    if (!s->rw) done;

    a_dup(u8c, root_s, u8bDataC(s->h->root));
    a_path(capodir);
    call(CAPOResolveDir, capodir, root_s);
    a_dup(u8c, dirslice, u8bDataC(capodir));

    switch (obj_type) {

    case DOG_OBJ_COMMIT: {
        //  Git object hash: SHA-1 of "commit <size>\0" + body.
        SHA1state st;
        SHA1Open(&st);
        char hdr[32] = {};
        int hlen = snprintf(hdr, sizeof(hdr), "commit %zu",
                            (size_t)$len(blob));
        test(hlen > 0 && hlen + 1 < (int)sizeof(hdr), FAILSANITY);
        u8cs hdr_slice = {(u8cp)hdr, (u8cp)hdr + hlen + 1};  // with NUL
        u8cs body_slice = {blob[0], blob[1]};
        SHA1Feed(&st, hdr_slice);
        SHA1Feed(&st, body_slice);
        sha1 sha = {};
        SHA1Close(&st, &sha);

        u8 hex[40] = {};
        u8 *hp[2] = {hex, hex + 40};
        u8cs bin = {sha.data, sha.data + 20};
        call(HEXu8sFeedSome, hp, bin);
        u8cs sha40 = {(u8cp)hex, (u8cp)hex + 40};
        call(CAPOCommitAppend, dirslice, sha40);
        done;
    }

    case DOG_OBJ_BLOB: {
        if ($empty(path)) done;  // no ext → no tokenizer
        size_t plen = (size_t)$len(path);
        u8cs ext = {};
        CAPOFindExt(ext, path[0], plen);
        if ($empty(ext) || !CAPOKnownExt(ext)) done;

        u8cs source = {blob[0], blob[1]};
        call(CAPOIndexFile, s->entries, source, ext, path);

        //  Flush when the scratch run hits CAPO_FLUSH_AT.  Dedup halves
        //  the run often enough that early flushing wastes I/O; follow
        //  the existing reindex policy (CAPO.c:486-503).  sDedup moves
        //  the buffer's idle pointer back in place, so a delayed flush
        //  leaves the scratch compacted.
        size_t pending = u64bDataLen(s->entries);
        if (pending >= CAPO_FLUSH_AT) {
            u64sp data = u64bData(s->entries);
            u64sSort(data);
            u64sDedup(data);
            size_t unique = $len(data);
            if (unique * 2 > pending) {
                u64cs run = {(u64cp)data[0], (u64cp)data[1]};
                call(CAPOIndexWrite, dirslice, run, s->seqno);
                s->seqno++;
                u64bReset(s->entries);
            }
        }
        done;
    }

    case DOG_OBJ_TREE:
    case DOG_OBJ_TAG:
    default:
        (void)blob; (void)path;
        done;
    }
}
