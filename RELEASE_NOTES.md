# Alpha 1 Release Notes

## Compiler Properties
````
char: 8bit unsigned by default
integer: 16 bit
long: 32bit
pointer: 16bit

All data is stored big-endian. Register variables are supported.

````

## General Status

The front end is little modified from cc65 so should be robust. The integer
and character support has been fairly well tested and the compiler can build
a mostly functional Fuzix kernel and userspace.

32bit maths is a lot less tested. The basics should be right and it appears
to be working correctly for the Fuzix time handling code.

The assembler is reasonably well tested as is the linker. The linker is
written from scratch so may well have a few glitches left to find.

The cc68 command supports a reasonable set of the usual cc options.

## General Limitations

The 680x processors have a single index register. In general this is not a big
problem and the code generated is acceptable. Functions that copy data around
can however generate quite horrible code. The library supports hand written
implementations of key functions where this matters - memcpy for example.

The compiler works internally on 16bit values. It has some understanding of
byte operations. Providing signed char is not used the extra code where it
uses 16bit operations has a minimal cost (at least on 6803 and 6303) and the
optimizer can already clean some of it up. Signed char is very expensive and
should be avoided if possible.

The 6303 code generator will use xgdx. This makes 6303 code incompatible
with the 68HC11. Use 6803 if 68HC11 compatibility is required.

Currently an argument cannot be a register variable. Declaring one register
will ignore the hint.

As the compiler has direct page temporaries and register variables any
re-entrant code for interrupts or signals will need to save these first.

## Calling Conventions

cc68 uses cc65 like conventions for the 6800 processor. Arguments are
stacked left to right, varargs are handled specially and the called function
performs the stack clean up.

For the 6803/6303 processors the same basic rules apply but the caller
performs stack clean up in a more conventional fashion. 6800 and 680x
compiler output are not compatible.

The stack ordering may change if I find a good way to modify the compiler
to stack the arguments in classic C style.

Function returns are in the D register (ie AB on 6800). A 16bit direct
page register called sreg is used for 32bit maths and also to return the
upper half of a 32bit result. All registers are assumed destroyed by the
called function including direct page temporaries. Register variables in
the direct page are saved and restored by the called function if used.

## Unimplemented Functionality

Bitfields have not been tested at all. They may work but if they do it's by
accident.

Floating point is not supported at all by the compiler backend as with cc65.
The front end supports floating point types so this could in theory be added
at least for 32bit float. Doing so would need someone to implement fpadd,
fpsub, fpneg, fpmul, fpmod, fpdiv and maybe a couple of other basic operations
between stack and D + dp registers.

You cannot insert source code as comments into the assembly output for
debugging.

There is no provision to directly generate Fuzix relocatable binaries.

cc68 does not yet support sensible diagnostics for many command line errors
but just reports 'usage...'. This will change once the arguments are a bit
more stable.

The -l option on the front end does not yet resolve short names of libraries
(eg -lc).

## Known Incomplete

The 6800 code generator is still not completed and only minimally debugged.

Optimization is fairly basic at this point.

The Flex support code is just a sketch. Most of the library code is not
heavily tested and some of it is unfinished.

## Install

make; make install

It will by default place itself in /opt/cc68.

No special tools should be needed.

## Usage

cc68 [options] [files]

The file types supported are '.c' - which will be preprocessed and compiled,
'.S' which will be preprocessed and assembled, '.s' which will be assembled,
and '.o' for object files.

Most of the standard C compiler options are supported. In addition the
-M otion generates a map file, and -t name sets the target to this name.
Currently the system supports fuzix, mc10 and flex targets.

When debugging the '-X' option keeps the temporary files. The .@ file is the
compiler output, the '.s' file is the results of the optimizing pass.

## Additional Credits

The cc68 compiler would not be possible without the work of Ullrich von
Bassewitz and the many other contributors to the cc65 code.

The cc68 assembler is derived from the assembler by Mark Williams Co.
released by Robert Swartz. Please see as68/COPYRIGHT.MWC.

The optimizer pass uses copt by Christopher W Fraser.