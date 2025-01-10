""`
Floats are normal 64bit ISO floats with all the normal quirks.
```
12.3,
1.23e1,
1.23E+1,
0.123e+2,
123E-1,


```
Integers are signed 64-bit, no surprises. 
```
-9223372036854775808,


```
References are logical timestamps, essentially.
They are 128-bit, two-part (src and seq), rendered in hex.
```
fff-0,
fff-000,
0fff-0,


```
Strings are UTF-8.
Within a string, newlines can be escaped in two ways, or non escaped in multiline strings.
```
"\n",
"\u000a",
`
`,


```
Terms are any Base64 strings with formal structure: true, false, null, hashes, UUIDs, dates, etc.
```
SHA1~d066c56e1a298f76a14829c3c5d88734562a5c5d,


```
Tuples are just tuples of any other FIRST or PLEX elements (couples, triples...).
All tuple elements can be *replaced* but the first one.
There are two notations for the tuples: the generic bracketed one and the compact colon notation.
```
1:2:3,
<1,2,3>,
```A tuple of one is the element itself```,
<"one">,
"one",
<<<"one">>>,
```
An empty tuple is effectively the "null" value.
```
<,,>,
::,
<<>,<>,<>>,
<>:<>:<>,
< <> <> <> >,


```
Linear collections are vectors/arrays of FIRST/PLEX elements.
Differently from tuples, it is possible to *splice* a linear collection.
```
[1, 2, 3,],
[1,2,3,<>],
[1 2 3 <>],


```
Eulerian collections are sorted sets / maps.
To turn a set into a map, use tuples.
In the JDR notation, sets may go unsorted.
Any duplicate keys will be merged.
```
{ , 1, 2:3, 4:5},
{<> 4:5 2:3 1},
{<> <1> 2:2 2:3 <4,5>},
{ ,,,, 1, 2:3, 4:5},


```
Multiplexed collections are counters, version vectors, etc.
These are divided/sorted by the contributor.
Any entries by the same contributor get merged.
```
(1@a1ece-1, 2@b0b-2),
( 1@b0b-1 1@a1ece-1 2@b0b-2),

