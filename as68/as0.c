/*
 * Assembler.
 * Command line processing
 * and main driver.
 *
 * FIXME: normal Unix as option parsing.
 */
#include	"as.h"

#include <unistd.h>

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

static void usage(void)
{
	fprintf(stderr, "as [-o object.o] source.s.\n");
	exit(1);
}

static void oom(void)
{
	fprintf(stderr, "Out of memory.\n");
	exit(1);
}

static char *xstrdup(const char *p)
{
	char *n = strdup(p);
	if (!n)
		oom();
	return n;
}

int main(int argc, char *argv[])
{
	char *ifn;
	char *ofn = NULL;
	char *p, *e;
	int i;
	int c;

	int opt;

	/* Lots of options need adding yet */
	while ((opt = getopt(argc, argv, "o:")) != -1) {
		switch (opt) {
		case 'o':
			ofn = optarg;
			break;
		default:
			usage();
			break;
		}
	}
	if (optind != argc - 1)
		usage();
	ifn = argv[optind];

	if ((ifp=fopen(ifn, "r")) == NULL) {
		fprintf(stderr, "%s: cannot open\n", ifn);
		exit(BAD);
	}

	if (ofn == NULL) {
		ofn = xstrdup(ifn);
		p = strrchr(ofn, '.');
		if (p == NULL || p[1] == 0) {
			fprintf(stderr, "%s: expected extensions.\n", ifn);
			exit(BAD);
		}
		p[1] = 'o';
		p[2] = '\0';
	}

	if ((ofp=fopen(ofn, "w")) == NULL) {
		fprintf(stderr, "%s: cannot create.\n", ofn);
		exit(BAD);
	}
	syminit();
	fname = xstrdup(ifn);
	for (pass=0; pass<4; ++pass) {
		/* FIXME: for some platforms we should just do 0 and 3 */
		outpass();
		line = 1;
		memset(dot, 0, sizeof(dot));
		fseek(ifp, 0L, 0);
		while (fgets(ib, NINPUT, ifp) != NULL) {
			/* Pre-processor output */
			if (*ib == '#' && ib[1] == ' ') {
				free(fname);
				line = strtoul(ib +2, &p, 10);
				p++;
				e = strrchr(p , '"');
				if (e)
					*e = 0;
				fname = xstrdup(p);
			/* Normal assembly */
			} else {
				ep = &eb[0];
				ip = &ib[0];
				if (setjmp(env) == 0)
					asmline();
				++line;
			}
		}
		/* Don't continue once we know we failed */
		if (noobj)
			break;
	}
	if (!noobj) {
		pass = 3;
		outeof();
	} else {
		if (unlink(ofn))
			perror(ofn);
	}
	/* Return an error code if no object was created */
	exit(noobj);
}

