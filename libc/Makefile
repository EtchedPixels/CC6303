OBJ = asrax.o asreax1.o asreax8.o
OBJ += bneg.o directpage.o jumpx.o 
OBJ += laddeq.o ladd.o land.o lbneg.o lcmp.o
OBJ += leq.o lge.o lgt.o lle.o llt.o lne.o lor.o lsave.o
OBJ += lsubeq.o ltest.o luge.o lugt.o lule.o lult.o lxor.o 
OBJ += makebool.o mulax3.o mulax10.o pop4.o sub.o
OBJ += shr.o shrax.o shreax1.o shreax8.o swap.o
OBJ += tosasreax.o tosdivax.o tosshlax.o tossubeax.o tosudivax.o tosumodax.o
OBJ += tosumulax.o
OBJ += divide.o

OBJ += __cpu_to_le16.o __cpu_to_le32.o
OBJ += _memcpy.o _memset.o _strlen.o _strcpy.o

CRT = crt0.o

all: $(CRT) libc.a

libc.a: $(OBJ)
	ar rc libc.a $(OBJ)

%.o: %.s
	../as68/as68 $^


clean:
	rm -f $(OBJ) $(CRT) libc.a *~

	