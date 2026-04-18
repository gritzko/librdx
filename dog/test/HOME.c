//  HOME: TOML config getter.
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
        "origin = \"ssh://host/repo\"\n");

    a_cstr(root, tmp);
    home h = {};
    call(HOMEOpen, &h, root, NO);

    u8 vbuf[128];

    // --- hit: [user] name ---
    {
        u8s val = {vbuf, vbuf + sizeof(vbuf)};
        u8 *val_start = val[0];
        a_cstr(sect, "user");
        a_cstr(key,  "name");
        call(HOMEGetConfig, &h, val, sect, key);
        u8cs got = {val_start, val[0]};
        a_cstr(wantn, "Ada Lovelace");
        want($eq(got, wantn));
    }

    // --- hit: [user] email ---
    {
        u8s val = {vbuf, vbuf + sizeof(vbuf)};
        u8 *val_start = val[0];
        a_cstr(sect, "user");
        a_cstr(key,  "email");
        call(HOMEGetConfig, &h, val, sect, key);
        u8cs got = {val_start, val[0]};
        a_cstr(wante, "ada@example.com");
        want($eq(got, wante));
    }

    // --- hit: [remote] origin ---
    {
        u8s val = {vbuf, vbuf + sizeof(vbuf)};
        u8 *val_start = val[0];
        a_cstr(sect, "remote");
        a_cstr(key,  "origin");
        call(HOMEGetConfig, &h, val, sect, key);
        u8cs got = {val_start, val[0]};
        a_cstr(wanto, "ssh://host/repo");
        want($eq(got, wanto));
    }

    // --- miss: wrong key ---
    {
        u8s val = {vbuf, vbuf + sizeof(vbuf)};
        a_cstr(sect, "user");
        a_cstr(key,  "nope");
        want(HOMEGetConfig(&h, val, sect, key) == NOCONF);
    }

    // --- miss: wrong section ---
    {
        u8s val = {vbuf, vbuf + sizeof(vbuf)};
        a_cstr(sect, "nope");
        a_cstr(key,  "name");
        want(HOMEGetConfig(&h, val, sect, key) == NOCONF);
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
    a_cstr(sect, "user");
    a_cstr(key,  "name");
    want(HOMEGetConfig(&h, val, sect, key) == NOCONF);

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
