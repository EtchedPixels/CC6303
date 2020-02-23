OBJ = bneg.o jumpx.o laddeq.o ladd.o land.o lcmp.o leq.o lge.o lgt.o lle.o llt.o lne.o lor.o lsubeq.o luge.o lugt.o lule.o lult.o lxor.o makebool.o strlen.o swap.o

CRT = crt0.o

all: $(CRT) libc.a

libc.a: $(OBJ)
	ar rc libc.a $(OBJ)

%.o: %.s
	../as68/as68 $^


clean:
	rm -f $(OBJ) $(CRT) libc.a *~

	