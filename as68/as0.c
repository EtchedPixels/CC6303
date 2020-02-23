/*
 * Assembler.
 * Command line processing
 * and main driver.
 *
 * FIXME: normal Unix as option parsing.
 */
#include	"as.h"

FILE	*ifp;
FILE	*ofp;
FILE	*lfp;
char	eb[NERR];
char	ib[NINPUT];
char	*cp;
char	*ep;
char	*ip;
char	*fname;
VALUE	dot[NSEGMENT];
int	segment;
SYM	*phash[NHASH];
SYM	*uhash[NHASH];
int	pass;
int	line;
jmp_buf	env;
int	debug_write = 1 ;
int	noobj;
int	cpu_flags = ARCH_CPUFLAGS;

int main(int argc, char *argv[])
{
	char *ifn;
	char *ofn;
	char *p;
	int i;
	int c;

	/* FIXME: switch to getopt - add map, listing */
	ifn = NULL;
	for (i=1; i<argc; ++i) {
		p = argv[i];
		if (*p == '-') {
			while ((c = *++p) != 0) {
				switch (c) {
				/* FIXME: move this to z80 .setcpu */
				case '1':
					cpu_flags |= OA_8080_Z180;
					break;
				default:
					fprintf(stderr, "Bad option %c\n", c);
					exit(BAD);
				}
			}
		} else if (ifn == NULL)
			ifn = p;
		else {
			fprintf(stderr, "Too many source files\n");
			exit(BAD);
		}
	}
	if (ifn == NULL) {
		fprintf(stderr, "No source file\n");
		exit(BAD);
	}

	if ((ifp=fopen(ifn, "r")) == NULL) {
		fprintf(stderr, "%s: cannot open\n", ifn);
		exit(BAD);
	}

	ofn = strdup(ifn);
	if (ofn == NULL) {
		fprintf(stderr, "%s: out of memory.\n", ifn);
		exit(BAD);
	}
	p = strrchr(ofn, '.');
	if (p == NULL || p[1] == 0) {
		fprintf(stderr, "%s: expected extensions.\n", ifn);
		exit(BAD);
	}

	p[1] = 'o';
	p[2] = '\0';

	if ((ofp=fopen(ofn, "w")) == NULL) {
		fprintf(stderr, "%s: cannot create.\n", ofn);
		exit(BAD);
	}
	syminit();
	fname = ifn;
	for (pass=0; pass<3; ++pass) {
		outpass();
		line = 0;
		memset(dot, 0, sizeof(dot));
		fseek(ifp, 0L, 0);
		while (fgets(ib, NINPUT, ifp) != NULL) {
			++line;
			ep = &eb[0];
			ip = &ib[0];
			if (setjmp(env) == 0)
				asmline();
		}
	}
	pass = 2;
	outeof();
	/* Return an error code if no object was created */
	exit(noobj);
}

