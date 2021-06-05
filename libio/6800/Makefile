OBJ = _fputs.o _getchar.o _putchar.o _puts.o

all: libio6800.a

libio6800.a: $(OBJ)
	ar rc libio6800.a $(OBJ)

%.o: %.s
	../../as68/as68 $^

clean:
	rm -f $(OBJ) *~ libio6800.a
