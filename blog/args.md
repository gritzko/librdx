#   Arguments for a syncable data exchange format

After spending about quarter a century intermittently between academia and 
industry I feel the urge to share the progress of [my current project][r] as well 
as my views on what I call "the last bottleneck of the internet".

I dare to go a step further than my colleagues from [Ink&Switch][i], who started
the entire "local-first" movement. Or maybe I want to refine their approach.
In my opinion, the problem starts several steps earlier than developers
picking the architecture for their future app. It starts with the mental model
and the languages we use to coordinate computers. 
Everything else is downstream from that.

##  The internet's bottlenecks

Historically, the internet has been bottlenecked by many things: availability
of computation, storage, network, and qualified developers. Technically, this 
is the time when all the historical bottlenecks seem to be defeated, forever. 
A one-terabyte store can fit on my pinky nail. Forms of network connectivity 
are diverse, abundant and universally available. Computers in our pockets are 
immensely powerful. Finally, we can generate plausibly working code by megabytes,
just in case there is nothing readily available on GitHub.

Still, when I do a check-out in the Auchan supermarket, my device needs time to
fetch my client bar code (which never changes). Sometimes that fails after a wait.
On a grander scale, this strange computer behavior has much bigger consequences.
Some routine mishaps in [AWS][a] or [Cloudflare][c] lead to solid chunks of the internet
just falling flat in a giant domino effect. It seems that the network designed 
to withstand a nuclear holocaust can not withstand a misplaced comma nowadays.
Overall, it all increasingly feels like one of those projects Google can cancel
in a quarterly meeting. Something is just not good about the existing system.
It does not help that external challenges also grow (severed cables, shutdowns).

On the technical side, the fallacies of distributed computing still hold:
bandwidth is not infinite, network is not reliable, latency is not improving.
Some of the unfortunate effects I mentioned trace back to that.

Laws of nature are unavoidable, but human mistakes are not.

##  The language problem

I dared to try L.Wittgenstein's approach. What if the problem is in the language?
Namely, that the language we use prescribes our way of thinking about
the systems and inevitably leads us to the same outcomes, again and again?

The predominant paradigm for a computer-to-computer conversation was,
for decades, "give me something to draw on the screen" (torn tape, punch cards,
CRT, LCD, OLED, whatever). The information sent is ephemeral and is only good
for immediate consumption. That in turn led to "amnesic computing" where most
computers on the internet need to ask somebody else to know their own name,
whom they are talking to and what they should be doing. Once no one is responding,
and basically on any significant glitch in the chain of command, the entire thing
collapses like the Sauron's kingdom with amnesic orcs.

What if the paradigm changes to "tell me the facts, so I remember"?
Everyone acting on the best available information is a good baseline strategy.
One aspect preventing that is the lack of data versioning by default. Data rots.
Immediately. The best shot so far was adding the expiration time, and it helps.
But adding full versioning may turn "amnesic" into "autonomous". Computers can 
cache data, update it as necessary, and act on the best data available.
But, how much versioning do we need?

##  Technicalities of the data format

I pick JSON as an easy target, although most of the arguments apply to many other
formats equally well. JSON is a notation, so there is some guarantee that the other
side will parse the syntax, but there is no guarantee the document will be seen
the same. I do not even speak about "onthologies". Consider the case of a repeated
key in a map, for example. JSON has [many such cases][m]. 

Now, let's consider data mutations (edits, updates, etc). In theory, there are "JSON
patch" formats, but in fact, the outcome of (repeated) patching is not well defined,
especially in a distributed system that is more complicated than "one client, one
server".

[Replicated Data Exchange][r] format (RDX) is a JSON superset that has a formal
document model and a formal merge model defined. Precisely, down to the bit.
So, not only implementations understand each piece of data the same, 
they can merge them identically. That makes the format syncable: we can always 
send a patch over and let the other party arrive at the same identical state,
independently of any transient circumstances like the order of changes, repeats,
and so on. Broader, we can send data piecemeal and update later as necessary, all
with some healthy degree of negligence.

With no blockchain.

##  Freedom of representation

The first powerful consequence of having a formal document model is the fact
we can define RDX serializations as we see fit. As long as an RDX variant maps 
1:1 to the formal model, it is good to use. That is similar to the decoupling
the relational model introduced; we can pick a data structure that fits our
needs in a particular case (text, B-tree, SS-table or full LSM, etc). One 
can not say "RDX is verbose", for example, because nothing prevents bitpacked
binary RDX variant from being used, as long as we tested the 1:1 guarantee.
The serialization format is not an eternal damnation but a technicality.

The second consequence, the storage format no longer prescribes usage patterns.
RDX elements (e.g. a set) can be object-sized or collection-sized or table-sized
or database-sized. Or, maybe, internet-sized in case we all use one shared
object for some purpose. These are all good, if stored in appropriate format.
In particular, the skiplog-based binary RDX (SKIL) can handle search in large
nested objects in a way relational representation can not. That is a lot of
flexibility.

##  The merge operation

OK, but why do I put this much significance into the merge operation?
Here is the third powerful consequence: we can split data *by either axis*.
Consider an LSM database: it splits data both "temporally" (different SST chunks
have data of different age) and "spatially" (different key ranges). 
If you can merge, you can split! In other words, we have all possible freedom 
in choosing where to store which data, as long as we know how to merge it back.
This can be compared to running a Cassandra-like database "inside-out".

In fact, that is more. Not only updates can be implemented through merges, 
but also sharding, snapshotting and branching, and even access control (data of
different access levels being stored to different branches). Merges allow for
incremental querying, when we reveal the data landscape gradually like a map
in a computer game. Why start from scratch each time? (Why being amnesic?)

Hence, not only the formal model, but its mutation model is the key enabler.

##  The paradigm shift

How the world may look like if computers use a syncable data exchange format?

First of all, one server disappearing from the internet will no longer throw
the rest of the system into nothingness. When half the internet is down because
of an AWS outage, it is not really OK, but at least safe. What if drones start
falling from the sky instead? Hence, devices must be autonomous-by-default.

Second, devices will stop forgetting information they have been showing to you
just a couple seconds ago. My Auchan app will be able to show my client bar
code without asking the internet for help. 

Third, we may start talking about some meaningful use of all the computational
capacity we actually have. Not internet-of-shit level things, but actually
useful *and reliable* stuff. Putting smileys on photos is one thing, but anything
with mass and velocity better be very very reliable.

[m]: https://seriot.ch/software/parsing_json.html
[i]: https://www.inkandswitch.com/
[r]: https://github.com/gritzko/librdx
[a]: https://aws.amazon.com/premiumsupport/technology/pes/
[c]: https://www.cloudflarestatus.com/history
