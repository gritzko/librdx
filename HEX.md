#   Hexadecimal coding

Hex coding for any binary data.

Redefined mostly as an example how drain/feed semantics works
between buffers in different formats (here, hex and binary).
The conventions:

 1. Subroutine name starts with the module name,
    which is all-caps up to 4 chars (like a ticker symbol)
    The next word (lowercase) is preferably a verb.
 2. All subroutines that can fail return `ok64`.
 3. The returned result goes as the 1st parameter 
    (as a pointer, standard C).
 4. Slices are consumed, unless we imply the parameter
    must be consumed fully. Then, it is untouched.
    Either way, that is signalled through the slice 
    type, e.g.
      - `$cu8c` untouched -- not consumed, not changed,
      - `$u8c` consumed, content unchanged,
      - `$cu8` not consumed, content changed,
      - `$u8` consumed, changed.
 5. Subroutines do not know whether the memory was allocated,
    mapped or it is a part of the stack. They are given a
    generic memory range. There is no memory-owning HexBuffer 
    or HexStream or anything. Those would impede composition.
````
fun ok64 HEXfeed($u8 hex, $u8c bin);
fun ok64 HEXdrain($u8 bin, $u8c hex);
fun ok64 HEXfeedall($u8 hex, $cu8c bin);
fun ok64 HEXdrainall($u8 bin, $cu8c hex);
````
