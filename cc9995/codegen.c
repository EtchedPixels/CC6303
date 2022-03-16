/*
 *	CC9995	TMS9995 C Compiler
 *	(C) 2022 Alan Cox
 *
 *	This compiler is built out of a much modified CC65 and all new code
 *	is placed under the same licence as the original. Please direct all
 *	cc9995 bugs to the author not to the cc65 developers unless you find
 *	a bug that is also present in cc65.
 */
/*
 *	TMS9995 code generator, heavily based on the cc65 code generator
 *
 *	The main differences we have are
 *	- We have a real stack and use it
 *	- Functions clean up their own arguments
 *
 *	At this point we map sreg to R0, D to R1 and X to R2
 *	R3 is our stack (it's a software construct), R4 our frame pointer
 *
 *	R11 is used by the CPU for BL but we save and drop it early
 *
 *	At this point register support isn't done and there is no use of the
 *	other registers as a virtual top of stack during expressions etc
 *
 *	This is a word oriented CPU. Except for specific byte operations all
 *	accesses are word aligned, and many internal functions work on words.
 *	Confusingly byte operations use the top half of a register. It's not
 *	clear yet if the compiler will need to shift bytes on load and keep
 *	them internally 16bit or try and do byte ops on char. Given the C
 *	promotion rules it may well be the former.
 *
 *	(It seems demented until you realize it means that the 16bit shifts
 *	do the right thing, the carry just works accordingly and you can do
 *	sign extension ok). It's horrible for compare because there is no
 *	8bit compare immediate so you need to use a literal for byte
 *	compares.)
 *
 *	Subtraction is also a problem for 32bit maths and compares. A
 *	subtract of 0 sets the carry flag because of the CPU internal
 *	behaviour.
 *
 *	Shifts are a problem because although there is a multiple bit shift
 *	but only 16bit, and there is no shift carry in..
 *
 *	Due to the alignment rules chars are stacked as 2 bytes.
 *
 *	On the plus side:
 *	- All 16bit maths is clean and inlne
 *	- Hardware 16bit multiply and divide (signed and unsigned)
 *	- Direct access to any offset of a pointer including stack in many
 *	  instructions (at a 4 byte instruction cost for these forms)
 *
 *	We do not follow the 'traditional' TMS99xx model using BLWP and co
 *	to implement a stack and register windows because the TMS9995 has only
 *	256 bytes of internal fast 16bit RAM for register windows. Instead we
 *	treat it like a classic CPU with a software constructed stack
 *
 *	Q: Should we go back to cc65 like 'called function cleans stack'?
 *
 *	Think about a fastcall model - it can probably be done cheap to pass
 *	arg0 in R0/R1 for non varargs, and just stack them in the setup when
 *	we stack R11 and sometimes the old R4.
 */


/*****************************************************************************/
/*                                                                           */
/*                                 codegen.c                                 */
/*                                                                           */
/*                            6502 code generator                            */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 1998-2013, Ullrich von Bassewitz                                      */
/*                Roemerstrasse 52                                           */
/*                D-70794 Filderstadt                                        */
/* EMail:         uz@cc65.org                                                */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* common */
#include "check.h"
#include "cpu.h"
#include "inttypes.h"
#include "strbuf.h"
#include "xmalloc.h"
#include "xsprintf.h"
#include "version.h"

/* cc65 */
#include "asmcode.h"
#include "asmlabel.h"
#include "casenode.h"
#include "codeseg.h"
#include "dataseg.h"
#include "error.h"
#include "expr.h"
#include "global.h"
#include "segments.h"
#include "stackptr.h"
#include "util.h"
#include "codegen.h"


#define REGALLOC_BASE	8

static unsigned FramePtr;

static unsigned int flagsvalid;
static unsigned int flagstype;

/*****************************************************************************/
/*                                  Helpers                                  */
/*****************************************************************************/

static void RUse(unsigned int reg);


static void typeerror (unsigned type)
/* Print an error message about an invalid operand type */
{
    /* Special handling for floats here: */
    if ((type & CF_TYPEMASK) == CF_FLOAT) {
        Fatal ("Floating point type is currently unsupported");
    } else {
        Internal ("Invalid type in CF flags: %04X, type = %u", type, type & CF_TYPEMASK);
    }
}

static int GetRegVarOf(unsigned Flags, uintptr_t Label, long Offs)
{
    if ((Flags & CF_ADDRMASK) != CF_REGVAR)
        return 0;
    return REGALLOC_BASE + (unsigned)((Label+Offs) & 0xFFFF);
}

static const char* GetLabelName (unsigned Flags, uintptr_t Label, long Offs)
{
    static char Buf [256];              /* Label name */

    /* Create the correct label name */
    switch (Flags & CF_ADDRMASK) {

        case CF_STATIC:
            /* Static memory cell */
            if (Offs) {
                xsprintf (Buf, sizeof (Buf), "@%s%+ld", LocalLabelName (Label), Offs);
            } else {
                xsprintf (Buf, sizeof (Buf), "@%s", LocalLabelName (Label));
            }
            break;

        case CF_EXTERNAL:
            /* External label */
            if (Offs) {
                xsprintf (Buf, sizeof (Buf), "@_%s%+ld", (char*) Label, Offs);
            } else {
                xsprintf (Buf, sizeof (Buf), "@_%s", (char*) Label);
            }
            break;

        case CF_ABSOLUTE:
            /* Absolute address */
            xsprintf (Buf, sizeof (Buf), "@0x%04X", (unsigned)((Label+Offs) & 0xFFFF));
            break;

        case CF_REGVAR:
            /* Variable in register bank */
            /* FIXME: just offs ? */
            xsprintf (Buf, sizeof (Buf), "!R%u", REGALLOC_BASE + (unsigned)((Label+Offs) & 0xFFFF));
            break;

        default:
            Internal ("Invalid address flags: %04X", Flags);
    }

    /* Return a pointer to the static buffer */
    return Buf;
}


/* Generate the value of an offset from the stack base. We don't have to
   worry about reach */

static int GenOffset(int Offs, unsigned int *r)
{
    /* Frame pointer argument */
    if (Offs > 0 && FramePtr) {
        *r = 4;
        RUse(4);
        return Offs + 2;
    }
    Offs -= StackPtr;
    RUse(3);
    *r = 3;
    return Offs;
}

/*****************************************************************************/
/*                            Pre- and postamble                             */
/*****************************************************************************/



void g_preamble (void)
/* Generate the assembler code preamble */
{
    /* Identify the compiler version */
    AddTextLine (";");
    AddTextLine ("; File generated by cc9995 v %s", GetVersionAsString ());
    AddTextLine (";");

}



void g_fileinfo (const char* Name, unsigned long Size, unsigned long MTime)
/* If debug info is enabled, place a file info into the source */
{
    /* TODO */
}

/* We have generated code and thrown it away. OPTIMISE: we can do better in this
   case I think */
void g_removed (void)
{
    //InvalidateRegCache();
}


/*****************************************************************************/
/*                              Segment support                              */
/*****************************************************************************/

void g_userodata (void)
/* Switch to the read only data segment */
{
    UseDataSeg (SEG_RODATA);
}



void g_usedata (void)
/* Switch to the data segment */
{
    UseDataSeg (SEG_DATA);
}



void g_usebss (void)
/* Switch to the bss segment */
{
    UseDataSeg (SEG_BSS);
}



void g_segname (segment_t Seg)
/* Emit the name of a segment if necessary */
{
    /* Emit a segment directive for the data style segments */
    DataSeg* S;
    switch (Seg) {
        case SEG_RODATA: S = CS->ROData; break;
        case SEG_DATA:   S = CS->Data;   break;
        case SEG_BSS:    S = CS->BSS;    break;
        default:         S = 0;          break;
    }
    if (S) {
        DS_AddLine (S, ".segment\t\"%s\"", GetSegName (Seg));
    }
}

/*****************************************************************************/
/*                                   Aliaser                                 */
/*****************************************************************************/

struct regalias {
    uint8_t type;
#define ALIAS_UNKNOWN		0	/* No idea what it is */
#define ALIAS_REGISTER		1	/* Copy of register */
#define ALIAS_CONST		2	/* Constant */
#define ALIAS_LIVE		3	/* Unknown live content */
#define ALIAS_LIVECONST		4	/* Known live constant */
    uint16_t val;
};

static struct regalias alias[16];


static void RUse(unsigned int reg);

static void RBreakLinks(unsigned int reg)
{
    struct regalias *r = alias;
    unsigned int i;
    int rp = -1;

    fprintf(stderr, "RBreakLinks %d [", reg);
    for (i = 0; i < 16 ; i++) {
        if (r->type == ALIAS_REGISTER && r->val == reg) {
            /* Move the alias */
            if (rp != -1) {
                fprintf(stderr, "%d->%d", i, rp);
                r->val = rp;
            }
            else {
                /* First case.. crystalize a referenceable copy */
                /* Any further copies will reference the new live */
                fprintf(stderr, "%d", i);
                rp = i;
                RUse(i);
            }
        }
        r++;
    }
    fprintf(stderr, "]\n");
}

/*
 *	We keep register such that all aliases point to an actual
 *	instantiated register. That means that when we replace it
 *	we have little work to do
 *
 *	mod
 *	0	not modifying - discard
 *	1	
 */
static void RReplace(unsigned int reg, unsigned int mod)
{
    struct regalias *r = alias + reg;

    if (mod != 2)
        fprintf(stderr, "Replacing %d mod %d\n", reg, mod);    
    if (mod) {
        /* We will modify the register so recover the actual value */
        switch(r->type) {
            case ALIAS_REGISTER:
                AddCodeLine("MOV R%d, R%d", r->val, reg);
                r->type = ALIAS_LIVE;	/* Live value */
                flagsvalid = 0;
                break;
            case ALIAS_CONST:
                fprintf(stderr, "Realize const %d %d\n", reg, r->val);
                if (r->val == 0)
                    AddCodeLine("CLR R%d", reg);
                else {
                    AddCodeLine("LI R%d, %d", reg, r->val);
                    flagsvalid = 0;
                }
                r->type = ALIAS_LIVECONST;
                break;
            /* Cannot materialize an unknown alias */
            case ALIAS_UNKNOWN:
                if (mod == 1)
                    Internal("RRepUNK R%d", reg);
                break;
            case ALIAS_LIVECONST:
            case ALIAS_LIVE:
                /* Break any references */
                RBreakLinks(reg);
                break;
        }
    } else {
        if (r->type == ALIAS_LIVE)
            RBreakLinks(reg);
        r->type = ALIAS_UNKNOWN;
    }
}

static void RSetConstant(unsigned int reg, uint16_t val);

/*
 *	Remember a register is a copy of another
 */
static void RSetRegister(unsigned int reg, unsigned int r2)
{
    /* Walk the aliases to find the actual holding register */
    fprintf(stderr, "RSetRegister %d to %d: ", reg, r2);
    while(alias[r2].type == ALIAS_REGISTER)
        r2 = alias[r2].val;
    fprintf(stderr, "r2 root is %d, type %d\n", r2, alias[r2].type);
    /* Special case - constant 0 is best remembered as constant */
    if (alias[r2].type == ALIAS_CONST && alias[r2].val == 0) {
        RSetConstant(reg, 0);
        return;
    }
    /* Set up our alias */
    RReplace(reg, 0);
    fprintf(stderr, "RSetRegister %d now alias of %d\n", reg, r2);
    alias[reg].type = ALIAS_REGISTER;
    alias[reg].val = r2;
}

/*
 *	Remember a register is a constant
 */
static void RSetConstant(unsigned int reg, uint16_t val)
{
    unsigned i;
    struct regalias *r = alias;

    /* Firstly look to see the constant is in another register already. If
       so then we can instead declare ourselves an alias of the register as
       a register move is cheaper than a constant load (except 0) */
    if (val) {
        for (i = 0; i < 15; i++) {
            if (r->type == ALIAS_CONST && r->val == val) {
                RSetRegister(reg, i);
                return;
            }
            r++;
        }
    }
    /* Clean up any old value */
    RReplace(reg, 0);
    alias[reg].type = ALIAS_CONST;
    alias[reg].val = val;
}

static void RSetUnknown(unsigned int reg)
{
    RReplace(reg, 0);
}

static void RUse(unsigned int reg)
{
    RReplace(reg, 1);
}

static void RSetLive(unsigned int reg)
{
    /* Flush the old */
    RReplace(reg, 0);
    /* Remember it's now live */
    alias[reg].type = ALIAS_LIVE;
}

static void RFlush(void)
{
    unsigned i;
    struct regalias *r = alias;
    for (i = 0; i < 8; i++)
        r++->type = ALIAS_UNKNOWN;
    /* Caller register variables are assumed valid on entry */
    while(i < 16 )
        RSetLive(i++);
    /* Working registers magically set live */
    RSetLive(0);
    RSetLive(1);
    RSetLive(3);
    if (FramePtr)
        RSetLive(4);
}

/* Flush the state after a call. R0-R7 are modified (R3/R4 will in fact be
   saved), R0-R1 are live with the return code. As we did a CrytalizeCall before
   we called we know the fast writes over 0-7 are ok */
static void RFlushCall(void)
{
    unsigned i;
    struct regalias *r = alias;
    for (i = 0; i < REGALLOC_BASE; i++)
        r++->type = ALIAS_UNKNOWN;
    /* Working registers magically set live */
    RSetLive(0);
    RSetLive(1);
    RSetLive(3);
    if (FramePtr)
        RSetLive(4);
}

static void RCrystalize(void)
{
    unsigned int i;
    for (i = 0; i < 16; i++)
        RReplace(i, 2);
}

static void RCrystalizeMovable(void)
{
    unsigned int i;
    for (i = 0; i < 16; i++)
            RReplace(i, 2);
    RFlushCall();
}

/* Only deal with registers that need to be correctly assigned on entry. That
   is R8-R15. R1 may be needed but is handled in g_call for indirect */
static void RCrystalizeCall(void)
{
    unsigned int i;
    for (i = REGALLOC_BASE; i < 16; i++)
            RReplace(i, 2);
}

static int RIsConstant(unsigned int reg)
{
    while(alias[reg].type == ALIAS_REGISTER)
        reg = alias[reg].val;
    if (alias[reg].type == ALIAS_CONST)
        return 1;
    return 0;
}

static uint16_t RGetConstant(unsigned int reg)
{
    while(alias[reg].type == ALIAS_REGISTER)
        reg = alias[reg].val;
    if (!RIsConstant(reg))
        Internal("RGC");
    return alias[reg].val;
}

/* Resolve a register. We return the actual register to use in order to
   access this aliased register. We may generate code to crystalize
   constants or aliases to constants */
static int RResolve(unsigned int r)
{
    switch(alias[r].type) {
    case ALIAS_LIVE:
        return r;
    case ALIAS_CONST:
        RUse(r);
        return r;
    case ALIAS_UNKNOWN:
        Internal("RResUnk");
    case ALIAS_REGISTER:
        while(alias[r].type == ALIAS_REGISTER)
            r = alias[r].val;
        return RResolve(r);
    }
    Internal("RResT");
}


static void MoveReg(unsigned int s, unsigned int d)
{
    /* Just mark d as a clone of s, the register handler will figure the
       rest out */
    fprintf(stderr, "MoveReg %d %d\n", s, d);
    RSetRegister(d, s);
}
/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/

unsigned sizeofarg (unsigned flags)
/* Return the size of a function argument type that is encoded in flags */
{
    switch (flags & CF_TYPEMASK) {

        /* As it's a word machine we push chars as two bytes */
        case CF_CHAR:
            return 2;

        case CF_INT:
            return 2;

        case CF_LONG:
            return 4;

        case CF_FLOAT:
            return 4;

        default:
            typeerror (flags);
            /* NOTREACHED */
            return 2;
    }
}


/* We need to track the state of X versus S here. As we push and pop
   we may well be able to supress the use of TSX but we can't assume the
   offset versus S is the same any more */
int pop (unsigned flags)
/* Pop an argument of the given size */
{
    unsigned s = sizeofarg (flags);
    return StackPtr += s;
}

int push (unsigned flags)
/* Push an argument of the given size */
{
    unsigned s = sizeofarg (flags);
    return StackPtr -= s;
}

static int WordsSame(unsigned int x)
{
    return (x >> 16) == (x & 0xFFFF);
}

/* Set a register with an immediate value. The only optimization for
   TMS9995 is for 0 */
static void Assign(unsigned reg, uint16_t val)
{
    RSetConstant(reg, val);
}

static void Assign32(unsigned rh, unsigned rl, unsigned int val)
{
    if (val && WordsSame(val)) {
        Assign(rh, val);
        RSetRegister(rl, rh);
    } else {
        Assign(rh, val >> 16);
        Assign(rl, val & 0xFFFF);
    }
}

static void AddImmediate(unsigned reg, uint16_t val)
{
    if (RIsConstant(reg)) {
        RSetConstant(reg, RGetConstant(reg) + val);
        return;
    }
    RUse(reg);
    switch(val) {
    case 0:
        break;
    case 1:
        AddCodeLine("INC R%d", reg);
        break;
    case 2:
        AddCodeLine("INCT R%d", reg);
        break;
    case 0xFFFF:
        AddCodeLine("DEC R%d", reg);
        break;
    case 0xFFFE:
        AddCodeLine("DECT R%d", reg);
        break;
    default:
        AddCodeLine("AI R%d, %d", reg, val);
    }
}

/* Turn a register and offset into the correct form depending upon whether
   the offset is 0 */
static char *IndirectForm(unsigned int r, unsigned int offset)
{
    static char ibuf[64];
    /* Materialize the pointer */
    r = RResolve(r);
    if (offset)
        sprintf(ibuf, "@%d(R%d)", offset, r);
    else
        sprintf(ibuf, "*R%d", r);
    return ibuf;
}

/*
 * Byte fetches and stores are not pretty
 *
 * TODO:
 * We may want to keep track of whether R1 / R2 are currently in byte
 * format or word and adjust when needed only. Does mean a lot of checks
 * around the code though.
 *
 * Need to think about the stacked value format as well. Do we co-oerce
 * char values to integer when pushing (given they usually already are) and
 * if so that implies we in turn want to use word load/stores on them and
 * word align 8bit variables on the internal stack, so we don't use byteindirect
 * on the local cases. ?
 */

/*
 * Fetch a byte into a register by register/offset. Used for stack and
 * pointer dereferences.
 */
static void FetchByteIndirect(unsigned int r, unsigned int Offs, unsigned Flags)
{
    unsigned int d = 1;
    unsigned int nr;

    /* If we are doing D into D then we can flush X */
    if (Flags & CF_USINGX)
        d = 2;
    else
        RSetUnknown(2);

    AddCodeLine(";FBI %d %d", r, d);

    nr = RResolve(r);

    AddCodeLine(";FBI ->%d", nr);
    /* We will commonly find that nr is 1 and d is 1. For now tackle
       this case by using a different code pattern. We could in theory
       just use R2 as the target and MoveReg so the following code uses
       R2, but most stuff will force it into R1 anyway */

    /* Character sized */
    if (Flags & CF_UNSIGNED) {
        if (nr != d) {
            RSetLive(d);
            AddCodeLine("CLR R%d", d);
            AddCodeLine("MOVB %s,R%d", IndirectForm(nr, Offs), d);
            AddCodeLine("SWPB R%d", d);
        } else {
            RSetLive(d);
            AddCodeLine("MOVB %s,R%d", IndirectForm(nr, Offs), d);
            AddCodeLine("SRL R%d, 8", d);
        }
    } else {
        RSetLive(d);
        AddCodeLine("MOVB %s,R%d", IndirectForm(nr, Offs), d);
        AddCodeLine("SRA R%d, 8", d);
    }
    AddCodeLine(";fbi");
    flagsvalid = 1;
    flagstype = Flags;
}

/*
 * Fetch a byte from a label and offset for static or global variables
 */
static void FetchByteStatic(const char *lbuf, unsigned Flags)
{
    unsigned int d = 1;
    unsigned int r;

    if (Flags & CF_USINGX)
        d = 2;

    r = RResolve(d);
    
    if (Flags & CF_UNSIGNED) {
        AddCodeLine("CLR R%d", d);
        AddCodeLine("MOVB %s, R%d", lbuf, d);
        AddCodeLine("SWPB R%d", d);
    } else {
        AddCodeLine("MOVB %s, R%d", lbuf, d);
        AddCodeLine("SRA R%d,8", d);
    }
    MoveReg(r, d);
}

/*
 * Store a byte to a label/offset for static or global variables
 */
static void PutByteStatic(const char *lbuf, unsigned Flags)
{
    unsigned r = 1;
    if (Flags & CF_USINGX)
        r = 2;
    r = RResolve(r);
    AddCodeLine("SWPB R%d", r);
    AddCodeLine("MOVB R%d, %s", r, lbuf);
    AddCodeLine("SWPB R%d", r);
}

/*
 * Store a register to a register/offset. Used for stack and pointer
 * dereferences.
 */
static void PutByteIndirect(unsigned int r, unsigned int Offs, unsigned int Flags)
{
    unsigned int d = 1;
    unsigned int nr, nd;
    if (Flags & CF_USINGX)
        d = 2;
    
    if (r == d)
        Internal("PInd");

    nd = RResolve(d);
    nr = RResolve(r);

    /* We may have a register collision */
    if (nr == nd) {
        if (nr != d) {
            RUse(d);
            nd = d;
        } else {
            RUse(r);
            nr = r;
        }
    }
                
    AddCodeLine("SWPB R%d", nd);
    AddCodeLine("MOVB R%d, %s", nd, IndirectForm(nr, Offs));
    AddCodeLine("SWPB R%d", nd);
}

/*
 * Literals are needed for certain things because many instructions have
 * an indirect form but not an immediate one.
 *
 * TODO: nicer way to merge common literals.
 * Spot "literals" that are consts in a register at that moment ?
 */

unsigned MakeLiteral(uint16_t val)
{
    unsigned L;
    L = GetLocalLabel();
    g_defdatalabel(L);
    AddDataLine(".word %d", val);
    return L;
}

/*****************************************************************************/
/*              Virtual register stack pool                                  */
/*****************************************************************************/

static unsigned int nreg = 0;
static unsigned int regbase = 0;
static unsigned int regdisable;

#define REG_STACK	0xFFFF
#define REG_OFFSET	8
#define REG_MASK	3

/* The interaction here with the aliasing is decidedly non optimal but allows
   for worst case crystalization of values. */
static unsigned int RegPush(void)
{
    if (nreg == 4) {
        AddImmediate(3, -2);
        RUse(3);
        AddCodeLine("MOV R%d, *R3", regbase + REG_OFFSET);
        push(CF_INT);
        flagsvalid = 0;
        regbase++;
        if (regbase == 4)
            regbase = 0;
    } else {
        nreg++;
    }
    return (regbase + nreg - 1) & REG_MASK;
}

static unsigned int RegPop(void)
{
    /* We have popped off into real stack territory */
    if (regdisable)
        return REG_STACK;

    if (nreg == 0) {
        regbase--;
        return REG_STACK;
    }
    /* Register */
    return (regbase + --nreg) & REG_MASK;
}

static void RegPoolEmpty(const char *p)
{
    if (nreg)
        Internal("RegPool %s", p);
}

static void PopReg(unsigned int r)
{
    unsigned int sr = RegPop();
    if (sr == REG_STACK) {
        pop(CF_INT);
        AddCodeLine("MOV *R3+, R%d", r);
    } else
        MoveReg(sr + REG_OFFSET, r);
}

static char *PopRegName(void)
{
    static char buf[32];
    unsigned int sr = RegPop();
    if (sr == REG_STACK) {
        pop(CF_INT);
        return("*R3+");
    }
    sprintf(buf, "R%d", sr + REG_OFFSET);
    return buf;
}
  
static unsigned int PopRegNumber(void)
{
    unsigned int sr = RegPop();
    if (sr == REG_STACK) {
        Internal("PRN");
    }
    return sr + REG_OFFSET;
}
  
static void DropRegStack(void)
{
    unsigned int sr = RegPop();
    if (sr == REG_STACK) {
        pop(CF_INT);
        RUse(3);
        AddCodeLine("INCT R3");
    }
}

static char *PushRegName(void)
{
    static char buf[32];
    unsigned int sr = RegPush();
    sprintf(buf, "R%d", sr + REG_OFFSET);
    return buf;
}
  
static unsigned PushRegNumber(void)
{
    return RegPush() + REG_OFFSET;
}
  
static char *TopRegName(void)
{
    static char buf[32];
    if (nreg == 0) {
        RUse(3);
        return("*R3");
    }
    sprintf(buf, "R%d", RResolve(((regbase + nreg) & REG_MASK) + REG_OFFSET));
    return buf;
}

static char *StkRegName(uint16_t offset)
{
    static char buf[32];
    uint16_t nr = offset / 2;

    /* Asking for the word at the top of real stack */
    if (nreg == nr) {
        RUse(3);
        return("*R3");
    }
    if (nreg > nr) 	/* Word on virtual stack */
        sprintf(buf, "R%d", RResolve(((regbase + nreg - nr) & REG_MASK) + REG_OFFSET));
    /* Up the real stack */
    else {
        /* Allow for registerised stack */
        offset -= 2 * nr;
        RUse(3);
        sprintf(buf, "@%d(R3)", offset);
    }
    return buf;
}

static void PushReg(unsigned int r)
{
    unsigned int rs = RegPush();
    MoveReg(r, rs + REG_OFFSET);
}

static void PushConst(uint16_t val)
{
    unsigned int rs = RegPush();
    RSetConstant(rs + REG_OFFSET, val);
}

static void PopReg32(unsigned int rh, unsigned int rl)
{
    PopReg(rh);
    PopReg(rl);
}

static void PushReg32(unsigned int rh, unsigned int rl)
{
    PushReg(rl);
    PushReg(rh);
}

static void PushConst32(uint32_t val)
{
    unsigned int rs = RegPush();
    RSetConstant(rs + REG_OFFSET, val);
    rs = RegPush();
    RSetConstant(rs + REG_OFFSET, val >> 16);
}

/* Adjusts the SP tracking itself */
static void GrowStack(int val)
{
    if (val < 0)
        Internal("GrowStack %d", val);
    /* FIXME: we should optimise this to do a single AI if we will spill
       and then use offsets ?? */
    /* Spill registers and make space */
    while(nreg && (val -= 2))
        RegPush();
    /* And just adjust for the remainder */
    AddImmediate(3, -val);
    StackPtr -= val;
}

static void ShrinkStack(int val)
{
    int nr = val / 2;	/* Number of reg slots we lose */
    if (nreg >= nr) {
        nreg -= nr;
        return;
    }
    if (val < 0)
        Internal("ShrinkStack %d", val);
    val -= 2 * nreg;	/* Virtual registers */
    /* We are shrinking real stack too */
    AddImmediate(3, val);
    StackPtr += val;
}

/* We may have a bunch of stuff in registers that will become arguments
   or locals. Turn it into a real stack so we can take addresses of it
   and pass it around */
static void ResolveStack(void)
{
    unsigned n = 0;
    RUse(3);
    while(nreg) {
        unsigned rv = ((regbase + n) % REG_MASK) + REG_OFFSET;
        AddCodeLine("DECT R3");
        AddCodeLine("MOV R%d, *R3", RResolve(rv));
        RSetUnknown(rv);
        push(CF_INT);
        nreg--;
        flagsvalid = 0;
    }
    regbase = 0;
}

static void FlushStack(void)
{
    regbase = 0;
    nreg = 0;
}

static void DisableRegStack(void)
{
    ResolveStack();
    regdisable = 1;
}

static void EnableRegStack(void)
{
    if (regdisable == 0)
        Internal("ERS");
    regdisable = 0;
}

/*****************************************************************************/
/*                      Functions handling local labels                      */
/*****************************************************************************/

void g_defcodelabel (unsigned label)
/* Define a local code label */
{
    AddCodeLine("%s:", LocalLabelName(label));
    /* Overly pessimistic but we don't know what our branch will come from */
    //InvalidateRegCache();
}

void g_defdatalabel (unsigned label)
/* Define a local data label */
{
    /* FIXME: we need to pass down alignment info */
    AddDataLine("\t.even");
    AddDataLine ("%s:", LocalLabelName (label));
}



void g_aliasdatalabel (unsigned label, unsigned baselabel, long offs)
/* Define label as a local alias for baselabel+offs */
{
    /* We need an intermediate buffer here since LocalLabelName uses a
    ** static buffer which changes with each call.
    */
    StrBuf L = AUTO_STRBUF_INITIALIZER;
    SB_AppendStr (&L, LocalLabelName (label));
    SB_Terminate (&L);
    AddDataLine ("%s .equ\t%s+%ld",
                 SB_GetConstBuf (&L),
                 LocalLabelName (baselabel),
                 offs);
    SB_Done (&L);
}



/*****************************************************************************/
/*                     Functions handling global labels                      */
/*****************************************************************************/



void g_defgloblabel (const char* Name)
/* Define a global label with the given name */
{
    /* Global labels are always data labels */
    AddDataLine ("_%s:", Name);
}



void g_defexport (const char* Name, int ZP)
/* Export the given label */
{
    AddTextLine ("\t.export\t\t_%s", Name);
}



void g_defimport (const char* Name, int ZP)
/* Import the given label */
{
}



void g_importstartup (void)
/* Forced import of the startup module */
{
}



void g_importmainargs (void)
/* Forced import of a special symbol that handles arguments to main */
{
}



/*****************************************************************************/
/*                          Function entry and exit                          */
/*****************************************************************************/


void g_enter (const char *name, unsigned flags, unsigned argsize)
/* Function prologue */
{
    push (CF_INT);		/* Return address */

    AddCodeLine(".export _%s", name);
    AddCodeLine(".even");
    AddCodeLine("_%s:",name);

    FlushStack();
    RFlush();

    /* Uglies from the L->R stack handling for varargs */
    if ((flags & CF_FIXARGC) == 0) {
        /* Frame pointer is required for varargs mode. We could avoid
           it otherwise */
        push(CF_INT);
        AddImmediate(3, -2);
        AddCodeLine("MOV R11,*R3");
        push(CF_INT);
        AddImmediate(3, -2);
        AddCodeLine("MOV R4,*R3");
        AddCodeLine("MOV R3, R4");	/* R4 is FP */
        AddCodeLine("A R0, R4");	/* Adjust FP for arguments */
        RSetLive(3);
        RSetLive(4);
        FramePtr = 1;
    } else { 
        FramePtr = 0;
        /* Save the return address: FIXME we really want to merge all of this
           and the local setup nicely */
        RSetLive(3);
        push(CF_INT);
        AddImmediate(3, -2);
        AddCodeLine("MOV R11,*R3");

    }
    //InvalidateRegCache();
}



void g_leave(int voidfunc, unsigned flags, unsigned argsize)
/* Function epilogue */
{
    /* Should always be zero : however ignore this being wrong if we took
       a C level error as that may be the real cause. Only valid code failing
       this check is a problem */

    /* The bits we will undo */
    pop(CF_PTR);
    pop(CF_PTR);
    if (FramePtr)
        pop(CF_PTR);

    if (StackPtr && !ErrorCount)
        Internal("g_leave: stack unbalanced by %d", StackPtr);

    /* Recover the previous frame pointer if we were vararg */
    if (FramePtr) {
        AddCodeLine("MOV *R3+,R4");
    }
    AddCodeLine("MOV *R3+,R11");
    AddCodeLine("RT");
}




/*****************************************************************************/
/*                           Fetching memory cells                           */
/*****************************************************************************/



void g_getimmed (unsigned Flags, unsigned long Val, long Offs)
/* Load a constant into the primary register, or into R2 for some special
   cases we want to optimize */
{
    unsigned short W1,W2;

    if ((Flags & CF_CONST) != 0) {

        /* Numeric constant */
        switch (Flags & CF_TYPEMASK) {

            case CF_CHAR:
            case CF_INT:
                if (Flags & CF_USINGX)
                    Assign(2, Val);
                else
                    Assign(1, Val);
                break;

            case CF_LONG:
                /* Split the value into 4 bytes */
                W1 = (unsigned short) (Val >> 0);
                W2 = (unsigned short) (Val >> 16);
                if (Flags & CF_USINGX) {
                    Assign(2, W1);
                } else {
                    /* Load the value */
                    Assign(0, W2);
                    if (W1 && W1 != W2)
                        Assign(1, W1);
                    else
                        MoveReg(0, 1);
                }
                break;

            default:
                typeerror (Flags);
                break;

        }

    } else {
        /* Some sort of label */
        int r = GetRegVarOf(Flags, Val, Offs);
        int dr = (Flags & CF_USINGX) ? 2 : 1;
        const char* Label = GetLabelName (Flags, Val, Offs);

        /* Register case is different */
        if (r)
            MoveReg(r, dr);
        else {
            /* Load the address into the primary */
            RSetLive(dr);
            AddCodeLine("LI R%d, %s", dr, Label);
        }
    }
}

void g_getstatic (unsigned flags, uintptr_t label, long offs)
/* Fetch an static memory cell into the primary register */
{
    /* Create the correct label name */
    const char* lbuf = GetLabelName (flags, label, offs);
    int r = GetRegVarOf(flags, label, offs);
    int rd = (flags & CF_USINGX) ? 2 : 1;

    /* Check the size and generate the correct load operation */
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            if (r)
                RUse(r);
            /* Byte ops are not pretty */
            if ((flags & CF_FORCECHAR) || (flags & CF_TEST)) {
                RSetLive(1);
                AddCodeLine("MOVB %s, R1", lbuf);   /* load A from the label */
                AddCodeLine("SWPB R1");	/* Preserves flags */
                flagsvalid = 1;
                flagstype = flags;
            } else
                FetchByteStatic(lbuf, flags);
            break;

        case CF_INT:
            if (r)
                MoveReg(r, rd);
            else {
                RSetLive(rd);
                AddCodeLine ("MOV %s, R%d", lbuf, rd);
            }
            /* Try and materialize it immediately */
            if (flags & CF_TEST) {
                RUse(rd);
                flagsvalid = 1;
                flagstype = flags;
            }
            break;

        case CF_LONG:
            if (flags & CF_TEST) {
                RSetLive(1);
                AddCodeLine("MOV %s, R1", lbuf);
                AddCodeLine("OR %s, R1", lbuf + 2);
                flagsvalid = 1;
                flagstype = CF_INT;	/* Folded */
            } else {
                RSetLive(0);
                AddCodeLine("MOV %s, R0", lbuf);
                RSetLive(rd);
                AddCodeLine("MOV %s, R%d", lbuf + 2, rd);
            }
            break;

        default:
            typeerror (flags);

    }
}

/*
 *	Local variables either arguments or auto. These are word aligned
 *	and byte sized variables are allocated a word. This massively
 *	improves performance at a small stack cost.
 */
void g_getlocal (unsigned Flags, int Offs)
/* Fetch specified local object (local var). */
{
    unsigned int r;

    Offs = GenOffset(Offs, &r);

    /* FIXME: FramePointer forms for get/put local */
    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            RSetLive(1);
            AddCodeLine("MOV %s, R1", IndirectForm(r, Offs));
            flagsvalid = 1;
            flagstype = Flags;
            break;

        case CF_LONG:
            RSetLive(0);
            RSetLive(1);
            AddCodeLine("MOV %s, R0", IndirectForm(r, Offs));
            AddCodeLine("MOV %s, R1", IndirectForm(r, Offs));
            /* FIXME - test inline */
            if (Flags & CF_TEST)
                g_test (Flags);
            break;

        default:
            typeerror (Flags);
    }
}

void g_getlocal_x (unsigned Flags, int Offs)
/* Fetch specified local object (local var) into r2. */
{
    unsigned int r;
    Offs = GenOffset(Offs, &r);

    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            RSetLive(2);
            AddCodeLine ("MOV %s, R2", IndirectForm(r, Offs));
            flagsvalid = 1;
            flagstype = Flags;
            break;

        case CF_LONG:
            RSetLive(0);
            AddCodeLine ("MOV %s, R0", IndirectForm(r, Offs));
            RSetLive(2);
            AddCodeLine ("MOV %s, R2", IndirectForm(r, Offs + 2));
            break;

        default:
            typeerror (Flags);
    }
}


void g_primary_to_x(void)
{
    MoveReg(1, 2);
}

void g_x_to_primary(void)
{
    MoveReg(2, 1);
}

/* Get a variable into R1 or R0/R1 through the pointer in R1 */
void g_getind (unsigned Flags, unsigned Offs)
/* Fetch the specified object type indirect through the primary register
** into the primary register
*/
{
    /* FIXME */
    NotViaX();

    /* Handle the indirect fetch */
    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
            /* Character sized */
            MoveReg(1, 2);
            FetchByteIndirect(2, Offs, Flags);
            break;

        case CF_INT:
            AddCodeLine("MOV %s, R1", IndirectForm(1, Offs));
            RSetLive(1);
            flagsvalid = 1;
            flagstype = Flags;
            break;

        case CF_LONG:
            AddCodeLine("MOV %s, R0", IndirectForm(1, Offs));
            AddCodeLine("MOV %s, R1", IndirectForm(1, Offs + 2));
            RSetLive(0);
            RSetLive(1);
            /* FIXME: inline test ? */
            if (Flags & CF_TEST)
                g_test (Flags);
            break;

        default:
            typeerror (Flags);

    }
}

void g_leasp (unsigned Flags, int Offs)
/* Fetch the address of the specified symbol */
{
    /* FramePtr indicates a Varargs function where arguments are relative
       to fp */
    AddCodeLine(";leasp %d %d\n", FramePtr, Offs);
    if (FramePtr && Offs > 0) {
        /* Because of the frame pointer */
        Offs += 2;
        /* FIXME: is there an R1 use of this, and is there a D use of it in
           6800 or does 6800 have a bug here ?? */
        MoveReg(4, 2);
        AddImmediate(2, Offs);
        return;
    }
    Offs = -Offs;
    /* Calculate the offset relative to sp */
    Offs += StackPtr;

    AddCodeLine(";+leasp %d %d\n", FramePtr, Offs);

    if (!(Flags & CF_USINGX)) {
        MoveReg(3, 1);
        AddImmediate(1, Offs);
        return;
    }
    MoveReg(3, 2);
    AddImmediate(2, Offs);
}

/*****************************************************************************/
/*                             Store into memory                             */
/*****************************************************************************/

void g_putstatic (unsigned flags, uintptr_t label, long offs)
/* Store the primary register into the specified static memory cell */
{
    /* Create the correct label name */
    const char* lbuf = GetLabelName (flags, label, offs);
    unsigned int r = 1;
    unsigned int rd = GetRegVarOf(flags, label, offs);
    if (flags & CF_USINGX)
        r = 2;

    r = RResolve(r);

    /* Check the size and generate the correct store operation */
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            if (rd)
                MoveReg(r, rd);
            else
                PutByteStatic(lbuf, flags);
            break;

        case CF_INT:
            if (rd)
                MoveReg(r, rd);
            else
                AddCodeLine("MOV R%d, %s", r, lbuf);
            break;

        case CF_LONG:
            AddCodeLine("MOV R%d, %s + 2", r, lbuf);
            r = RResolve(0);
            AddCodeLine("MOV R%d, %s", r, lbuf);
            break;

        default:
            typeerror (flags);

    }
}

/*
 *	Locals are always allocated two bytes on a word boundary
 *	even if char. This makes a big performance difference.
 */
void g_putlocal (unsigned Flags, int Offs, long Val)
/* Put data into local object. */
{
    unsigned int r;
    unsigned int d;
    Offs = GenOffset (Offs, &r);

    NotViaX();
    /* Need to deal with frame ptr form FIXME */

    /* FIXME: Offset 0 generate *Rn+ ?? , or fix up in assembler ? */
    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            if (Flags & CF_CONST)
                Assign(1, Val);
            d = RResolve(1);
            AddCodeLine("MOV R%d, %s", d, IndirectForm(r, Offs));
            break;
        case CF_LONG:
            if (Flags & CF_CONST) {
                Assign(1, Val >> 16);
                d = RResolve(0);
                AddCodeLine("MOV R%d, %s", d, IndirectForm(r, Offs));
                d = RResolve(1);
                if (WordsSame(Val))
                    Assign(1, Val);
                AddCodeLine("MOV R%d, %s", d, IndirectForm(r, Offs + 2));
            } else {
                d = RResolve(0);
                AddCodeLine("MOV R%d, %s", d, IndirectForm(r, Offs));
                d = RResolve(1);
                AddCodeLine("MOV R%d, %s", d, IndirectForm(r, Offs + 2));
            }
            break;

        default:
            typeerror (Flags);

    }
}

void g_putind (unsigned Flags, unsigned Offs)
/* Store the specified object type in the primary register at the address
** on the top of the stack
*/
{
    unsigned r = PopRegNumber();
    unsigned d;

    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
            /* FIXME: we need to track if R1 is reversed and then we can
               minimise this nonsense all over */
            PutByteIndirect(r, Offs, Flags);
            break;

        case CF_INT:
            d = RResolve(1);
            AddCodeLine("MOV R%d, %s", d, IndirectForm(r, Offs));
            break;

        case CF_LONG:
            d = RResolve(0);
            AddCodeLine("MOV R%d, %s", d, IndirectForm(r, Offs));
            d = RResolve(1);
            AddCodeLine("MOV R%d, %s", d, IndirectForm(r, Offs + 2));
            break;

        default:
            typeerror (Flags);

    }
}



/*****************************************************************************/
/*                    type conversion and similiar stuff                     */
/*****************************************************************************/


void g_toslong (unsigned flags)
/* Make sure, the value on TOS is a long. Convert if necessary */
{
    unsigned L;
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
        case CF_INT:
            NotViaX();	/* For now */
            if (flags & CF_UNSIGNED) {
                /* Push a new upper 16bits of zero */
                PushConst(0);
            } else {
                /* Copy the tos into the new tos and then fix the sign */
                char *t = TopRegName();
                char *n = PushRegName();
                L = GetLocalLabel();
                AddCodeLine("MOV %s, %s", t, n);
                AddCodeLine("CLR %s", n);	/* Flags not affected */
                AddCodeLine("JGE %s", LocalLabelName(L));
                AddCodeLine("DEC %s", n);	/* 0xFFFF */
                g_defcodelabel(L);
            }
            break;

        case CF_LONG:
            break;

        default:
            typeerror (flags);
    }
}

void g_tosint (unsigned flags)
/* Make sure, the value on TOS is an int. Convert if necessary */
{
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
            /* FIXME: do we need to sign extend here ? */
        case CF_INT:
            break;
        case CF_LONG:
            /* We have a big endian long so just throw out the top */
            DropRegStack();
            break;
        default:
            typeerror (flags);
    }
}

static void g_regchar (unsigned Flags)
/* Make sure, the value in the primary register is in the range of char. Truncate if necessary */
{
    RUse(1);
    if (Flags & CF_UNSIGNED)
        AddCodeLine("ANDI R1, 0xFF");
    else {
        AddCodeLine("SWPB R1");
        AddCodeLine("ASR R1,8");
    }
}

void g_regint (unsigned Flags)
/* Make sure, the value in the primary register an int. Convert if necessary */
{
    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
            if (Flags & CF_FORCECHAR)
                /* Conversion is from char */
                g_regchar (Flags);
            /* FALLTHROUGH */
        case CF_INT:
        case CF_LONG:
            break;
        default:
            typeerror (Flags);
    }
}

void g_reglong (unsigned Flags)
/* Make sure, the value in the primary register a long. Convert if necessary */
{
    unsigned L;

    switch (Flags & CF_TYPEMASK) {

        case CF_CHAR:
            NotViaX();
            RUse(1);
            if (Flags & CF_FORCECHAR) {
                if (Flags & CF_UNSIGNED)
                    AddCodeLine("ANDI R1, 0xFF");
                else
                    AddCodeLine("SRA R1, 8");
                break;   
            }
            /* FALLTHROUGH */

        case CF_INT:
            Assign(0, 0);
            if ((Flags & CF_UNSIGNED)  == 0) {
                /* Crystalize it so we don't mess up flags etc */
                RUse(0);
                RUse(1);
                L = GetLocalLabel();
                AddCodeLine ("CI R1, 0x8000");
                AddCodeLine ("JLT %s", LocalLabelName (L));
                AddCodeLine ("DEC R0");
                g_defcodelabel (L);
            }
            break;

        case CF_LONG:
            break;

        default:
            typeerror (Flags);
    }
}

unsigned g_typeadjust (unsigned lhs, unsigned rhs)
/* Adjust the integer operands before doing a binary operation. lhs is a flags
** value, that corresponds to the value on TOS, rhs corresponds to the value
** in (e)ax. The return value is the the flags value for the resulting type.
*/
{
    unsigned ltype, rtype;
    unsigned result;

    /* Get the type spec from the flags */
    ltype = lhs & CF_TYPEMASK;
    rtype = rhs & CF_TYPEMASK;

    /* Check if a conversion is needed */
    if (ltype == CF_LONG && rtype != CF_LONG && (rhs & CF_CONST) == 0) {
        /* We must promote the primary register to long */
        g_reglong (rhs);
        /* Get the new rhs type */
        rhs = (rhs & ~CF_TYPEMASK) | CF_LONG;
        rtype = CF_LONG;
    } else if (ltype != CF_LONG && (lhs & CF_CONST) == 0 && rtype == CF_LONG) {
        /* We must promote the lhs to long */
        if (lhs & CF_REG) {
            g_reglong (lhs);
        } else {
            g_toslong (lhs);
        }
        /* Get the new rhs type */
        lhs = (lhs & ~CF_TYPEMASK) | CF_LONG;
        ltype = CF_LONG;
    }

    /* Determine the result type for the operation:
    **  - The result is const if both operands are const.
    **  - The result is unsigned if one of the operands is unsigned.
    **  - The result is long if one of the operands is long.
    **  - Otherwise the result is int sized.
    */
    result = (lhs & CF_CONST) & (rhs & CF_CONST);
    result |= (lhs & CF_UNSIGNED) | (rhs & CF_UNSIGNED);
    if (rtype == CF_LONG || ltype == CF_LONG) {
        result |= CF_LONG;
    } else {
        result |= CF_INT;
    }
    return result;
}

unsigned g_typecast (unsigned lhs, unsigned rhs)
/* Cast the value in the primary register to the operand size that is flagged
** by the lhs value. Return the result value.
*/
{
    /* Check if a conversion is needed */
    if ((rhs & CF_CONST) == 0) {
        switch (lhs & CF_TYPEMASK) {

            case CF_LONG:
                /* We must promote the primary register to long */
                g_reglong (rhs);
                break;

            case CF_INT:
                /* We must promote the primary register to int */
                g_regint (rhs);
                break;

            case CF_CHAR:
                /* We must truncate the primary register to char */
                g_regchar (lhs);
                break;

            default:
                typeerror (lhs);
        }
    }

    /* Do not need any other action. If the left type is int, and the primary
    ** register is long, it will be automagically truncated. If the right hand
    ** side is const, it is not located in the primary register and handled by
    ** the expression parser code.
    */

    /* Result is const if the right hand side was const */
    lhs |= (rhs & CF_CONST);

    /* The resulting type is that of the left hand side (that's why you called
    ** this function :-)
    */
    return lhs;
}



void g_scale (unsigned flags, long val)
/* Scale the value in the primary register by the given value. If val is positive,
** scale up, is val is negative, scale down. This function is used to scale
** the operands or results of pointer arithmetic by the size of the type, the
** pointer points to.
*/
{
    int p2;

    NotViaX();

    /* Value may not be zero */
    RUse(1);
    if (val == 0) {
        Internal ("Data type has no size");
    } else if (val > 0) {
        /* Going left.. */
        /* Scale up */
        if ((p2 = PowerOf2 (val)) > 0 && p2 <= 4) {
            /* Factor is 2, 4, 8 and 16, use special function */
            switch (flags & CF_TYPEMASK) {
                case CF_CHAR:
                    if (flags & CF_FORCECHAR) {
                        AddCodeLine ("SLA R1, %d", p2);
                        /* FIXME: is this masking needed ? */
                        AddCodeLine("ANI R1,0xFF");
                        break;
                    }
                    /* FALLTHROUGH */
                case CF_INT:
                    AddCodeLine("SLA R1, %d", p2);
                    break;

                case CF_LONG:
                    /* Helper for calls ? */
                    RUse(0);
                    AddCodeLine ("BL shleax%d", p2);
                    break;

                default:
                    typeerror (flags);
            }
        } else if (val != 1) {
            /* Will multiply into a pair high/low 
               value into R0 and then MPY R1,R0 puts the result as we want */
            switch (flags & CF_TYPEMASK) {
                case CF_CHAR:
                case CF_INT:
                    Assign(0, (uint16_t)val);
                    RSetLive(0);
                    RSetLive(1);
                    /* Use a multiplication instead */
                    AddCodeLine("MPY R1, R0");
                    break;
                default:
                    /* Use a helper multiply instead (should never be needed) */
                    g_mul (flags | CF_CONST, val);
                    break;
            }
        }
    } else {
        /* Scale down : going right */
        val = -val;
        if ((p2 = PowerOf2 (val)) > 0 && p2 <= 4) {

            RUse(1);
            /* Factor is 2, 4, 8 and 16 use special function */
            switch (flags & CF_TYPEMASK) {

                /* This works for byte and int */
                case CF_CHAR:
                    /* Byte is a pain */
                    AddCodeLine("SWPB");
                    if (flags & CF_UNSIGNED)
                        AddCodeLine("SRL R1,%d", p2);
                    else
                        AddCodeLine ("SRA R1,%d", p2);
                    AddCodeLine("SWPB");
                    /* FIXME: Do we need to mask ? */
                    break;
                case CF_INT:
                    if (flags & CF_UNSIGNED)
                        AddCodeLine("SRL R1,%d", p2);
                    else
                        AddCodeLine ("SRA R1,%d", p2);
                    break;

                case CF_LONG:
                    RUse(0);
                    if (flags & CF_UNSIGNED)
                        AddCodeLine ("BL lsreax%d", p2);
                    else
                        AddCodeLine ("BL asreax%d", p2);
                    break;

                default:
                    typeerror (flags);

            }

        } else if (val != 1) {
            /* Use a division instead. It's not a simple as a DIV instruction
               so let the helper do the work */
            g_div (flags | CF_CONST, val);

        }
    }
}



/*****************************************************************************/
/*              Adds and subs of variables fix a fixed address               */
/*****************************************************************************/

void g_addlocal (unsigned flags, int offs)
/* Add a local variable to d */
{
    unsigned L;
    unsigned int r;

    offs = GenOffset(offs, &r);

    RUse(1);

    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
            /* We keep locals in word format so we can just use word
               operations on them */
        case CF_INT:
            AddCodeLine ("A %s, R1", IndirectForm(r, offs));
            break;

        case CF_LONG:
            RUse(0);
            L = GetLocalLabel();
            AddCodeLine ("A %s, R1", IndirectForm(r, offs + 2));
            AddCodeLine ("JOC %s", LocalLabelName(L));
            AddCodeLine ("INC R0");
            g_defcodelabel(L);
            AddCodeLine ("A %s, R0", IndirectForm(r, offs));
            break;

        default:
            typeerror (flags);

    }
}

void g_addstatic (unsigned flags, uintptr_t label, long offs)
/* Add a static variable to ax */
{
    /* Create the correct label name */
    const char* lbuf = GetLabelName (flags, label, offs);
    unsigned int r = GetRegVarOf(flags, label, offs);
    unsigned L;

    NotViaX();

    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            if (r == 0) {
                FetchByteStatic(lbuf, flags | CF_USINGX);
                AddCodeLine("A R2, R1");
                AddCodeLine("ANDI R1, 0xFF");
            } else {
                MoveReg(r, 1);
                RUse(1);
            }
            AddCodeLine("A R2, R1");
            AddCodeLine("ANDI R1, 0xFF");
            break;

        case CF_INT:
            RUse(1);
            if (r == 0)
                AddCodeLine("A %s, R1", lbuf);
            else
                AddCodeLine("A R%d, R1", RResolve(r));
            break;

        case CF_LONG:
            /* No long register type */
            RUse(0);
            RUse(1);
            L = GetLocalLabel();
            AddCodeLine ("A %s+2, R1", lbuf);
            AddCodeLine ("JOC %s", LocalLabelName(L));
            AddCodeLine ("INC R0");
            g_defcodelabel(L);
            AddCodeLine ("A %s, R0", lbuf);
            break;

        default:
            typeerror (flags);

    }
}



/*****************************************************************************/
/*                           Special op= functions                           */
/*****************************************************************************/



void g_addeqstatic (unsigned flags, uintptr_t label, long offs,
                    unsigned long val)
/* Emit += for a static variable */
{
    /* Create the correct label name */
    const char* lbuf = GetLabelName (flags, label, offs);
    unsigned int r = GetRegVarOf(flags, label, offs);
    uint16_t v16 = val;

    /* Check the size and determine operation */
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            NotViaX();
            if (flags & CF_FORCECHAR) {
                if (r == 0) {
                    FetchByteStatic(lbuf, flags);
                    RUse(1);	/* Resolve ? FIXME */
                } else { 
                    MoveReg(r, 1);
                    RUse(1);
                }
                if (flags & CF_CONST) {
                    switch(val) {
                        case 1:
                            AddCodeLine("INC R1");
                            break;
                        case 2:
                            AddCodeLine("INCT R1");
                            break;
                        default:
                            AddImmediate(1, (uint8_t) val);
                            break;
                    }
                } else
                    AddCodeLine("A %s, R1", lbuf);
                PutByteStatic(lbuf, flags);
                AddCodeLine("ANDI R1, 0xFF");
                break;
            }
            /* FALLTHROUGH */

        case CF_INT:
            /* So we keep the result in R1 as well */
            if (flags & CF_CONST) {
                switch(v16) {
                case 1:
                    if (r)
                        RUse(r);
                    AddCodeLine("INC %s", lbuf);
                    break;
                case 2:
                    if (r)
                        RUse(r);
                    AddCodeLine("INCT %s", lbuf);
                    break;
                case 0xFFFF:
                    if (r)
                        RUse(r);
                    AddCodeLine("DEC %s", lbuf);
                    break;
                case 0xFFFE:
                    if (r)
                        RUse(r);
                    AddCodeLine("DECT %s", lbuf);
                    break;
                default:
                    if (r)
                        MoveReg(r, 1);
                    else
                        AddCodeLine ("MOV %s, R1", lbuf);
                    AddImmediate(1, val);
                    if (r)
                        MoveReg(1, r);
                    else
                        AddCodeLine ("MOV R1, %s", lbuf);
                }
                if (r == 0)
                    AddCodeLine("MOV %s, R1", lbuf);
                else
                    MoveReg(r, 1);
            } else {
                if (r) { 
                    RUse(r);
                    AddCodeLine("A R1, R%d", r);
                } else
                    AddCodeLine("A R1, %s", lbuf);
                RSetLive(1);
                AddCodeLine("MOV %s, R1", lbuf);
                /* Peephole opportunity */
            }
            break;

        case CF_LONG:
            NotViaX();
            lbuf = GetLabelName (flags, label, offs);
            if (flags & CF_CONST) {
                if (val < 0x10000) {
                    RSetLive(1);
                    RSetLive(2);
                    AddCodeLine("LI R2, %s", lbuf);
                    AddCodeLine("LI R1, %d", (uint32_t)val);
                    AddCodeLine("BL laddeqstatic16");
                } else {
                    g_getstatic (flags, label, offs);
                    g_inc (flags, val);
                    g_putstatic (flags, label, offs);
                }
            } else {
                /* Might be worth inlining ? */
                RSetLive(2);
                RSetLive(0);
                RSetLive(1);
                AddCodeLine ("LI R2,%s", lbuf);
                AddCodeLine ("BL laddeq");
            }
            break;

        default:
            typeerror (flags);
    }
}

void g_addeqlocal (unsigned flags, int Offs, unsigned long val)
/* Emit += for a local variable */
{
    unsigned L;
    unsigned int r;
    NotViaX();
    
    Offs = GenOffset(Offs, &r);

    /* Check the size and determine operation */
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
            /* Our local is word sized so we are fine */
        case CF_INT:
            if (flags & CF_CONST)
                Assign(1, val);
            RUse(1);
            AddCodeLine("A %s, R1", IndirectForm(r, Offs));
            AddCodeLine("MOV R1, %s", IndirectForm(r, Offs));
            break;

        case CF_LONG:
            if (flags & CF_CONST)
                Assign32(0, 1, val);
            RUse(0);
            RUse(1);
            L = GetLocalLabel();
            AddCodeLine("A %s, R1", IndirectForm(r, Offs + 2));
            AddCodeLine("JNC %s", LocalLabelName(L));
            AddCodeLine("INC R0");
            g_defcodelabel(L);
            AddCodeLine("A %s, R0", IndirectForm(r, Offs));
            /* Hopefully we can trim this with the peepholer */
            AddCodeLine("MOV R1, %s", IndirectForm(r, Offs + 2));
            AddCodeLine("MOV R0, %s", IndirectForm(r, Offs));
            break;

        default:
            typeerror (flags);
    }
}


void g_addeqind (unsigned flags, unsigned offs, unsigned long val)
/* Emit += for the location with address in R1 */
{
    unsigned L;
    unsigned r;
    /* Check the size and determine operation */
    NotViaX();

    r = RResolve(1);

    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            MoveReg(r, 0);
            FetchByteIndirect(0, offs, flags);
            AddImmediate(1, val & 0xFF);
            PutByteIndirect(0, offs, flags);
            break;

        case CF_INT:
            MoveReg(r, 0);
            RUse(0);	/* For now FIXME */
            RSetLive(1);
            AddCodeLine("MOV %s, R1", IndirectForm(0, offs));
            AddImmediate(1, val);
            RUse(1);
            AddCodeLine("MOV R1, %s", IndirectForm(0, offs));
            break;
            
        case CF_LONG:
            if (flags & CF_CONST)
                Assign32(0, 1, val);
            L = GetLocalLabel();
            MoveReg(1, 2);
            RSetLive(1);
            RSetLive(0);
            AddCodeLine("A %s, R1", IndirectForm(2, offs + 2));
            AddCodeLine("JNC %s", LocalLabelName(L));
            AddCodeLine("INC R0");
            g_defcodelabel(L);
            AddCodeLine("A %s, R0", IndirectForm(2, offs));
            /* Hopefully we can trim this with the peepholer */
            AddCodeLine("MOV R1, %s", IndirectForm(2, offs + 2));
            AddCodeLine("MOV R0, %s", IndirectForm(2, offs));
            break;


        default:
            typeerror (flags);
    }
}

/* CPU is symmetrical here so we can rewrite all of these in terms of
   addition */

void g_subeqstatic (unsigned flags, uintptr_t label, long offs,
                    unsigned long val)
/* Emit -= for a static variable */
{
    g_addeqstatic(flags, label, offs, -val);
}

void g_subeqlocal (unsigned flags, int Offs, unsigned long val)
/* Emit -= for a local variable */
{
    g_addeqlocal(flags, Offs, -val);
}

void g_subeqind (unsigned flags, unsigned offs, unsigned long val)
/* Emit -= for the location with address in X */
{
    g_addeqind(flags, offs, -val);
}

/*****************************************************************************/
/*                 Add a variable address to the value in R1                */
/*****************************************************************************/

void g_addaddr_local (unsigned flags attribute ((unused)), int offs)
/* Add the address of a local variable to R1 */
{
    /* Add the offset */
    NotViaX();
    
    if (offs > 0 && FramePtr) {
        RUse(1);
        AddCodeLine("A R4, R1");
        offs += 2;
        AddImmediate(1, offs);
    } else {
        offs -= StackPtr;
        AddImmediate(1, offs);
        RUse(3);
        AddCodeLine("A R3, R1");
    }
    RSetLive(1);
}

void g_addaddr_static (unsigned flags, uintptr_t label, long offs)
/* Add the address of a static variable to d */
{
    /* Create the correct label name */
    const char* lbuf = GetLabelName (flags, label, offs);
    unsigned int r = GetRegVarOf(flags, label, offs);

    if (r)
        RUse(r);
    if (flags & CF_USINGX) {
        RUse(2); 
        AddCodeLine ("A %s, R2", lbuf);
    }  else {
        RUse(1);
        AddCodeLine ("A %s, R1", lbuf);
    }
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* This is used internally for certain increment cases only */

/*
 *	We should look for a free registers and borrow them if possible
 *	instead, with some kind of memory of what we are up to.
 */
 
void g_save (unsigned flags)
/* Copy primary register to hold register. */
{
    unsigned int r;
    NotViaX ();

    /* Check the size and determine operation */
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
        case CF_INT:
            r = RResolve(1);
            AddCodeLine("MOV R%d, tmp1", r);
            break;

        case CF_LONG:
            r = RResolve(0);
            AddCodeLine("MOV R%d, tmp1", r);
            r = RResolve(1);
            AddCodeLine("MOV R%d, tmp1 + 2", r);
            break;

        default:
            typeerror (flags);
    }
}

void g_restore (unsigned flags)
/* Copy hold register to primary. */
{
    NotViaX();
    /* Check the size and determine operation */
    RSetLive(1);
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
        case CF_INT:
            AddCodeLine("MOV tmp1, R1");
            break;

        case CF_LONG:
            RSetLive(0);
            AddCodeLine ("MOV tmp1, R0");
            AddCodeLine ("MOV tmp1 + 2, R1");
            break;

        default:
            typeerror (flags);
    }
}

void g_cmp (unsigned flags, unsigned long val)
/* Immediate compare. The primary register will not be changed, Z flag
** will be set.
*/
{
    NotViaX();

    RUse(1);
    /* Check the size and determine operation */
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            /* This masking change to the register is OK. We are changing
               only unused bits */
            if (flags & CF_FORCECHAR) {
                AddCodeLine ("ANDI R1, 0xFF");
                AddCodeLine ("CI R1, %d", (unsigned char)val);
                break;
            }
            /* FALLTHROUGH */

        case CF_INT:
            AddCodeLine("CI R1, %d", (uint16_t)val);
            break;

        case CF_LONG:
            Internal ("g_cmp: Long compares not implemented");
            break;

        default:
            typeerror (flags);
    }
}

/* FIXME: Audit to ensure no flags setting based methods go via this as it
   destroys the flags */
static void oper (unsigned Flags, unsigned long Val, const char* const* Subs)
/* Encode a binary operation. subs is a pointer to four strings:
**      0       --> Operate on ints
**      1       --> Operate on unsigneds
**      2       --> Operate on longs
**      3       --> Operate on unsigned longs
**
**  Sets up X for accesses. Don't use oper for magic via X code
*/
{
    ResolveStack();
    /* Determine the offset into the array */
    if (Flags & CF_UNSIGNED) {
        ++Subs;
    }
    if ((Flags & CF_TYPEMASK) == CF_LONG) {
        Subs += 2;
    }

    /* Load the value if it is not already in the primary */
    if (Flags & CF_CONST) {
        /* Load value - we call the helpers with the value in D, except
           for longs */
        g_getimmed (Flags, Val, 0);
    }

    RSetUnknown(2);
    RCrystalize();
    AddCodeLine ("BL %s", *Subs);
    RFlushCall();
    RSetLive(0);
    RSetLive(1);

    /* The operation will pop it's argument */
    pop (Flags);
}

void g_test (unsigned flags)
/* Test the value in the primary and set the condition codes */
{
    NotViaX();
    RUse(1);
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
            if (flags & CF_FORCECHAR) {
                AddCodeLine ("ANDI R1, 0xFF");
                flagsvalid = 1;
                flagstype = flags;
                break;
            }
            /* FALLTHROUGH */
        case CF_INT:
            AddCodeLine ("CI R1,0");
            flagsvalid = 1;
            flagstype = flags;
            break;

        case CF_LONG:
            RUse(0);
            if (flags & CF_UNSIGNED) {
                AddCodeLine ("BL utsteax");
            } else {
                AddCodeLine ("BL tsteax");
            }
            flagsvalid = 1;
            flagstype = flags;
            break;

        default:
            typeerror (flags);

    }
}

void g_test_eq(unsigned flags)
/* Test the value in the primary and set the condition codes */
{
    NotViaX();
    RUse(1);
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
            if (flags & CF_FORCECHAR) {
                AddCodeLine ("ANDI R1, 0xFF");
                flagsvalid = 1;
                flagstype = flags;
                break;
            }
            /* FALLTHROUGH */
        case CF_INT:
            AddCodeLine ("MOV R1,R1");
            flagsvalid = 1;
            flagstype = flags;
            break;

        case CF_LONG:
            RUse(0);
            if (flags & CF_UNSIGNED) {
                AddCodeLine ("BL utsteax");
            } else {
                AddCodeLine ("BL tsteax");
            }
            flagsvalid = 1;
            flagstype = flags;
            break;

        default:
            typeerror (flags);

    }
}



void g_push (unsigned flags, unsigned long val)
/* Push the primary register or a constant value onto the stack */
{
    unsigned int r;
    if ((flags & CF_TYPEMASK) != CF_LONG) {
        if (flags & CF_CONST)
            Assign(PushRegNumber(), val);
        else {
            r = RResolve(1);
            PushReg(r);
        }
        return;
    }
    /* FIXME: do 32bit nicer versions to match 16 */
    if (flags & CF_CONST) {
        if (WordsSame(val)) {
            Assign(1, val);
            r = RResolve(1);
            PushReg(r);
            PushReg(r);
        } else {
            Assign32(0, 1, val);
            r = RResolve(1);
            PushReg32(RResolve(0), r);
        }
    } else {
        r = RResolve(1);
        PushReg32(RResolve(0), r);
    }
}

/* Sometimes it's better to push as we go */
void g_push_now(unsigned flags, unsigned long val)
{
    /* Should usually be a no-op */
    ResolveStack();

    if (flags & CF_CONST)
        fprintf(stderr, "Push const %ld", val);
    if ((flags & CF_TYPEMASK) != CF_LONG) {
        AddCodeLine("DECT R3");
        if (flags & CF_CONST) {
            if (val == 0)
                AddCodeLine("CLR *R3");
            else {
                Assign(1, val);
                RUse(1);
                AddCodeLine("MOV R1,*R3");
            }
        } else
            AddCodeLine("MOV R1,*R3");
        push(flags);
        return;
    }
    if (flags & CF_CONST) {
        if (WordsSame(val)) {
            if (val == 0) {
                AddCodeLine("DECT R3");
                AddCodeLine("CLR *R3");
                AddCodeLine("DECT R3");
                AddCodeLine("CLR *R3");
            } else {
                Assign(1, val);
                RUse(1);
                AddCodeLine("DECT R3");
                AddCodeLine("MOV R1,*R3");
                AddCodeLine("DECT R3");
                AddCodeLine("MOV R1,*R3");
            }
        } else {
            if ((val & 0xFFFF) == 0) {
                AddCodeLine("DECT R3");
                AddCodeLine("CLR *R3");
            } else {
                Assign(1, val);
                RUse(1);
                AddCodeLine("DECT R3");
                AddCodeLine("MOV R1,*R3");
            }
            val >>= 16;
            if ((val & 0xFFFF) == 0) {
                AddCodeLine("DECT R3");
                AddCodeLine("CLR *R3");
            } else {
                Assign(1, val);
                RUse(1);
                AddCodeLine("DECT R3");
                AddCodeLine("MOV R1,*R3");
            }
        }
    } else {
        RUse(0);
        RUse(1);
        AddCodeLine("DECT R3");
        AddCodeLine("MOV R1,*R3");
        AddCodeLine("DECT R3");
        AddCodeLine("MOV R0,*R3");
    }
    push(flags);
}

void g_swap (unsigned flags)
/* Swap the primary register and the top of the stack. flags give the type
** of *both* values (must have same size).
*/
{
    NotViaX();
    unsigned int rh;
    unsigned int rl = RResolve(1);
    
    RSetLive(2);

    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            AddCodeLine("MOV %s, R2", TopRegName());
            AddCodeLine("MOV R%d, %s", rl, TopRegName());
            MoveReg(2, 1);
            break;
        case CF_LONG:
            rh = RResolve(0);
            AddCodeLine("MOV %s, R2", TopRegName());
            AddCodeLine("MOV R%d, %s", rh, TopRegName());
            MoveReg(2, 0);
            AddCodeLine("MOV %s, R2", StkRegName(2));
            AddCodeLine("MOV R%d, %s", rl, StkRegName(2));
            MoveReg(2, 1);
            break;
        default:
            typeerror (flags);

    }
}

/* Flags tells us if it is varargs, ArgSize is the number of *extra* bytes
   pushed for the ... */
void g_call (unsigned Flags, const char* Label, int ArgSize)
/* Call the specified subroutine name */
{
    RCrystalizeCall();
    ResolveStack();
    if ((Flags & CF_FIXARGC) == 0)
        Assign(0, ArgSize);
    AddCodeLine("BL _%s", Label);
}

/* Flags tells us if it is varargs, ArgSize is the number of *extra* bytes
   pushed for the ... */
void g_callind (unsigned Flags, int Offs, int ArgSize)
/* Call subroutine indirect */
{
    RCrystalizeCall();
    ResolveStack();
    if ((Flags & CF_LOCAL) == 0) {
        /* Address is in R1 */
        RUse(1);
        if ((Flags & CF_FIXARGC) == 0)
            Assign(0, ArgSize);
        AddCodeLine ("BL R1");
    } else {
        unsigned int r;
        Offs = GenOffset(Offs, &r);
        if ((Flags & CF_FIXARGC) == 0)
            Assign(0, ArgSize);
        /* FIXME: func call from vararg function - and check 680x is correct */
        AddCodeLine("MOV %s, R2", IndirectForm(r, Offs));
        AddCodeLine("BL R2");
    }
}

void g_jump (unsigned Label)
/* Jump to specified internal label number */
{
    RSetUnknown(2);
    RCrystalize();
    ResolveStack();
    AddCodeLine ("B @%s", LocalLabelName (Label));
}

/* Used for jumping to the function end on a return. It's the one case we
   know we can flush the pipes */
void g_jump_nofix (unsigned Label)
/* Jump to specified internal label number */
{
    /* Return value */
    RUse(0);
    RUse(1);
    FlushStack();
    AddCodeLine ("B @%s", LocalLabelName (Label));
}

/* FIXME: we may want a version that is half and half - dump unstacked
   temporaries but not globals. We can then use that for loop logic */

void g_truejump (unsigned flags attribute ((unused)), unsigned label)
/* Jump to label if zero flag clear */
{
    /* Think we can avoid crystalizing 2 ? */
    RSetUnknown(2);
    RCrystalize();
    ResolveStack();
    /* We may have destroyed the status flag in resolving and the like */
    if (flagsvalid == 0)
        g_test_eq(flagstype);
    AddCodeLine ("LJNE %s", LocalLabelName (label));
}

void g_falsejump (unsigned flags attribute ((unused)), unsigned label)
/* Jump to label if zero flag set */
{
    /* Think we can avoid crystalizing 2 ? */
    RSetUnknown(2);
    RCrystalize();
    ResolveStack();
    /* We may have destroyed the status flag in resolving and the like */
    if (flagsvalid == 0)
        g_test_eq(flagstype);
    AddCodeLine ("LJEQ %s", LocalLabelName (label));
}

void g_lateadjustSP (unsigned label)
/* Adjust stack based on non-immediate data */
{
    /* Think we can avoid crystalizing 2 (maybe 0 and 1) ? */
    RSetUnknown(2);
    RCrystalize();
    ResolveStack();	/* Ick */
    AddCodeLine("A @%s,R3", LocalLabelName(label));
}

void g_drop (unsigned Space)
/* Drop space allocated on the stack */
{
    ShrinkStack(Space);
}

void g_space (int Space)
/* Create or drop space on the stack */
{
    if (Space < 0)
        g_drop(-Space);
    else
        GrowStack(Space);
}

void g_cstackcheck (void)
/* Check for a C stack overflow */
{
    AddCodeLine ("BL cstkchk");
}

void g_stackcheck (void)
/* Check for a stack overflow */
{
    AddCodeLine ("BL stkchk");
}

void g_add (unsigned flags, unsigned long val)
/* Primary = TOS + Primary */
{
    unsigned L;
    unsigned r;
    /* This will work much better once we add the virtual reg stack */
    static const char* const ops[4] = {
        "tosaddax", "tosaddax", "tosaddeax", "tosaddeax"
    };

    r = (flags & CF_USINGX) ? 2 : 1;

    if (flags & CF_CONST) {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                if (RIsConstant(r))
                    RSetConstant(r, RGetConstant(r) + val);
                else {
                    RUse(r);
                    AddCodeLine("AI R%d, %d", r, (uint16_t)val);
                }
                break;
            case CF_LONG:
                RUse(0);
                RUse(r);
                if (val & 0xFFFF) {
                    L = GetLocalLabel();
                    AddCodeLine("AI R%d, %d", r, (uint16_t)val);
                    AddCodeLine("JNC %s", LocalLabelName(L));
                    AddCodeLine("INC R0");
                    g_defcodelabel(L);
                }
                if (val >> 16)
                    AddCodeLine("AI R0, %d", (uint16_t)(val >> 16));
                break;
        }
    } else {
        /* We can optimize some of these in terms of ,x */
        RUse(r);
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                AddCodeLine("A %s, R%d", PopRegName(), r);
                return;
            case CF_LONG:
                RUse(0);
                L = GetLocalLabel();
                AddCodeLine("A %s, R%d", StkRegName(2), r);
                AddCodeLine("JNC %s", LocalLabelName(L));
                AddCodeLine("INC R0");
                g_defcodelabel(L);
                AddCodeLine("A %s, R0", PopRegName());
                return;
                
        }
        oper (flags, val, ops);
    }
}

void g_sub (unsigned flags, unsigned long val)
/* Primary = TOS - Primary */
{
    static const char* const ops[4] = {
        "tossubax", "tossubax", "tossubeax", "tossubeax"
    };

    NotViaX();
    if (flags & CF_CONST) {
        /* This shouldn't ever happen as the compiler will turn constant
           subtracts in this form into something else */
        flags &= ~CF_FORCECHAR; /* Handle chars as ints */
        g_push (flags & ~CF_CONST, 0);
    }
    /* It would be nice to spot these higher up and turn them from TOS - Primary
       into negate and add */
    RUse(1);
    switch(flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            AddCodeLine("S R1, %s", TopRegName());
            AddCodeLine("MOV %s, R1", PopRegName());
            return;
        /* Long subtract is hard because of the carry weirdness */
    }
    oper (flags, val, ops);
}

void g_rsub (unsigned flags, unsigned long val)
/* Primary = Primary - TOS */
{
    static const char* const ops[4] = {
        "tosrsubax", "tosrsubax", "tosrsubeax", "tosrsubeax"
    };
    unsigned int r = (flags & CF_USINGX) ? 2 : 1;

    RUse(r);
    if (flags & CF_CONST) {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                AddImmediate(r, -val);
                break;
            case CF_LONG:
                NotViaX();
                flags &= CF_CONST;
                g_push(flags & ~CF_CONST, 0);
                oper(flags, val, ops);
                break;
        }
    } else {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                AddCodeLine("S %s, R%d", PopRegName(), r);
                return;
        }
        NotViaX();
        oper (flags, val, ops);
    }
}

void g_mul (unsigned flags, unsigned long val)
/* Primary = TOS * Primary */
{
    static const char* const ops[4] = {
        "tosmulax", "tosumulax", "tosmuleax", "tosumuleax"
    };

    int p2;

    NotViaX();
    /* Do strength reduction if the value is constant and a power of two */
    if (flags & CF_CONST && (p2 = PowerOf2 (val)) >= 0) {
        /* Generate a shift instead */
        g_asl (flags, p2);
        return;
    }

    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                Assign(0, val);
                RUse(0);
                RUse(1);
                AddCodeLine("MPY R1, R0");	/* Result ends up in R0/R1 */
                return;

            case CF_LONG:
                break;

            default:
                typeerror (flags);
        }

        /* If we go here, we didn't emit code. Push the lhs on stack and fall
        ** into the normal, non-optimized stuff.
        */
        flags &= ~CF_FORCECHAR; /* Handle chars as ints */
        g_push (flags & ~CF_CONST, 0);

    } else {
            NotViaX();
            /* TOS / Primary */
            switch(flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                RUse(0);
                RUse(1);
                /* R0 x R1 into R0/R1 */
                AddCodeLine("MOV %s, R0", PopRegName());
                AddCodeLine("MPY R1, R0");
                return;
           }
    }
    /* Use long way over the stack */
    oper (flags, val, ops);
}



void g_div (unsigned flags, unsigned long val)
/* Primary = TOS / Primary */
{
    unsigned L;
    static const char* const ops[4] = {
        "tosdivax", "tosudivax", "tosdiveax", "tosudiveax"
    };

    /* Do strength reduction if the value is constant and a power of two */
    int p2;

    /* Constant is R0/1 / const */
    if ((flags & CF_CONST) && (p2 = PowerOf2 (val)) >= 0) {
        /* Generate a shift instead */
        g_asr (flags, p2);
    } else {
        /* Generate a division */
        if (flags & CF_CONST) {
            switch(flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                L = MakeLiteral(val);
                Assign(0, 0);
                RUse(0);
                RUse(1);
                if ((flags & CF_UNSIGNED) == 0)
                    /* We have to use a literal for this */
                    AddCodeLine("DIVS @%s", LocalLabelName(L));
                else
                    AddCodeLine("DIV @%s, R0", LocalLabelName(L));
               /* Quotient is the bit we want */
                MoveReg(0, 1);
                return;
            default:
                /* lhs is not on stack */
                flags &= ~CF_FORCECHAR;     /* Handle chars as ints */
                g_push (flags & ~CF_CONST, 0);
            }
        } else {
            NotViaX();
            /* TOS / Primary */
            switch(flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                /* Some shuffling needed. We need R0/R1 to be the base value
                   and we need the existing R1 somewhere */
                MoveReg(1, 2);
                AddCodeLine("MOV %s, R1", PopRegName());
                Assign(0, 0);
                RUse(0);
                RUse(1);
                RUse(2);
                if ((flags & CF_UNSIGNED) == 0)
                    AddCodeLine("DIVS R2");
                else
                    AddCodeLine("DIV R0, R2");
                MoveReg(0, 1);
                return;
            }
        }
        oper (flags, val, ops);
    }
}

void g_mod (unsigned flags, unsigned long val)
/* Primary = TOS % Primary */
{
    static const char* const ops[4] = {
        "tosmodax", "tosumodax", "tosmodeax", "tosumodeax"
    };
    int p2;
    unsigned L;

    NotViaX();
    /* Check if we can do some cost reduction */
    if ((flags & CF_CONST) && (flags & CF_UNSIGNED) && val != 0xFFFFFFFF && (p2 = PowerOf2 (val)) >= 0) {
        /* We can do that with an AND operation */
        g_and (flags, val - 1);
    } else {
        /* Generate a division */
        if (flags & CF_CONST) {
            switch(flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                L = MakeLiteral(val);
                Assign(0, 0);
                RUse(0);
                RUse(1);
                if ((flags & CF_UNSIGNED) == 0)
                    /* We have to use a literal for this */
                    AddCodeLine("DIVS @%s", LocalLabelName(L));
                else
                    AddCodeLine("DIV @%s, R0", LocalLabelName(L));
               /* Remainder is in R1 */
                return;
            /* Fall through */
            default:
                /* lhs is not on stack */
                flags &= ~CF_FORCECHAR;     /* Handle chars as ints */
                g_push (flags & ~CF_CONST, 0);
            }
        } else {
            NotViaX();
            /* TOS / Primary */
            switch(flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                /* Some shuffling needed. We need R0/R1 to be the base value
                   and we need the existing R1 somewhere */
                MoveReg(1, 2);
                Assign(0, 0);
                RUse(2);
                RUse(0);
                RUse(1);
                AddCodeLine("MOV %s, R1", PopRegName());
                if ((flags & CF_UNSIGNED) == 0)
                    AddCodeLine("DIVS R2");
                else
                    AddCodeLine("DIV R0, R2");
                /* Remainder is in R1 */
                return;
            }
        }
        oper (flags, val, ops);
    }
}

void g_or (unsigned flags, unsigned long val)
/* Primary = TOS | Primary */
{
    NotViaX();	/* FIXME */
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                val &= 0xFFFF;
            case CF_LONG:
                if (val & 0xFFFF) {
                    RUse(1); 
                    AddCodeLine("ORI R1, %d", (uint16_t)val);
                }
                if (val >> 16) {
                    RUse(0);
                    AddCodeLine("ORI R0, %d", (uint16_t)(val >> 16));
                }
                break;

            default:
                typeerror (flags);
        }
        return;
    }

    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            RUse(1);
            AddCodeLine("OR %s, R1", PopRegName());
            return;
        case CF_LONG:
            RUse(0);
            RUse(1);
            AddCodeLine("OR %s, R0", PopRegName());
            AddCodeLine("OR %s, R1", PopRegName());
            return;
    }
}

/*
 *	XOR has no immediate form
 */
void g_xor (unsigned flags, unsigned long val)
/* Primary = TOS ^ Primary */
{
    NotViaX();

    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        switch (flags & CF_TYPEMASK) {

            case CF_CHAR:
            case CF_INT:
                if (val & 0xFFFF) {
                    Assign(2, val);
                    RUse(1);
                    RUse(2);
                    AddCodeLine("XOR R2, R1");
                }
                return;
            case CF_LONG:
                /* There are optimisations on these for near values using
                   INC/DEC/INCT/DECT - needs a helper for OR/XOR/AND etc */
                if (WordsSame(val)) {
                    Assign(2, val);
                    RUse(2);
                    RUse(0);
                    RUse(1);
                    AddCodeLine("XOR R2, R1");
                    AddCodeLine("XOR R2, R0");
                } else {
                    if (val & 0xFFFF) {
                        Assign(2, val);
                        RUse(2);
                        RUse(1);
                        AddCodeLine("XOR R2, R1");
                    }
                    if (val >> 16) {
                        RUse(2);
                        RUse(0);
                        Assign(2, val >> 16);
                        AddCodeLine("XOR R2, R0");
                    }
                }
                return;

            default:
                typeerror (flags);
        }
    }

    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            RUse(1);
            AddCodeLine("XOR %s, R1", PopRegName());
            return;
        case CF_LONG:
            RUse(0);
            RUse(1);
            AddCodeLine("XOR %s, R0", PopRegName());
            AddCodeLine("XOR %s, R1", PopRegName());
            return;
    }
}

void g_and (unsigned Flags, unsigned long Val)
/* Primary = TOS & Primary */
{
    NotViaX();
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (Flags & CF_CONST) {

        switch (Flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                if (Val == 0)
                    Assign(1, 0);
                else {
                    RUse(1);                    
                    AddCodeLine("ANDI R1, %d", (uint16_t)Val);
                }
                return;
            case CF_LONG:
                /* Trap the many common easy 32bit cases we can do inline */
                if (Val <= 0xFFFF) {
                    Assign(0, 0);
                    RUse(1);
                    AddCodeLine("ANDI R1, %d", (uint16_t)Val);
                    return;
                } else if (Val >= 0xFFFF0000UL) {
                    RUse(1);
                    AddCodeLine("ANDI R1, %d", (uint16_t)(Val >> 16));
                    return;
                } else if (!(Val & 0xFFFF)) {
                    AddCodeLine("ANDI R0, %d", (uint16_t)(Val >> 16));
                    AddCodeLine("CLR R1");
                    return;
                } else {
                    RUse(0);
                    RUse(1);
                    AddCodeLine("ANDI R0, %d", (uint16_t)(Val >> 16));
                    AddCodeLine("ANDI R1, %d", (uint16_t)Val);
                }
                break;
            default:
                typeerror (Flags);
        }
    }
    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            RUse(1);
            AddCodeLine("AND %s, R1", PopRegName());
            return;
        case CF_LONG:
            RUse(0);
            RUse(1);
            AddCodeLine("AND %s, R0", PopRegName());
            AddCodeLine("AND %s, R1", PopRegName());
            return;
    }
}

void g_asr (unsigned flags, unsigned long val)
/* Primary = TOS >> Primary */
{
    unsigned L, L2, L3;
    static const char* const ops[4] = {
        "tosasrax", "tosshrax", "tosasreax", "tosshreax"
    };

    NotViaX();

    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                /* C lets us do this however we like - be nice */
                if (val > 15)
                    Assign(1, 0);
                else if (flags & CF_UNSIGNED) {
                    RUse(1);
                    AddCodeLine("SRL R1, %d", (int)(val & 15)); 
                } else {
                    RUse(1);
                    AddCodeLine("SRA R1, %d", (int)(val & 15));
                }
                return;
            case CF_LONG:
                if (val == 16 && (flags & CF_UNSIGNED)) {
                    MoveReg(0, 1);
                    Assign(0, 0);
                    return;
                }
                if (val > 16 && (flags & CF_UNSIGNED)) {
                    MoveReg(0, 1);
                    Assign(0, val - 16);
                    RUse(0);
                    RUse(1);
                    AddCodeLine("SRL R1, R0");
                    Assign(0, 0);
                    return;
                }
                /* Need to be sure we can use R2 here.. */
                L = GetLocalLabel();
                L2 = GetLocalLabel();
                L3 = GetLocalLabel();
                Assign(2, val & 31);
                RUse(0);
                RUse(1);
                RUse(2);
                g_defcodelabel(L);
                if (flags & CF_UNSIGNED)
                    AddCodeLine("SRL R0");
                else
                    AddCodeLine("SRA R0");
                AddCodeLine("JOC %s", LocalLabelName(L2));
                AddCodeLine("SRL R1");
                AddCodeLine("JMP %s", LocalLabelName(L3));
                g_defcodelabel(L2);
                AddCodeLine("SRL R1");
                AddCodeLine("ORI R1,0x8000");
                g_defcodelabel(L3);
                AddCodeLine("DEC R2");
                AddCodeLine("JNE %s", LocalLabelName(L));
                return;

            default:
                typeerror (flags);
        }

        /* If we go here, we didn't emit code. Push the lhs on stack and fall
        ** into the normal, non-optimized stuff. Note: The standard stuff will
        ** always work with ints.
        */
        flags &= ~CF_FORCECHAR;
         g_push (flags & ~CF_CONST, 0);
    } else {
        switch(flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                MoveReg(1, 0);
                RSetLive(1);
                AddCodeLine("MOV %s, R1", PopRegName());
                RUse(0);
                if (flags & CF_UNSIGNED)
                    AddCodeLine("SRL R1, R0");
                else
                    AddCodeLine("SRA R1, R0");
                return;
        }
    }
    /* Use long way over the stack */
    oper (flags, val, ops);
}

void g_asl (unsigned flags, unsigned long val)
/* Primary = TOS << Primary */
{
    unsigned L, L2, L3;
    static const char* const ops[4] = {
        "tosaslax", "tosshlax", "tosasleax", "tosshleax"
    };

    NotViaX();

    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                /* C lets us do this however we like - be nice */
                if (val > 15)
                    Assign(1, 0);
                else {
                    RUse(1);
                    AddCodeLine("SLA R1, %d", (uint16_t)val);
                }
                return;

            case CF_LONG:
                if (val == 16) {
                    MoveReg(1, 0);
                    Assign(1, 0);
                    return;
                }
                if (val > 16) {
                    Assign(0, val - 16);
                    RUse(0);
                    RUse(1);
                    AddCodeLine("SLA R1, R0");
                    AddCodeLine("MOV R1, R0");
                    Assign(1, 0);
                    return;
                }
                /* The hard way */
                /* Need to be sure we can use R2 here.. */
                L = GetLocalLabel();
                L2 = GetLocalLabel();
                L3 = GetLocalLabel();
                Assign(2, val & 31);
                g_defcodelabel(L);
                RUse(2);
                RUse(0);
                RUse(1);
                AddCodeLine("SLA R1");
                AddCodeLine("JOC %s", LocalLabelName(L2));
                AddCodeLine("SLA R0");
                AddCodeLine("JMP %s", LocalLabelName(L3));
                g_defcodelabel(L2);
                AddCodeLine("SLA R0");
                AddCodeLine("INC R0");
                g_defcodelabel(L3);
                AddCodeLine("DEC R2");
                AddCodeLine("JNE %s", LocalLabelName(L));
                return;

            default:
                typeerror (flags);
        }

        /* If we go here, we didn't emit code. Push the lhs on stack and fall
        ** into the normal, non-optimized stuff. Note: The standard stuff will
        ** always work with ints.
        */
        flags &= ~CF_FORCECHAR;
        g_push (flags & ~CF_CONST, 0);
    } else {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                MoveReg(1, 0);
                RSetLive(1);
                AddCodeLine("MOV %s, R1", PopRegName());
                RUse(0);
                AddCodeLine("SLA R1, R0");
                return;
        }
    }
    /* Use long way over the stack */
    oper (flags, val, ops);
}


void g_neg (unsigned Flags)
/* Primary = -Primary */
{
    unsigned L;

    NotViaX();

    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            RUse(1);
            AddCodeLine("NEG R1");
            break;
        case CF_LONG:
            RUse(0);
            RUse(1);
            L = GetLocalLabel();
            AddCodeLine ("INV R0");
            AddCodeLine ("INV R1");
            AddCodeLine ("INC R1");
            AddCodeLine ("JNC @%s", LocalLabelName(L));
            AddCodeLine ("INC R0");
            g_defcodelabel(L);
            break;
        default:
            typeerror (Flags);
    }
}

/*
 *	Shortest form I can think of at the moment as we have no carry
 *	based operations.
 */
void g_bneg (unsigned flags)
/* Primary = !Primary */
{
    unsigned L;

    NotViaX();
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
            RUse(1);
            L = GetLocalLabel();
            /* FIXME: is this needed */
            AddCodeLine ("ANDI R1,0xFF"); /* Test for zero, short way */
            AddCodeLine ("CLR R1");	/* Doesn't touch flags */
            AddCodeLine ("JNE %s", LocalLabelName(L));
            AddCodeLine ("INC R1");	/* Was zero so make one */
            g_defcodelabel(L);
            break;

        case CF_INT:
            RUse(1);
            L = GetLocalLabel();
            AddCodeLine ("OR R1,R1");	/* Test for zero, short way */
            AddCodeLine ("CLR R1");	/* Doesn't touch flags */
            AddCodeLine ("JNE %s", LocalLabelName(L));
            AddCodeLine ("INC R1");	/* Was zero so make one */
            g_defcodelabel(L);
            break;

        case CF_LONG:
            RUse(0);
            RUse(1);
            L = GetLocalLabel();
            AddCodeLine ("OR R0,R1");	/* Test for zero, short way */
            AddCodeLine ("CLR R1");	/* Doesn't touch flags */
            AddCodeLine ("JNE %s", LocalLabelName(L));
            AddCodeLine ("INC R1");	/* Was zero so make one */
            g_defcodelabel(L);
            break;

        default:
            typeerror (flags);
    }
}

void g_com (unsigned Flags)
/* Primary = ~Primary */
{
    NotViaX();
    switch (Flags & CF_TYPEMASK) {

        case CF_CHAR:
        case CF_INT:
            RUse(1);
            AddCodeLine("INV R1");
            break;

        case CF_LONG:
            RUse(0);
            RUse(1);
            AddCodeLine("INV R0");
            AddCodeLine("INV R1");
            break;

        default:
            typeerror (Flags);
    }
}

void g_inc (unsigned flags, unsigned long val)
/* Increment the primary register by a given number */
{
    int r;

    /* Don't inc by zero */
    if (val == 0)
        return;

    r = 1;
    if (flags & CF_USINGX)
        r = 2;
    /* Generate code for the supported types */
    flags &= ~CF_CONST;
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            AddImmediate(r, val);
            break;
        case CF_LONG:
            NotViaX();
            g_add (flags | CF_CONST, val);
            break;

        default:
            typeerror (flags);

    }
}



void g_dec (unsigned flags, unsigned long val)
/* Decrement the primary register by a given number */
{
    /* There are no special cases for inc v dec on this CPU so ... */
    g_inc (flags, -val);
}

/*
** Following are the conditional operators. They compare the TOS against
** the primary and put a literal 1 in the primary if the condition is
** true, otherwise they clear the primary register
*/

void g_eq (unsigned flags, unsigned long val)
/* Test for equal */
{
    unsigned L;

    NotViaX();
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                RUse(1);
                AddCodeLine("CI R1, %d", (uint16_t)val);
                AddCodeLine ("BL booleq");
                return;

            case CF_LONG:
                RUse(1);
                RUse(0);
                L = GetLocalLabel();
                AddCodeLine("CI R1, %d", (uint16_t)val);
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("CI R0, %d", (uint16_t)(val >> 16));
                g_defcodelabel(L);
                AddCodeLine("BL booleq");
                return;

            default:
                typeerror (flags);
        }

        /* If we go here, we didn't emit code. Push the lhs on stack and fall
        ** into the normal, non-optimized stuff. Note: The standard stuff will
        ** always work with ints.
        */
        flags &= ~CF_FORCECHAR;
        g_push (flags & ~CF_CONST, 0);
    }
    /* We can optimize some of these in terms of subd ,x */
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            RUse(1);
            AddCodeLine("C R1, %s", PopRegName());
            AddCodeLine ("BL booleq");
            return;
        case CF_LONG:
            RUse(0);
            RUse(1);
            L = GetLocalLabel();
            AddCodeLine("CI %s,R0", PopRegName());
            AddCodeLine("JNE %s", LocalLabelName(L));
            AddCodeLine("CI %s, R1", TopRegName());
            g_defcodelabel(L);
            AddCodeLine("BL booleq");
            DropRegStack();
            return;
        default:
            typeerror (flags);
    }
}

void g_ne (unsigned flags, unsigned long val)
/* Test for not equal */
{
    unsigned L;

    /* OPTIMIZE look at x for NULL case only */
    NotViaX();
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        switch (flags & CF_TYPEMASK) {

            case CF_CHAR:
            case CF_INT:
                RUse(1);
                AddCodeLine("CI R1, %d", (uint16_t)val);
                AddCodeLine ("BL boolne");
                return;

            case CF_LONG:
                RUse(0);
                RUse(1);
                L = GetLocalLabel();
                AddCodeLine("CI R1, %d", (uint16_t)val);
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("CI R0, %d", (uint16_t)(val >> 16));
                g_defcodelabel(L);
                AddCodeLine("BL boolne");
                return;

            default:
                typeerror (flags);
        }
    }

    /* We can optimize some of these in terms of subd ,x */
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            RUse(1);
            AddCodeLine("C %s, R1", PopRegName());
            AddCodeLine ("BL boolne");
            return;
        case CF_LONG:
            RUse(0);
            RUse(1);
            L = GetLocalLabel();
            AddCodeLine("CI %s, R0", PopRegName());
            AddCodeLine("JNE %s", LocalLabelName(L));
            AddCodeLine("CI %s, R1", TopRegName());
            g_defcodelabel(L);
            AddCodeLine("BL boolne");
            DropRegStack();
            return;
        default:
            typeerror (flags);
    }
}

void g_lt (unsigned flags, unsigned long val)
/* Test for less than */
{
    char *f = "boolult";
    unsigned L;

    NotViaX();
    
    if (flags & CF_UNSIGNED)
        f = "boollt";

    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        /* Give a warning in some special cases */
        if (val == 0) {
            Warning ("Condition is never true");
            Assign(1, 0);
            return;
        }

        /* Look at the type */
        RUse(1);
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                if (flags & CF_FORCECHAR)
                    AddCodeLine ("ANDI R1,0xFF");
            case CF_INT:
                AddCodeLine("CI R1, %d", (uint16_t)val);
                AddCodeLine ("BL %s", f);
                return;
            case CF_LONG:
                RUse(0);
                L = GetLocalLabel();
                AddCodeLine ("CI R0, %d", (uint16_t)(val >> 16));
                AddCodeLine ("JNE %s", LocalLabelName(L));
                AddCodeLine ("CI R1, %d", (uint16_t)val);
                g_defcodelabel(L);
                AddCodeLine ("BL %s", f);
                return;
            default:
                typeerror (flags);
        }
    } else {
        RUse(1);
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                if (flags & CF_FORCECHAR)
                    AddCodeLine ("ANDI R1,0xFF");
            case CF_INT:
                AddCodeLine("C %s, R1", PopRegName());
                AddCodeLine ("BL %s", f);
                return;
            case CF_LONG:
                RUse(0);
                L = GetLocalLabel();
                AddCodeLine("C %s, R0", PopRegName());
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("C %s, R1", TopRegName());
                g_defcodelabel(L);
                AddCodeLine("BL %s", f);
                DropRegStack();
                return;
        }
    }
}

void g_le (unsigned flags, unsigned long val)
/* Test for less than or equal to */
{
    unsigned L;

    NotViaX();
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        /* Look at the type */
        switch (flags & CF_TYPEMASK) {

            case CF_CHAR:
                if (val > 0xFF) {
                    Warning("Condition is always true");
                    Assign(1, 1);
                    return;
                } 
                RUse(1);
                if (flags & CF_FORCECHAR)
                    AddCodeLine("ANDI R1, 0xFF");
            case CF_INT:
                if (flags & CF_UNSIGNED) {
                    /* Unsigned compare */
                    if (val < 0xFFFF) {
                        /* Use < instead of <= because the former gives
                        ** better code on the 6502 than the latter.
                        */
                        g_lt (flags, val+1);
                    } else {
                        /* Always true */
                        Warning ("Condition is always true");
                        Assign(1, 1);
                    }
                } else {
                    /* Signed compare */
                    if ((long) val < 0x7FFF) {
                        g_lt (flags, val+1);
                    } else {
                        /* Always true */
                        Warning ("Condition is always true");
                        /* FIXME: review the return1 cases on 680x */
                        Assign(1, 1);
                    }
                }
                return;

            case CF_LONG:
                if (flags & CF_UNSIGNED) {
                    /* Unsigned compare */
                    if (val < 0xFFFFFFFF) {
                        /* Use < instead of <= because the former gives
                        ** better code on the 6502 than the latter.
                        */
                        g_lt (flags, val+1);
                    } else {
                        /* Always true */
                        Warning ("Condition is always true");
                        Assign(1, 1);
                    }
                } else {
                    /* Signed compare */
                    if ((long) val < 0x7FFFFFFF) {
                        g_lt (flags, val+1);
                    } else {
                        /* Always true */
                        Warning ("Condition is always true");
                        Assign(1, 1);
                    }
                }
                return;

            default:
                typeerror (flags);
        }

        /* If we go here, we didn't emit code. Push the lhs on stack and fall
        ** into the normal, non-optimized stuff. Note: The standard stuff will
        ** always work with ints.
        */
        flags &= ~CF_FORCECHAR;
        g_push (flags & ~CF_CONST, 0);
    } else {
        RUse(1);
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                AddCodeLine("ANDI R1, 0xFF");
            case CF_INT:
                AddCodeLine("C %s, R1", PopRegName());
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL boolule");
                else
                    AddCodeLine ("BL boolle");
                return;
            case CF_LONG:
                RUse(0);
                L = GetLocalLabel();
                AddCodeLine("C %s, R0", PopRegName());
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("C %s, R1", TopRegName());
                g_defcodelabel(L);
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL boolule");
                else
                    AddCodeLine ("BL boolle");
                DropRegStack();
                return;
        }
    }
}

void g_gt (unsigned flags, unsigned long val)
/* Test for greater than */
{
    unsigned L;
    NotViaX();
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        /* Look at the type */
        switch (flags & CF_TYPEMASK) {

            case CF_CHAR:
                RUse(1);
                if (flags & CF_FORCECHAR)
                    AddCodeLine("ANDI R1,0xFF");
                /* FALLTHROUGH */
            case CF_INT:
                if (flags & CF_UNSIGNED) {
                    /* Unsigned compare */
                    if (val == 0) {
                        /* If we have a compare > 0, we will replace it by
                        ** != 0 here, since both are identical but the latter
                        ** is easier to optimize.
                        */
                        g_ne (flags, val);
                    } else if (val < 0xFFFF) {
                        /* Use >= instead of > because the former gives better
                        ** code on the 6502 than the latter.
                        */
                        g_ge (flags, val+1);
                    } else {
                        /* Never true */
                        Warning ("Condition is never true");
                        Assign(1, 0);
                    }
                } else {
                    /* Signed compare */
                    if ((long) val < 0x7FFF) {
                        g_ge (flags, val+1);
                    } else {
                        /* Never true */
                        Warning ("Condition is never true");
                        Assign(1, 0);
                    }
                }
                return;

            case CF_LONG:
                if (flags & CF_UNSIGNED) {
                    /* Unsigned compare */
                    if (val == 0) {
                        /* If we have a compare > 0, we will replace it by
                        ** != 0 here, since both are identical but the latter
                        ** is easier to optimize.
                        */
                        g_ne (flags, val);
                    } else if (val < 0xFFFFFFFF) {
                        /* Use >= instead of > because the former gives better
                        ** code on the 6502 than the latter.
                        */
                        g_ge (flags, val+1);
                    } else {
                        /* Never true */
                        Warning ("Condition is never true");
                        Assign(1, 0);
                    }
                } else {
                    /* Signed compare */
                    if ((long) val < 0x7FFFFFFF) {
                        g_ge (flags, val+1);
                    } else {
                        /* Never true */
                        Warning ("Condition is never true");
                        Assign(1, 0);
                    }
                }
                return;

            default:
                typeerror (flags);
        }
    } else {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                if (flags & CF_FORCECHAR) {
                    RUse(1);
                    AddCodeLine("AND R1, 0xFF");
                }
            case CF_INT:
                RUse(1);
                AddCodeLine("C %s, R1", PopRegName());
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL boolugt");
                else
                    AddCodeLine ("BL boolgt");
                return;
            case CF_LONG:
                RUse(0);
                L = GetLocalLabel();
                AddCodeLine("C %s, R0", PopRegName());
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("C %s, R1", TopRegName());
                g_defcodelabel(L);
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL boolugt");
                else
                    AddCodeLine ("BL boolgt");
                DropRegStack();
                return;
            default:
                typeerror (flags);
        }
    }
}

void g_ge (unsigned flags, unsigned long val)
/* Test for greater than or equal to */
{
    unsigned L;
    NotViaX();

    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {
        /* Give a warning in some special cases */
        if (val == 0 && (flags & CF_UNSIGNED)) {
            Warning ("Condition is always true");
            Assign(1, 1);
            return;
        }

        /* Look at the type */
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                RUse(1);
                if (flags & CF_FORCECHAR)
                    AddCodeLine("ANDI R1, 0xFF");
            case CF_INT:
                RUse(1);
                AddCodeLine("CI R1, %d", (uint16_t)val);
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL booluge");
                else
                    AddCodeLine ("BL boolge");
                return;
            case CF_LONG:
                RUse(0);
                L = GetLocalLabel();
                AddCodeLine ("CI R0, %d", (uint16_t)val >> 16);
                AddCodeLine ("JNE %s", LocalLabelName(L));
                AddCodeLine ("CI R1, %d", (uint16_t)val);
                g_defcodelabel(L);
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL booluge");
                else
                    AddCodeLine ("BL boolge");
                return;
            default:
                typeerror (flags);
        }
    } else {
        RUse(1);
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                if (flags & CF_FORCECHAR)
                    /* FIXME probably should do this to both sides ? */
                    AddCodeLine("ANDI R1, 0xFF");
            case CF_INT:
                AddCodeLine("C %s,R1", PopRegName());
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL booluge");
                else
                    AddCodeLine ("BL boolge");
                return;
            case CF_LONG:
                RUse(0);
                L = GetLocalLabel();
                AddCodeLine("C %s, R0", PopRegName());
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("C %s, R1", TopRegName());
                g_defcodelabel(L);
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL booluge");
                else
                    AddCodeLine ("BL boolge");
                DropRegStack();
                return;
            default:
                typeerror (flags);
        }
    }
}

/*****************************************************************************/
/*                         Allocating static storage                         */
/*****************************************************************************/


void g_res (unsigned n)
/* Reserve static storage, n bytes */
{
    AddDataLine ("\t.ds\t%u", n);
}

void g_defdata (unsigned flags, unsigned long val, long offs)
/* Define data with the size given in flags */
{
    if (flags & CF_CONST) {

        /* Numeric constant */
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                AddDataLine ("\t.byte\t$%02lX", val & 0xFF);
                break;

            case CF_INT:
                AddDataLine ("\t.word\t$%04lX", val & 0xFFFF);
                break;

            case CF_LONG:
                AddDataLine ("\t.word\t$%04lX", (val >> 16) & 0xFFFF);
                AddDataLine ("\t.word\t$%04lX", val & 0xFFFF);
                break;

            default:
                typeerror (flags);
                break;

        }

    } else {
        /* Create the correct label name */
        const char* Label = GetLabelName (flags, val, offs);
        /* Labels are always 16 bit */
        AddDataLine ("\t.word\t%s", Label);
    }
}

void g_defbytes (const void* Bytes, unsigned Count)
/* Output a row of bytes as a constant */
{
    unsigned Chunk;
    char Buf [128];
    char* B;

    /* Cast the buffer pointer */
    const unsigned char* Data = (const unsigned char*) Bytes;

    /* Output the stuff */
    while (Count) {

        /* How many go into this line? */
        if ((Chunk = Count) > 16) {
            Chunk = 16;
        }
        Count -= Chunk;

        /* Output one line */
        strcpy (Buf, "\t.byte\t");
        B = Buf + 7;
        do {
            B += sprintf (B, "$%02X", *Data++);
            if (--Chunk) {
                *B++ = ',';
            }
        } while (Chunk);

        /* Output the line */
        AddDataLine ("%s", Buf);
    }
}

void g_zerobytes (unsigned Count)
/* Output Count bytes of data initialized with zero */
{
    if (Count > 0) {
        AddDataLine ("\t.dst%u", Count);
    }
}

void g_initregister (unsigned Label, unsigned Reg, unsigned Size)
/* Initialize a register variable from static initialization data */
{
    AddCodeLine("FIXME");
}

/*
 *	Make the stack space and fill it. As we keep any variable on
 *	the stack word aligned this is safe I think ;)
 */
void g_initauto (unsigned Label, unsigned Size)
{
    unsigned CodeLabel = GetLocalLabel ();

    /* FIXME: for short blocks inline ? */
    RSetLive(2);
    AddCodeLine("LI R2, %s", GetLabelName (CF_STATIC, Label, 0));
    Assign(1, Size / 2);
    AddImmediate(3, -Size);
    MoveReg(3, 0);
    RUse(2);
    RUse(0);
    RUse(1);
    g_defcodelabel(CodeLabel);
    AddCodeLine("MOV *R2+,*R0+");
    AddCodeLine("DEC R1");
    AddCodeLine("JNE %s", LocalLabelName(CodeLabel));
    
    if (Size & 1)
        AddCodeLine("MOVB *R2+,*R0+");
    /* The caller will adjust the stack on the basis we made the pushes */
}

void g_initstatic (unsigned InitLabel, unsigned VarLabel, unsigned Size)
/* Initialize a static local variable from static initialization data */
{
    unsigned int n;
    if (Size <= 8) {
        while(Size > 1) {
            Size -= 2;
            AddCodeLine("MOV %s+%d, %s+%d",
                GetLabelName(CF_STATIC, InitLabel, Size), n,
                GetLabelName(CF_STATIC, VarLabel, Size), n);
                n += 2;
        }
        if (Size) {
            AddCodeLine("MOVB %s+%d, %s+%d",
                GetLabelName(CF_STATIC, InitLabel, 0), n,
                GetLabelName(CF_STATIC, VarLabel, 0), n);
        }
    } else {
        /* Use the easy way here: memcpy() */
        AddImmediate(3, -6);
        g_getimmed (CF_STATIC, VarLabel, 0);
        AddCodeLine ("MOV R1, *R3");
        g_getimmed (CF_STATIC, InitLabel, 0);
        AddCodeLine ("MOV R1, @2(R3)");
        g_getimmed (CF_INT | CF_UNSIGNED | CF_CONST, Size, 0);
        AddCodeLine ("MOV R1, @4(R3)");
        ResolveStack();
        RCrystalizeCall();
        AddCodeLine ("BL %s", GetLabelName (CF_EXTERNAL, (uintptr_t) "memcpy", 0));
        AddImmediate(3, 6);
    }
}

/* The code that follows may be moved around */
void g_moveable (void)
{
    /* I think this is sufficient */
    ResolveStack();	/* Ouch ?? needed ?? */
    RCrystalizeMovable();
}

/* For one byte registers we end up saving an extra byte. It avoids having
   to touch D so who cares. We don't allow long registers */

void g_save_regvar(int Offset, int Reg, unsigned Size)
{
    /* We may need to resolve this ? */
    Reg += REGALLOC_BASE;
    AddImmediate(3, -2);
    RUse(3);
    RUse(Reg);
    AddCodeLine("MOV R%d,@R3", Reg);
    push(CF_INT);
}

void g_swap_regvars(int offs, int reg, unsigned size)
{
    unsigned int r;
    /* Always 2 bytes, R2 is always free */
    offs = GenOffset(offs, &r);
    RSetLive(2);
    reg += REGALLOC_BASE;
    RSetLive(reg);
    AddCodeLine("MOV %s, R2", IndirectForm(r, offs));
    AddCodeLine("MOV R%d, %s", reg, IndirectForm(r, offs));
    MoveReg(2, reg);
}

void g_restore_regvar(int Offset, int Reg, unsigned Size)
{
    unsigned int r;
    if (Offset >= 0) {
        Offset = GenOffset(Offset, &r);
        AddCodeLine("MOV %s, R%d", IndirectForm(r, Offset), Reg + REGALLOC_BASE);
    }
    else
        PopReg(Reg + REGALLOC_BASE);
}

    
/*****************************************************************************/
/*                             Switch statement                              */
/*****************************************************************************/

/*
 *	TODO: rethink this in word based manner - perhaps a binary search
 *	16bit wide, and for 24/32bit a binary search for a table ptr of
 *	16bit binary search ?
 */

void g_switch (Collection* Nodes, unsigned DefaultLabel, unsigned Depth)
/* Generate code for a switch statement */
{
    unsigned NextLabel = 0;
    unsigned I;

    NotViaX();

    /* Setup registers and determine which compare insn to use */
    const char* Compare;
    switch (Depth) {
        case 1:
            Compare = "cmpb #$%02X";
            break;
        case 2:
            Compare = "cmpa #$%02X";
            break;
        case 3:
            AddCodeLine ("psha");
            AddCodeLine ("ldaa @sreg+1");
            Compare = "cmpa #$%02X";
            break;
        case 4:
            AddCodeLine ("psha");
            AddCodeLine ("ldaa @sreg");
            Compare = "cmpa #$%02X";
            break;
        default:
            Internal ("Invalid depth in g_switch: %u", Depth);
    }

    /* Walk over all nodes */
    for (I = 0; I < CollCount (Nodes); ++I) {

        /* Get the next case node */
        CaseNode* N = CollAtUnchecked (Nodes, I);

        /* If we have a next label, define it */
        if (NextLabel) {
            g_defcodelabel (NextLabel);
            NextLabel = 0;
        }

        /* Do the compare */
        AddCodeLine (Compare, CN_GetValue (N));
        if (Depth > 2)
            AddCodeLine("pula");

        /* If this is the last level, jump directly to the case code if found */
        if (Depth == 1) {

            /* Branch if equal */
            g_falsejump (0, CN_GetLabel (N));

        } else {

            /* Determine the next label */
            if (I == CollCount (Nodes) - 1) {
                /* Last node means not found */
                g_truejump (0, DefaultLabel);
            } else {
                /* Jump to the next check */
                NextLabel = GetLocalLabel ();
                g_truejump (0, NextLabel);
            }

            /* Check the next level */
            g_switch (N->Nodes, DefaultLabel, Depth-1);

        }
    }

    /* If we go here, we haven't found the label */
    g_jump (DefaultLabel);
}



/*****************************************************************************/
/*                       Optimizer Hinting                                   */
/*****************************************************************************/

void g_statement(void)
{
    /* FIXME: need to sort out how to spot the end of func return to deal
       with the 0/1 valid case.. no g_statement somehere?? */
    ResolveStack();
//    RSetUnknown(0);
//    RSetUnknown(1);
    RSetUnknown(2);
    /* Eventually we want to resolvestack where we expect it to be needed
       post local declarations and instead check here */
//    RegPoolEmpty("stmt");	/* Check for a register stack misalignment */
    AddCodeLine(";statement");
}

/*****************************************************************************/
/*                       User supplied assembler code                        */
/*****************************************************************************/



void g_asmcode (struct StrBuf* B)
/* Output one line of assembler code. */
{
    AddCodeLine ("%.*s", (int) SB_GetLen (B), SB_GetConstBuf (B));
}
