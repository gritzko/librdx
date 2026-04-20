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
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/CLI.h"
#include "dog/FRAG.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"
#include "spot/LESS.h"

// --- Verb / flag tables ---

char const *const SPOT_CLI_VERBS[] = {
    "get", "status", "help", NULL
};

//  Spot val-flags: -f -g -s -r -p -C --fork --proc --grep
//  --spot --replace --pcre --context
char const SPOT_CLI_VAL_FLAGS[] =
    "-f\0-g\0-s\0-r\0-p\0-C\0"
    "--fork\0--proc\0--grep\0--spot\0--replace\0--pcre\0--context\0";

// --- Helpers ---

static void spot_usage(void) {
    fprintf(stderr,
        "Usage: spot [--flags] [URI...]\n"
        "\n"
        "  spot                               incremental index update\n"
        "  spot -i | --index                  full reindex\n"
        "  spot -f N | --fork N               parallel reindex on N cores\n"
        "  spot -u | --uncommitted            index staged + unstaged changes\n"
        "  spot -U | --untracked              also index untracked (new) files\n"
        "  spot --hook                        post-commit incremental update\n"
        "  spot -s \"pattern\" .ext             code snippet search\n"
        "  spot -s \"pat\" -r \"repl\" .ext       code snippet search + replace\n"
        "  spot -g \"text\" [.ext]              grep (substring)\n"
        "  spot -p \"regex\" [.ext]             regex grep\n"
        "  spot '#pattern.ext'                URI-style search\n"
        "\n"
        "Patterns: single-letter placeholders (a-z match one token/group,\n"
        "A-Z match multiple tokens). Two spaces = skip gap.\n"
        "\n"
        "Diff/merge tools live in graf — see `graf --help`.\n"
    );
}

static b8 argIsExt(u8csc a) {
    if ($len(a) < 2 || a[0][0] != '.') return NO;
    return CAPOKnownExt(a);
}

// --- Entry ---

ok64 SPOTExec(cli *c) {
    sane(c);
    spotp dog = &SPOT;

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
    if ($eq(c->verb, v_get)) {
        call(CAPOHook, reporoot);
        done;
    }

    b8 do_index = CLIHas(c, "-i") || CLIHas(c, "--index");
    b8 do_update = CLIHas(c, "--update");
    b8 do_status = CLIHas(c, "--status");
    b8 is_hook = CLIHas(c, "--hook");
    b8 force_tlv = CLIHas(c, "-t") || CLIHas(c, "--tlv");
    b8 do_uncommitted = CLIHas(c, "-u") || CLIHas(c, "--uncommitted");
    b8 do_untracked = CLIHas(c, "-U") || CLIHas(c, "--untracked");
    if (do_untracked) do_uncommitted = YES;

    u32 nfork = 0;
    CLIFlag(v, c, "-f");
    if (!$empty(v)) nfork = (u32)atoi((char *)v[0]);
    CLIFlag(v, c, "--fork");
    if (!$empty(v)) nfork = (u32)atoi((char *)v[0]);

    u32 proc = UINT32_MAX;
    CLIFlag(v, c, "--proc");
    if (!$empty(v)) proc = (u32)atoi((char *)v[0]);

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
        !do_index && !is_hook && $empty(spot_rep);
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
    } else if (do_update) {
        ret = CAPOHook(reporoot);
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
    } else if (do_uncommitted) {
        ret = CAPOUncommitted(reporoot, do_untracked);
    } else if (is_hook) {
        ret = CAPOHook(reporoot);
    } else if (nfork > 0 && proc != UINT32_MAX) {
        ret = CAPOReindexProc(reporoot, nfork, proc);
    } else if (nfork > 0) {
        a_path(capodir);
        vcall("resolve_dir", CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));
        vcall("mkdir_p", FILEMakeDirP, $path(capodir));

        a_path(self);
        a$rg(a0, 0);
        a_cstr(spot_name, "spot");
        HOMEResolveSibling(NULL, self, spot_name, a0);

        pid_t pids[256];
        u32 n = nfork;
        if (n > 256) n = 256;

        fprintf(stderr, "spot: forking %u workers\n", n);
        a_dup(u8c, selfS, u8bDataC(self));
        char nstr[256][16] = {};
        char kstr[256][16] = {};
        for (u32 k = 0; k < n; k++) {
            snprintf(nstr[k], sizeof(nstr[k]), "%u", n);
            snprintf(kstr[k], sizeof(kstr[k]), "%u", k);
            u8cs wargs[] = {
                u8slit("spot"),
                u8slit("--fork"),
                u8scstr(nstr[k]),
                u8slit("--proc"),
                u8scstr(kstr[k]),
            };
            u8css wargv = {wargs, wargs + 5};
            vcall("spawn_worker", FILESpawn, selfS, wargv, NULL, NULL,
                  &pids[k]);
        }

        int failures = 0;
        for (u32 k = 0; k < n; k++) {
            int rc = 0;
            ok64 r = FILEReap(pids[k], &rc);
            if (r != OK || rc != 0) {
                fprintf(stderr, "spot: worker %u failed (status %d)\n",
                        k, rc);
                failures++;
            }
        }

        if (failures > 0)
            fprintf(stderr, "spot: %d workers failed\n", failures);

        fprintf(stderr, "spot: compacting all runs\n");
        vcall("compact_all", CAPOCompactAll, dirslice);
        vcall("commit_write", CAPOCommitWrite, reporoot, dirslice);
        fprintf(stderr, "spot: done\n");
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
    } else if (do_index) {
        ret = CAPOReindex(reporoot);
    } else if (c->nuris > 0) {
        fprintf(stderr, "spot: file display moved to bro\n");
        ret = FAILSANITY;
    } else {
        ret = CAPOHook(reporoot);
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
// Currently only blob objects contribute to the trigram/symbol
// index. Tree/commit/tag objects are no-ops here (sniff and graf
// handle those). `path` gives the repo-relative location the blob
// lives at — the extension is used to pick the tokenizer.
ok64 SPOTUpdate(u8 obj_type, u8cs blob, u8csc path) {
    sane(1);
    (void)obj_type; (void)blob; (void)path;
    // TODO: when obj_type == KEEP_OBJ_BLOB, extract ext from path
    // and call CAPOIndexFile on the blob content.
    done;
}
