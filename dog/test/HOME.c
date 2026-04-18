//  HOME: TOML config getter keyed by dotted path.
//
#include "dog/HOME.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

static void seed_config(char const *root, char const *body) {
    char dir[256];
    snprintf(dir, sizeof(dir), "%s/.dogs", root);
    mkdir(dir, 0755);
    char path[256];
    snprintf(path, sizeof(path), "%s/.dogs/config", root);
    FILE *f = fopen(path, "w");
    fputs(body, f);
    fclose(f);
}

ok64 HOMETestGet() {
    sane(1);
    call(FILEInit);

    char tmp[] = "/tmp/dog-home-XXXXXX";
    want(mkdtemp(tmp) != NULL);

    seed_config(tmp,
        "# dogs config\n"
        "[user]\n"
        "name  = \"Ada Lovelace\"\n"
        "email = \"ada@example.com\"\n"
        "[remote]\n"
        "origin = \"ssh://host/repo\"\n"
        "[a.b]\n"
        "c = \"nested\"\n");

    a_cstr(root, tmp);
    home h = {};
    call(HOMEOpen, &h, root, NO);

    u8 vbuf[128];

    // --- hit: [user] name ---
    {
        u8s val = {vbuf, vbuf + sizeof(vbuf)};
        u8 *val_start = val[0];
        a_cstr(u, "user");
        a_cstr(n, "name");
        a_path(needle, u, n);
        call(HOMEGetConfig, &h, val, $path(needle));
        u8cs got = {val_start, val[0]};
        a_cstr(wantn, "Ada Lovelace");
        want($eq(got, wantn));
    }

    // --- hit: [remote] origin ---
    {
        u8s val = {vbuf, vbuf + sizeof(vbuf)};
        u8 *val_start = val[0];
        a_cstr(r, "remote");
        a_cstr(o, "origin");
        a_path(needle, r, o);
        call(HOMEGetConfig, &h, val, $path(needle));
        u8cs got = {val_start, val[0]};
        a_cstr(wanto, "ssh://host/repo");
        want($eq(got, wanto));
    }

    // --- hit: nested [a.b] c ---
    {
        u8s val = {vbuf, vbuf + sizeof(vbuf)};
        u8 *val_start = val[0];
        a_cstr(a, "a");
        a_cstr(b, "b");
        a_cstr(k, "c");
        a_path(needle, a, b, k);
        call(HOMEGetConfig, &h, val, $path(needle));
        u8cs got = {val_start, val[0]};
        a_cstr(wantn, "nested");
        want($eq(got, wantn));
    }

    // --- miss: wrong key ---
    {
        u8s val = {vbuf, vbuf + sizeof(vbuf)};
        a_cstr(u, "user");
        a_cstr(n, "nope");
        a_path(needle, u, n);
        want(HOMEGetConfig(&h, val, $path(needle)) == NOCONF);
    }

    // --- miss: wrong section ---
    {
        u8s val = {vbuf, vbuf + sizeof(vbuf)};
        a_cstr(s, "nope");
        a_cstr(n, "name");
        a_path(needle, s, n);
        want(HOMEGetConfig(&h, val, $path(needle)) == NOCONF);
    }

    HOMEClose(&h);
    char rm[512];
    snprintf(rm, sizeof(rm), "rm -rf %s", tmp);
    system(rm);
    done;
}

ok64 HOMETestMissingFile() {
    sane(1);
    call(FILEInit);

    char tmp[] = "/tmp/dog-home-XXXXXX";
    want(mkdtemp(tmp) != NULL);

    a_cstr(root, tmp);
    home h = {};
    call(HOMEOpen, &h, root, NO);

    u8 vbuf[64];
    u8s val = {vbuf, vbuf + sizeof(vbuf)};
    a_cstr(u, "user");
    a_cstr(n, "name");
    a_path(needle, u, n);
    want(HOMEGetConfig(&h, val, $path(needle)) == NOCONF);

    HOMEClose(&h);
    char rm[512];
    snprintf(rm, sizeof(rm), "rm -rf %s", tmp);
    system(rm);
    done;
}

ok64 maintest() {
    sane(1);
    fprintf(stderr, "HOMETestGet...\n");
    call(HOMETestGet);
    fprintf(stderr, "HOMETestMissingFile...\n");
    call(HOMETestMissingFile);
    fprintf(stderr, "all passed\n");
    done;
}

TEST(maintest)
