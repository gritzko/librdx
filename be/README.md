#   II. Beagle SCM: HTTP/CLI interface

[**Part I. SCM as a database for the code**][1]

Despite L.Torvalds initially described git as "information manager
from hell", it was a very innovative revision control system. 
That was 20 years ago and the architecture shows its age, its core 
limitations causing heaps of accidental complexity. A jungle of
commands, options and syntaxes with overlapping concerns,
combinatorial maneuvers between worktree, stash, staging, commits,
local and remote branches, and finally the abstraction of eternal 
cryptographically protected record we all rewrite daily to do the
most basic stuff.

`git` is a *filesystem*, it says so [on the box][c] and it stores
*blobs*. That makes it rather blunt when it comes to merging the
changes. That is the reason behind lots of git ceremony and
limitations. With LLMs, generating code is cheap; sorting it all
out is more work though. Here, git distracts and limits developers.

Beagle is a *database* for your code, it stores coarse AST (abstract
syntax tree), so it can do merges deterministically, non-intrusively,
and fully aware of the syntax. It can address and describe changes
in terms of symbols (functions, classes), not just files and hunks.
Blob-level versioning remains as a fallback only. For Beagle, there
is a file tree and AST trees of individual files with named nodes.
"How did this function look a month ago"? "What new calls did my
function accrete in a week?" This model allows to build much sharper
tools. Remember that git codebase is 310KLoC of C code and about the
same amount of sh/perl/tcl. That is x15 more than LevelDB, a third
of PostgreSQL, about three SQLites, and generally in the ballpark of
a general-purpose database. But, the underlying model limits greatly 
what we can query git about. Beagle's model is less limiting.

Technically, Beagle is able to version and merge AST trees as CRDTs. That
differentiates it from other stack-of-patches VCSes, such as Pijul, Darcs and
others. Fundamentally, CRDTs add some metadata to avoid lots of guessing
later on. (Diff is algorithmic guessing, 3-way merge too.) Data units get ids
and/or timestamps, become addressable, so merges become deterministic and
non-intrusive. One can attach and detach branches by a checkbox. That does not
ensure semantic correctness, but at least one can iterate on it faster.

CRDT's non-intrusive merges give lots of freedom in slicing and dicing larger
repos into branches, overlays, submodules, and so on. If you can merge, you
can split. Want to keep a subdir both here and as a separate project? No problem. 
Want to keep LLM .md files in a separate overlay, only make them visible on request?
Also easy. CRDT gives the freedom in splitting and joining along all the axes.

The classic approach to complexity: minimize primitives, but make them
composable. Fundamentally, Beagle moves changes between worktrees and the repo,
with 2x2=4 potential data maneuvers (repo to repo, repo to worktree, worktree to
repo, within worktree). That is way lower than git's 6x6=36 (local and remote
branches, commits, stash, staging and worktree). At the *plumbing* layer, the 3
maneuvers involving Beagle are served by commands PUT, GET, POST respectively
(yes, HTTP vocabulary on the command line).

The smallest unit of repo *change* is a waypoint, which is a nameless 
Ctrl+S type of event. The most basic workflow is just a trail of Ctrl+S/Ctrl+Z
events creating a set of AST changes. Post-factum, changes can be selected,
recombined and declared a new commit or branch, or added to an existing one. Any
*porcelain* command is a mosaic of GET/POST/PUT over some subset/union of
changes. The difference between snapshot, branch, staging, stash or overlay is
all but non-existant. These are named groupings of changes. An overlay is a
"permanent editable changeset" that can be attached/detached to/from a branch.
A branch is a short-lived stack of changes forked off the head. Technically,
either of them is just a set of delta files (SSTs) producing some state of
the worktree. git-style and Beagle-style porcelain can be used in parallel.

The next section talks about Beagle's porcelain project/ repo/ branch/ overlay
model which is slightly different from git's: repos are closer to git repos
(but one level up), branches are like git branches but lighter, and overlays
have no parallel in git at all.  CRDT merges are deterministic and
non-intrusive, so one can merge left and right, using worktree as a palette
for blending. In fact, the entire porcelain story is a way to sort changes
into orderly boxes of different colors (repos, branches, overlays, snapshots)
with different labels on them.

The section after that talks about Beagle's core/plumbing commands: `GET`,
`POST`, `PUT` and `DELETE`.

Skip next two sections if you want to see the resulting UX first.  Long story
short: mainly the same four commands plus URI-based syntax for everything.

##  Beagle SCM: repos, projects and branches

How to make a command/ referral language flexible enough to
express all the use cases by composing a minimal number of plain
intuitive primitives?  This problem is essentially a language
problem.

In respect to addressing, Beagle bets on URIs. What worked for
a World Wide Web in all its vastness, should also work for
intra/inter repo referencing.

Encouraged by that idea, Beagle sets the scope of the system to
*global*.  One key feature of git was to only version an entire
project as a whole.  Lets think: what can we do to version an
entire working system, all sources and configs, so each repo is
a small GitHub hosting a number of *projects*?

If we want to limit ourselves to 4 basic kinds of *maneuvers*,
those are:

 1. moving changes from worktree to the repo,
 2. moving changes from the repo to a worktree,
 3. moving things in the worktree,
 4. moving things in the repo.

We assume the current worktree is linked to one fixed place in
the repo.  Things look a bit too primitive so far. Then, we
chalk the repo into squares:

 1. there are *repos* which have public identity,
    their names are FQDN-like, e.g. `main.team.company.com` or
    `release.product.entity.org`;
 2. orthogonally, repos are divided into *projects*, also with public
    identity, e.g. `@gritzko/librdx` (like a GitHub path).
    So a full URI is like `http://main.replicated.live/@gritzko/librdx`

Here the maneuver #4 gets subdivided into submaneuvers, the most frequent
case being changeset exchange between repos. Note that repos are
not scoped to a project. When we create a repo, we
"fork the world". That mostly makes sense because projects form their own
dependency graphs anyway, so version *alpha* of project *A* needs version
*beta* of project *B* and so on. Once we create a repo, we *may* put in
all the relevant code. With syntax-aware CRDT merge, we can be a bit
bolder in forking things, as we retain enough metadata to ease merges.

On top of that, the mapping between file system paths and projects is
not 1:1. First of all, one project can have several worktrees, that is
normal. Second, one worktree can contain several blended branches or
projects. Merging the branches is nothing special, let's talk about
the other case. Suppose we want to split one project into the base and
its *overlays*. For example, prompts, plans and TODOs live in the same
dirs in the worktree, but belong to a different overlay project in the
repo, `@gritzko/librdx` vs `@gritzko/librdx.ai`. We can work with the
source, we can add the AI work docs, or we can deal with prompts and
logs separately from sources.

The last caveat for those familiar with `git` (all of us) is *branches*.
Apart from the *head*, a project can have multiple marked *branches*,
which are supposed to merge in near future. The distinction here is that
branches are scoped to a project/repo, and have no public identity.
When each developer teams up with AI, cheaper transient branching is
necessary, locally and within a team. Beagle branches are somewhat
lighter than `git` branches. While a branch is essentially a sticky note
on a hash, CRDT merges are deterministic and non-intrusive, so merging
(blending) branches invokes much less work and ceremony than merging
git branches.


##  Plumbing: `GET POST PUT DELETE`

Back to the original question, lets see whether an URI based referencing
language and 4 HTTP verbs are sufficient to express the operations we want.
GET, POST, PUT and DELETE correspond to maneuvers #2, #1, #4, #4 resp.
Maneuver #3 is `cp`, `rm`, `vim`, etc.

 1. `GET http://repo.team.entity.org/project?branchA` simple checkout
    of a particular branch version (may need to clone first);
 2. `GET //repo2` switching the repo;
 3. `GET /project/dir/file.txt` checkout one file;
 4. `POST ./file.txt` stage one file (it gets imported into the repo,
    but the branch does not move yet);
 5. `DELETE somefile.txt` delete;
 6. `PUT ./file.txt?branchB` merge in file changes from other branch;
 7. `GET ?branchB` switch the branch;
 8. `GET ?timestamp-origin` checkout a version by its timestamp;
 9. `GET ?4d2130` checkout a version by its hash;
10. `GET ?branchA#has(x)` list all uses of symbol `x` in `branchA`;
11. `POST /project?branchA` commit all changes to a branch;
12. `PUT //repo2?branchC` merge a branch of another repo;
13. `POST ?stash; GET ?branchA` stash the changes;
14. `POST ?branchA` commit changes (import, move the branch);
15. `GET ?branchA#has(int,getX)` from the branch, list all AST* nodes that
    have children `int` and `getX` (likely declaration and definition
    of `int getX()`;
16. `GET //repo/project/dir#has(int,getX)` same but fancier;
17. `PUT http://remote.repo.team.entity.org` big time pull;

In fact, most everyday commands would break down into several
`GET`, `POST`, `PUT`, `DELETE` calls as, for example, refreshing
the work tree also requres temporary stash of worktree changes
and their merge back into the refreshed version. Similarly, push
to a remote repo is first a `POST` to a local copy and then `PUT`
to a remote server.

While it is handy that the plumbing layer of CLI is virtually
identical to the HTTP interface, for user convenience we need
the "porcelain" layer doing all the everyday combos in one go.


##  Porcelain

The mission of the porcelain command layer is to let the user
rely on the power of the technology while keeping him/her safe
and sane.

Both plumbing and porcelain layers turn to be quite compact so
far and most of nuance is coded into URIs while CLI verbs only
define the general maneuver.  One tradeoff here is that the user
must have some intuition of URI syntax. LLMs certainly have it,
so no worry if you don't.

Code is hypertext, IDE is a browser. Beagle is your curl/wget,
a simple reliable everyday tool.

### Typical work cycle

Same as plumbing, porcelain commands implement three maneuvers:

  * `get` data from repo to worktree,
  * `post` data from worktree to repo,
  * `put` moves data laterally in a repo.

There are some shortcuts for combos, but most of work is get,
post, put. The most straightforward linear workflow looks like:

 1. `be get //repo/project`         clone/checkout a worktree
 2. `be come ?branch`              fork off a branch (combo of `be post` +
                                  `be get ?branch`)
 3. ...                            do some work
 4. `be post`                      commit/stage all branch changes
 5. `be put`                       merge in the head (or
                                   `be get ?head ?branch ... be post`,
				   a subtly more delicate way to
				   achieve same result)
 6. ...                            verify things work as intended
 7. `be post ?head`                merge into the head
 8. ...go p.3

Mixing branches is done by the same `get` verb but with multiple
arguments. Use worktree as a palette where you mix and blend
colors. Once satisfied, lay the paint on the canvas (post it back
to the repo).

  * `be get ?branchA ?branchB ?head`
  * `be post ?branchABH`

CRDT merge never fails, technically. That does not guarantee
that your worktree would build or run correctly. Semantics is
entirely your(s LLM's) responsibility. Beagle allows to merge/
undo/ juggle changes quickly. That is the best thing SCM can do.

### Handy commands and aliases

There are aliases/combos for typical cases, e.g.

  * `be come ?branch`              make the worktree version into a branch
  * `be diff`                      diff to the head (default, 3way)
  * `be lay`                       make a waypoint commit
  * `be mark "Comment" "Story..."` make a "classic" verbose commit
  * `be moan`                      rollback one post
  * `be rate`                      mark the current commit
  * `be fit`                       merge into the head (`be post ?head`)
  * `be`                           overview of the current state (more than `status`)

Some shells treat `?` as a special symbol, we may skip it most of
the time.  There is risk of URI `?query` being confused for a file
name and other things, so in this doc `?` is never skipped. Still,
`be get ?featureA ?tweakB` should be OK (most of the time).

### Git equivalents

Beagle is balanced differently than `git`. There is one Beagle
repo per system, Beagle repos are between git branches and
git repos, while Beagle branches are lighter than git branches (may
see them as patch stacks). Approximate command equivalents:

  * `git init dir/`               `be post dir/`
  * `git stash push`               there is no difference between stash
                                   and any other commit, so `be post ?mystash`
                                   is enough
  * `git add a.txt b.txt`          same, `be post a.txt b.txt`
  * `git clone http://uri`        `be get http://repo.team.entity.org`
  * `git push origin a:b`         `be post http://repo` repo names are FQDNs
  * `git pull origin b:a`         `be get http://repo ? && be post` where `?`
                                   is the expression for the current worktree's
				   repo/branch formula
  * `git merge xxx`               `be get ?branchA ?branchB`
  * `git status`                  `be`

Beagle (will) implement combos for key git commands.

### Fancier operations

We started with a claim that Beagle is a database, not a
filesystem. It stores a basic AST tree of the source code, which
allows for basic code manipulation and search. That is a great
opportunity to minimize busywork both for users and LLMs.
That is especially valuable when digging code written by
somebody else (which is the case in the overwhelming % of cases
as individual contributors rely on LLM more and more).
Here are some examples of less trivial Beagle commands.

  * `be get /project /project.ai`  blend project and its prompt overlay
                                   (technically, a separate project)
  * `be get ?head ?branch`         blend head and branch (no repo changes)
  * `be put ?branch#DoThing`       cherry pick a symbol from a branch
                                   (will extract a patch based on the AST* tree)
  * `be get ?branch#DoThing`       same, but no commit, worktree only
  * `be put ./file.txt?branch`     cherry pick a file from a branch
  * `be get ./file.txt?branch`     get a file from a branch (no commit)
  * `be get ./file.txt?branch#Some`  cherry pick a symbol in a file
  * `be put ?featureA&featureB`    merge in two branches
  * `be post ?newbranch`           fork
  * `be post //newrepo`            big time fork
  * `be diff ?head#SomeClass`      find any changes to SomeClass since `head`
                                   (prints out patches)
  * `be diff ./file.txt?v1.2`      find all changes to file.txt since v1.2
  * `be diff #has(DoThing,int)`    diff `int DoThing()` specifically
  * `be get ?#todo(asan)`          find things to sanitize, any branch

### Verbless syntax

In fact, the semantic load on the verbs of `be` CLI is to give
the direction data moves in. We may also use a convention with
no verbs at all: `be uri_dest uri_src1 uri_src2...`

That way, `be - ?branchA //repoB` is a merge into a working
tree, while `be //release ?head ?tweaks` is a merge into the
release repo head bypassing the working tree (reckless).

Overall, verbless use allows non-standard/advanced use patterns.


##  Navigating the history

Beagle's commit model is supposed to resolve git's common pain
points and ossified workarounds. git's ideal commit model sees
a commit as something eternal; an [unbroken Merkle chain][s] of
commits goes back to version 0, blockchain-like. But real life
is messy, so we have a number of workarounds to avoid enshrining
everyday hacks in project's "blockchain". Those are rebase, 
squash or (my favorite, `-m fix`) learning to live with disorderly 
histories. History rewriting in git is an advanced and extensive
topic, a yardstick of developer's expertise. As it often happens,
workarounds may need workarounds of their own, and so on.

<img align=right width="40%" src="./img/nav.jpg"/>
Beagle unifies in-repo "buckets": staging or stashing is done by
the same kind of a data container, no different from commit,
branch, or tag, except for the labelling. Beagle makes unnamed
inter-commit states (waypoints) shareable, and all commits in
general aggregatable, so rebasing and squashing become part of
the vanilla model, not an override. CRDTs ease that a lot.
The idea of Beagle commits is to be "undo-redo, but persistent".
If Ctrl+S triggers a commit, there is nothing wrong about it.

On the technical side, Beagle's branches are very much like git's
tags, just labels for hashes pointing at system states. Here,
things do not differ from git that much.  The `head` branch is the
main public version. `get ?head` or `get ?feature`
switches the worktree to a different branch.

Beagle's [Merkle structure][m] is aligned with its [LSM][l] structure.
A project's state is technically a stack of SST files in a
repo. Each (newer, smaller) file references the hash of the
previous (older, larger) file. When pulling changes from other
replica, we can verify that this Tower of Hanoi is mostly
unchanged, except for some smaller files on the top that are
easy to inspect. A full chain-of-commits history is inspectable,
in theory, if all the historical commit files are preserved
somewhere (likely S3).

Waypoint commits are write transactions with no tag attached. One can
address them by time or hash, but their replication to other
replicas is not guaranteed. Those are of local interest and
might be compacted into larger files and garbage collected. That
is the standard LSM way of things.  Important commits are marked
with "sticky notes" (branches and tags). Those are preserved.

Finally, a CHANGELOG document lists all the regular commits
and their attributes: times, dates, comments, authors, hashes,
signatures. These are produced by `be mark` (changelog insert +
`be post` combo). The mission of a changelog is to explain the
rationale behind the changes, as the changes themselves can be
cheaply calculated. Then, changelog changes would serve as 
commit descriptions.

Beagle's model is not exactly real-time, but more like
"continuous", especially if compared to git's commit chains.
Want to send the current uncommited version to CI? Go ahead.

One performance bonus of this architecture is that old deleted
data is not tied in the repo forever. It gradually fades away,
unless intentionally preserved. No replica is required to
maintain full history.

All versions are recoverable (if files can still be found), so
`be get ?26219b4L5j` would recover worktree to a historical
version by a timestamp (base64 coded in this case) or a
hash(let), e.g. `be get ?4d2130` (hex sha256 prefix) or `be get
?4d213077e2bdd7d83e101a82ed070934cd8e2af6d8ded3dc64905736f8a820cb`.
The system will do its best to interpret informal inputs, like
`be get ?head,15:50` or `be get ?head,"skiplist"` based on the
CHANGELOG document and the existing waypoint commits.

Overall, Beagle reworks the model for the LLM age commit volume
and frequency, mainly by borrowing tricks from the most scalable
databases.


##  Querying the AST* tree

What Beagle internally processes is not exactly AST but BASON, a
binary JSON format (a budget variant of [RDX][r]). Beagle employs
*codecs* to import and export files into/from BASON. Hence, most
queries have to rely on generic document tree structure.
The exact codec machinery may vary, e.g. a `*.c` file may be
im/exported with: general text codec, tree-sitter based codec,
`clang` AST based codec or "git mode" fallback. Changing the codec
resets file's history. Apart from the tree structure per se,
codecs may *tag* nodes (the bit budget is rather tight there).
That way, queries may distinguish function from a class,
invocation from declaration, and so on.

Based on that rather generic information, we can have 80/20 of
your typical code navigation: callers/callees, definitions,
todos, and so on. That is way more accurate than `grep` (how
do you grep for a function body?). Still, this may fall short 
of full IDE capabilities. For an inquiring agent, that might be
just right though. 

 * `be grep "search"` trigram-accelerated substring search:
   the trigram index narrows candidates to ~1/4096 of files
   before reading any content, making search nearly instant
   even for large repos
 * `mdp(worktree)` grep for markdown paragraphs (not lines)
 * `has(int,getLen)` find nodes having children `int` and
   `getLen` (e.g. a typical C function definition)
 * `fn(int,getLen)` find specifically tagged function definitions
 * `use(getLen)` find uses of a symbol
 * `funcs(use(getLen))` find functions using a symbol
 * `files(use(getLen))` find files using a symbol
 * `todo(fuzz)` search for TODOs mentioning fuzzing
 * ...and so on

Query notation is RDX, although that hardly matters as it is
generic enough. Each query produces a set of document elements
(AST* nodes) that a command can be scoped to (diff, get, post,
etc). So, for example, we can change signature of a function and
commit specifically those hunks by a one-liner.

If your next question is how to make this work efficiently,
wait for Part III.

----

Acknowledgements. A.Borzilov, N. Prokopov (aka tonsky), J.Syrowiecki
contributed feedback and ideas for this draft.

[**Part I. SCM as a database for the code**][1]

**Part III. Inner workings of CRDT revision control.**

**Part IV. Experiments.**

**Part V. The Vision.**

[1]: partI.md
[j]: https://wizardzines.com/comics/git-cheat-sheet
[c]: https://git-scm.com/book/en/v2/Git-Internals-Git-Objects
[s]: https://www.youtube.com/watch?v=tVIM2xLbQBs
[l]: https://www.cs.umb.edu/~poneil/lsmtree.pdf
[m]: https://en.wikipedia.org/wiki/Merkle_tree
[r]: https://replicated.wiki/rdx/
