
/*
 * Z8 assembler.

 *
 * Tables: FIXME -  we want an address space of 'register' ?
 */

#include	"as.h"

SYM	sym[] = {
	{	0,	"defb",		TDEFB,		XXXX	},
	{	0,	"defw",		TDEFW,		XXXX	},
	{	0,	"defs",		TDEFS,		XXXX	},
	{	0,	"defm",		TDEFM,		XXXX	},
	{	0,	"org",		TORG,		XXXX	},
	{	0,	"equ",		TEQU,		XXXX	},
	{	0,	"export",	TEXPORT,	XXXX	},
	{	0,	".byte",	TDEFB,		XXXX	},
	{	0,	".word",	TDEFW,		XXXX	},
	{	0,	".blkb",	TDEFS,		XXXX	},
	{	0,	".ascii",	TDEFM,		XXXX	},
	{	0,	".org",		TORG,		XXXX	},
	{	0,	".equ",		TEQU,		XXXX	},
	{	0,	".export",	TEXPORT,	XXXX	},
	{	0,	"abs",		TSEGMENT,	ABSOLUTE},
	{	0,	"code",		TSEGMENT,	CODE	},
	{	0,	"data",		TSEGMENT,	DATA	},
	{	0,	"bss",		TSEGMENT,	BSS	},
	{	0,	"zp",		TSEGMENT,	ZP	},
	{	0,	"common",	TSEGMENT,	COMMON	},
	{	0,	"literal",	TSEGMENT,	LITERAL	},
	{	0,	"commondata",	TSEGMENT,	COMMONDATA },
	{	0,	"buffers",	TSEGMENT,	BUFFERS	},
	{	0,	".abs",		TSEGMENT,	ABSOLUTE},
	{	0,	".code",	TSEGMENT,	CODE	},
	{	0,	".data",	TSEGMENT,	DATA	},
	{	0,	".bss",		TSEGMENT,	BSS	},
	{	0,	".zp",		TSEGMENT,	ZP	},
	{	0,	".common",	TSEGMENT,	COMMON	},
	{	0,	".literal",	TSEGMENT,	LITERAL	},
	{	0,	".commondata",	TSEGMENT,	COMMONDATA },
	{	0,	".buffers",	TSEGMENT,	BUFFERS	},
	
	/* The MC96 has a fairly clean instruction set */

	/* Mostly register ops. Shifts take two arguments, a count or reg
	   and a target reg. Reg 00-0F can't be used for a shift count */
	{	0,	"skip",		TIMPL,		0x00	},
	{	0,	"clr",		TREG,		0x01	},
	{	0,	"not",		TREG,		0x02	},
	{	0,	"neg",		TREG,		0x03	},
	{	0,	"xch",		TXCH,		0x04	},
	{	0,	"dec",		TREG,		0x05	},
	{	0,	"ext",		TREGL,		0x06	},
	{	0,	"inc",		TREG,		0x07	},
	{	0,	"shr",		TSH,		0x08	},
	{	0,	"shl",		TSH,		0x09	},
	{	0,	"shra",		TSH,		0x0A	},
	/* 0B is other form of xch */
	{	0,	"shrl",		TSHL,		0x0C	},
	{	0,	"shll",		TSHL,		0x0D	},
	{	0,	"shral",	TSHL,		0x0E	},
	{	0,	"norml",	TNORM,		0x0F	},

	/* 0x10 is not used */
	{	0,	"clrb",		TREGB,		0x11	},
	{	0,	"notb",		TREGB,		0x12	},
	{	0,	"negb",		TREGB,		0x13	},
	{	0,	"xch",		TXCHB,		0x14	},
	{	0,	"decb",		TREGB,		0x15	},
	{	0,	"extb",		TREG,		0x16	},
	{	0,	"incb",		TREGB,		0x17	},
	{	0,	"shrb",		TSHB,		0x18	},
	{	0,	"shlb",		TSHB,		0x19	},
	{	0,	"shrab",	TSHB,		0x1A	},
	/* 1B is other form of xch */
/*	{	0,	"est",		TEST,		0x1C	},
	{	0,	"estb",		TESTB,		0x1E	},*/

	/* Short calls - 11bit */
	{	0,	"sjmp",		TSHORT,		0x20	},	
	{	0,	"scall",	TSHORT,		0x28	},	
	/* Bitops */
	{	0,	"jbc",		TJBIT,		0x30	},
	{	0,	"jbs",		TJBIT,		0x38	},
	/* Base operation set */	
	{	0,	"and",		TOP23,		0x4060	},
	{	0,	"add",		TOP23,		0x4464	},
	{	0,	"sub",		TOP23,		0x4868	},
	{	0,	"andb",		TOP23B,		0x5070	},
	{	0,	"addb",		TOP23B,		0x5474	},
	{	0,	"subb",		TOP23B,		0x5878	},
	{	0,	"or",		TWOOP,		0x80	},
	{	0,	"xor",		TWOOP,		0x84	},
	{	0,	"cmp",		TWOOP,		0x88	},
	/* 8C is mul */
	{	0,	"orb",		TWOOPB,		0x90	},
	{	0,	"xorb",		TWOOPB,		0x94	},
	{	0,	"cmpb",		TWOOPB,		0x98	},
	/* 9C is div */
	{	0,	"ld",		TWOOP,		0xA0	},
	{	0,	"addc",		TWOOPB,		0xA4	},
	{	0,	"subc",		TWOOPB,		0xA8	},
	{	0,	"ldbze",	TWOOPB,		0xAC	},
	{	0,	"ldb",		TWOOPB,		0xB0	},
	{	0,	"addcb",	TWOOPB,		0xB4	},
	{	0,	"subcb",	TWOOPB,		0xB8	},
	{	0,	"ldbse",	TWOOPB,		0xBC	},
	/* Store - basically the same but no immediate form */
	{	0,	"st",		TSTORE,		0xC0	},
	/* Bmov uses the missing store immediate */
	{	0,	"bmov",		TBMOV,		0xC1	},
	{	0,	"stb",		TSTOREB,	0xC4	},
	/* cmpl uses the missing store immediate */
	{	0,	"cmpl",		TWOOPL,		0xC1	},

	/* Single argument */
	{	0,	"push",		TONE,		0xC8	},
	{	0,	"pop",		TDONE,		0xCC	},

	/* Conditional Branches */
	{	0,	"jnst",		TJCC,		0xD0	},
	{	0,	"jnh",		TJCC,		0xD1	},
	{	0,	"jgt",		TJCC,		0xD2	},
	{	0,	"jnc",		TJCC,		0xD3	},
	{	0,	"jnvt",		TJCC,		0xD4	},
	{	0,	"jnv",		TJCC,		0xD5	},
	{	0,	"jge",		TJCC,		0xD6	},
	{	0,	"jne",		TJCC,		0xD7	},
	{	0,	"jst",		TJCC,		0xD8	},
	{	0,	"jh",		TJCC,		0xD9	},
	{	0,	"jle",		TJCC,		0xDA	},
	{	0,	"jc",		TJCC,		0xDB	},
	{	0,	"jvt",		TJCC,		0xDC	},
	{	0,	"jv",		TJCC,		0xDD	},
	{	0,	"jlt",		TJCC,		0xDE	},
	{	0,	"je",		TJCC,		0xDF	},

	/* Lot of miscellanous */
	{	0,	"djnz",		TDJNZ,		0xE0	},
	{	0,	"djnzw",	TDJNZW,		0xE1	},
	{	0,	"tijmp",	TIMM16,		0xE2	},
	/* br [xxx] is a special case indirect only */
	{	0,	"br",		TBR,		0xE3	},
/*	{	0,	"ebr",		TEBR,		0xE3	},
	{	0,	"ebmovi",	TEBMOVI,	0xE4	},
	{	0,	"ejmp",		TIMM24,		0xE6	}, */
	{	0,	"ljmp",		TIMM16,		0xE7	},
/*	{	0,	"eld",		TELD,		0xE8	},
	{	0,	"eldb",		TELDB,		0xEA	},
	{	0,	"dpts",		TDPTS,		0xEC	},
	{	0,	"epts",		TEPTS,		0xED	}, */
	/* EE is reserved no-op */
	{	0,	"lcall",	TIMM16,		0xEF	},	

	/* Implicit */
	{	0,	"ret",		TIMPL,		0xF0	},
/*	{	0,	"ecall",	TECALL,		0xF1	}, */
	{	0,	"pushf",	TIMPL,		0xF2	},
	{	0,	"popf",		TIMPL,		0xF3	},
	{	0,	"pusha",	TIMPL,		0xF4	},
	{	0,	"popa",		TIMPL,		0xF5	},
	{	0,	"idlpd",	TIMPL,		0xF6	},
	{	0,	"trap",		TIMPL,		0xF7	},
	{	0,	"clrc",		TIMPL,		0xF8	},
	{	0,	"setc",		TIMPL,		0xF9	},
	{	0,	"di",		TIMPL,		0xFA	},
	{	0,	"ei",		TIMPL,		0xFB	},
	{	0,	"clrvt",	TIMPL,		0xFC	},
	{	0,	"nop",		TIMPL,		0xFD	},
	/* FE is prefix for mul/div forms */
	{	0,	"rst",		TIMPL,		0xFF	},

	/* Mul and div - have alignment rules and 2/3 op forms for mul */
	{	0,	"mul",		TMUL,		0xFE4C	},
	{	0,	"mulu",		TMUL,		0x4C	},
	{	0,	"div",		TDIV,		0xFE8C	},
	{	0,	"divu",		TDIV,		0x8C	},
	{	0,	"mulb",		TMULB,		0xFE5C	},
	{	0,	"mulub",	TMULB,		0x5C	},
	{	0,	"divb",		TDIVB,		0xFE9C	},
	{	0,	"divub",	TDIVB,		0x9C	},
};

        
/*
 * Set up the symbol table.
 * Sweep through the initializations
 * of the "phash", and link them into the
 * buckets. Because it is here, a
 * "sizeof" works.
 */
void syminit(void)
{
	SYM *sp;
	int hash;

	sp = &sym[0];
	while (sp < &sym[sizeof(sym)/sizeof(SYM)]) {
		hash = symhash(sp->s_id);
		sp->s_fp = phash[hash];
		phash[hash] = sp;
		++sp;
	}
}

char *etext[] = {
	"unexpected character",		/* 10 */
	"phase error",			/* 11 */
	"multiple definitions",		/* 12 */
	"syntax error",			/* 13 */
	"must be absolute",		/* 14 */
	"missing delimiter",		/* 15 */
	"invalid constant",		/* 16 */
	"Bcc out of range",		/* 17 */
	"register 0-15 required",	/* 18 */
	"address required",		/* 19 */
	"invalid ID",			/* 20 */
	"invalid instruction form",	/* 21 */
	"divide by 0",			/* 22 */
	"constant out of range",	/* 23 */
	"data in BSS",			/* 24 */
	"segment overflow",		/* 25 */
	"data in ZP",			/* 26 */
	"segment conflict",		/* 27 */
	"register must be even"		/* 28 */
};

/*
 * Make sure that the
 * mode and register fields of
 * the type of the "ADDR" pointed to
 * by "ap" can participate in an addition
 * or a subtraction.
 */
void isokaors(ADDR *ap, int paren)
{
	int mode;

	mode = ap->a_type&TMMODE;
	if (mode == TUSER)
		return;
	aerr(ADDR_REQUIRED);
}
