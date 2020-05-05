OBJ =  asr.o asrax.o asreax.o
OBJ += bneg.o compleax.o directpage.o jumpx.o pop2.o pop4.o
OBJ += land.o lbneg.o
OBJ += leq.o lge.o lgt.o lle.o llt.o lne.o lor.o lsave.o
OBJ += lsubeq.o luge.o lugt.o lule.o lult.o lxor.o 
OBJ += makebool.o mulax3.o mulax5.o mulax6.o mulax9.o mulax10.o mulax12.o
OBJ += negeax.o sub.o
OBJ += shlax.o shleax.o shr.o shrax.o shreax.o shreax8.o swap.o
OBJ += tosasleax.o
OBJ += tosasreax.o tosdivax.o tosshlax.o tossubeax.o tosudivax.o tosumodax.o
OBJ += tosudiveax.o tosmodeax.o tosdiveax.o
OBJ += divide.o divide32x32.o
OBJ += fixfp.o

OBJ += __cpu_to_le16.o __cpu_to_le32.o
OBJ += _memcpy.o _memset.o _strlen.o _strcpy.o
OBJ += _isalnum.o _isalpha.o _isascii.o _isblank.o _iscntrl.o
OBJ += _isdigit.o _isgraph.o _islower.o _isprint.o _ispunct.o
OBJ += _isspace.o _isupper.o _isxdigit.o
OBJ += _tolower.o _toupper.o
OBJ += _longjmp.o _setjmp.o

all: $(OBJ)

%.o: %.s
	../as68/as68 $^

clean:
	rm -f $(OBJ) *~

	