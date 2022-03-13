/*
 * TMS9995 assembler.
 *
 * Tables
 */

#include	"as.h"

SYM	sym[] = {
	{	0,	"r0",		TWR,		0	},
	{	0,	"r1",		TWR,		1	},
	{	0,	"r2",		TWR,		2	},
	{	0,	"r3",		TWR,		3	},
	{	0,	"r4",		TWR,		4	},
	{	0,	"r5",		TWR,		5	},
	{	0,	"r6",		TWR,		6	},
	{	0,	"r7",		TWR,		7	},
	{	0,	"r8",		TWR,		8	},
	{	0,	"r9",		TWR,		9	},
	{	0,	"r10",		TWR,		10	},
	{	0,	"r11",		TWR,		11	},
	{	0,	"r12",		TWR,		12	},
	{	0,	"r13",		TWR,		13	},
	{	0,	"r14",		TWR,		14	},
	{	0,	"r15",		TWR,		15	},

	{	0,	"defb",		TDEFB,		XXXX	},
	{	0,	"defw",		TDEFW,		XXXX	},
	{	0,	"defs",		TDEFS,		XXXX	},
	{	0,	"db",		TDEFB,		XXXX	},
	{	0,	"dw",		TDEFW,		XXXX	},
	{	0,	"ds",		TDEFS,		XXXX	},
	{	0,	"defm",		TDEFM,		XXXX	},
	{	0,	"org",		TORG,		XXXX	},
	{	0,	"equ",		TEQU,		XXXX	},
	{	0,	"export",	TEXPORT,	XXXX	},
	{	0,	".byte",	TDEFB,		XXXX	},
	{	0,	".word",	TDEFW,		XXXX	},
	{	0,	".ds",		TDEFS,		XXXX	},
	{	0,	".ascii",	TDEFM,		XXXX	},
	{	0,	".org",		TORG,		XXXX	},
	{	0,	".equ",		TEQU,		XXXX	},
	{	0,	".export",	TEXPORT,	XXXX	},
	{	0,	".abs",		TSEGMENT,	ABSOLUTE},
	{	0,	".code",	TSEGMENT,	CODE	},
	{	0,	".data",	TSEGMENT,	DATA	},
	{	0,	".bss",		TSEGMENT,	BSS	},
	{	0,	".zp",		TSEGMENT,	ZP	},

	/* Instructions by format */

	/* 4.5.1 "Dual operand, multiple addressing for source/dest" */

        /* 4bits of op code, then two mode/reg pairs */	
	{	0,	"a",		TDOMA,		0xC000	},
	{	0,	"ab",		TDOMA,		0xD000	},
	{	0,	"c",		TDOMA,		0x8000  },
	{	0,	"cb",		TDOMA,		0x9000  },
	{	0,	"s",		TDOMA,		0x6000	},
	{	0,	"sb",		TDOMA,		0x7000	},
	{	0,	"soc",		TDOMA,		0xE000	},
	{	0,	"socb",		TDOMA,		0xF000	},
	{	0,	"szc",		TDOMA,		0x4000  },
	{	0,	"szcb",		TDOMA,		0x5000	},
	{	0,	"mov",		TDOMA,		0xC000	},
	{	0,	"movb",		TDOMA,		0xD000	},
	
	/* 4.5.2 "Dual operand, multiple addressing modes, for source and
	        workspace for dest */

        /* 6bits of opcode, a WP reg nibble and mode/reg pair */
        {	0,	"coc",		TDOMAW,		0x2000	},
        {	0,	"czc",		TDOMAW,		0x2400  },
        {	0,	"xor",		TDOMAW,		0x2800	},
        {	0,	"mpy",		TDOMAW,		0x3800  },
        {	0,	"div",		TDOMAW,		0x3C00	},
        
        /* 4.5.3 signed multiply and divide */

        /* 10 bits of opcodee and a mode/reg pair */        
        {	0,	"mpys",		TSMD,		0x08C0	},
        {	0,	"divs",		TSMD,		0x0880	},
        
        /* 4.5.4 XOP */
        
        /* 6 bits of opcode, an op code nibble and a mode/reg pair */
        {	0,	"xop",		TXOP,		0x2C00	},

        /* 4.5.5 Single operand */
        
        /* 10 bits of opcode and a mode/reg pair */
        {	0,	"b",		TSOP,		0x0440	},
        {	0,	"bl",		TSOP,		0x0680  },
        {	0,	"blwp",		TSOP,		0x0400	},
        {	0,	"clr",		TSOP,		0x04C0	},
        {	0,	"seto",		TSOP,		0x0700	},
        {	0,	"inv",		TSOP,		0x0540	},
        {	0,	"neg",		TSOP,		0x0500	},
        {	0,	"abs",		TSOP,		0x0740	},
        {	0,	"swpb",		TSOP,		0x06C0	},
        {	0,	"inc",		TSOP,		0x0580	},
        {	0,	"inct",		TSOP,		0x05C0	},
        {	0,	"dec",		TSOP,		0x0600	},
        {	0,	"dect",		TSOP,		0x0640	},
        {	0,	"x",		TSOP,		0x0480	},

        /* 4.5.6 CRU multi-bit */

        /* 6 bits of opcode, 4 bits of field width, and a mode/reg pair */
        {	0,	"ldcr",		TCRUM,		0x3000	},
        {	0,	"stcr",		TCRUM,		0x3400	},

        /* 4.5.7 CRU single-bit */

        /* 8 bits of opcode and an 8 bit signed displacement */
        {	0,	"sbo",		TCRUS,		0x1D00	},
        {	0,	"sbz",		TCRUS,		0x1E00	},
        {	0,	"tb",		TCRUS,		0x1F00	},

        /* 4.5.8 JUMP instructions */

        /* 8 bits of opcode and an 8 bit signed displacement from word
           following jump */
        {	0,	"jeq",		TJUMP,		0x1300	},
        {	0,	"jgt",		TJUMP,		0x1500	},
        {	0,	"jh", 		TJUMP,		0x1B00	},
        {	0,	"jhe",		TJUMP,		0x1400	},
        {	0,	"jl",		TJUMP,		0x1A00	},
        {	0,	"jle",		TJUMP,		0x1200	},
        {	0,	"jlt",		TJUMP,		0x1100	},
        {	0,	"jmp",		TJUMP,		0x1000	},
        {	0,	"jnc",		TJUMP,		0x1700	},
        {	0,	"jne",		TJUMP,		0x1600	},
        {	0,	"jno",		TJUMP,		0x1900	},
        {	0,	"joc",		TJUMP,		0x1800	},
        {	0,	"jop",		TJUMP,		0x1C00	},

        /* FIXME: consider adding pseudo Bxx ops that do self sizing ? */

        /* 4.5.9 Shift instructions */

        /* 8bits of opcode, 4 bits of shift count (0 uses WR0) WR0 = 0
           is a 16bit shift...., 4 bits of working register */       
        {	0,	"sla",		TSHIFT,		0x0A00	},
        {	0,	"sra",		TSHIFT,		0x0800	},
        {	0,	"src",		TSHIFT,		0x0B00	},
        {	0,	"srl",		TSHIFT,		0x0900	},
    
        /* 4.5.10 Immediate register instructions */

        /* 10bits of op code, 0, 4bits of working register, followed by
           a data word */        
        {	0,	"ai",		TIMM,		0x0220	},
        {	0,	"andi",		TIMM,		0x0240	},
        {	0,	"ci",		TIMM,		0x0280	},
        {	0,	"li",		TIMM,		0x0200	},
        {	0,	"ori",		TIMM,		0x02C0	},

        /* 4.5.11 Internal register load immediate */
        
        /* 10bits of opcode then 0s, followed by a data word */
        {	0,	"lwpi",		TIRL,		0x02E0	},
        {	0,	"limi",		TIRL,		0x0300	},

        /* 4.5.12 Internal register load and store */
        
        /* 12 bits of opcode followed by a working register */
        {	0,	"stst",		TIRLS,		0x02C0	},
        {	0,	"lst",		TIRLS,		0x0080	},
        {	0,	"stwp",		TIRLS,		0x02A0	},
        {	0,	"lwp",		TIRLS,		0x0090	},
        
        /* 4.5.13 RTWP */

        /* implicit */        
        {	0,	"rtwp",		TIMPL,		0x0380	},

        /* 4.5.14 External instructions */

        /* implicit */
        {	0,	"idle",		TIMPL,		0x0340	},
        {	0,	"rset",		TIMPL,		0x0360	},
        {	0,	"ckof",		TIMPL,		0x03C0	},
        {	0,	"ckon",		TIMPL,		0x03A0	},
        {	0,	"lrex",		TIMPL,		0x03E0	},

        /* Pseudo instructions some assemblers allow */
        /* RT = BR *R11 */
        {	0,	"rt",		TIMPL,		0x045B	},
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
	"branch out of range",		/* 17 */
	"invalid register",		/* 18 */
	"address required",		/* 19 */
	"invalid ID",			/* 20 */
	"",				/* 21 */
	"divide by 0",			/* 22 */
	"constant out of range",	/* 23 */
	"data in BSS",			/* 24 */
	"segment overflow",		/* 25 */
	"register required",		/* 26 */
	"segment conflict",		/* 27 */
	"R0 or constant"		/* 28 */
	"too many Jcc",			/* 29 */
	"cannot index with R0",		/* 30 */
	"alignment error"		/* 31 */
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
