OBJ = asreax8.o bneg.o compleax.o __cpu_to_le16.o __cpu_to_le32.o
OBJ += divide32x32.o laddeq.o ladd.o land.o lbneg.o lcmp.o lor.o
OBJ += lsave.o lsubeq.o ltest.o lxor.o makebool.o _memcpy.o _memset.o
OBJ += mulax10.o mulax12.o mulax3.o mulax5.o mulax7.o mulax9.o
OBJ += multiply32x32.o negeax.o pop4.o
OBJ += shleax.o shrax.o shr.o _strcpy.o sub.o swap.o
OBJ += tosasreax.o tosdivax.o tosshlax.o tossubeax.o tosudivax.o tosumulax.o

CRT = crt0.o

all: $(CRT) lib6803.a

lib6803.a: $(OBJ)
	ar rc lib6803.a $(OBJ)

%.o: %.s
	../as68/as68 $^


clean:
	rm -f $(OBJ) $(CRT) lib6803.a *~

	