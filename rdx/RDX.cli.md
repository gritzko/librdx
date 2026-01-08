# RDX Command Line Interface

The `rdx` CLI provides tools for working with RDX (Replicated Data eXchange) data and managing LSM-style repositories.

## Repository Structure

```
.rdx/
├── key              # 96-byte keypair (64 secret + 32 public)
├── current          # symlink to current branch (e.g., branches/main)
└── branches/
    ├── main/
    │   └── {timestamp}_{replica_id}.skil
    └── feature/
        └── {timestamp}_{replica_id}.skil
```

- **Replica ID**: 60-bit prefix of public key, encoded in RON Base64 (10 chars)
- **Timestamps**: RON Base64 encoded, providing lexicographic ordering
- **SKIL files**: Immutable; edits create new files, merges resolve on read

## Repository Commands

### `rdx init`

Initialize a new repository in the current directory.

```bash
$ rdx init
Initialized repository with replica ID: 9RUTXNI7Fb
```

Creates `.rdx/` directory with keypair and `branches/main/`.

### `rdx add <inputs...>`

Add data to the current branch, creating a new `.skil` file.

```bash
$ rdx add '{name:"Alice", score:100}'
Created .rdx/branches/main/26088A~XA6_9RUTXNI7Fb.skil (48 bytes)

$ rdx add file.jdr @other_branch
Created .rdx/branches/main/26088B~123_9RUTXNI7Fb.skil (96 bytes)
```

Inputs can be:
- Inline RDX/JDR: `'{key:"value"}'`
- Files: `data.jdr`, `data.tlv`, `data.skil`
- Directories: `./data/` (scans for supported files)
- Branch references: `@branch` (expands to `.rdx/branches/branch/`)

## Branch Commands

### `rdx branch`

List all branches.

```bash
$ rdx branch
Branches:
  feature
  main
  release
```

### `rdx use`

Show or set the current branch.

```bash
$ rdx use
Current branch: main (default)

$ rdx use @feature
Switched to branch: feature

$ rdx use
Current branch: feature
```

The current branch is stored as a symlink at `.rdx/current`.

### `rdx fork <name> [@sources...]`

Create a new branch with optional sources.

**Empty branch:**
```bash
$ rdx fork experiment
Created branch: experiment
```

**Fork from existing branch (hard-links files):**
```bash
$ rdx fork feature @main
Forked branch: feature from @main
```

Hard-linking is efficient: files share storage until modified.

**Merge multiple sources:**
```bash
$ rdx fork release @feature @hotfix '{version:"1.0"}'
Created branch: release (merged, 128 bytes)
```

### `rdx drop @branch`

Delete a branch. Cannot drop `main`.

```bash
$ rdx drop @feature
Dropped branch: feature
```

## Data Commands

### `rdx jdr <inputs...>`

Output merged data as JDR (human-readable format).

```bash
$ rdx jdr @main
{(name,"Alice"),(score,100)}

$ rdx jdr file1.skil file2.skil
{(merged,"data")}
```

### `rdx tlv <inputs...>`

Output merged data as TLV (binary format).

### `rdx merge <inputs...>`

Merge inputs, output in default format.

### `rdx cat <inputs...>`

Concatenate inputs without merging.

### `rdx norm <inputs...>`

Normalize data (sort keys, deduplicate).

### `rdx strip <inputs...>`

Strip metadata from data.

### `rdx hash <inputs...>`

Compute BLAKE256 hash of merged data.

```bash
$ rdx hash @main
Simple BLAKE256: 7a3f...
```

### `rdx now`

Print current RON timestamp.

```bash
$ rdx now
26088ABC12
```

## Input Syntax

All data commands accept flexible inputs:

| Syntax | Description |
|--------|-------------|
| `'{...}'` | Inline JDR data |
| `file.jdr` | JDR file |
| `file.tlv` | TLV file |
| `file.skil` | SKIL file |
| `dir/` | Directory (scans for files) |
| `@branch` | Branch reference (→ `.rdx/branches/branch/`) |

## File Formats

| Extension | Format | Description |
|-----------|--------|-------------|
| `.jdr` | JDR | Human-readable JSON-like format |
| `.tlv` | TLV | Binary Type-Length-Value encoding |
| `.skil` | SKIL | TLV with skip list index for fast seeks |

## Examples

**Basic workflow:**
```bash
# Initialize
rdx init

# Add data
rdx add '{user:"alice", email:"alice@example.com"}'
rdx add '{user:"bob", email:"bob@example.com"}'

# View
rdx jdr @main
```

**Branching workflow:**
```bash
# Create feature branch
rdx fork feature @main
rdx use @feature

# Work on feature
rdx add '{feature_flag:true}'

# View difference
echo "main:" && rdx jdr @main
echo "feature:" && rdx jdr @feature

# Merge back (create release from both)
rdx fork release @main @feature '{version:"1.0"}'
```

**Import existing data:**
```bash
rdx add ./exported_data/
rdx add legacy.jdr config.tlv
```
