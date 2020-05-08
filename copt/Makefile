OBJS = copt.o
REGEX_OBJS = regex/regcomp.o  regex/regerror.o regex/regexec.o  regex/regfree.o

CFLAGS += -std=gnu99 -DLOCAL_REGEXP -I.

OBJS += $(REGEX_OBJS)

all: copt

copt:	$(OBJS)
	$(CC) -o copt $(LDFLAGS) $(OBJS)

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $(LOCAL_CFLAGS) $(INCLUDES) $^

clean:
	rm -f $(OBJS) copt *~ */*~


#Dependencies

copt.o: copt.c 
