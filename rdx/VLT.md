#   Value-Length-Type coding

The usual [TLV][T] (type-length-value) coding has to specify content length before adding the content.
That may be a huge inconvenience as we may not known the length before we serialize the data fully.
VLT coding is exactly the opposite of TLV: the header goes after the content.
Later, VLT can be recoded to TLV in one pass.

[T]: ../abc/TLV.md
