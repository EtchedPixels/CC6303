OBJ = asrax.o asreax1.o asreax8.o
OBJ += bneg.o directpage.o jumpx.o 
OBJ += laddeq.o ladd.o land.o lbneg.o lcmp.o
OBJ += leq.o lge.o lgt.o lle.o llt.o lne.o lor.o lsave.o
OBJ += lsubeq.o ltest.o luge.o lugt.o lule.o lult.o lxor.o 
OBJ += makebool.o mulax3.o mulax10.o strlen.o sub.o swap.o
OBJ += shr.o shrax.o shreax1.o shreax8.o
OBJ += tosasreax.o tosdivax.o tosshlax.o tossubeax.o tosudivax.o tosumulax.o
OBJ += divide.o

CRT = crt0.o

all: $(CRT) libc.a

libc.a: $(OBJ)
	ar rc libc.a $(OBJ)

%.o: %.s
	../as68/as68 $^


clean:
	rm -f $(OBJ) $(CRT) libc.a *~

	