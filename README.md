# CC6303
A C compiler for the 6803/6303 processors (but probably never the 6800)

This is based upon cc65 [https://github.com/cc65/cc65] but involves doing
some fairly brutal things to the original compiler. As such I currently have
no plans to merge it back the other way.

In particular cc65 has a model where the code is generated into a big array
which is parsed as it goes into all sorts of asm level info which drives
optimizer logic. It also uses it to allow the compiler to re-order blocks
and generate code then change its mind - some of which I need to put back
with a new generator.

cc65 alas doesn't use it to generate classic C ordered argument evaluations
which it could have done so easily, but cc6303 will do.

The assembler to go with it is in the Fuzix tree as it's generated from
the MWC assembler bits used there plus my linker and tools.

## Status

It can just about compile a simple "return 0;" at this point. A lot of
further code cleanup and the new output side are needed to generate
any useful code.

## TODO

- Strip out lots more unused cc65 code. There is a lot of unused code,
  and a load of dangling header references and so on left to resolve.
- Write a new output engine that just keeps a linked list of lines and can
  do the needed line removals
- Add the ability to stack/pop code generation so that we can fix the
  argument ordering
- Either it or the assembler needs to be able to guesstimate label distances
  so that it can generate inverse branches. Traditionally this would be in
  the assembler


