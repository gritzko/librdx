#   Position on Braid and HTTP Sync

The [Braid working group][B] aims to extend HTTP with data syncing semantics.
The group has produced a [draft][d] and holds regular meetings.
The participants all belong to an Invisible College group.
There is no industry participation.

I (gritzko) comment on the draft and the direction of work as an author of one potential data syncing tech, RDX.
The draft itself implies that the data syncing machinery should be pluggable, so why not RDX.

First of all, whom the draft is written for?
People who will have to implement it are the authors of Web servers and Web browsers.
If so, the document is in fact a petition to Google Inc and a handful of lesser players.
Unless those players are fully onboard with the proposal, it would be wise to proceed cautiously.
For example, not to demand any changes to HTTP but to define a thin layer on top of it.
That way, it can work without Google and others.
They are free to catch up later, no worry.

The other issue at hand is the fact that syncing algos and data formats should be pluggable.
Indeed, there is no single clear standard in this domain.
In some domain we might even have such a de-facto standard. 
Consider `sqlite` as *the* standard for embedded SQL databases.
But, `sqlite` is one particular piece of software; as such, it can not become a standard.
Every browser has `sqlite`, but you can not use the API from JavaScript for that very particular reason.

If algos and data formats can not be specified in advance, the value of a common standard becomes unclear.
It should make things compatible, but different sync algos are (most likely) incompatible.
The most we can expect here is the browser being able to recognize the mechanics of syncing, roughly. 
If so, it can play along with whatever algorithm is in fact implemented by the Web site in question.

All of the above being said, the problem in fact exists and the solutions are being routinely improvised.
If so, wouldn't we benefit from at least some convention on the subject?
That may not be an internet standard on day 1 (or day 1001), but precedents are ample and reinvention is evil.
I will try to hypothesize the least constraining convention, just to invite some comments.

 1. We speak of the certain type of Web sites, which are like "HTTP interfaces to user-contributed data banks".
    That might be GitHub or that might be some aggregator. Social/messaging platforms may also fall into this
    category. [BlueSky][s] is particularly relevant.
 2. First and foremost, we divide the entire thing into a "data bank" and an "interface".
    The state of the data bank can be described by a cryptographic hash.
    The interface is a set of view functions that map parts of the data bank into (mostly HTML) strings.
    Those functions we will consider pure and immutable, for the sake of simplicity.
 3. Each view function can have arbitrary parameters: object ids, paths, UI state parameters, etc.

Now, we want to express all these things in terms of an HTTP request/response in the most compatible way.
For example:

 1. The name of the data bank is expressed as a host name.
 2. The state hash goes into the HTTP ETag. Change of state causes caches to clear.
 3. The HTTP URI path identifies the view function.
 4. The HTTP URI query identifies the view function parameters.

At this point, we defined how to *read* our data bank.
Syncing is a separate matter.

From my experience, syncing is not necessarily done in the terms of the underlying data model.
The client-side code might not have the full data model logic.
Edits can be made by "impure" functions that accept some view-domain parameters and map them into the model-domain.
Here is our C of MVC, by the way.
We reinvented MVC again, right.

This way or another, we might have a very relaxed convention akin to ReST, but sync-centric.

Can this become React-at-the-browser-level?
Sure it can, if:

 1. view functions are isomorphic, so the browser can execute them locally;
 2. the state is accessible on the client (at least in part);
 3. updates can be received incrementally;
 4. optionally, the call-graph of view-functions is visible to the browser, so
    all the redrawing and refreshing logic can be derived from that.


[B]: https://braid.org
[d]: https://datatracker.ietf.org/doc/html/draft-toomim-httpbis-braid-http#section-2.1
[s]: https://bsky.app

## Replies
- Michael Thumim replied at https://braid.org/post/response-to-gritzko
