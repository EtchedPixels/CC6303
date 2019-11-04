#
#	Build 6803/6303 version of the tools
#
all: as6303 ld6303 nm6303

HDR = as.h ld.h obj.h

AOBJ = as0.o as1-6303.o as2.o as3.o as4.o as6-6303.o

CFLAGS = -DTARGET_6303

as6303: $(HDR) $(AOBJ)
	cc -o as6303 $(AOBJ)

ld6303: $(HDR) ld.o
	cc -o ld6303 ld.o

nm6303: $(HDR) nm.o
	cc -o nm6303 nm.o

clean:
	rm -f *.o *~
	rm -f nm6303 ld6303 as6303
