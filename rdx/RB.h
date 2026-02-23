#ifndef RDX_RB_H
#define RDX_RB_H

//  RB: Red-Black tree indexed RDX format
//
//  A self-contained record format with embedded RB tree linkage for
//  efficient sorted access. Unlike the legacy format (separate TLV buffer
//  + index array), each record contains its own tree pointers.
//
//  ## Record Format
//
//      lit len [rb_left rb_right rb_parent rdx_parent] idlen id value
//              └──────────────── u32 x 4 (16 bytes) ───────────────┘
//
//      lit         - type literal (P/L/E/X/F/I/R/S/T)
//      len         - total record length (varint)
//      rb_left     - byte offset to left child in RB tree (sibling order)
//      rb_right    - byte offset to right child in RB tree
//      rb_parent   - byte offset to RB parent, high bit = red flag
//      rdx_parent  - byte offset to containing PLEX (RDX tree)
//      idlen id    - element ID (varint length + bytes)
//      value       - type-specific value bytes
//
//  ## Two Orthogonal Tree Structures
//
//  1. RDX tree (logical nesting):
//     - PLEX records contain children
//     - rdx_parent links child → container offset
//     - Defines the data model structure
//
//  2. RB tree (sorted sibling index):
//     - rb_left, rb_right, rb_parent link siblings within one container
//     - Each container's children form their own RB tree
//     - Provides O(log n) sorted access
//
//  ## PLEX vs FIRST Records
//
//  FIRST (primitive) records:
//      value = actual data (int, float, string bytes, etc.)
//
//  PLEX (container) records:
//      value = children_root (u32 byte offset)
//              Points to root of children's RB tree, or RB_NIL if empty
//
//  ## Navigation
//
//  Into (descend into PLEX):
//      1. Read children_root from PLEX's value area
//      2. That offset is the RB tree root for children
//      3. Walk tree for sorted iteration
//
//  Next (iterate siblings):
//      In-order RB tree traversal using rb_left/rb_right/rb_parent
//
//  Outo (ascend to parent):
//      Follow rdx_parent offset back to containing PLEX
//
//  Seek (find element):
//      Binary search in RB tree using rb_left/rb_right
//
//  ## Offsets
//
//  All offsets are byte offsets within the buffer.
//  RB_NIL (0x7FFFFFFF) indicates null/empty.
//
//  ## Example Structure
//
//      Buffer: [root EULER record] [child1 record] [child2 record] ...
//
//      Root EULER:
//          lit='E', len=..., rb_*=NIL (root has no siblings),
//          rdx_parent=NIL (top level), id=..., children_root=offset_of_child1
//
//      Child1 (STRING):
//          lit='S', len=..., rb_left=NIL, rb_right=offset_of_child2,
//          rb_parent=NIL (tree root), rdx_parent=offset_of_root, id=...,
//          "hello"
//
//      Child2 (STRING):
//          lit='S', len=..., rb_left=NIL, rb_right=NIL,
//          rb_parent=offset_of_child1, rdx_parent=offset_of_root, id=...,
//          "world"

#include "RDX.h"
#include "abc/01.h"

#define RB_RED_BIT (1U << 31)

// RB linkage header embedded in each record (16 bytes)
typedef struct {
    u32 rb_left;     // byte offset to left child (RB tree, sibling order)
    u32 rb_right;    // byte offset to right child (RB tree)
    u32 rb_parent;   // byte offset to RB parent, high bit = red flag
    u32 rdx_parent;  // byte offset to containing PLEX (RDX tree)
} RBlink;

typedef RBlink* RBlinkp;
typedef RBlink const* RBlinkconstant;

// Check if node is red (high bit of rb_parent)
fun b8 RBisRed(u32 rb_parent) { return (rb_parent & RB_RED_BIT) != 0; }

// Get parent offset (mask out red bit)
// Returns RB_NIL if parent is nil (handles both red and black nil)
fun u32 RBparentOff(u32 rb_parent) {
    u32 off = rb_parent & ~RB_RED_BIT;
    // 0x7FFFFFFF is "black NIL" (RB_NIL with red bit cleared)
    return (off == (RB_NIL & ~RB_RED_BIT)) ? RB_NIL : off;
}

// Combine offset with red flag (preserves NIL)
fun u32 RBsetRed(u32 off, b8 red) {
    if (off == RB_NIL) return RB_NIL;  // NIL is always "black"
    return red ? (off | RB_RED_BIT) : (off & ~RB_RED_BIT);
}

#endif
