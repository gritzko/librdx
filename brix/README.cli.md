#   BRIX command line utility

The `brix` command performs all the operations on BRIX repos.
A general command syntax involves a subject, a verb and an object.

  - A subject can be a repo, a branch or a version.
  - A verb is just that, a verb: `show`, `see`, `list` and so on.
  - An object can be anything, depending on the verb: 
    a branch, a version, an object or a file, or any piece of data.

Fundamentally, the subject is a consistent set of RDX BRIX data the
command operates on. That may be a repo, a specific branch or a 
version.

##  The Verbs

 1. `init [path]` creates a new empty repo.
 2. `see (file.rdx|branch|@version|#hash)*`
     1. `jee b0b-1 bob.jdr`
     2. `tee b0b-1 README`
 4. `get id+`
 4. `show (id|name|key|index)*`
     1. `jdr b0b-1 bob.jdr`
     2. `text b0b-1 README`
 5. `diff`
 6. `delta`
 7. `sync`
 8. `fork`

##  CLI/URI correspondence

A user may interface with the system through local API, HTTP API or CLI.
For the practical reasons, all the access methods should be congruent.
The standard URI form for RDX store access is:

`http://branch.server.dom/verb/object/path.form`


