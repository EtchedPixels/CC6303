/*
 *	Warrex Centurion
 */

#include	"as.h"

SYM	sym[] = {
	{	0,	"a",		TWR,		RA	},
	{	0,	"b",		TWR,		RB	},
	{	0,	"x",		TWR,		RX	},
	{	0,	"y",		TWR,		RY	},
	{	0,	"z",		TWR,		RZ	},
	{	0,	"s",		TWR,		RS	},
	/* We don't really understand what these two are about */
	{	0,	"g",		TWR,		RG	},
	{	0,	"h",		TWR,		RH	},

	{	0,	"al",		TBR,		RAL	},
	{	0,	"ah",		TBR,		RAH	},
	{	0,	"bl",		TBR,		RBL	},
	{	0,	"bh",		TBR,		RBH	},
	{	0,	"xl",		TBR,		RXL	},
	{	0,	"xh",		TBR,		RXH	},
	{	0,	"yl",		TBR,		RYL	},
	{	0,	"yh",		TBR,		RYH	},
	{	0,	"zl",		TBR,		RZL	},
	{	0,	"zh",		TBR,		RZH	},
	{	0,	"gl",		TBR,		RGL	},
	{	0,	"gh",		TBR,		RGH	},
	{	0,	"hl",		TBR,		RHL	},
	{	0,	"hh",		TBR,		RHH	},

	{	0,	"pc",		TSR,		RPC	},

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
	{	0,	".ds",		TDEFS,		XXXX	},
	{	0,	".ascii",	TDEFM,		XXXX	},
	{	0,	".org",		TORG,		XXXX	},
	{	0,	".equ",		TEQU,		XXXX	},
	{	0,	".export",	TEXPORT,	XXXX	},
	{	0,	"abs",		TSEGMENT,	ABSOLUTE},
	{	0,	"code",		TSEGMENT,	CODE	},
	{	0,	"data",		TSEGMENT,	DATA	},
	{	0,	"bss",		TSEGMENT,	BSS	},
	{	0,	"discard",	TSEGMENT,	DISCARD	},
	{	0,	"common",	TSEGMENT,	COMMON	},
	{	0,	".abs",		TSEGMENT,	ABSOLUTE},
	{	0,	".code",	TSEGMENT,	CODE	},
	{	0,	".data",	TSEGMENT,	DATA	},
	{	0,	".bss",		TSEGMENT,	BSS	},
	{	0,	".discard",	TSEGMENT,	DISCARD	},
	{	0,	".common",	TSEGMENT,	COMMON	},
	{	0,	".literal",	TSEGMENT,	LITERAL	},

	/* 0x0X		:	Implicit */
	{	0,	"halt",		TIMPL,		0x00	},
	{	0,	"nop",		TIMPL,		0x01	},
	{	0,	"fsn",		TIMPL,		0x02	},
	{	0,	"fcn",		TIMPL,		0x03	},
	{	0,	"fsi",		TIMPL,		0x04	},
	{	0,	"fci",		TIMPL,		0x05	},
	{	0,	"fsc",		TIMPL,		0x06	},
	{	0,	"fcc",		TIMPL,		0x07	},
	{	0,	"fca",		TIMPL,		0x08	},
	{	0,	"ret",		TIMPL,		0x09	},
	{	0,	"iretc",	TIMPL,		0x0A	},
	{	0,	"delay",	TIMPL,		0x0E	},
	{	0,	"sysret",	TIMPL,		0x0F	},

	/* 0x1X		:	Branches */
	{	0,	"bc",		TREL8,		0x10	},
	{	0,	"bcc",		TREL8,		0x11	},
	{	0,	"bn",		TREL8,		0x12	},
	{	0,	"bnc",		TREL8,		0x13	},
	{	0,	"bz",		TREL8,		0x14	},
	{	0,	"bnz",		TREL8,		0x15	},
	{	0,	"blt",		TREL8,		0x16	},
	{	0,	"bge",		TREL8,		0x17	},
	{	0,	"ble",		TREL8,		0x18	},
	{	0,	"bgt",		TREL8,		0x19	},
	{	0,	"bs1",		TREL8,		0x1A	},
	{	0,	"bs2",		TREL8,		0x1B	},
	{	0,	"bs3",		TREL8,		0x1C	},
	{	0,	"bs4",		TREL8,		0x1D	},
	
	/* Special magic forms : these will be smart one day */
	{	0,	"jc",		TBRA16,		0x10	},
	{	0,	"jcc",		TBRA16,		0x11	},
	{	0,	"jn",		TBRA16,		0x12	},
	{	0,	"jnc",		TBRA16,		0x13	},
	{	0,	"jz",		TBRA16,		0x14	},
	{	0,	"jnz",		TBRA16,		0x15	},
	{	0,	"jlt",		TBRA16,		0x16	},
	{	0,	"jge",		TBRA16,		0x17	},
	{	0,	"jle",		TBRA16,		0x18	},
	{	0,	"jgt",		TBRA16,		0x19	},
	{	0,	"js1",		TBRA16,		0x1A	},
	{	0,	"js2",		TBRA16,		0x1B	},
	{	0,	"js3",		TBRA16,		0x1C	},
	{	0,	"js4",		TBRA16,		0x1D	},
	

	/* 0x2X		:	Mix of ALU and misc */
	/* Short forms apply to AL, long forms any register */
	/* 0x3x versions are word and apply to A, long to any register */

	{	0,	"inc",		TREGA,		0x20	},
	{	0,	"dec",		TREGA,		0x21	},
	{	0,	"clr",		TREGA,		0x22	},
	{	0,	"not",		TREGA,		0x23	},
	{	0,	"srl",		TREGA,		0x24	},
	{	0,	"sll",		TREGA,		0x25	},
	{	0,	"rrc",		TREG,		0x26	},
	{	0,	"rlc",		TREG,		0x27	},

	/* 0x2E/F	:	Special */

	{	0,	"mov",		TMOVE,		0x00	},
	{	0,	"mmu0",		TMMU,		0x200C  },
	{	0,	"mmu1",		TMMU,		0x201C	},
	{	0,	"rdmaaddr",	TDMA,		0x2F00	},
	{	0,	"sdmaaddr",	TDMA,		0x2F01	},
	{	0,	"rdmalen",	TDMA,		0x2F02	},
	{	0,	"sdmalen",	TDMA,		0x2F03	},
	{	0,	"dmamode",	TDMAM,		0x2F04	},
	{	0,	"dmaen",	TIMPL,		0x2F06	},
	
	/* 0x4X		:	ALU operations, short are AL,BL */
	/* 0x5X		:	ALU operations, short are AX,BX */

	{	0,	"add",		TREG2A,		0x40	},
	{	0,	"sub",		TREG2A,		0x41	},
	{	0,	"and",		TREG2A,		0x42	},
	/* These forms have no short word version */
	{	0,	"or",		TREG2ANWS,	0x43	},
	{	0,	"xor",		TREG2ANWS,	0x44	},
	/* Move handled elsewhere */

	/* 0x6X		; 	Entirely word moves with X	*/

	/* 0x7X		:	Calls and other */
	{	0,	"jump",		TJUMP,		0x70	},
	{	0,	"call",		TJUMP,		0x78	},
	{	0,	"syscall",	TIMPL,		0x76	},
	
	/* 0x8x-0xFF	:	A and B load/store */

	{	0,	"ld",		TLOAD,		0x00	},
	{	0,	"st",		TSTORE,		0x00	},
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
	"index out of range",		/* 18 */
	"address required",		/* 19 */
	"invalid ID",			/* 20 */
	"bad addressing mode",		/* 21 */
	"divide by 0",			/* 22 */
	"constant out of range",	/* 23 */
	"data in BSS",			/* 24 */
	"segment overflow",		/* 25 */
	"data in direct page",		/* 26 */
	"segment conflict",		/* 27 */
	"too many Jcc instructions",	/* 28 */
	"register required",		/* 29 */
	"word register required",	/* 30 */
	"byte register required",	/* 31 */
	"AL or BL only",		/* 32 */
	"A B or X only",		/* 33 */
	"Can't indirect",		/* 34 */
	"Invalid addressing mode",	/* 35 */
	"Out of range",			/* 36 */
	"A register only",		/* 37 */
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
