OBJ = 
CRT = crt0.o

all: libc.a $(CRT)

libc.a: $(OBJ)
ifneq ($(OBJ),)
	ar rc libc.a $(OBJ)
else
	touch libc.a
endif

%.o: %.s
	../as68/as68 $^

clean:
	rm -f $(OBJ) $(CRT) libc.a *~

	