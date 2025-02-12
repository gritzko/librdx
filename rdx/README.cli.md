#   RDX Command Line Utility

A toolbox utility; converts between RDX-TLV and RDX-JDR, merges, produces patches.
A pipeline invocation of J-to-TLV, merge and TLV-to-JDR respectively:
````
    $ echo "<1:2>,<1:1:3>" | rdx tlv | rdx merge | rdx j
    <1:2:3>
````
A merge operation on files:
````
    $ cat test.rdxj 
    {1:2},
    {1@2-2:6},
    {eight},
    $ cat test2.rdxj 
    {3:4,4:5,"seven"}
    $ rdx merge test.rdxj test2.rdxj  | rdx j
    {<@2-2 1:6>,3:4,4:5,"seven",eight}
````


