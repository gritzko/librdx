  # RDX Command Line Utilities

 ## rdx

A toolbox utility; converts between RDX-TLV and RDX-J, merges, produces patches.

A pipeline invocation of J-to-TLV, merge and TLV-to-J respectively:
````
    $ echo "<1:2>,<1:1:3>" | ./cli/rdx tlv | ./cli/rdx merge | ./cli/rdx j
    <1:2:3>
````

 ## ok64
