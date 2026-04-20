# keeper refs format

`.dogs/keeper/refs` is the URI→URI append-only reflog. One mapping per
line:

    26416FJreE ?tags/v2.9.1 ?5c9159de87e41cf14ec5f2132afb5a06f35c26b3

i.e. 

    <ron60-time>\t<from-uri>\t<to-uri>\n

**Remote-attributed refs use the full origin URI as prefix.** No
`origin`-like shortcuts, so no `?remotes/origin/<name>`. Format:

       <origin-uri>?heads/<name>
       <origin-uri>?tags/<name>

For SSH and explicit-host transports, the URI is what the user
typed (`localhost:src/git`, `//localhost/path`, etc.). For local
path access, canonicalise to `file:///<absolute-path>`.

**Worktree state is NOT in keeper/refs.** The per-worktree branch +
commit pointer lives in `sniff/at.log` (see `sniff/AT.md`).
keeper/refs carries only replicated refs (heads/tags of this repo
and remote-attributed equivalents).
