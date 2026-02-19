#   I. SCM as a database for the code

<img align=right width="40%" src="./img/tools.jpg"/>
Software development is changing rapidly and the tool stack has yet to catch
up. As we see, the value of IDEs diminishes as developers are less inclined to
edit the code now. More and more of the work is browsing and talking to LLMs,
less and less is coding and debugging. About 8 years ago I gave a talk at the
internal JetBrains conference "Code is hypertext, IDE is a browser". Those
points look even more relevant now: effective browsing of code and history is
a prerequisite to effective understanding. Understanding underlies everything
now. No understanding = no control, then a developer is like a rider who fell
off a LLM horse with his foot caught in the stirrup (you may search YouTube to
understand what I mean).

`git` is increasingly becoming a point of friction. LLMs have high throughput
in regard to code edits. Sorting out the changes then takes disproportionate
time and often repeats your previous work, if you *actually* read the diffs during
the session, which I highly recommend. Even single-person development now becomes
collaborative: at the very least, your collaborator is an LLM. In calm waters,
running several agents is nothing special. Then I have an entire team, with
all the merges and rebases (which we like to do beyond any measure).

That is why I think it is the right time to look for git replacements, and
that is why I am working on one.  I definitely reject the "git compatible"
approach despite the immense gravitation of the existing mass of git repos.
[`jj`][j] to `git` is what `subversion` was to `cvs`. What we need is what
`git` was to `cvs`: a level-up. All the long-standing and all the new issues are
all rooted in the core architecture of git. In any other case, those issues
would be fixed by now just by gradual and incremental improvement.

The issues are:

 * The monorepo problem: git has difficulty dividing the codebase into modules
   and joining them back. Apart from the fact that git submodules have been
   improvised clumsily and haunt us ever since, the very conceptual approach to
   splitting and joining the code is lacking. All the Big-monorepo companies
   either use something else or build on top of git.

 * The split/join problem has way more implications. Suppose, for example, I
   want to keep my prompts and plans in a separate repo, but join them in when
   necessary. Or, go full JTPP - "just the prompt, please".
   Git has no solution for such "overlay branches", in principle.
   There is a source tree in git, there is a build tree somewhere else, and
   there is a prompt/todo/plan tree in yet another different place.

 * The merge/rebase problem: merge commits create quite a lot of friction,
   while rebases discard the context and imply hierarchy. Fundamentally, git
   merges are an act of will, they are not deterministic. Hence, a merge has
   to be recorded with all the ceremony. On top of that, git is not
   syntax-aware, so false conflicts are pretty common.
   Manual resolution of trivial conflicts is another aspect of friction.

 * Lack of any any code insight features better than `grep`. If SCM is a
   database for the code, there must be a well-developed query language.
   I want to see what changed in a particular function since day D or
   what new uses it had in that time. Like git meets IDEA.
   IDE/LSP gives us spatial structure of the code, SCM adds temporal dimension.
   That is especially valuable when investigating what agents *actually* did.

 * Data accretion problem: once you commit things into the repo, they are tied
   in the Merkle graph forever. There are ways to receive only the latest
   version, but any general pay-as-you-go mode is lacking.
   git's data integrity model is blockchain-like: all or nothing.
   (In fact, a lot of history is trimmed by rebases, as things would be
   unmanageable otherwise. That is also an issue, as the actual lineage of
   an edit gets discarded entirely.)

 * The data model problem: git internally works with blobs, which is quite
   blunt. In fact, we got to the bottom of it: git is a [content-addressable
   filesystem][c], not a content-addressable database.

Overall, we need a *database* for the code!

Again, these points I mentioned at various conferences during the past 10
years, and many other people in the CRDT community talked about "overlay
branches" and "CRDT revision control" for 10-15 years. In essence it all boils
down to two things:

 1. versioning data structures, not blobs and
 2. having formal deterministic merge algorithms (associative, commutative,
    idempotent).

One approach to it was to represent text as a CRDT vector of letters, and it
was quite popular in the field. [Zed's DeltaDB][d] aligns with that
[approach][a]. I also made such systems in the past. It is safe to assume it
the default. On the other hand, if we look into the inners of any [JetBrains
IDE][p] or [LLVM internals][l], we will see AST trees. Because code has
structure. If you want to treat all source code the same, you use line-based
text (like all UNIX tools do). If you want to do fancy stuff, you parse the
source and work with ASTs. Git is a filesystem, so it treats everything as a
blob (git diff receives input blobs and reconstructs the most plausible edits
[algorithmically][m]).

Here I see the opportunity: a revision control system working with AST-like
trees, with very formal, deterministic and reversible split/join/fork/merge
semantics and a structure-aware query language. As a substrate, I use
[Replicated Data eXchange format (RDX)][x], a JSON superset with very nice
CRDT merge semantics.

[**Part II. CLI and REST interfaces**][2]

**Part III. Inner workings of CRDT revision control.**

**Part IV. Experiments.**

**Part V. The Vision.**

[a]: https://zed.dev/blog/crdts
[c]: https://git-scm.com/book/en/v2/Git-Internals-Git-Objects
[x]: https://github.com/gritzko/librdx
[d]: https://zed.dev/blog/sequoia-backs-zed#introducing-deltadb-operation-level-version-control
[p]: https://www.jetbrains.com/help/idea/psi-viewer.html
[l]: https://clang.llvm.org/docs/IntroductionToTheClangAST.html
[j]: https://www.jj-vcs.dev/latest/
[m]: https://nathaniel.ai/myers-diff/
[2]: partII.md
