// RB.c - Red-black tree indexed RDX format (self-contained records)
#include "RB.h"
#include "RDX.h"

#include "abc/PRO.h"
#include "abc/ZINT.h"

// Layout: next=current record, bulk=buffer [past,data,idle,end]
// Buffer starts with 4-byte header storing tree root offset
#define RB_HEADER_SIZE 4
#define RBbase(x)     ((x)->bulk[1])
#define RBwritePos(x) (((u8**)(x)->bulk)[2])
#define RBbufEnd(x)   ((x)->bulk[3])

// Children root stored in loc (valid after positioning on PLEX)
#define RBchildRoot(x)       ((x)->loc)
#define RBchildRootSet(x, v) ((x)->loc = (v))

// Root tree stored in buffer header (first 4 bytes of data area)
#define RBrootTree(x)        (*(u32*)RBbase(x))
#define RBrootTreePtr(x)     ((u32*)RBbase(x))

// Get record body start (after lit + len header)
fun u8cp RBbodyStart(u8cp rec) {
    u8cp p = rec + 1;  // skip lit
    if ((*p & 0xE0) == 0x20) {
        return p + 1;  // short TLV
    }
    while (*p & 0x80) p++;  // long TLV varint
    return p + 1;
}

// Get RBlink from current record
fun RBlinkp RBcurrentLink(rdxp x) {
    if (!x->next) return NULL;
    return (RBlinkp)RBbodyStart(x->next);
}

fun RBlinkconstant RBcurrentLinkConst(rdxcp x) {
    if (!x->next) return NULL;
    return (RBlinkconstant)RBbodyStart(x->next);
}

// Get RBlink at offset (with bounds check)
fun RBlinkp RBat(rdxp x, u32 off) {
    if (off == RB_NIL) return NULL;
    u8p base = RBbase(x);
    u8p end = RBwritePos(x);
    if (off + 18 > (u64)(end - base)) return NULL;
    return (RBlinkp)RBbodyStart(base + off);
}

fun RBlinkconstant RBatConst(rdxcp x, u32 off) {
    if (off == RB_NIL) return NULL;
    u8cp base = RBbase(x);
    u8cp end = RBwritePos(x);
    if (off + 18 > (u64)(end - base)) return NULL;
    return (RBlinkconstant)RBbodyStart(base + off);
}

// Current record offset
fun u32 RBoff(rdxcp x) {
    if (!x->next) return RB_NIL;
    return (u32)(x->next - RBbase(x));
}

// Position iterator on record at offset
fun ok64 RBposition(rdxp x, u32 off) {
    sane(x);
    if (off == RB_NIL) {
        x->next = NULL;
        x->type = 0;
        x->loc = UINT32_MAX;
        return END;
    }
    u8p base = RBbase(x);
    u8p data_end = RBwritePos(x);
    if (off + 18 > (u64)(data_end - base)) {
        x->next = NULL;
        x->type = 0;
        return ok64sub(RDXBAD, RON_a);
    }

    u8p rec = base + off;
    x->next = rec;  // current record
    u8 lit = rec[0];
    if (lit >= 128) return ok64sub(RDXBAD, RON_b);  // Invalid literal
    x->type = RDX_TYPE_LIT_REV[lit];
    if (x->type == 0) return ok64sub(RDXBAD, RON_c);

    // Parse record length
    u8p p = rec + 1;
    u32 body_len = 0;
    if ((*p & 0xE0) == 0x20) {
        body_len = *p & 0x1F;
        p++;
    } else {
        u32 shift = 0;
        while (*p & 0x80) {
            body_len |= (*p++ & 0x7F) << shift;
            shift += 7;
        }
        body_len |= (*p++ & 0x7F) << shift;
    }

    u8p rec_end = p + body_len;

    // Parse id and value
    p += sizeof(RBlink);  // skip RBlink

    // Parse idlen + compressed id
    u32 idlen = 0;
    u32 shift = 0;
    while (*p & 0x80) {
        idlen |= (*p++ & 0x7F) << shift;
        shift += 7;
    }
    idlen |= (*p++ & 0x7F) << shift;

    if (idlen > 0) {
        u8cs id_slice = {p, p + idlen};
        ZINTu8sDrain128(id_slice, &x->id.seq, &x->id.src);
        p += idlen;
    } else {
        zero(x->id);
    }

    // Parse value (from p to record end)
    u8cs val = {p, rec_end};

    switch (x->type) {
        case RDX_TYPE_INT:
            ZINTu8sDrainInt(&x->i, val);
            break;
        case RDX_TYPE_FLOAT:
            ZINTu8sDrainFloat(&x->f, val);
            break;
        case RDX_TYPE_STRING:
            u8csMv(x->s, val);
            x->flags = RDX_UTF_ENC_UTF8;
            break;
        case RDX_TYPE_TERM:
            u8csMv(x->t, val);
            break;
        case RDX_TYPE_REF:
            ZINTu8sDrain128(val, &x->r.seq, &x->r.src);
            break;
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX:
            // PLEX: value is children_root (u32)
            if ($len(val) >= 4) {
                memcpy(&x->loc, *val, 4);
            } else {
                RBchildRootSet(x, RB_NIL);
            }
            break;
    }

    done;
}

// Tree operations using iterator
fun void RBsetNodeRed(rdxp x, u32 off, b8 red) {
    RBlinkp n = RBat(x, off);
    if (n) n->rb_parent = (red ? (RBparentOff(n->rb_parent) | RB_RED_BIT)
                               : RBparentOff(n->rb_parent));
}

fun void RBsetParent(rdxp x, u32 off, u32 parent_off) {
    RBlinkp n = RBat(x, off);
    if (n) {
        b8 red = RBisRed(n->rb_parent);
        n->rb_parent = red ? (parent_off | RB_RED_BIT) : parent_off;
    }
}

fun void RBrotateLeft(rdxp x, u32p root, u32 x_off) {
    RBlinkp xn = RBat(x, x_off);
    if (!xn) return;
    u32 y_off = xn->rb_right;
    RBlinkp yn = RBat(x, y_off);
    if (!yn) return;

    xn->rb_right = yn->rb_left;
    if (yn->rb_left != RB_NIL) {
        RBsetParent(x, yn->rb_left, x_off);
    }

    u32 xp = RBparentOff(xn->rb_parent);
    RBsetParent(x, y_off, xp);
    if (xp == RB_NIL) {
        *root = y_off;
    } else {
        RBlinkp xparent = RBat(x, xp);
        if (!xparent) return;
        if (x_off == xparent->rb_left) {
            xparent->rb_left = y_off;
        } else {
            xparent->rb_right = y_off;
        }
    }

    yn->rb_left = x_off;
    RBsetParent(x, x_off, y_off);
}

fun void RBrotateRight(rdxp x, u32p root, u32 y_off) {
    RBlinkp yn = RBat(x, y_off);
    if (!yn) return;
    u32 x_off = yn->rb_left;
    RBlinkp xn = RBat(x, x_off);
    if (!xn) return;

    yn->rb_left = xn->rb_right;
    if (xn->rb_right != RB_NIL) {
        RBsetParent(x, xn->rb_right, y_off);
    }

    u32 yp = RBparentOff(yn->rb_parent);
    RBsetParent(x, x_off, yp);
    if (yp == RB_NIL) {
        *root = x_off;
    } else {
        RBlinkp yparent = RBat(x, yp);
        if (!yparent) return;
        if (y_off == yparent->rb_left) {
            yparent->rb_left = x_off;
        } else {
            yparent->rb_right = x_off;
        }
    }

    xn->rb_right = y_off;
    RBsetParent(x, y_off, x_off);
}

fun void RBinsertFixup(rdxp x, u32p root, u32 z_off) {
    while (z_off != *root) {
        RBlinkp z = RBat(x, z_off);
        if (!z) break;
        u32 p_off = RBparentOff(z->rb_parent);
        if (p_off == RB_NIL) break;
        RBlinkp p = RBat(x, p_off);
        if (!p || !RBisRed(p->rb_parent)) break;

        u32 gp_off = RBparentOff(p->rb_parent);
        if (gp_off == RB_NIL) break;
        RBlinkp gp = RBat(x, gp_off);
        if (!gp) break;

        if (p_off == gp->rb_left) {
            u32 u_off = gp->rb_right;
            RBlinkp u = RBat(x, u_off);
            if (u && RBisRed(u->rb_parent)) {
                RBsetNodeRed(x, p_off, NO);
                RBsetNodeRed(x, u_off, NO);
                RBsetNodeRed(x, gp_off, YES);
                z_off = gp_off;
            } else {
                if (z_off == p->rb_right) {
                    z_off = p_off;
                    RBrotateLeft(x, root, z_off);
                    z = RBat(x, z_off);
                    p_off = RBparentOff(z->rb_parent);
                    p = RBat(x, p_off);
                    gp_off = RBparentOff(p->rb_parent);
                }
                RBsetNodeRed(x, p_off, NO);
                RBsetNodeRed(x, gp_off, YES);
                RBrotateRight(x, root, gp_off);
            }
        } else {
            u32 u_off = gp->rb_left;
            RBlinkp u = RBat(x, u_off);
            if (u && RBisRed(u->rb_parent)) {
                RBsetNodeRed(x, p_off, NO);
                RBsetNodeRed(x, u_off, NO);
                RBsetNodeRed(x, gp_off, YES);
                z_off = gp_off;
            } else {
                if (z_off == p->rb_left) {
                    z_off = p_off;
                    RBrotateRight(x, root, z_off);
                    z = RBat(x, z_off);
                    p_off = RBparentOff(z->rb_parent);
                    p = RBat(x, p_off);
                    gp_off = RBparentOff(p->rb_parent);
                }
                RBsetNodeRed(x, p_off, NO);
                RBsetNodeRed(x, gp_off, YES);
                RBrotateLeft(x, root, gp_off);
            }
        }
    }
    RBsetNodeRed(x, *root, NO);
}

// Forward declaration
fun u32 RBmin(rdxcp x, u32 off);

// Transplant: replace subtree rooted at u with subtree rooted at v
fun void RBtransplant(rdxp x, u32p root, u32 u_off, u32 v_off) {
    RBlinkp u = RBat(x, u_off);
    if (!u) return;

    u32 up = RBparentOff(u->rb_parent);
    if (up == RB_NIL) {
        *root = v_off;
    } else {
        RBlinkp parent = RBat(x, up);
        if (!parent) return;
        if (u_off == parent->rb_left) {
            parent->rb_left = v_off;
        } else {
            parent->rb_right = v_off;
        }
    }
    if (v_off != RB_NIL) {
        RBsetParent(x, v_off, up);
    }
}

// Delete fixup after node removal
fun void RBdeleteFixup(rdxp x, u32p root, u32 x_off, u32 x_parent) {
    while (x_off != *root) {
        RBlinkp xn = RBat(x, x_off);
        if (xn && RBisRed(xn->rb_parent)) break;

        RBlinkp p = RBat(x, x_parent);
        if (!p) break;

        if (x_off == p->rb_left) {
            u32 w_off = p->rb_right;
            RBlinkp w = RBat(x, w_off);
            if (!w) break;

            if (RBisRed(w->rb_parent)) {
                RBsetNodeRed(x, w_off, NO);
                RBsetNodeRed(x, x_parent, YES);
                RBrotateLeft(x, root, x_parent);
                p = RBat(x, x_parent);
                w_off = p->rb_right;
                w = RBat(x, w_off);
                if (!w) break;
            }

            RBlinkp wl = RBat(x, w->rb_left);
            RBlinkp wr = RBat(x, w->rb_right);
            b8 wl_black = (w->rb_left == RB_NIL || !wl || !RBisRed(wl->rb_parent));
            b8 wr_black = (w->rb_right == RB_NIL || !wr || !RBisRed(wr->rb_parent));

            if (wl_black && wr_black) {
                RBsetNodeRed(x, w_off, YES);
                x_off = x_parent;
                p = RBat(x, x_parent);
                x_parent = p ? RBparentOff(p->rb_parent) : RB_NIL;
            } else {
                if (wr_black) {
                    RBsetNodeRed(x, w->rb_left, NO);
                    RBsetNodeRed(x, w_off, YES);
                    RBrotateRight(x, root, w_off);
                    p = RBat(x, x_parent);
                    w_off = p->rb_right;
                    w = RBat(x, w_off);
                }
                RBsetNodeRed(x, w_off, RBisRed(p->rb_parent));
                RBsetNodeRed(x, x_parent, NO);
                if (w->rb_right != RB_NIL) {
                    RBsetNodeRed(x, w->rb_right, NO);
                }
                RBrotateLeft(x, root, x_parent);
                x_off = *root;
            }
        } else {
            // Mirror case
            u32 w_off = p->rb_left;
            RBlinkp w = RBat(x, w_off);
            if (!w) break;

            if (RBisRed(w->rb_parent)) {
                RBsetNodeRed(x, w_off, NO);
                RBsetNodeRed(x, x_parent, YES);
                RBrotateRight(x, root, x_parent);
                p = RBat(x, x_parent);
                w_off = p->rb_left;
                w = RBat(x, w_off);
                if (!w) break;
            }

            RBlinkp wl = RBat(x, w->rb_left);
            RBlinkp wr = RBat(x, w->rb_right);
            b8 wl_black = (w->rb_left == RB_NIL || !wl || !RBisRed(wl->rb_parent));
            b8 wr_black = (w->rb_right == RB_NIL || !wr || !RBisRed(wr->rb_parent));

            if (wl_black && wr_black) {
                RBsetNodeRed(x, w_off, YES);
                x_off = x_parent;
                p = RBat(x, x_parent);
                x_parent = p ? RBparentOff(p->rb_parent) : RB_NIL;
            } else {
                if (wl_black) {
                    RBsetNodeRed(x, w->rb_right, NO);
                    RBsetNodeRed(x, w_off, YES);
                    RBrotateLeft(x, root, w_off);
                    p = RBat(x, x_parent);
                    w_off = p->rb_left;
                    w = RBat(x, w_off);
                }
                RBsetNodeRed(x, w_off, RBisRed(p->rb_parent));
                RBsetNodeRed(x, x_parent, NO);
                if (w->rb_left != RB_NIL) {
                    RBsetNodeRed(x, w->rb_left, NO);
                }
                RBrotateRight(x, root, x_parent);
                x_off = *root;
            }
        }
    }
    if (x_off != RB_NIL) {
        RBsetNodeRed(x, x_off, NO);
    }
}

// Unlink node from tree (for reinsertion)
fun ok64 RBunlink(rdxp x, u32p root, u32 z_off) {
    sane(x && root);

    RBlinkp z = RBat(x, z_off);
    if (!z) return ok64sub(RDXBAD, RON_d);

    u32 y_off = z_off;
    b8 y_orig_red = RBisRed(z->rb_parent);
    u32 x_off = RB_NIL;
    u32 x_parent = RB_NIL;

    if (z->rb_left == RB_NIL) {
        x_off = z->rb_right;
        x_parent = RBparentOff(z->rb_parent);
        RBtransplant(x, root, z_off, z->rb_right);
    } else if (z->rb_right == RB_NIL) {
        x_off = z->rb_left;
        x_parent = RBparentOff(z->rb_parent);
        RBtransplant(x, root, z_off, z->rb_left);
    } else {
        y_off = RBmin(x, z->rb_right);
        RBlinkp y = RBat(x, y_off);
        if (!y) return ok64sub(RDXBAD, RON_e);
        y_orig_red = RBisRed(y->rb_parent);
        x_off = y->rb_right;

        if (RBparentOff(y->rb_parent) == z_off) {
            x_parent = y_off;
        } else {
            x_parent = RBparentOff(y->rb_parent);
            RBtransplant(x, root, y_off, y->rb_right);
            y = RBat(x, y_off);  // refresh after transplant
            y->rb_right = z->rb_right;
            RBsetParent(x, y->rb_right, y_off);
        }
        RBtransplant(x, root, z_off, y_off);
        y = RBat(x, y_off);  // refresh after transplant
        y->rb_left = z->rb_left;
        RBsetParent(x, y->rb_left, y_off);
        RBsetNodeRed(x, y_off, RBisRed(z->rb_parent));
    }

    // Clear z's tree links (keep rdx_parent for hierarchy)
    z->rb_left = RB_NIL;
    z->rb_right = RB_NIL;
    z->rb_parent = RB_NIL;

    if (!y_orig_red) {
        RBdeleteFixup(x, root, x_off, x_parent);
    }

    done;
}

// Find minimum in subtree
fun u32 RBmin(rdxcp x, u32 off) {
    if (off == RB_NIL) return RB_NIL;
    RBlinkconstant n = RBatConst(x, off);
    if (!n) return RB_NIL;
    while (n->rb_left != RB_NIL) {
        off = n->rb_left;
        n = RBatConst(x, off);
        if (!n) return RB_NIL;
    }
    return off;
}

// Find successor
fun u32 RBsuccessor(rdxcp x, u32 off) {
    if (off == RB_NIL) return RB_NIL;
    RBlinkconstant n = RBatConst(x, off);
    if (!n) return RB_NIL;
    if (n->rb_right != RB_NIL) {
        return RBmin(x, n->rb_right);
    }
    u32 p_off = RBparentOff(n->rb_parent);
    while (p_off != RB_NIL) {
        RBlinkconstant p = RBatConst(x, p_off);
        if (!p) return RB_NIL;
        if (off != p->rb_right) break;
        off = p_off;
        p_off = RBparentOff(p->rb_parent);
    }
    return p_off;
}

// Write a new record, returns offset
fun ok64 RBwrite(rdxp x, u32p out_off, u32 rdx_parent) {
    sane(x && out_off);

    u8p base = RBbase(x);
    u8p pos = RBwritePos(x);
    u8p end = RBbufEnd(x);

    // Bounds check: 64 bytes for header + fixed values
    test(pos + 64 <= end, RDXNOROOM);
    // For STRING/TERM, also check value length
    if (x->type == RDX_TYPE_STRING) test(pos + 64 + u8csLen(x->s) <= end, RDXNOROOM);
    if (x->type == RDX_TYPE_TERM) test(pos + 64 + u8csLen(x->t) <= end, RDXNOROOM);

    u32 rec_off = (u32)(pos - base);
    u8p p = pos;

    // Write lit
    *p++ = RDX_TYPE_LIT[x->type];

    // Reserve space for len (up to 5 bytes for varint32)
    u8p len_pos = p;
    p += 5;

    // Write RBlink
    RBlinkp link = (RBlinkp)p;
    link->rb_left = RB_NIL;
    link->rb_right = RB_NIL;
    link->rb_parent = RB_NIL;
    link->rdx_parent = rdx_parent;
    p += sizeof(RBlink);

    // Write id
    a_pad(u8, id_buf, 20);
    if (x->id.src != 0 || x->id.seq != 0) {
        ZINTu8sFeed128(id_buf_idle, x->id.seq, x->id.src);
    }
    u8sJoin(id_buf_idle, id_buf);
    u64 id_len = $len(id_buf_datac);

    u64 tmp = id_len;
    while (tmp >= 0x80) {
        *p++ = (u8)(tmp | 0x80);
        tmp >>= 7;
    }
    *p++ = (u8)tmp;

    if (id_len > 0) {
        memcpy(p, id_buf[0], id_len);
        p += id_len;
    }

    // Write value
    u8s val = {p, end};

    switch (x->type) {
        case RDX_TYPE_INT:
            ZINTu8sFeedInt(val, &x->i);
            p = val[0];
            break;
        case RDX_TYPE_FLOAT:
            ZINTu8sFeedFloat(val, &x->f);
            p = val[0];
            break;
        case RDX_TYPE_STRING: {
            u32 slen = (u32)u8csLen(x->s);
            memcpy(p, *x->s, slen);
            p += slen;
            break;
        }
        case RDX_TYPE_TERM: {
            u32 tlen = (u32)u8csLen(x->t);
            memcpy(p, *x->t, tlen);
            p += tlen;
            break;
        }
        case RDX_TYPE_REF:
            ZINTu8sFeed128(val, x->r.seq, x->r.src);
            p = val[0];
            break;
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX: {
            u32 nil = RB_NIL;
            memcpy(p, &nil, 4);
            p += 4;
            break;
        }
    }

    // Calculate body length and encode
    u32 body_len = (u32)(p - len_pos - 5);
    u8 len_buf[5];
    u32 len_bytes;

    if (body_len <= 31) {
        // Use short TLV format: 0x20 | len
        len_buf[0] = (u8)(body_len | 0x20);
        len_bytes = 1;
    } else if (body_len < 64) {
        // Values 32-63 would conflict with short TLV (0x20-0x3F)
        // Force 2-byte varint encoding
        len_buf[0] = (u8)(body_len | 0x80);
        len_buf[1] = 0;
        len_bytes = 2;
    } else {
        // Use standard varint format (no conflict with short TLV)
        u8p lp = len_buf;
        u32 blen = body_len;
        while (blen >= 0x80) {
            *lp++ = (u8)(blen | 0x80);
            blen >>= 7;
        }
        *lp++ = (u8)blen;
        len_bytes = (u32)(lp - len_buf);
    }

    // Move body to close the gap (5 - len_bytes gap)
    u32 gap = 5 - len_bytes;
    if (gap > 0) {
        memmove(len_pos + len_bytes, len_pos + 5, body_len);
        p -= gap;
    }

    // Write length
    memcpy(len_pos, len_buf, len_bytes);

    // Update write position
    RBwritePos(x) = p;

    *out_off = rec_off;
    done;
}

// Insert record into tree
fun ok64 RBinsert(rdxp x, u32p root, u32 n_off) {
    sane(x && root);

    RBlinkp n = RBat(x, n_off);
    if (!n) return ok64sub(RDXBAD, RON_f);

    if (*root == RB_NIL) {
        *root = n_off;
        n->rb_parent = RB_NIL;
        return OK;
    }

    // BST insert
    u32 y_off = RB_NIL;
    u32 cur = *root;

    // Save current position
    u8cp save_rec = x->next;
    u8 save_type = x->type;

    while (cur != RB_NIL) {
        y_off = cur;

        RBlinkp cur_link = RBat(x, cur);
        if (!cur_link) break;  // Invalid offset, stop

        // Compare with node at cur
        rdx existing = {.format = RDX_FMT_RB, .bulk = x->bulk, .ptype = x->ptype};
        if (RBposition(&existing, cur) != OK) break;

        rdx newrec = {.format = RDX_FMT_RB, .bulk = x->bulk, .ptype = x->ptype};
        if (RBposition(&newrec, n_off) != OK) break;

        if (rdxZ(&newrec, &existing)) {
            cur = cur_link->rb_left;
        } else {
            cur = cur_link->rb_right;
        }
    }

    // Restore position
    x->next = save_rec;
    x->type = save_type;

    RBsetParent(x, n_off, y_off);
    RBsetNodeRed(x, n_off, YES);

    rdx y = {.format = RDX_FMT_RB, .bulk = x->bulk, .ptype = x->ptype};
    RBposition(&y, y_off);

    rdx newrec = {.format = RDX_FMT_RB, .bulk = x->bulk, .ptype = x->ptype};
    RBposition(&newrec, n_off);

    RBlinkp y_link = RBat(x, y_off);
    if (!y_link) return ok64sub(RDXBAD, RON_g);

    if (rdxZ(&newrec, &y)) {
        y_link->rb_left = n_off;
    } else {
        y_link->rb_right = n_off;
    }

    RBinsertFixup(x, root, n_off);
    done;
}

// Get children_root pointer from PLEX record
fun u32p RBchildrenRoot(rdxp x) {
    if (!x->next) return NULL;
    if (!rdxTypePlex(x)) return NULL;

    u8p body = (u8p)RBbodyStart(x->next);
    u8p p = body + sizeof(RBlink);

    // Skip idlen + id
    u32 idlen = 0;
    while (*p & 0x80) idlen |= (*p++ & 0x7F);
    idlen |= *p++;
    p += idlen;

    return (u32p)p;
}

//
// Public API
//

// Move to next sibling in sorted order (or sequential for TUPLE)
ok64 rdxNextRB(rdxp x) {
    sane(x);

    if (!x->next) {
        // First call or re-entering container
        // Root-level: tree root in buffer header; nested: children root in loc
        u32 root = (x->ptype == 0) ? RBrootTree(x) : x->loc;
        if (root == RB_NIL) {
            x->type = 0;
            return END;
        }

        if (x->ptype == RDX_TYPE_TUPLE) {
            // TUPLE children: first is root itself
            return RBposition(x, root);
        }

        // Other containers: find minimum in RB tree
        u32 first = RBmin(x, root);
        return RBposition(x, first);
    }

    // Get next element
    u32 cur = RBoff(x);

    if (x->ptype == RDX_TYPE_TUPLE) {
        // TUPLE children: follow right links sequentially
        RBlinkconstant link = RBcurrentLinkConst(x);
        u32 nxt = link ? link->rb_right : RB_NIL;
        if (nxt == RB_NIL) {
            x->next = NULL;
            x->type = 0;
            x->loc = UINT32_MAX;
            return END;
        }
        return RBposition(x, nxt);
    }

    // Other containers: get RB successor
    u32 nxt = RBsuccessor(x, cur);
    if (nxt == RB_NIL) {
        x->next = NULL;
        x->type = 0;
        x->loc = UINT32_MAX;
        return END;
    }

    return RBposition(x, nxt);
}

// Navigate into PLEX container
ok64 rdxIntoRB(rdxp c, rdxp p) {
    sane(c && p && rdxTypePlex(p));

    c->format = RDX_FMT_RB;
    c->bulk = p->bulk;
    c->ptype = p->type;
    c->next = NULL;

    // Get children_root from parent's plex
    RBchildRootSet(c, RBchildRoot(p));

    done;
}

// Navigate out of container
ok64 rdxOutoRB(rdxp c, rdxp p) {
    sane(c && p);
    // Parent state unchanged - just return
    done;
}

// Append child to TUPLE (sequential linking, no RB balancing)
fun ok64 RBappendToTuple(rdxp x, u32p root_ptr, u32 rec_off) {
    sane(x && root_ptr);

    RBlinkp n = RBat(x, rec_off);
    if (!n) return ok64sub(RDXBAD, RON_h);

    if (*root_ptr == RB_NIL) {
        // First child
        *root_ptr = rec_off;
        n->rb_parent = RB_NIL;
    } else {
        // Find last child (rightmost)
        u32 cur = *root_ptr;
        RBlinkp cur_link = RBat(x, cur);
        while (cur_link && cur_link->rb_right != RB_NIL) {
            cur = cur_link->rb_right;
            cur_link = RBat(x, cur);
        }
        if (!cur_link) return ok64sub(RDXBAD, RON_i);

        // Append as right child
        cur_link->rb_right = rec_off;
        n->rb_parent = cur;  // No red bit - TUPLE children don't need color
    }

    n->rb_left = RB_NIL;
    n->rb_right = RB_NIL;
    done;
}

// Write next element
ok64 rdxWriteNextRB(rdxp x) {
    sane(x);

    // Get parent offset:
    // - If positioned on a record, read from rdx_parent field
    // - Otherwise, use loc (set by rdxWriteIntoRB)
    u32 rdx_parent = RB_NIL;
    if (x->next) {
        RBlinkconstant link = RBcurrentLinkConst(x);
        if (link) rdx_parent = link->rdx_parent;
    } else if (x->ptype != 0) {
        // First child write - plex holds parent_off from rdxWriteIntoRB
        rdx_parent = RBchildRoot(x);
    }

    // Check if this is first child of TUPLE in EULER (deferred insert)
    b8 insert_tuple = NO;
    u32 tuple_off = RB_NIL;
    u32p grandparent_root = NULL;
    u32 grandparent_off = RB_NIL;

    if (x->ptype == RDX_TYPE_TUPLE && rdx_parent != RB_NIL) {
        rdx tuple = {.format = RDX_FMT_RB, .bulk = x->bulk};
        if (RBposition(&tuple, rdx_parent) == OK) {
            u32p tuple_children = RBchildrenRoot(&tuple);
            // First child if children_root is NIL
            if (tuple_children && *tuple_children == RB_NIL) {
                // Check grandparent type
                RBlinkconstant tlink = RBcurrentLinkConst(&tuple);
                if (tlink && tlink->rdx_parent != RB_NIL) {
                    rdx grandparent = {.format = RDX_FMT_RB, .bulk = x->bulk};
                    if (RBposition(&grandparent, tlink->rdx_parent) == OK) {
                        if (grandparent.type == RDX_TYPE_EULER) {
                            insert_tuple = YES;
                            tuple_off = rdx_parent;
                            grandparent_root = RBchildrenRoot(&grandparent);
                            grandparent_off = tlink->rdx_parent;
                        }
                    }
                }
            }
        }
    }

    // Write new record
    u32 rec_off = 0;
    call(RBwrite, x, &rec_off, rdx_parent);

    // Insert into appropriate tree
    if (rdx_parent != RB_NIL) {
        rdx parent = {.format = RDX_FMT_RB, .bulk = x->bulk};
        call(RBposition, &parent, rdx_parent);
        u32p root_ptr = RBchildrenRoot(&parent);

        if (root_ptr) {
            if (x->ptype == RDX_TYPE_TUPLE) {
                // TUPLE children: sequential append
                call(RBappendToTuple, x, root_ptr, rec_off);
            } else if (x->type == RDX_TYPE_TUPLE && x->ptype == RDX_TYPE_EULER) {
                // TUPLE in EULER: defer insert until key arrives
                // Record exists with rdx_parent, just not linked to siblings
            } else {
                // Other containers: RB insert
                call(RBinsert, x, root_ptr, rec_off);
            }
        }
    } else {
        // Root-level: insert into tree (tracked in opt)
        u32p root_ptr = RBrootTreePtr(x);
        call(RBinsert, x, root_ptr, rec_off);
    }

    // Insert TUPLE into grandparent EULER now that key exists
    if (insert_tuple && grandparent_root) {
        // Use grandparent context for correct comparator (ptype = EULER)
        rdx gctx = {.format = RDX_FMT_RB, .bulk = x->bulk, .ptype = RDX_TYPE_EULER};
        RBposition(&gctx, grandparent_off);
        call(RBinsert, &gctx, grandparent_root, tuple_off);
    }

    // Position on new record
    return RBposition(x, rec_off);
}

// Enter container for writing (with optional find-or-create)
ok64 rdxWriteIntoRB(rdxp c, rdxp p) {
    sane(c && p && rdxTypePlex(p));

    c->format = RDX_FMT_RB;
    c->bulk = p->bulk;
    c->ptype = p->type;

    u32 parent_off = RBoff(p);
    u32p root_ptr = RBchildrenRoot(p);
    u32 children_root = root_ptr ? *root_ptr : RB_NIL;

    // Find-or-create: if c->type is set, search for it
    if (c->type != 0) {
        u32 cur = children_root;
        u32 found = RB_NIL;

        while (cur != RB_NIL) {
            rdx existing = {.format = RDX_FMT_RB, .bulk = c->bulk, .ptype = c->ptype};
            RBposition(&existing, cur);

            if (rdxZ(c, &existing)) {
                cur = RBatConst(c, cur)->rb_left;
            } else if (rdxZ(&existing, c)) {
                cur = RBatConst(c, cur)->rb_right;
            } else {
                found = cur;
                break;
            }
        }

        if (found != RB_NIL) {
            // Found - position on it
            c->type = 0;
            return RBposition(c, found);
        }

        // Not found - create new
        u32 rec_off = 0;
        call(RBwrite, c, &rec_off, parent_off);

        if (root_ptr) {
            call(RBinsert, c, root_ptr, rec_off);
        }

        c->type = 0;
        return RBposition(c, rec_off);
    }

    // No search - just set up for writing children
    c->next = NULL;
    RBchildRootSet(c, parent_off);  // Store parent offset for first child write

    done;
}

// Exit container after writing
ok64 rdxWriteOutoRB(rdxp c, rdxp p) {
    sane(c && p);
    // Write position already updated in buffer via extra
    // Parent can re-read if needed
    // Note: empty TUPLEs in EULER are invalid/non-normalized and won't appear
    done;
}
