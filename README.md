# lisaobj

The `lisaobj` tool currently dumps the data structures in a Lisa object
files.

Eventually the goal is to provide more tooling around accessing this
data to make it easier to dump files in a way that allows them to be
recreated with patches.


## Missing Pieces

There are a couple of things not yet implemented that may be of interest.


### Old-Style Executable Blocks

These and their associated jump table structures would be useful for
people examining older binaries, such as those for the Lisa 1.


### Application of Relocations

It's unclear whether relocations actually need to be applied to code
blocks in order for them to be reasonable to disassemble. If so, then
`lisaobj` should have a way to do this.


### Direct Code Extraction

It would be convenient to have a subcommand that can extract the code
modules from an object file directly into their own appropriately-named
files.


## lisapack

Also included is a `lisapack` utility that can be used to pack and
unpack code using the Lisa code-compression algorithm, which is useful
for testing.


## Copyright

Copyright © 2026 Christopher M. Hanson. All rights reserved.
See file `COPYING` for details.
