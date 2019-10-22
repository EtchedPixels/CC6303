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


## BIG ISSUES

- We can make much better use of X in some situations than the cc65 code
  based generator really understands. In particular we want to be able to
  tell the expression evaluation "try and evaluate this into X without using
  D". In practice that means simple constants and stack offsets. That will
  improve some handling of helpers. We can't do that much with it because
  we need to be in D for maths.

- Sorting out psha/b ordering versus x and in memory big endian. Right now
  it's definitely wrong all over the place

- Sort out stack offsets

- Sort out entry/exit code. It would be nice to be able to dump the pre/post
  entry code if not used. It would also be nice to generate different
  methods according to size. With a small stack offset pulx/ins are
  attractive, with void or u8 returns abx is attractive too

- Fetch pointers via X when we can, especially on 6803. In particular also
  deal with pre/post-inc of statics (but not alas pre/post inc locals) with

````
	ldx $foo
	inx
	stx $foo
	dex
````