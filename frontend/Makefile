#
#	Build 6803/6303 version of the tools
#
all: cc68

AOBJ = cc.o

CFLAGS = -DBINPATH=\"/opt/cc68/bin/\" -DLIBPATH=\"/opt/cc68/lib/\" -DINCPATH=\"/opt/cc68/include/\"

cc68: cc.o
	cc -o cc68 cc.o

clean:
	rm -f *.o *~
	rm -f cc68
