OBJ = asrax.o asreax.o
OBJ += bneg.o compleax.o directpage.o jumpx.o pop4.o
OBJ += land.o lbneg.o
OBJ += leq.o lge.o lgt.o lle.o llt.o lne.o lor.o lsave.o
OBJ += lsubeq.o luge.o lugt.o lule.o lult.o lxor.o 
OBJ += makebool.o mulax3.o mulax5.o mulax9.o mulax10.o mulax12.o
OBJ += sub.o
OBJ += shleax.o shr.o shrax.o shreax.o shreax8.o swap.o
OBJ += tosasreax.o tosdivax.o tosshlax.o tossubeax.o tosudivax.o tosumodax.o
OBJ += tosumuleax.o
OBJ += divide32x32.o

OBJ += __cpu_to_le16.o __cpu_to_le32.o
OBJ += _memcpy.o _memset.o _strlen.o _strcpy.o

CRT = crt0.o

all: $(CRT) libc.a

libc.a: $(OBJ)
	ar rc lib6800.a $(OBJ)

%.o: %.s
	../as68/as68 $^


clean:
	rm -f $(OBJ) $(CRT) lib6800.a *~

	