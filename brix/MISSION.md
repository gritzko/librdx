#   BRIX data architecture

When [Swarm.js][s] was launched 10 years ago, nobody did JavaScript CRDT data syncing.
These days, there is no shortage in JavaScript syncing frameworks. (No list here,
cause I do not keep track anymore!) Some are CRDT, others cache relational data
or even run a database in WebAssembly. It is time to level up the game!

The BRIX data architecture implies that "public data" will be like "public WiFi".
You download datasets to your device and you keep them continuously synced.
Lots of large datasets. All linked. Why does it make sense?

We already have "supercomputers in our pockets". 1TB SD card? Easy peasy!
With the developments in AI, edge devices finally can make sense of that data.
Would a self-driving bus benefit from traffic light timetables, traffic cam
data, bus stop cam data? Sure. Are any people waiting on the next stop?
Is there a traffic jam? That is all relevant. These decisions better made at
the edge. Move it to the cloud and some electromagnetic interference would
be able to send a car off the cliff. Or the bus will stop in the tunnel.
"Sorry, my humanoid robot is epileptic in elevators!" An average factory
floor is one big bunch of electromagnetic interference, by the way.

So far, the human-computer interface was a bottleneck. Now, our devices greatly
improve at understanding what we want so they can shovel the data.
What we should do here is to change the paradigm from "ingestion" to "syncing".
These are basically two ways to keep the data up-to-date: either ingest into
the cloud or sync at the edge. Obviously, in case #1 you are the product.

Want to see [barefoot developers][b] using AI tools? But, where will they get
the data? Because: garbage in - garbage out. I call that "the GIGObytes problem".
To make things sane and meaningful, the data must be available first.
With integrity guarantees and provenance. Because if your LLM ingests GIGObytes
of random internet texts, which are, in turn, increasingly LLM-generated...
you get the idea. The data may not even be public, but it should be somehow
available.

BRIX is specifically an architecture for massive data synchronization at the edge.
BRIX is based on the Log-Structured Merge Tree architecture, which underlies
the most immense databases created at Big Tech companies. LSM is inherently based
on the *merge* operation. [RDX][r] CRDT merge is way more powerful than LWW merge
you normally see in likes of Cassandra, BigTable, and others. Still, in terms of
computational complexity, it is the same: O(n) single-pass merge-sort-like algo.
The basic building block of LSM (the "brick") is an SST file. A BRIX SST file is
more like a git object: hashed, signed, cryptographically referenced.

The questions to be answered by the BRIX project:

 1. can we build a web of interlinked databases using RDX LSM?
 2. can we arrange those databases to be mergeable/blendable in arbitrary ways?
    (this one is a pipe dream of CRDT research for maybe 15 years)
 3. how do we work with these point-of-view custom database amalgamations?

BRIX is an ongoing research project. Stay tuned.

                                        Victor "gritzko" Grishchenko

[s]: http://github.com/gritzko/swarm
[b]: https://maggieappleton.com/home-cooked-software
[r]: ../rdx/
