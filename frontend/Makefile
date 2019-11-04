#
#	Build 6803/6303 version of the tools
#
all: cc6303

AOBJ = cc.o

CFLAGS = -DBINPATH=\"/opt/fuzix/cc6303/bin/\" -DLIBPATH=\"/opt/fuzix/cc6303/lib/\"

cc6303: cc.o
	cc -o cc6303 cc.o

clean:
	rm -f *.o *~
	rm -f cc6303
