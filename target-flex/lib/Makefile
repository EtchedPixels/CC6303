OBJ = flexcon.o fms.o

OBJ += errno.o fclose.o fgetc.o fopen.o fputc.o fputs.o
OBJ += rewind.o unlink.o
OBJ += opendir.o

all: libflex.a

libflex.a: $(OBJ)
	ar rc libflex.a $(OBJ)

%.o: %.s
	../../as68/as68 $^

clean:
	rm -f $(OBJ) *~