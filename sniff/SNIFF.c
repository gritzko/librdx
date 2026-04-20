//  SNIFF: file path registry + filesystem change log.
//
//  Paths are owned by keeper (`.dogs/keeper/paths.log` via KEEPIntern /
//  KEEPPath / KEEPPathCount).  Sniff owns state.log: a flat append-only
//  stream of wh64 entries recording per-path state (BLOB/TREE hashlet,
//  CHECKOUT mtime, CHANGED mtime).
//
//  Singleton: `sniff SNIFF` in BSS; SNIFFOpen populates, SNIFFClose
//  tears down.  All helpers reach the singleton directly.
//
#include "SNIFF.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "keeper/GIT.h"
#include "keeper/PATHS.h"
#include "keeper/WALK.h"

// --- Singleton ---

sniff SNIFF = {};

static b8 sniff_is_open(void) { return SNIFF.h != NULL; }
static b8 sniff_is_rw = NO;

// On SNIFFOpen we may open keeper too; track whether we did so to
// decide on SNIFFClose whether to KEEPClose it.
static b8 sniff_opened_keep = NO;

// Append a wh64 pair (BLOB + CHECKOUT) to changes.  Bootstrap path:
// ls-files lists only regular files, so the base type is always BLOB.
static ok64 sniff_write_pair(u32 idx, u64 hashlet, u64 checkout) {
    sane(SNIFF.changes);
    wh64 h = wh64Pack(SNIFF_BLOB, idx, hashlet);
    wh64 c = wh64Pack(SNIFF_CHECKOUT, idx, checkout);
    call(FILEBookEnsure, SNIFF.changes, 2 * sizeof(wh64));
    u8p *idle = (u8p *)&SNIFF.changes[2];
    memcpy(*idle, &h, sizeof(wh64));
    *idle += sizeof(wh64);
    memcpy(*idle, &c, sizeof(wh64));
    *idle += sizeof(wh64);
    done;
}

// --- Bootstrap ---
//
//  Seed the path registry + state from `git ls-files -s` on the
//  worktree.  Only runs on a fresh SNIFFOpen(rw) with zero paths in
//  the keeper registry.
static ok64 sniff_bootstrap(u8cs reporoot) {
    sane($ok(reporoot));
    a_pad(u8, cmd, 2 * FILE_PATH_MAX_LEN);
    a_cstr(pre, "git -C ");
    a_cstr(suf, " ls-files -s 2>/dev/null");
    call(u8bFeed, cmd, pre);
    call(u8bFeed, cmd, reporoot);
    call(u8bFeed, cmd, suf);
    call(u8bFeed1, cmd, 0);
    FILE *fp = popen((char *)u8bDataHead(cmd), "r");
    if (!fp) done;

    char line[1024];
    u32 count = 0;
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = 0;
        if (len == 0) continue;

        char *tab = strchr(line, '\t');
        if (!tab) continue;
        char *sha_start = strchr(line, ' ');
        if (!sha_start || sha_start >= tab) continue;
        sha_start++;

        a_pad(u8, shabin, 8);
        u8cs hex16 = {(u8cp)sha_start, (u8cp)sha_start};
        if (tab - sha_start >= 16)
            hex16[1] = (u8cp)sha_start + 16;
        else
            continue;
        HEXu8sDrainSome(shabin_idle, hex16);

        a$str(path, tab + 1);
        u32 idx = KEEPIntern(&KEEP, path);

        u64 hashlet = WHIFFHashlet40((sha1cp)u8bDataHead(shabin));
        call(sniff_write_pair, idx, hashlet, 0);
        count++;
    }
    pclose(fp);
    if (count > 0)
        fprintf(stderr, "sniff: seeded %u path(s) from git\n", count);
    done;
}

// --- Open ---

ok64 SNIFFOpen(home *h, b8 rw) {
    sane(h);

    if (sniff_is_open()) {
        if (rw && !sniff_is_rw) return SNIFFOPENRO;
        return SNIFFOPEN;
    }

    // Open keeper singleton (nests into outer caller's KEEPOpen if any).
    ok64 kr = KEEPOpen(h, rw);
    if (kr != OK && kr != KEEPOPEN) return kr;
    sniff_opened_keep = (kr == OK);

    sniff *s = &SNIFF;
    zerop(s);
    s->h = h;
    sniff_is_rw = rw;

    a_dup(u8c, reporoot, u8bDataC(h->root));
    a_cstr(dogs, ".dogs");
    a_cstr(sniffdir, "sniff");
    a_path(dir, reporoot, dogs, sniffdir);
    if (rw) call(FILEMakeDirP, $path(dir));

    //  state.log: stream of 8-byte-aligned wh64 entries.  Scan by quad
    //  to skip the FILEBook zero-filled tail past logical EOF.
    #define SCAN_WH64_END(buf) do {                                \
        u8p e = (u8p)(buf)[3];                                     \
        u8p b = (u8p)(buf)[0];                                     \
        size_t n = (size_t)(e - b) / sizeof(wh64);                 \
        u64 *arr = (u64 *)b;                                       \
        while (n > 0 && arr[n - 1] == 0) n--;                      \
        ((u8 **)(buf))[2] = b + n * sizeof(wh64);                  \
    } while (0)

    {
        a_cstr(sf, "state.log");
        a_path(cp, reporoot, dogs, sniffdir, sf);
        ok64 o = FILEBook(&s->changes, $path(cp), SNIFF_CHG_BOOK);
        if (o == OK) {
            SCAN_WH64_END(s->changes);
        } else if (rw) {
            call(FILEBookCreate, &s->changes, $path(cp),
                 SNIFF_CHG_BOOK, 4096);
        } else {
            fail(o);
        }
    }
    #undef SCAN_WH64_END

    //  Bootstrap from git ls-files only on a fresh registry.
    if (rw && KEEPPathCount(&KEEP) == 0) {
        call(sniff_bootstrap, reporoot);
    }

    //  Ensure the root-dir path "/" is interned.  Its SNIFF_TREE
    //  hashlet is the base tree (staged by PUT/DELETE, committed by
    //  POST).
    if (rw) (void)SNIFFRootIdx();

    done;
}

//  Stub.  Keeper now owns path derivation during fetch (UNPK
//  KEEPIntern's tree entries); sniff doesn't need a separate
//  ingestion path.
ok64 SNIFFUpdate(u8 obj_type, u8cs blob, u8csc path) {
    (void)obj_type; (void)blob; (void)path;
    sane(1);
    done;
}

// --- Intern / Path / Count (thin wrappers over keeper) ---

ok64 SNIFFInternIdx(u8cs path, u32 *idx) {
    sane(idx);
    *idx = KEEPIntern(&KEEP, path);
    done;
}

u32 SNIFFIntern(u8cs path) {
    return KEEPIntern(&KEEP, path);
}

u32 SNIFFInternDir(u8cs path) {
    if ($empty(path)) return SNIFFRootIdx();
    if (*$last(path) == '/') return SNIFFIntern(path);
    a_pad(u8, tmp, 2048);
    u8bFeed(tmp, path);
    u8bFeed1(tmp, '/');
    u8cs dp = {u8bDataHead(tmp), tmp[2]};
    return SNIFFIntern(dp);
}

u32 SNIFFRootIdx(void) {
    a_cstr(root, "/");
    return SNIFFIntern(root);
}

u64 SNIFFBaseTree(void) {
    return SNIFFGet(SNIFF_TREE, SNIFFRootIdx());
}

ok64 SNIFFPath(u8csp out, u32 index) {
    sane(out);
    return KEEPPath(&KEEP, index, out);
}

u32 SNIFFCount(void) {
    return KEEPPathCount(&KEEP);
}

// --- Sort ---

static int sniff_cmp_idx(void const *a, void const *b) {
    u8cs pa = {}, pb = {};
    SNIFFPath(pa, *(u32 const *)a);
    SNIFFPath(pb, *(u32 const *)b);
    return u8cscmp(&pa, &pb);
}

ok64 SNIFFSort(void) {
    sane(1);
    sniff *s = &SNIFF;
    u32 n = SNIFFCount();
    if (u32bDataLen(s->sorted) > 0) u32bReset(s->sorted);
    if (n == 0) done;
    if (u32bLen(s->sorted) < n) {
        u32bFree(s->sorted);
        call(u32bAllocate, s->sorted, n);
    }
    for (u32 i = 0; i < n; i++) {
        u32 **idle = u32bIdle(s->sorted);
        **idle = i;
        ++*idle;
    }
    qsort(u32bDataHead(s->sorted), n, sizeof(u32), sniff_cmp_idx);
    done;
}

// --- Record / Get ---

ok64 SNIFFRecord(u8 type, u32 index, u64 off) {
    sane(SNIFF.changes);
    wh64 entry = wh64Pack(type, index, off);
    call(FILEBookEnsure, SNIFF.changes, sizeof(wh64));
    u8p *idle = (u8p *)&SNIFF.changes[2];
    memcpy(*idle, &entry, sizeof(wh64));
    *idle += sizeof(wh64);
    done;
}

u64 SNIFFGet(u8 type, u32 index) {
    u64cp head = (u64cp)u8bDataHead(SNIFF.changes);
    u64cp end  = (u64cp)u8bIdleHead(SNIFF.changes);
    while (end > head) {
        --end;
        wh64 e = *end;
        if (e != 0 && wh64Type(e) == type && wh64Id(e) == index)
            return wh64Off(e);
    }
    return 0;
}

// --- Compact ---

ok64 SNIFFCompact(void) {
    sane(1);
    sniff *s = &SNIFF;
    u32 n = SNIFFCount();
    if (n == 0) done;

    typedef struct {
        u32 idx; u64 hashlet; u64 checkout; u8 type;
    } live_entry;
    Bu8 ent_mem = {};
    call(u8bAllocate, ent_mem, (u64)n * sizeof(live_entry));
    live_entry *entries = (live_entry *)u8bHead(ent_mem);
    u32 live = 0;

    for (u32 i = 0; i < n; i++) {
        u8cs p = {};
        if (SNIFFPath(p, i) != OK) continue;
        u8 t = SNIFFIsDir(i) ? SNIFF_TREE : SNIFF_BLOB;
        u64 h = SNIFFGet(t, i);
        u64 c = SNIFFGet(SNIFF_CHECKOUT, i);
        if (h == 0 && c == 0) continue;
        entries[live].idx = i;
        entries[live].hashlet = h;
        entries[live].checkout = c;
        entries[live].type = t;
        live++;
    }

    u8bReset(s->changes);
    for (u32 i = 0; i < live; i++) {
        wh64 h = wh64Pack(entries[i].type, entries[i].idx, entries[i].hashlet);
        wh64 c = wh64Pack(SNIFF_CHECKOUT, entries[i].idx, entries[i].checkout);
        call(FILEBookEnsure, s->changes, 2 * sizeof(wh64));
        u8p *idle = (u8p *)&s->changes[2];
        memcpy(*idle, &h, sizeof(wh64)); *idle += sizeof(wh64);
        memcpy(*idle, &c, sizeof(wh64)); *idle += sizeof(wh64);
    }

    u8bFree(ent_mem);
    done;
}

// --- Close ---

ok64 SNIFFClose(void) {
    sane(1);
    if (!sniff_is_open()) return OK;
    sniff *s = &SNIFF;
    if (s->changes) { FILETrimBook(s->changes); FILEUnBook(s->changes); }
    u32bFree(s->sorted);
    zerop(s);
    sniff_is_rw = NO;
    //  If we opened the keeper as part of SNIFFOpen, close it too.
    if (sniff_opened_keep) {
        sniff_opened_keep = NO;
        KEEPClose();
    }
    done;
}

// --- Parent-commit helpers ---

ok64 SNIFFParentTreeSha(sha1 *tree_out, u8cs parent_hex) {
    sane(tree_out && $ok(parent_hex));

    size_t hexlen = $len(parent_hex);
    if (hexlen > 15) hexlen = 15;
    u64 hashlet = WHIFFHexHashlet60(parent_hex);

    Bu8 cbuf = {};
    call(u8bAllocate, cbuf, 1UL << 24);
    u8 ctype = 0;
    ok64 o = KEEPGet(&KEEP, hashlet, hexlen, cbuf, &ctype);
    if (o != OK) { u8bFree(cbuf); return o; }

    if (ctype == DOG_OBJ_TAG) {
        u8cs body = {u8bDataHead(cbuf), u8bIdleHead(cbuf)};
        u8cs field = {}, value = {};
        sha1 tag_sha = {};
        a_raw(tag_bin, tag_sha);
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if ($empty(field)) break;
            if ($len(field) == 6 && memcmp(field[0], "object", 6) == 0 &&
                $len(value) >= 40) {
                u8cs hex40 = {value[0], $atp(value, 40)};
                HEXu8sDrainSome(tag_bin, hex40);
                break;
            }
        }
        u64 ch = WHIFFHashlet60(&tag_sha);
        u8bReset(cbuf);
        o = KEEPGet(&KEEP, ch, 15, cbuf, &ctype);
        if (o != OK) { u8bFree(cbuf); return o; }
    }
    if (ctype != DOG_OBJ_COMMIT) { u8bFree(cbuf); fail(SNIFFFAIL); }

    u8cs commit_body = {u8bDataHead(cbuf), u8bIdleHead(cbuf)};
    o = GITu8sCommitTree(commit_body, tree_out->data);
    u8bFree(cbuf);
    return o;
}

typedef struct {
    sha1 *shas;
    u32   capacity;
} collect_ctx;

static ok64 collect_visit(u8cs path, u8 kind, u8cp esha, u8cs blob,
                           void0p vctx) {
    (void)blob;
    collect_ctx *c = (collect_ctx *)vctx;
    if (kind == WALK_KIND_SUB) return WALKSKIP;

    u32 idx;
    if (kind == WALK_KIND_DIR && $empty(path))
        idx = SNIFFRootIdx();
    else if (kind == WALK_KIND_DIR)
        idx = SNIFFInternDir(path);
    else
        idx = SNIFFIntern(path);

    if (idx < c->capacity)
        memcpy(c->shas[idx].data, esha, 20);
    return OK;
}

ok64 SNIFFCollectParentTree(u8cs parent_hex, sha1 *sha_tab, u32 capacity) {
    sane(sha_tab);
    if (!$ok(parent_hex) || $empty(parent_hex)) done;

    sha1 tree_sha = {};
    call(SNIFFParentTreeSha, &tree_sha, parent_hex);

    collect_ctx ctx = {.shas = sha_tab, .capacity = capacity};
    return WALKTreeLazy(&KEEP, tree_sha.data, collect_visit, &ctx);
}

ok64 SNIFFCollectBaseTree(sha1 *sha_tab, u32 capacity) {
    sane(sha_tab);
    u64 base = SNIFFBaseTree();
    if (base == 0) done;

    Bu8 tbuf = {};
    call(u8bAllocate, tbuf, 1UL << 24);
    u8 otype = 0;
    ok64 o = KEEPGet(&KEEP, base << 20, 10, tbuf, &otype);
    if (o != OK || otype != DOG_OBJ_TREE) {
        u8bFree(tbuf);
        return o == OK ? SNIFFFAIL : o;
    }
    sha1 tree_sha = {};
    KEEPObjSha(&tree_sha, DOG_OBJ_TREE, u8bDataC(tbuf));
    u8bFree(tbuf);

    collect_ctx ctx = {.shas = sha_tab, .capacity = capacity};
    return WALKTreeLazy(&KEEP, tree_sha.data, collect_visit, &ctx);
}
