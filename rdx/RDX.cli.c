#include "RDX.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "abc/01.h"
#include "abc/BUF.h"
#include "abc/FILE.h"
#include "abc/NACL.h"
#include "abc/POL.h"
#include "abc/PRO.h"
#include "abc/S.h"

a_cstr(USAGE, "Usage: rdx verb inputs\n");

a_cstr(EXT_JDR, ".jdr");
a_cstr(EXT_TLV, ".tlv");
a_cstr(EXT_SKIL, ".skil");

// Repository structure constants
a_cstr(RDX_DIR, ".rdx");
a_cstr(RDX_KEY_FILE, "key");
a_cstr(RDX_BRANCHES_DIR, "branches");
a_cstr(RDX_MAIN_BRANCH, "main");
a_cstr(RDX_CURRENT_LINK, "current");  // symlink to current branch

u8 RDX_FMT_DEFAULT = RDX_FMT_JDR;

ok64 AddFileInput(voidp arg, path8p path) {
    sane(arg && path8Sane(path));
    rdxbp inputs = (rdxbp)arg;
    rdxp n = 0;
    call(rdxbFedP, inputs, &n);
    if (u8csHasSuffix(u8bDataC(path), EXT_JDR)) {
        n->format = RDX_FMT_JDR;
    } else if (u8csHasSuffix(u8bDataC(path), EXT_SKIL)) {
        n->format = RDX_FMT_SKIL;
    } else if (u8csHasSuffix(u8bDataC(path), EXT_TLV)) {
        n->format = RDX_FMT_TLV;
    } else {
        done;
    }
    int fd = FILE_CLOSED;
    u8b buf = {};
    call(FILEMapRO, buf, path);
    $mv(n->data, u8bData(buf));
    done;
}

// Expand @branchname to .rdx/branches/branchname/
ok64 ExpandBranchPath(path8 path, u8csp arg) {
    sane(u8bOK(path) && u8csOK(arg));
    if (u8csLen(arg) > 1 && **arg == '@') {
        // @branch syntax - expand to .rdx/branches/branch/
        call(u8bFeed, path, RDX_DIR);
        call(path8Push, path, RDX_BRANCHES_DIR);
        a_rest(u8c, branchname, arg, 1);  // skip '@'
        call(path8Push, path, branchname);
    } else {
        call(u8bFeed, path, arg);
    }
    done;
}

ok64 AddInput(rdxbp inputs, u8csp arg) {
    sane(rdxbOK(inputs) && u8csOK(arg));
    a_path(path, "");
    call(ExpandBranchPath, path, arg);
    struct stat s = {};

    if (OK == FILEStat(&s, path)) {
        if ((s.st_mode & S_IFMT) == S_IFDIR) {
            call(FILEScanFiles, path, AddFileInput, (voidp)inputs);
        } else {
            call(AddFileInput, (voidp)inputs, path);
        }
    } else {
        rdxp n = 0;
        call(rdxbFedP, inputs, &n);
        zerop(n);
        n->format = RDX_FMT_JDR;
        $mv(n->data, arg);
    }

    done;
}

ok64 CmdY(rdxg inputs, u8 fmt) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    rdx in = {};
    if (rdxgLeftLen(inputs) == 1) {
        in = rdxsAt(rdxgLeft(inputs), 0);
    } else {
        in.format = RDX_FMT_Y;
        rdxgMv(in.ins, inputs);
    }
    rdx out = {.format = fmt | RDX_FMT_WRITE};
    int fd = STDOUT_FILENO;
    call(u8bMap, FILE_BUFS[fd], GB * 2);
    $mv(out.into, u8bIdle(FILE_BUFS[fd]));  // FIXME
    try(rdxCopyF, &out, &in, (voidf)FILEFlush, &fd);
    $mv(u8bIdle(FILE_BUFS[fd]), out.into);  // FIXME
    then try(FILEFlushAll, &fd);
    u8bUnMap(FILE_BUFS[fd]);
    done;
}

a_cstr(VERB_JDR, "jdr");
a_cstr(VERB_MERJ, "merj");
ok64 CmdJDR(rdxg inputs) { return CmdY(inputs, RDX_FMT_JDR); }

a_cstr(VERB_Q, "q");
a_cstr(VERB_QUERY, "query");
ok64 CmdQuery(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    done;
}

a_cstr(VERB_HASH, "hash");
ok64 CmdHash(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    rdx in = {};
    if (rdxgLeftLen(inputs) == 1) {
        in = rdxsAt(rdxgLeft(inputs), 0);
    } else {
        in.format = RDX_FMT_Y;
        rdxgMv(in.ins, inputs);
    }
    blake256 blake = {};
    call(rdxHashBlake, &in, &blake);
    a_rawc(hash, blake);
    a_pad(u8, hex, 65);
    HEXFeed(hex_idle, hash);
    u8sFeed1(hex_idle, 0);
    printf("Simple BLAKE256: %s\n", *hex_data);
    done;
}

a_cstr(VERB_Y, "y");
a_cstr(VERB_MERGE, "merge");
a_cstr(VERB_TLV, "tlv");
a_cstr(VERB_STRIP, "strip");
a_cstr(VERB_NOW, "now");
a_cstr(VERB_NORM, "norm");
a_cstr(VERB_CAT, "cat");
a_cstr(VERB_INIT, "init");
a_cstr(VERB_ADD, "add");
a_cstr(VERB_BRANCH, "branch");
a_cstr(VERB_DROP, "drop");
a_cstr(VERB_FORK, "fork");
a_cstr(VERB_USE, "use");

ok64 CmdNorm(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    int fd = STDOUT_FILENO;
    call(u8bMap, FILE_BUFS[fd], GB * 2);
    rdx out = {.format = RDX_FMT_DEFAULT | RDX_FMT_WRITE};
    $mv(out.into, u8bIdle(FILE_BUFS[fd]));
    call(rdxMerge, &out, inputs);
    $mv(u8bIdle(FILE_BUFS[fd]), out.into);
    u8bFeed1(FILE_BUFS[fd], '\n');
    try(FILEFlushAll, &fd);
    u8bUnMap(FILE_BUFS[fd]);
    done;
}

ok64 CmdCat(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    int fd = STDOUT_FILENO;
    call(u8bMap, FILE_BUFS[fd], GB * 2);
    rdx out = {.format = RDX_FMT_JDR | RDX_FMT_WRITE};
    $mv(out.into, u8bIdle(FILE_BUFS[fd]));
    call(rdxMerge, &out, inputs);
    $mv(u8bIdle(FILE_BUFS[fd]), out.into);
    u8bFeed1(FILE_BUFS[fd], '\n');
    try(FILEFlushAll, &fd);
    u8bUnMap(FILE_BUFS[fd]);
    done;
}

ok64 CmdStrip(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));
    int fd = STDOUT_FILENO;
    call(u8bMap, FILE_BUFS[fd], GB * 2);
    rdx out = {.format = RDX_FMT_DEFAULT | RDX_FMT_WRITE};
    $mv(out.into, u8bIdle(FILE_BUFS[fd]));
    call(rdxStrip, &out, *inputs);
    $mv(u8bIdle(FILE_BUFS[fd]), out.into);
    u8bFeed1(FILE_BUFS[fd], '\n');
    try(FILEFlushAll, &fd);
    u8bUnMap(FILE_BUFS[fd]);
    done;
}

ron60 RONNow();
ok64 CmdNow(rdxg inputs) {
    ron60 now = RONNow();
    printf("%s\n", ok64str(now));
    return OK;
}

// Extract 60-bit replica ID from public key (first 10 RON Base64 chars)
fun ok64 ReplicaIDFromPubKey(u8s into, edpub256 const* pubkey) {
    sane(into && pubkey);
    // Take first 60 bits and encode as RON Base64
    u64 bits = 0;
    memcpy(&bits, pubkey->_8, sizeof(u64));
    bits &= ron60Max;  // mask to 60 bits
    call(RONutf8sFeed64, into, bits);
    done;
}

// Initialize .rdx repository with keypair
ok64 CmdInit(rdxg inputs) {
    sane(1);

    // Build .rdx path
    a_path(rdx_path, "");
    call(u8bFeed, rdx_path, RDX_DIR);

    // Check if already initialized
    struct stat s = {};
    if (OK == FILEStat(&s, rdx_path)) {
        fprintf(stderr, "Repository already initialized at %s\n", *RDX_DIR);
        fail(RDXBADARG);
    }

    // Create .rdx directory
    call(FILEMakeDir, rdx_path);

    // Generate keypair
    edpub256 pubkey = {};
    edsec512 seckey = {};
    call(NACLed25519create, &pubkey, &seckey);

    // Save keypair (secret key followed by public key, 64+32=96 bytes)
    a_path(key_path, "");
    call(u8bFeed, key_path, RDX_DIR);
    call(path8Push, key_path, RDX_KEY_FILE);
    int fd = FILE_CLOSED;
    call(FILECreate, &fd, key_path);
    a_rawc(secdata, seckey);
    a_rawc(pubdata, pubkey);
    call(FILEFeedall, fd, secdata);
    call(FILEFeedall, fd, pubdata);
    call(FILEClose, &fd);

    // Create branches directory
    a_path(branches_path, "");
    call(u8bFeed, branches_path, RDX_DIR);
    call(path8Push, branches_path, RDX_BRANCHES_DIR);
    call(FILEMakeDir, branches_path);

    // Create main branch directory
    a_path(main_path, "");
    call(u8bFeed, main_path, RDX_DIR);
    call(path8Push, main_path, RDX_BRANCHES_DIR);
    call(path8Push, main_path, RDX_MAIN_BRANCH);
    call(FILEMakeDir, main_path);

    // Print replica ID (60 bits of pubkey as RON Base64)
    a_pad(u8, repid, 16);
    call(ReplicaIDFromPubKey, repid_idle, &pubkey);
    u8sFeed1(repid_idle, 0);
    printf("Initialized repository with replica ID: %s\n", *repid_data);

    done;
}

// Load replica ID from .rdx/key (pubkey is at offset 64)
ok64 LoadReplicaID(ron60* id) {
    sane(id);
    a_path(key_path, "");
    call(u8bFeed, key_path, RDX_DIR);
    call(path8Push, key_path, RDX_KEY_FILE);

    u8b buf = {};
    call(FILEMapRO, buf, key_path);
    // Key file is 96 bytes: 64 secret + 32 public
    test(u8bDataLen(buf) == sizeof(edsec512) + sizeof(edpub256), RDXBADFILE);

    edpub256 pubkey = {};
    memcpy(&pubkey, *u8bDataC(buf) + sizeof(edsec512), sizeof(edpub256));
    u8bUnMap(buf);

    // Extract 60-bit ID
    u64 bits = 0;
    memcpy(&bits, pubkey._8, sizeof(u64));
    *id = bits & ron60Max;
    done;
}

// Get current branch name (reads symlink, defaults to "main")
// Writes branch name to provided buffer, returns slice to the name
ok64 GetCurrentBranch(u8b namebuf) {
    sane(u8bOK(namebuf));
    
    a_path(link_path, "");
    call(u8bFeed, link_path, RDX_DIR);
    call(path8Push, link_path, RDX_CURRENT_LINK);
    
    a_pad(u8, target, FILE_PATH_MAX_LEN);
    ssize_t len = readlink(path8CStr(link_path), (char*)*target_data, FILE_PATH_MAX_LEN - 1);
    
    if (len < 0) {
        // No symlink, default to main
        call(u8sFeed, u8bIdle(namebuf), RDX_MAIN_BRANCH);
    } else {
        // Extract branch name after last /
        target_data[0][len] = 0;
        u8c const* name = *target_data;
        for (ssize_t i = 0; i < len; i++) {
            if (target_data[0][i] == '/') name = target_data[0] + i + 1;
        }
        u8cs namestr = {name, target_data[0] + len};
        call(u8sFeed, u8bIdle(namebuf), namestr);
    }
    done;
}

// Add input data to current branch, creating a new .skil file
ok64 CmdAdd(rdxg inputs) {
    sane(rdxgOK(inputs) && rdxgLeftLen(inputs));

    // Load replica ID
    ron60 repid = 0;
    call(LoadReplicaID, &repid);

    // Get current timestamp
    ron60 now = RONNow();

    // Build filename: timestamp_replicaid.skil
    a_pad(u8, fname, 64);
    call(RONutf8sFeed64, fname_idle, now);
    u8sFeed1(fname_idle, '_');
    call(RONutf8sFeed64, fname_idle, repid);
    a_cstr(skilext, ".skil");
    call(u8sFeed, fname_idle, skilext);
    u8sFeed1(fname_idle, 0);

    // Get current branch
    a_pad(u8, branchname, 64);
    call(GetCurrentBranch, branchname);
    
    // Build path: .rdx/branches/<current>/<filename>
    a_path(filepath, "");
    call(u8bFeed, filepath, RDX_DIR);
    call(path8Push, filepath, RDX_BRANCHES_DIR);
    call(path8Push, filepath, branchname_datac);
    call(path8Push, filepath, fname_datac);

    // Merge all inputs and write as SKIL
    // Use memory-mapped buffer to avoid stack overflow
    u8b outbuf = {};
    call(u8bMap, outbuf, GB);
    
    rdx out = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE};
    $mv(out.into, u8bIdle(outbuf));

    // Set up skip list tracking
    a_pad0(u64, skipbuf, 1024);
    out.extra = (voidp)skipbuf;

    call(rdxMerge, &out, inputs);
    $mv(u8bIdle(outbuf), out.into);

    // Write to file
    size_t datalen = u8bDataLen(outbuf);
    int fd = FILE_CLOSED;
    call(FILECreate, &fd, filepath);
    call(FILEFeedall, fd, u8bDataC(outbuf));
    call(FILEClose, &fd);
    u8bUnMap(outbuf);

    printf("Created %s (%zu bytes)\n", path8CStr(filepath), datalen);
    done;
}

// List branches callback
ok64 PrintBranchName(voidp arg, path8p path) {
    sane(path8Sane(path));
    u8csp data = u8bDataC(path);
    // Find last component
    u8c const* name = *data;
    $for(u8c, p, data) if (*p == '/') name = p + 1;
    u8c const* end = u8csTerm(data);
    // Skip . and ..
    if (end - name == 1 && *name == '.') done;
    if (end - name == 2 && name[0] == '.' && name[1] == '.') done;
    if (end > name) printf("  %.*s\n", (int)(end - name), name);
    done;
}

// rdx branch - list all branches
ok64 CmdBranch(rdxg inputs) {
    sane(rdxgOK(inputs));
    a_path(branches_path, "");
    call(u8bFeed, branches_path, RDX_DIR);
    call(path8Push, branches_path, RDX_BRANCHES_DIR);
    printf("Branches:\n");
    call(FILEScan, branches_path, FILE_SCAN_DIRS, PrintBranchName, NULL);
    done;
}

// rdx drop @branch - delete a branch
ok64 CmdDrop(u8css rawargs) {
    sane(u8cssLen(rawargs) == 1);
    
    u8csp arg = *u8cssAtP(rawargs, 0);
    test(u8csLen(arg) > 1 && **arg == '@', RDXBADARG);
    a_rest(u8c, name, arg, 1);
    
    // Prevent dropping main
    test(!$eq(name, RDX_MAIN_BRANCH), RDXBADARG);
    
    a_path(branch_path, "");
    call(u8bFeed, branch_path, RDX_DIR);
    call(path8Push, branch_path, RDX_BRANCHES_DIR);
    call(path8Push, branch_path, name);
    
    struct stat s = {};
    test(OK == FILEStat(&s, branch_path), RDXBADARG);
    
    // Delete branch directory recursively
    call(FILERmDir, branch_path, true);
    
    printf("Dropped branch: %.*s\n", (int)u8csLen(name), (char*)*name);
    done;
}

// Hard-link files from source to dest directory
// Note: path8 uses u8b where PAST contains the directory prefix and DATA is the filename.
// The full path is from buffer start to end of DATA. Use path8CStr() to get full path.
ok64 HardLinkFile(voidp arg, path8p src) {
    sane(arg && path8Sane(src));
    path8p dest_dir = (path8p)arg;
    
    // Get filename from source path (last component after final '/')
    u8c const* src_path_start = dest_dir[0];  // buffer start
    u8c const* src_path_end = *u8bDataC(src) + u8bDataLen(src);  // end of DATA
    
    u8csp srcdata = u8bDataC(src);
    u8c const* fname = *srcdata;
    $for(u8c, p, srcdata) if (*p == '/') fname = p + 1;
    u8cs filename = {fname, u8csTerm(srcdata)};
    
    // Build dest path: copy full dest_dir path, then append filename
    a_path(dest, "");
    // Full path is from buffer start to end of data
    u8c const* dest_start = dest_dir[0];
    u8c const* dest_end = *u8bDataC(dest_dir) + u8bDataLen(dest_dir);
    u8cs dest_full = {dest_start, dest_end};
    call(u8bFeed, dest, dest_full);
    call(path8Push, dest, filename);
    
    int ret = link(path8CStr(src), path8CStr(dest));
    testc(ret == 0, FILEfail);
    done;
}

// rdx fork newname [@sources...]
ok64 CmdFork(u8css args) {
    sane(u8cssLen(args) >= 1);
    
    // First arg is new branch name (may have @ prefix)
    u8csp newname = *u8cssAtP(args, 0);
    if (u8csLen(newname) > 0 && **newname == '@') {
        a_rest(u8c, rest, newname, 1);
        newname = rest;
    }
    
    // Create new branch directory
    a_path(new_path, "");
    call(u8bFeed, new_path, RDX_DIR);
    call(path8Push, new_path, RDX_BRANCHES_DIR);
    call(path8Push, new_path, newname);
    
    struct stat s = {};
    if (OK == FILEStat(&s, new_path)) {
        fprintf(stderr, "Branch already exists: %.*s\n", 
                (int)u8csLen(newname), (char*)*newname);
        fail(RDXBADARG);
    }
    call(FILEMakeDir, new_path);
    
    // If only one arg, just create empty branch
    if (u8cssLen(args) == 1) {
        printf("Created branch: %.*s\n", (int)u8csLen(newname), (char*)*newname);
        done;
    }
    
    // If exactly one source with @, hard-link files
    if (u8cssLen(args) == 2) {
        u8csp srcarg = *u8cssAtP(args, 1);
        if (u8csLen(srcarg) > 1 && **srcarg == '@') {
            a_rest(u8c, srcname, srcarg, 1);
            a_path(src_path, "");
            call(u8bFeed, src_path, RDX_DIR);
            call(path8Push, src_path, RDX_BRANCHES_DIR);
            call(path8Push, src_path, srcname);
            
            call(FILEScanFiles, src_path, HardLinkFile, (voidp)new_path);
            printf("Forked branch: %.*s from @%.*s\n",
                   (int)u8csLen(newname), (char*)*newname,
                   (int)u8csLen(srcname), (char*)*srcname);
            done;
        }
    }
    
    // Multiple sources - merge them into a new .skil file
    // Process args as inputs (skip first which is branch name)
    a_pad0(rdx, merge_inputs, 64);
    a_rest(u8cs, rest_args, args, 1);
    $for(u8cs, arg, rest_args) {
        call(AddInput, merge_inputs, *arg);
    }
    
    // Load replica ID and timestamp
    ron60 repid = 0;
    call(LoadReplicaID, &repid);
    ron60 now = RONNow();
    
    // Build filename
    a_pad(u8, fname, 64);
    call(RONutf8sFeed64, fname_idle, now);
    u8sFeed1(fname_idle, '_');
    call(RONutf8sFeed64, fname_idle, repid);
    call(u8sFeed, fname_idle, EXT_SKIL);
    u8sFeed1(fname_idle, 0);
    
    a_path(out_path, "");
    // Full path is from buffer start to end of data
    u8c const* np_start = new_path[0];
    u8c const* np_end = *u8bDataC(new_path) + u8bDataLen(new_path);
    u8cs np_full = {np_start, np_end};
    u8bFeed(out_path, np_full);
    call(path8Push, out_path, fname_datac);
    
    // Merge and write
    u8b outbuf = {};
    call(u8bMap, outbuf, GB);
    rdx out = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE};
    $mv(out.into, u8bIdle(outbuf));
    a_pad0(u64, skipbuf, 1024);
    out.extra = (voidp)skipbuf;
    
    rdxgp din = rdxbDataIdle(merge_inputs);
    call(rdxMerge, &out, din);
    $mv(u8bIdle(outbuf), out.into);
    
    size_t datalen = u8bDataLen(outbuf);
    int fd = FILE_CLOSED;
    call(FILECreate, &fd, out_path);
    call(FILEFeedall, fd, u8bDataC(outbuf));
    call(FILEClose, &fd);
    u8bUnMap(outbuf);
    
    printf("Created branch: %.*s (merged, %zu bytes)\n",
           (int)u8csLen(newname), (char*)*newname, datalen);
    done;
}

// rdx use @branch - set current branch via symlink
// rdx use (no args) - show current branch
ok64 CmdUse(u8css rawargs) {
    sane(1);
    
    a_path(link_path, "");
    call(u8bFeed, link_path, RDX_DIR);
    call(path8Push, link_path, RDX_CURRENT_LINK);
    
    if (u8cssLen(rawargs) == 0) {
        // Show current branch
        a_pad(u8, target, FILE_PATH_MAX_LEN);
        ssize_t len = readlink(path8CStr(link_path), (char*)*target_data, FILE_PATH_MAX_LEN - 1);
        if (len < 0) {
            printf("Current branch: main (default)\n");
        } else {
            // Target is like "branches/feature" - extract branch name after last /
            target_data[0][len] = 0;  // null terminate
            u8c const* name = *target_data;
            for (ssize_t i = 0; i < len; i++) {
                if (target_data[0][i] == '/') name = target_data[0] + i + 1;
            }
            printf("Current branch: %s\n", name);
        }
        done;
    }
    
    // Set current branch
    test(u8cssLen(rawargs) == 1, RDXBADARG);
    u8csp arg = *u8cssAtP(rawargs, 0);
    test(u8csLen(arg) > 1 && **arg == '@', RDXBADARG);
    a_rest(u8c, name, arg, 1);
    
    // Verify branch exists
    a_path(branch_path, "");
    call(u8bFeed, branch_path, RDX_DIR);
    call(path8Push, branch_path, RDX_BRANCHES_DIR);
    call(path8Push, branch_path, name);
    
    struct stat s = {};
    if (OK != FILEStat(&s, branch_path)) {
        fprintf(stderr, "Branch not found: %.*s\n", (int)u8csLen(name), (char*)*name);
        fail(RDXBADARG);
    }
    
    // Build relative symlink target: branches/<name>
    a_pad(u8, target, FILE_PATH_MAX_LEN);
    call(u8sFeed, target_idle, RDX_BRANCHES_DIR);
    u8sFeed1(target_idle, '/');
    call(u8sFeed, target_idle, name);
    u8sFeed1(target_idle, 0);
    
    // Remove old symlink if exists
    unlink(path8CStr(link_path));
    
    // Create new symlink
    int ret = symlink((char*)*target_data, path8CStr(link_path));
    testc(ret == 0, FILEfail);
    
    printf("Switched to branch: %.*s\n", (int)u8csLen(name), (char*)*name);
    done;
}

ok64 rdxcli() {
    sane(1);
    u8cssp args = u8csbData(STD_ARGS);
    if (u8cssLen(args) < 2) {
        FILEerr(USAGE);
        fail(BADARG);
    }
    call(FILEInit);

    if (u8csHasSuffix(*u8csbAtP(STD_ARGS, 0), EXT_JDR)) {
        RDX_FMT_DEFAULT = RDX_FMT_JDR;
    } else if (u8csHasSuffix(*u8csbAtP(STD_ARGS, 0), EXT_TLV)) {
        RDX_FMT_DEFAULT = RDX_FMT_TLV;
    } else if (u8csHasSuffix(*u8csbAtP(STD_ARGS, 0), EXT_SKIL)) {
        RDX_FMT_DEFAULT = RDX_FMT_SKIL;
    }

    // rdx merge|hash|strip|jdr|tlv|etc inputs*

    u8csp verb = *u8csbAtP(STD_ARGS, 1);

    a_pad0(rdx, inputs, 64);
    a_rest(u8cs, inn, u8csbData(STD_ARGS), 2);
    $for(u8cs, arg, inn) call(AddInput, inputs, *arg);

    rdxgp din = rdxbDataIdle(inputs);

    if ($eq(verb, VERB_JDR)) {
        call(CmdY, din, RDX_FMT_JDR);
    } else if ($eq(verb, VERB_JDR)) {
        call(CmdY, rdxbDataIdle(inputs), RDX_FMT_JDR);
    } else if ($eq(verb, VERB_Q)) {
        call(CmdQuery, din);
    } else if ($eq(verb, VERB_HASH)) {
        call(CmdHash, din);
    } else if ($eq(verb, VERB_QUERY)) {
        call(CmdQuery, din);
    } else if ($eq(verb, VERB_MERGE)) {
        call(CmdY, din, RDX_FMT_DEFAULT);
    } else if ($eq(verb, VERB_TLV)) {
        call(CmdY, din, RDX_FMT_TLV);
    } else if ($eq(verb, VERB_MERJ)) {
        call(CmdY, din, RDX_FMT_JDR);
    } else if ($eq(verb, VERB_NOW)) {
        call(CmdNow, 0);
    } else if ($eq(verb, VERB_NORM)) {
        call(CmdNorm, din);
    } else if ($eq(verb, VERB_STRIP)) {
        call(CmdStrip, din);
    } else if ($eq(verb, VERB_CAT)) {
        call(CmdCat, din);
    } else if ($eq(verb, VERB_INIT)) {
        call(CmdInit, din);
    } else if ($eq(verb, VERB_ADD)) {
        call(CmdAdd, din);
    } else if ($eq(verb, VERB_BRANCH)) {
        call(CmdBranch, din);
    } else if ($eq(verb, VERB_DROP)) {
        call(CmdDrop, inn);
    } else if ($eq(verb, VERB_FORK)) {
        call(CmdFork, inn);
    } else if ($eq(verb, VERB_USE)) {
        call(CmdUse, inn);
    } else {
        fprintf(stderr, "Unknown command %s.\n%s", *verb, *USAGE);
    }

    done;
}

MAIN(rdxcli);
