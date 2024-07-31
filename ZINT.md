#   "Zipped" integers

A somewhat more performant TLV-centric variation of variable 
length integers, aka. "varints". No LEB128, as we mention the
length in the TLV header.
