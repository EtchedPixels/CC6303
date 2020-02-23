OBJ = strlen.o
CRT = crt0.o

all: $(CRT) libc.a

libc.a: $(OBJ)
	ar rc libc.a $(OBJ)

%.o: %.s
	../as68/as68 $^


clean:
	rm -f $(OBJ) $(CRT) libc.a *~

	