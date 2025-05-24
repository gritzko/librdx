#   RDX command line interfaces

RDX/JDR has a rather lightweight parser able to tokenize integers,
strings and various syntactic structures. There is an obvious 
gain in adopting RDX for command language serialization. 
That way, we can unify:

 1. command line syntax (CLI), 
 2. config file syntax, 
 3. REPL syntax and 
 4. network protocol syntax.

...thus minimizing the cognitive and the coding effort.


The general convention is to use one tuple for one command,
the first element being the verb and the others are arguments.
In the case of no arguments, we just use the verb (RDX Term).
For example, a simple script in JDR may look like:
````
    parse: "input.jdr"
    merge
    print: "output.jdr"
````
A wire protocol for an RDX storage may look like:
````
    > get: Bob-af8k
    < {@Bob-af8k 1:one 2:two 3:three}
    > get: Bob-af8
    < FLYnone
    > <add {@Bob-af8k 4:four 5:five}>
    < OK
````

Using the same conventions, a CLI syntax should look like:
````
    ./rdx parse:"file.jdr"
````
Here, we may need some CLI-specific *adaptations* as UNIX shell
has its own punctuation.
To make autocompletion work we allow for unquoted strings:
````
    ./rdx parse: ./file.jdr
    ./rdx parse: file.jdr, merge, print
````
...we also have to escape many things, like `(` or `<`... and that's about it.

In scripts we may need to use named parameters or variables
The convention is to have them numbered `~1` or named `~name`.
Here, tilde `~` replaces `$` as `~` is a RON Base64 symbol
and `$` is not. Hence, variable names are RDX Terms.

