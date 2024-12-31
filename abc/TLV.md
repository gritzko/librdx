#   ToyTLV Type-Length-Value binary coding

TLV is the most widely used approach to constructing binary protocols.
Protobuf is TLV under the hood, as well as many others.
ToyTLV is the most basic TLV implementation one can make. 
The baseline ToyTLV record is like:

 1. The record type as a letter `[A-Z]` (one byte),
 2. the payload length as a 32-bit little-endian number (4 bytes),
 3. followed by the payload/value of the specified length.

As a size optimization, *short* records should use one byte for the length. 
Their record type letter is lowercased then.
Using the uppercase form for 0..255 byte records counts as "overlong encoding".

ToyTLV does not define any semantics (ints are no different from strings, etc).
It is an *enveloping* format only.

ToyTLV supports key-value records, which is a very common case.
In such a record, the payload is divided into two parts: key and value.
The first byte of the payload specifies the length of the key.
Namely, a TLKV record is like:

 1. record type (lower/upper case),
 2. payload length (1/4 bytes resp),
 3. key length (1 byte),
 4. key (as specified),
 5. value (the rest).
