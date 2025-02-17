# CRDT: from revolutionary to routine

Conflict-Free Replicated Data Types (CRDTs) have been initially 
proposed to ease the development of collaborative Web apps.
Later, their ability to sync data both ways, as opposed to one-way
master-slave, evolved into the broader local-first paradigm.

Essentially, CRDTs buy deterministic convergence at the price of 
versioning metadata. That created the long-standing problem 
of overheads, a major impediment to the practical use of CRDTs. 

Replicated Data eXchange format (RDX) is an ongoing project 
to make CRDTs practical for use in commodity Log-Structured 
Merge (LSM) databases. Conveniently, popular LSMs support merge 
operators. RDX merge algorithms are all single-pass iterator 
heap based, which is standard for LSMs. 

The main issue was the CRDT metadata that consists of three 
components:

 1. version vectors to describe replica state,
 2. logical timestamps to version data elements,
 3. tombstones to prevent resurrection of the dead.

RDX experimented with many ways to tackle the problem.
The most practical one was based on versioning the data in 
coarser blocks using the natural tendency of every LSM to
separate new and old data as immutable SST files.

Looking from that angle,

 1. version vectors are not used,
 2. sequential numbering replaces logical timestamps,
 3. the need for tombstones is reduced;
    with epoch compaction, eliminated.

Such a CRDT LSM database is hardly different from a regular LSM.
Except, it syncs well.

As a proof of concept, we converted PebbleDB and RocksDB to use 
RDX CRDT with no major difficulty. There is an ongoing work to 
develop an RDX-native SST format.

LSM databases scale enormously. Consider Amazon Dynamo, Google's 
BigTable or Cassandra as used by Apple.
CRDT LSM can likely scale even further by reaching
into the client side. That may advance the state of the art in
databases and maybe the very definition of what a database is.

