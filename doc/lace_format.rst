Lace Format
-----------
This document describes an ascii representation for usage
with a lace tool (brother and others).

Characters:
' ' - empty, no stitch (only for reference)
'.' - stitch, no lace manipulation (only for reference)
'<' - normal lace, move this stitch to the left.
'>' - normal lace, move this stitch to the right.
'\' - fine lace, move this stitch to the left.
'/' - fine lace, move this stitch to the right.

Limitations
~~~~~~~~~~~
Same manipulators cannot be used twice on the same direction,
for example, this is invalid:

   ...<<.......<...
or
   ...<.<..//..<...

while this is valid:

   ..<.<.......<...
or
   ..<.<.././..<...

Compilation
~~~~~~~~~~~
We provide a python script that validates, and renders this
into a raw image with memos:

   0 blinking means -> pass lace tool in Normal mode
   1 blinking means -> pass lace tool in Fine mode
   2 means          -> pass K carriage left and right over the stitches.



