# CC6303
A C compiler for the 6803/6303 processors (but probably never the 6800)

This is based upon cc65 [https://github.com/cc65/cc65] but involves doing
some fairly brutal things to the original compiler. As such I currently have
no plans to merge it back the other way.

In particular cc65 has a model where the code is generated into a big array
which is parsed as it goes into all sorts of asm level info which drives
optimizer logic. It also uses it to allow the compiler to re-order blocks
and generate code then change its mind.

## Status

The basic structure is now reasonably functional. You can "make" and "make
install" to get a complete compiler/assembler/linker/tools that appear
to generate actual binaries.

Simple code appears to be producing valid looking if not very good code.
Lots of more complicated things will blow up. In general integer code is
likely to be ok. Longs need a lot more debugging and 32bit support code
writing and debugging.

## How to use

For a simple test environment the easiest approach at this point is to
compile the code with cc68 and then link with a suitable crt.o (entry code)

ld68 -b -C startaddress crt.o mycode.o /opt/cc68/lib/lib6803.a

## TODO

- Strip out lots more unused cc65 code. There is a lot of unused code,
  and a load of dangling header references and so on left to resolve.

- Support -lfoo not just -llibfoo.a in the front end. There are other
  cc'isms we need to address as well.

- Remove remaining '6502' references.

- Strip out lots of unused options like -o.

- Make embedding C source into asm as comments work for debugging

- Allow the code segment to be set so code can be compiled into other
  segments like .discard

- The assembler uses 15 char names internally. The compiler does not. This
  leads to asm errors when the symbols clash.

- Maybe float: cc65 lacks float beyond the basic parsing support, so this
  means extending the back end to handle all the fp cases (probably via
  stack) and using the long handling paths for the non maths ops.

- Fuzix integration for the front end

## BIG ISSUES

- We can make much better use of X in some situations than the cc65 code
  based generator really understands. In particular we want to be able to
  tell the expression evaluation "try and evaluate this into X without using
  D". In practice that means simple constants and stack offsets. That will
  improve some handling of helpers. We can't do that much with it because
  we need to be in D for maths. Right now the worst of this is peepholed.

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

- copt has no idea about register usage analysis, dead code elimination etc.
  We could do far better with a proper processor that understood 680x not
  just a pattern handler. We fudge it a bit with hints but it's not ideal.

## 6800

The 6800 is not currently supported. It has a few gaps that are awkward to
address, in particular the fact it lacks PSHX and ABX. The compiler uses the
fact it can PSHX/PULX a lot to improve 16bit handling, for temporaries and
for other stuff. Supporting that will not be trivial - and I don't have a
6800 machine so it's not my problem!

## IDEAS

Stub code that is a chain of ins/des instructions with labels for each so
you can jsr to shift the stack by 4-n bytes in the ugly cases.
