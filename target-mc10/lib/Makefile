OBJ = mc10.o

all: libmc10.a crt0_mc10.o

libmc10.a: $(OBJ)
	ar rc libmc10.a $(OBJ)

%.o: %.s
	../../as68/as68 $^

clean:
	rm -f $(OBJ) *~ libmc10.a crt0_mc10.o
