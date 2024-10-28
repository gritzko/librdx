  # RDX Command Line Utilities

 ## rdx

A toolbox utility; converts between RDX-TLV and RDX-JDR, merges, produces patches.
A pipeline invocation of J-to-TLV, merge and TLV-to-JDR respectively:
````
    $ echo "<1:2>,<1:1:3>" | ./cli/rdx tlv | ./cli/rdx merge | ./cli/rdx j
    <1:2:3>
`
```

 ## lex

Converts ABC .lex grammar files into Ragel grammar files.
A boilerplate generator.

 ## ok64

Converts ok64 in its readable Base64 form into C const decimal.
