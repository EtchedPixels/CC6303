OBJ = asrax.o asreax.o asreax8.o
OBJ += bneg.o compleax.o directpage.o jumpx.o 
OBJ += laddeq.o ladd.o land.o lbneg.o lcmp.o
OBJ += leq.o lge.o lgt.o lle.o llt.o lne.o lor.o lsave.o
OBJ += lsubeq.o ltest.o luge.o lugt.o lule.o lult.o lxor.o 
OBJ += makebool.o mulax3.o mulax5.o mulax7.o mulax9.o mulax10.o mulax12.o
OBJ += negeax.o pop4.o sub.o
OBJ += shleax.o shr.o shrax.o shreax.o shreax8.o swap.o
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

	