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

Simple code appears to be producing valid looking if not very good code.
Lots of more complicating things will blow up, in particular some of the
handling for initialized local variables needs to be totally different to
the 6502 approach because we have a real stack.

## TODO

- Strip out lots more unused cc65 code. There is a lot of unused code,
  and a load of dangling header references and so on left to resolve.
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

- Sort out entry/exit code. It would be nice to be able to dump the pre/post
  entry code if not used. It would also be nice to generate different
  methods according to size. With a small stack offset pulx/ins are
  attractive, with void or u8 returns abx is attractive too, with a big one
  a framepointer is best.

- Fetch pointers via X when we can, especially on 6803. In particular also
  deal with pre/post-inc of statics (but not alas pre/post inc locals) with

````
	ldx $foo
	inx
	stx $foo
	dex
````
- Make sure we can tell if the result of a function is being evaluated or if
  the function returns void. In those cases we can use D (mostly importantly
  B) to use abx to fix the stack offsets.

- Is it worth re-implementing register variables. On cc65 they are a big
  help for some use cases as you have to indirect via ZP to do most things.
  On 680x ZP variables are just faster memory with short instructions except
  for some obscure 6303 features (AIM/OIM/etc)

## IDEAS

- Store 0,0,1 as 3 bytes in DP then we can load/add/sub 0 into D and X in
  less bytes via direct page

## BROKEN

- Need to push old reg vars when doing new
