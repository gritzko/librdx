#   Dog API

 1. Each dog provides a static library and an executable.
 2. The executables have common options:
      - `--index -i` rebuild the index
      - `--update -u` _update_ the index
      - `--status -s` report status (short)
      - `--tlv -t` provide output in hunk TLV mode (see HUNK)
      - `--install -i` install hook(s) in a git repo
 3. Each dog keeps its state in `$REPO_ROOT/.dogs/name`
 4. Dogs must understand the URI syntax of GURI. 
    Dog's CLI is callable as `name URI`.
 5. Dogs find their home and each other using dog/HOME
 6. If `.dogs/keeper` is present, keeper has the data; if
    not, `git` has the data
 7. Last-seen-commit tracking is in `.dog/name/COMMIT`.
 8. The static lib must have `name` control struct and
      - `ok64 DOGOpen(name* state, path8s home, b8 rw)`
      - `ok64 DOGClose(name* state)`
 9. `.dogs/DOGS` lists the dogs `beagle` invokes by default
10. `be` dispatches HTTP-like command vocabulary to 
    appropriate dogs.
