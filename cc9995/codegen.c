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



/*****************************************************************************/
/*                                  Helpers                                  */
/*****************************************************************************/



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



static const char* GetLabelName (unsigned Flags, uintptr_t Label, long Offs, int addr)
{
    static char Buf [256];              /* Label name */

    /* Create the correct label name */
    switch (Flags & CF_ADDRMASK) {

        case CF_STATIC:
            /* Static memory cell */
            if (Offs) {
                xsprintf (Buf, sizeof (Buf), "%s%+ld", LocalLabelName (Label), Offs);
            } else {
                xsprintf (Buf, sizeof (Buf), "%s", LocalLabelName (Label));
            }
            break;

        case CF_EXTERNAL:
            /* External label */
            if (Offs) {
                xsprintf (Buf, sizeof (Buf), "_%s%+ld", (char*) Label, Offs);
            } else {
                xsprintf (Buf, sizeof (Buf), "_%s", (char*) Label);
            }
            break;

        case CF_ABSOLUTE:
            /* Absolute address */
            xsprintf (Buf, sizeof (Buf), "0x%04X", (unsigned)((Label+Offs) & 0xFFFF));
            break;

        case CF_REGVAR:
            /* Variable in register bank */
            /* FIXME: just offs ? */
            xsprintf (Buf, sizeof (Buf), "R%u", (unsigned)((Label+Offs) & 0xFFFF));
            break;

        default:
            Internal ("Invalid address flags: %04X", Flags);
    }

    /* Return a pointer to the static buffer */
    return Buf;
}

/* Generate the value of an offset from the stack base. We don't have to
   worry about reach */

static int GenOffset(int Offs)
{
    /* Frame pointer argument */
    if (Offs > 0 && FramePtr)
        return Offs + 2;
    Offs -= StackPtr;
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

/* The code that follows may be moved around */
void g_moveable (void)
{
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
    if (val)
        AddCodeLine("LI R%d, %d", reg, val);
    else
        AddCodeLine("CLR R%d");
}

static void Assign32(unsigned rh, unsigned rl, unsigned int val)
{
    if (val && WordsSame(val)) {
        Assign(rh, val);
        AddCodeLine("MOV R%d,R%d", rh, rl);
    } else {
        Assign(rh, val >> 16;
        Assign(rl, val & 0xFFFF);
    }
}

/* Turn a register and offset into the correct form depending upon whether
   the offset is 0 */
static char *Indirect(unsigned int r, unsigned int offset)
{
    static char ibuf[64];
    if (offset)
        sprintf(ibuf, "@%d(R%d)", r, offset);
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
    if (Flags & CF_USINGR2)
        d = 2;

    if (r == d)
        Internal("FBInd")
    
    /* Character sized */
    if (Flags & CF_UNSIGNED) {
        AddCodeLine("CLR R%", d);
        /* FIXME: can any of these be zero (ie *Rn) */
        AddCodeLine("MOVB %s,R%d", Indirect(r, Offs), d);
        AddCodeLine("SWPB R%d", d);
    } else {
        AddCodeLine("MOVB %s,R%d", Indirect(r, Offs), d);
        AddCodeLine("SRA R%d, 8", d);
    }
}

/*
 * Fetch a byte from a label and offset for static or global variables
 */
static void FetchByteStatic(const char *lbuf, unsigned Flags)
{
    unsigned int d = 1;
    if (Flags & CF_USINGR2)
        d = 2;
    
    if (Flags & CF_UNSIGNED) {
        AddCodeLine("CLR R%d", d);
        AddCodeLine("MOVB @%s, R%d", lbuf, d);
        AddCodeLine("SWPB R%d", d);
    } else {
        AddCodeLine("MOVB @%s, R%d", lbuf, d);
        AddCodeLine("SRA R%d,8", d);
    }
}

/*
 * Store a byte to a label/offset for static or global variables
 */
static void PutByteStatic(const char *lbuf, unsigned Flags)
{
    unsigned r = 1;
    if (Flags & CF_USINGR2)
        r = 2;
    AddCodeLine("SWPB R%d", r);
    AddCodeLine("MOVB R%d, @%s", r, lbuf);
    AddCodeLine("SWPB R%d", r);
}

/*
 * Store a register to a register/offset. Used for stack and pointer
 * dereferences.
 */
static void PutByteIndirect(unsigned int r, unsigned int Offs, unsigned int Flags)
{
    unsigned int d = 1;
    if (Flags & CF_USINGR2)
        d = 2;
    
    if (r == d)
        Internal("PInd")

    AddCodeLine("SWPB R%d", d);
    AddCodeLine("MOVB R%d, %s", d, Indirect(r, Offs));
    AddCodeLine("SWPB R%d", d);
}

/*
 * Literals are needed for certain things because many instructions have
 * an indirect form but not an immediate one.
 *
 * TODO: nicer way to merge common literals.
 */

unsigned MakeLiteral(uint16_t val)
{
    unsigned L;
    L = GetLocalLabel();
    g_defdatalabel(L);
    AddDataLine(".word %d", val);
}

/*****************************************************************************/
/*                      Functions handling local labels                      */
/*****************************************************************************/

void g_defcodelabel (unsigned label)
/* Define a local code label */
{
    AddCodeLine("\t.even");
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
    AddCodeLine("_%s:",name);

    /* Uglies from the L->R stack handling for varargs */
    if ((flags & CF_FIXARGC) == 0) {
        /* Frame pointer is required for varargs mode. We could avoid
           it otherwise */
        AddCodeLine("ai R3, -8");
        AddCodeLine("mov R11,*R3");
        AddCodeLine("mov R4,@2(R3)");
        AddCodeLine("mov R3, R4");	/* R4 is FP */
        AddCodeLine("a R0, R4");	/* Adjust FP for arguments */
        FramePtr = 1;
    } else { 
        FramePtr = 0;
        /* Save the return address: FIXME we really want to merge all of this
           and the local setup nicely */
        AddCodeLine("ai R3, -4");
        AddCodeLine("mov R11,*R3");

    }
    //InvalidateRegCache();
}



void g_leave(int voidfunc, unsigned flags, unsigned argsize)
/* Function epilogue */
{
    /* Should always be zero : however ignore this being wrong if we took
       a C level error as that may be the real cause. Only valid code failing
       this check is a problem */

    if (StackPtr && !ErrorCount)
        Internal("g_leave: stack unbalanced by %d", StackPtr);

    /* Recover the previous frame pointer if we were vararg */
    if (FramePtr) {
        AddCodeLine("mov *R3+,R4");
    }
    AddCodeLine("mov *R3+,R11");
    AddCodeLine("rt");
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
                if (Flags & CF_USINGR2)
                    Assign(2, Val);
                else
                    Assign(1, Val);
                break;

            case CF_LONG:
                /* Split the value into 4 bytes */
                W1 = (unsigned short) (Val >> 0);
                W2 = (unsigned short) (Val >> 16);
                if (Flags & CF_USINGR2) {
                    Assign(2, W1);
                } else {
                    /* Load the value */
                    Assign(0, W2);
                    if (W1 && W1 != W2)
                        Assign(1, W1);
                    else
                        AddCodeLine("MOV R0,R1");
                }
                break;

            default:
                typeerror (Flags);
                break;

        }

    } else {

        /* Some sort of label */
        const char* Label = GetLabelName (Flags, Val, Offs, 1);

        /* Load the address into the primary */
        if (Flags & CF_USINGR2)
            AddCodeLine("LI R2, %s", Label);
        } else {
            AddCodeLine ("LI R1, %s", Label);
        }
    }
}



void g_getstatic (unsigned flags, uintptr_t label, long offs)
/* Fetch an static memory cell into the primary register */
{
    /* Create the correct label name */
    const char* lbuf = GetLabelName (flags, label, offs, 0);

    /* Check the size and generate the correct load operation */
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            /* Byte ops are not pretty */
            if ((flags & CF_FORCECHAR) || (flags & CF_TEST)) {
                AddCodeLine("MOVB @%s, R1", lbuf);   /* load A from the label */
                AddCodeLine("SWPB R1");	/* Preserves flags */
            } else
                FetchByteStatic(lbuf, flags)
            break;

        case CF_INT:
            if (flags & CF_USINGR2)
                    AddCodeLine ("MOV @%s, R2", lbuf);
            else
                    AddCodeLine ("MOV @%s, R1", lbuf);
            break;

        case CF_LONG:
            if (flags & CF_TEST) {
                AddCodeLine("MOV @%s, R1", lbuf);
                AddCodeLine("OR @%s, R1", lbuf + 2);
            } else {
                AddCodeLine("MOV @%s, R0", lbuf);
                if (flags & CF_USINGR2)
                    AddCodeLine("MOV @%s, R2", lbuf + 2);
                else
                    AddCodeLine("MOV @%s, R1", lbuf + 2);
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
    Offs = GenOffset(Offs);

    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            AddCodeLine("MOV %s, R1", Indirect(3,Offs));
            break;

        case CF_LONG:
            AddCodeLine("MOV %s, R0", Indirect(3, Offs));
            AddCodeLine("MOV %s, R1", Indirect(3, Offs));
            /* FIXME - test inline */
            if (Flags & CF_TEST)
                g_test (Flags);
            break;

        default:
            typeerror (Flags);
    }
}

void g_getlocal_r2 (unsigned Flags, int Offs)
/* Fetch specified local object (local var) into r2. */
{
    Offs = GenOffset(Offs);

    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            AddCodeLine ("MOV %s, R2", Indirect(3, Offs));
            break;

        case CF_LONG:
            AddCodeLine ("MOV %s, R0", Indirect(3, Offs));
            AddCodeLine ("MOV %s, R2", Indirect(3, Offs + 2));
            break;

        default:
            typeerror (Flags);
    }
}


void g_primary_to_x(void)
{
    AddCodeLine("MOV R1,R2");
}

void g_x_to_primary(void)
{
    AddCodeLine("MOV R2,R1");
}

/* Get a variable into R1 or R0/R1 through the pointer in R1 */
void g_getind (unsigned Flags, unsigned Offs)
/* Fetch the specified object type indirect through the primary register
** into the primary register
*/
{
    Offs = GenOffset (Offs);

    /* FIXME */
    NotViaR2();

    /* Handle the indirect fetch */
    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
            /* Character sized */
            AddCodeLine("MOV R1,R2");
            FetchByteIndirect(2, Offs, Flags);
            break;

        case CF_INT:
            AddCodeLine("MOV %s, R1", Indirect(1, Offs));
            break;

        case CF_LONG:
            AddCodeLine("MOV %s, R0", Indirect(1, Offs));
            AddCodeLine("MOV %s, R1", Indirect(1, Offs + 2));
            /* FIXME: inline test ? */
            if (Flags & CF_TEST) {
                g_test (Flags);
            }
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
        AddCodeLine("MOV R4, R2");
        AddCodeLine("AI %d, R2", Offs);
        return;
    }
    Offs = -Offs;
    /* Calculate the offset relative to sp */
    Offs += StackPtr;

    AddCodeLine(";+leasp %d %d\n", FramePtr, Offs);

    if (!(Flags & CF_USINGR2)) {
        AddCodeLine("MOV R3, R1");
        AddCodeLine("AI R1, %d", Offs);
        return;
    }
    AddCodeLine("MOV R3, R2");
    AddCodeLine("AI R2, %d", Offs);
}

/*****************************************************************************/
/*                             Store into memory                             */
/*****************************************************************************/

void g_putstatic (unsigned flags, uintptr_t label, long offs)
/* Store the primary register into the specified static memory cell */
{
    /* Create the correct label name */
    const char* lbuf = GetLabelName (flags, label, offs, 0);

    /* Check the size and generate the correct store operation */
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            /* FIXME: Truely horrible due to the byte in high thing */
            PutByteStatic(label, flags);
            break;

        case CF_INT:
            if (flags & CF_USINGR2)
                AddCodeLine("MOV R2, @%s", lbuf);
            else
                AddCodeLine("MOV R1, @%s", lbuf);
            break;

        case CF_LONG:
            AddCodeLine("MOV R0, @%s", lbuf);
            if (flags & CF_USINGR2)
                AddCodeLine("MOV R2, @%s + 2", lbuf);
            else
                AddCodeLine("MOV R1, @%s + 2", lbuf);
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
    Offs = GenOffset (Offs);

    NotViaR2();

    /* FIXME: Offset 0 generate *Rn+ ?? , or fix up in assembler ? */
    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            if (Flags & CF_CONST)
                Assign(1, Val);
            AddCodeLine("MOV R1, @%d(R2)", Offs);
            break;
        case CF_LONG:
            if (Flags & CF_CONST) {
                Assign(1, Val >> 16);
                AddCodeLine("MOV R0, %d(R2)", Offs);
                if ((Val >> 16) != (Val & 0xFFFF))
                    Assign(1, Val);
                AddCodeLine("MOV R1, %d(R2)", Offs + 2);
            } else {
                AddCodeLine("MOV R0, %d(R2)", Offs);
                AddCodeLine("MOV R1, %d(R2)", Offs + 2);
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
    AddCodeLine("MOV *R3+,R2");
    /* R2 now points into the object, and Offs is the remaining offset to
       allow for. D holds the stuff to store */                
    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
            /* FIXME: we need to track if R1 is reversed and then we can
               minimise this nonsense all over */
            PutByteIndirect(2, Offs, Flags);
            break;

        case CF_INT:
            AddCodeLine("MOV R1, @%d(R2)", Offs);
            break;

        case CF_LONG:
            AddCodeLine("MOV R0, @%d(R2)", Offs);
            AddCodeLine("MOV R1, @%d(R2)", Offs + 2);
            break;

        default:
            typeerror (Flags);

    }
    /* Pop the argument which is always a pointer */
    pop (CF_PTR);
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
            NotViaR2();	/* For now */
            if (flags & CF_UNSIGNED) {
                /* Push a new upper 16bits of zero */
                AddCodeLine("CLR R2"):
            } else {
                L = GetLocalLabel();
                AddCodeLine("MOV *R3, R2");
                AddCodeLine("CLR R2");	/* Flags not affected */
                AddCodeLine("JGE %s", LocalLabelName(L));
                AddCodeLine("DEC R2");	/* 0xFFFF */
                g_defcodelabel(L);
            }
            AddCodeLine("AI R3,-4");
            AddCodeLine("MOV R2,*R3");
            push (CF_INT);
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
            /* We have a big endian long at s+4,3,2,1 so we just need to
               throw out the top. */
            AddCodeLine("INCT R3");
            pop (CF_INT);
            break;
        default:
            typeerror (flags);
    }
}

static void g_regchar (unsigned Flags)
/* Make sure, the value in the primary register is in the range of char. Truncate if necessary */
{
    if (Flags & CF_UNSIGNED)
        AddCodeLine("ANDI R1, 0xFF");
    else {
        AddCodeLine("SWPB R1");
        AddCodeLIne("ASR R1,8");
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
            NotViaR2();
            if (Flags & CF_FORCECHAR) {
                if (Flags & CF_UNSIGNED)
                    AddCodeLine("ANDI R1, 0xFF");
                else
                    AddCodeLine("SRA R1, 8");
                break;   
            }
            /* FALLTHROUGH */

        case CF_INT:
            AddCodeLine("CLR R0");
            if ((Flags & CF_UNSIGNED)  == 0) {
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

    NotViaR2();
    /* Value may not be zero */
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
                    AddCodeLine("LI R0, %d", val);
                    /* Use a multiplication instead */
                    AddCodeLine("MPY R1, R0", val);
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

            /* Factor is 2, 4, 8 and 16 use special function */
            switch (flags & CF_TYPEMASK) {

                /* This works for byte and int */
                case CF_CHAR:
                    /* Byte is a pain */
                    AddCodeLine("SWPB");
                    if (flags & CF_UNSIGNED) {
                        AddCodeLine("SRL R1,%d", p2);
                    else
                        AddCodeLine ("SRA R1,%d", p2);
                    AddCodeLine("SWPB");
                    /* FIXME: Do we need to mask ? */
                    break;
                case CF_INT:
                    if (flags & CF_UNSIGNED) {
                        AddCodeLine("SRL R1,%d", p2);
                    else
                        AddCodeLine ("SRA R1,%d", p2);
                    break;

                case CF_LONG:
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

    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
            /* We keep locals in word format so we can just use word
               operations on them */
        case CF_INT:
            offs = GenOffset(offs);
            AddCodeLine ("A @%d(R3), R1", offs);
            break;

        case CF_LONG:
            L = GetLocalLabel();
            offs = GenOffset(offs);
            AddCodeLine ("A @%d(R3), R1", offs + 2);
            AddCodeLine ("JOC %s", LocalLabelName(L);
            AddCodeLine ("INC R0");
            g_defcodeLabel(L);
            AddCodeLine ("A @%d(R3), R0", offs);
            break;

        default:
            typeerror (flags);

    }
}



void g_addstatic (unsigned flags, uintptr_t label, long offs)
/* Add a static variable to ax */
{
    /* Create the correct label name */
    const char* lbuf = GetLabelName (flags, label, offs, 0);
    unsigned L;

    NotViaR2();

    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            /* FIXME: Horrible */
            FetchByteIndirect(2, lbuf, Flags);
            AddCodeLine("A R2, R1");
            AddCodeLine("ANDI R1, 0xFF");
            break;

        case CF_INT:
            AddCodeLine("A @%s, R1", lbuf);
            break;

        case CF_LONG:
            L = GetLocalLabel();
            AddCodeLine ("A @%s+2, R1", lbuf);
            AddCodeLine ("JOC %s", LocalLabelName(L);
            AddCodeLine ("INC R0");
            g_defcodeLabel(L);
            AddCodeLine ("A @%s, R0", lbuf);
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
    const char* lbuf = GetLabelName (flags, label, offs, 0);


    /* Check the size and determine operation */
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            NotViaR2();
            if (flags & CF_FORCECHAR) {
                FetchByteStatic(lbuf, flags);
                if (flags & CF_CONST) {
                    switch(val) {
                        case 1:
                            AddCodeLine("INC R1");
                            break;
                        case 2:
                            AddCodeLine("INCT R1");
                            break;
                        default:
                            AddCodeLine("AI R1, %d", val & 0xFF);
                            break;
                    }
                } else {
                    AddCodeLine("A @%s, R1", lbuf);
                PutByteStatic(lbuf, flags);
                AddCodeLine("AI R1, 0xFF");
                break;
            }
            /* FALLTHROUGH */

        case CF_INT:
            /* So we keep the result in R1 as well */
            if (flags & CF_CONST) {
                if (val == 1)
                    AddCodeLine("INC @%s", lbuf);
                else if (val == 2)
                    AddCodeLine("INCT @%s", lbuf);
                else
                    AddCodeLine("AI @%s, %d", val);
                AddCodeLine("MOV @%s, R1");
            } else {
                AddCodeLine("A R1, @%s", lbuf);
                AddCodeLine("MOV @%s, R1", lbuf);
                /* Peephole opportunity */
            }
            break;

        case CF_LONG:
            NotViaR2();
            lbuf = GetLabelName (flags, label, offs, 1);
            if (flags & CF_CONST) {
                if (val < 0x10000) {
                    AddCodeLine("LI R2, %s", lbuf);
                    AddCodeLine("LI R1, %d", val);
                    AddCodeLine("BL laddeqstatic16");
                } else {
                    g_getstatic (flags, label, offs);
                    g_inc (flags, val);
                    g_putstatic (flags, label, offs);
                }
            } else {
                /* Might be worth inlining ? */
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
    NotViaR2();
    
    /* Check the size and determine operation */
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
            /* Our local is word sized so we are fine */
        case CF_INT:
            Offs = GenOffset(Offs);
            if (flags & CF_CONST)
                AddCodeLine("LI R1, %d", val);
            AddCodeLine("A %s, R1", Indirect(3, Offs));
            AddCodeLine("MOV R1, %s", Indirect(3, Offs));
            break;

        case CF_LONG:
            if (flags & CF_CONST)
                Assign32(0, 1, val);
            Offs = GenOffset(Offs);
            L = GetLocalLabel();
            AddCodeLine("A %s, R1", Indirect(3, Offs + 2));
            AddCodeLine("JNC %s", LocalLabelName(L));
            AddCodeLine("INC R0");
            g_defcodelabel(L);
            AddCodeLine("A %s, R0", Indirect(3, Offs));
            /* Hopefully we can trim this with the peepholer */
            AddCodeLine("MOV R1", Indirect(3, Offs + 2));
            AddCodeLine("MOV R0", Indirect(3, Offs));
            break;

        default:
            typeerror (flags);
    }
}


void g_addeqind (unsigned flags, unsigned offs, unsigned long val)
/* Emit += for the location with address in R1 */
{

    /* Check the size and determine operation */
    NotViaR2();

    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
            AddCodeLine("MOV R1, R0");
            FetchByteIndirect(0, offs, flags);
            switch(val) {
            case 1:
                AddCodeLine("INC R1");
                break;
            case 2:
                AddCodeLine("INC R1");
                break;
            case 0xFFFF:
                AddCodeLine("DEC R1");
                break;
            case 0xFFFE:
                AddCodeLine("DECT R1");
                break;
            default:
                AddCodeLine("AI R1, %d", val);
            }
            PutByteIndirect(0, offs, flags);
            break;

        case CF_INT:
            AddCodeLine("MOV R1, R0");
            AddCodeLine("MOV @%d(R0), R1", offs);
            AddCodeLine("AI R1, %d", val);
            AddCodeLine("MOV R1, @%d(R0)", offs);
            break;
            
        case CF_LONG:
            if (flags & CF_CONST)
                Assign32(0, 1, val);
            Offs = GenOffset(Offs);
            L = GetLocalLabel();
            AddCodeLine("MOV R1, R2");
            AddCodeLine("A %s, R1", Indirect(2, Offs + 2));
            AddCodeLine("JNC %s", LocalLabelName(L));
            AddCodeLine("INC R0");
            g_defcodelabel(L);
            AddCodeLine("A %s, R0", Indirect(2, Offs));
            /* Hopefully we can trim this with the peepholer */
            AddCodeLine("MOV R1", Indirect(2, Offs + 2));
            AddCodeLine("MOV R0", Indirect(2, Offs));
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
    NotViaR2();
    
    if (offs > 0 && FramePtr) {
        AddCodeLine("A R4, R1");
        offs += 2;
        if (offs != 0)
            AddCodeLine("AI R1, %d", offs);
    } else {
        offs -= StackPtr;
        if (offs != 0)
            AddCodeLine("AI R1, %d", offs);
        AddCodeLine("A R3, R1");
    }
}

void g_addaddr_static (unsigned flags, uintptr_t label, long offs)
/* Add the address of a static variable to d */
{
    /* Create the correct label name */
    const char* lbuf = GetLabelName (flags, label, offs, 1);

    if (flags & CF_USINGR2)
        AddCodeLine ("AI R2, %s", lbuf);
    else
        AddCodeLine ("AI R1, %s", lbuf);
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
    NotViaR2 ();
    /* Check the size and determine operation */
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
        case CF_INT:
            AddCodeLine("MOV R1, @tmp1");
            break;

        case CF_LONG:
            AddCodeLine("MOV R0, @tmp1");
            AddCodeLine("MOV R1, @tmp1 + 2");
            break;

        default:
            typeerror (flags);
    }
}

void g_restore (unsigned flags)
/* Copy hold register to primary. */
{
    NotViaR2();
    /* Check the size and determine operation */
    switch (flags & CF_TYPEMASK) {

        case CF_CHAR:
        case CF_INT:
            AddCodeLine("MOV @tmp1, R1");
            break;

        case CF_LONG:
            AddCodeLine ("MOV @tmp1, R0");
            AddCodeLine ("MOV @tmp1 + 2, R1");
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
    unsigned L;

    NotViaR2();

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
            AddCodeLine("CI R1, %d", val);
            break;

        case CF_LONG:
            Internal ("g_cmp: Long compares not implemented");
            break;

        default:
            typeerror (flags);
    }
}

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

    AddCodeLine ("BL %s", *Subs);

    /* The operation will pop it's argument */
    pop (Flags);
}

void g_test (unsigned flags)
/* Test the value in the primary and set the condition codes */
{
    NotViaR2();
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
            if (flags & CF_FORCECHAR) {
                AddCodeLine ("ANDI R1, 0xFF");
                break;
            }
            /* FALLTHROUGH */
        case CF_INT:
            AddCodeLine ("CI R1,0");
            break;

        case CF_LONG:
            if (flags & CF_UNSIGNED) {
                AddCodeLine ("BL utsteax");
            } else {
                AddCodeLine ("BL tsteax");
            }
            break;

        default:
            typeerror (flags);

    }
}



void g_push (unsigned flags, unsigned long val)
/* Push the primary register or a constant value onto the stack */
{
    if ((flags & CF_CONST) && (flags & CF_TYPEMASK) != CF_LONG) {
    
        /* We always push 16bit values for temporaries */
        /* We need to try and optimize these as well as use the register
           spares for a virtual stack top */
        AddCodeLine ("LI R1, %d", val);
        AddCodeLine("AI R3,-2");
        AddCodeLine("MOV R1,*R3");
    } else {
        /* Value is not 16 bit or not constant */
        if (flags & CF_CONST) {
            /* We need to kill 8bit wide push/pop, expand char args etc */
            g_getimmed (flags | CF_USINGR2, val, 0);
            switch(flags & CF_TYPEMASK) {
                case CF_CHAR:
                case CF_INT:
                    AddCodeLine("AI R3, -2");
                    AddCodeLine("LI R1, %d", val);
                    AddCodeLine("MOV R1,*R3");
                    break;
                case CF_LONG:
                    AddCodeLine("AI R3, -4");
                    if (WordsSame(val)) {
                        Assign32(0, 1, val);
                        AddCodeLine("MOV R1,@2(R3)");
                        AddCodeLine("MOV R0,*R3");
                    } else {
                        Assign(1, val);
                        AddCodeLine("MOV R1,@2(R3)");
                        AddCodeLine("MOV R1,*R3");
                    }
                    break;
                default:
                    typeerror (flags);
                }
                push (flags);
                return;
            }
        }

        /* Push the primary register */
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                AddCodeLine("AI R3, -2");
                if (flags & CF_USINGR2)
                    AddCodeLine("MOV R2,*R3");
                else
                    AddCodeLine("MOV R1,*R3");
                break;
            case CF_LONG:
                AddCodeLine("AI R3, -4");
                if (flags & CF_USINGR2)
                    AddCodeLine("MOV R2,@2(R3)");
                else
                    AddCodeLine("MOV R1,@2(R3)");
                AddCodeLine("MOV R0,*R3");
                break;
            default:
                typeerror (flags);

        }
    }
    /* Adjust the stack offset */
    push (flags);
}

void g_swap (unsigned flags)
/* Swap the primary register and the top of the stack. flags give the type
** of *both* values (must have same size).
*/
{
    NotViaR2();
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            AddCodeLine("MOV *R3, R2");
            AddCodeLine("MOV R1, *R3");
            AddCodeLine("MOV R2, R1");
            break;
        case CF_LONG:
            AddCodeLine("MOV *R3, R2");
            AddCodeLine("MOV R0, *R3");
            AddCodeLine("MOV R2, R0");
            AddCodeLine("MOV @2(R3), R2");
            AddCodeLine("MOV R1, @2(R3)");
            AddCodeLine("MOV R2, R1");
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
    if ((Flags & CF_FIXARGC) == 0)
        Assign(0, ArgSize);
    AddCodeLine("BL @_%s", Label);
}

/* Flags tells us if it is varargs, ArgSize is the number of *extra* bytes
   pushed for the ... */
void g_callind (unsigned Flags, int Offs, int ArgSize)
/* Call subroutine indirect */
{
    if ((Flags & CF_LOCAL) == 0) {
        /* Address is in R1 */
        if ((Flags & CF_FIXARGC) == 0)
            Assign(0, ArgSize);
        AddCodeLine ("BL R1");
    } else {
        Offs = GenOffset(Offs);
        if ((Flags & CF_FIXARGC) == 0)
            Assign(0, ArgSize);
        AddCodeLine("MOV @%d(R3), R2", Offs);
        AddCodeLine("BL R2");
    }
}

void g_jump (unsigned Label)
/* Jump to specified internal label number */
{
    AddCodeLine ("B %s", LocalLabelName (Label));
}

void g_truejump (unsigned flags attribute ((unused)), unsigned label)
/* Jump to label if zero flag clear */
{
    AddCodeLine ("LJNE %s", LocalLabelName (label));
}

void g_falsejump (unsigned flags attribute ((unused)), unsigned label)
/* Jump to label if zero flag set */
{
    AddCodeLine ("LJEQ %s", LocalLabelName (label));
}

void g_lateadjustSP (unsigned label)
/* Adjust stack based on non-immediate data */
{
    AddCodeLine("A @%s,R3", LocalLabelName(label));
}

void g_drop (unsigned Space)
/* Drop space allocated on the stack */
{
    AddCodeLine("AI R3, %d", Space);
}

void g_space (int Space)
/* Create or drop space on the stack */
{
    AddCodeLine("AI R3, %d", -Space);
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
    /* This will work much better once we add the virtual reg stack */
    static const char* const ops[4] = {
        "tosaddax", "tosaddax", "tosaddeax", "tosaddeax"
    };

    if (flags & CF_CONST) {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                if (flags & CF_USINGR2)
                    AddCodeLine("AI %d, R2", val & 0xFFFF);
                else
                    AddCodeLine("AI %d, R1", val & 0xFFFF);
                break;
            case CF_LONG:
                if (val & 0xFFFF) {
                    L = GetLocalLabel();
                    if (flags & CF_USINGR2)
                        AddCodeLine("AI R2, %d", val & 0xFFFF);
                    else
                        AddCodeLine("AI R1, %d", val & 0xFFFF);
                    AddCodeLine("JNC %s", LocalLabelName(L));
                    AddCodeLine("INC R0");
                    g_defcodelabel(L);
                }
                if (val >> 16)
                    AddCodeLine("AI R0, %d", val >> 16);
                break;
        }
    } else {
        int offs;
        /* We can optimize some of these in terms of ,x */
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                if (flags & CF_USINGR2)
                    AddCodeLine("A *R3+, R2");
                else
                    AddCodeLine("A *R3+, R1");
                pop(flags);
                return;
            case CF_LONG:
                L = GetLocalLabel();
                if (flags & CF_USINGR2)
                    AddCodeLine("A @2(R3), R2");
                else
                    AddCodeLine("A @2(R3), R1");
                AddCodeLine("JNC %s", LocalLabelName(L));
                AddCodeLine("INC R0");
                g_defcodelabel(L);
                AddCodeLine("A *R3+, R0");
                AddCodeLine("INCT R3");
                pop(flags);
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

    NotViaR2();
    if (flags & CF_CONST) {
        /* This shouldn't ever happen as the compiler will turn constant
           subtracts in this form into something else */
        flags &= ~CF_FORCECHAR; /* Handle chars as ints */
        g_push (flags & ~CF_CONST, 0);
    }
    /* It would be nice to spot these higher up and turn them from TOS - Primary
       into negate and add */
    switch(flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            AddCodeLine("S R1, *R3");
            AddCodeLine("MOV *R3+, R1");
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
    if (flags & CF_CONST) {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                if (flags & CF_USINGR2)
                    AddCodeLine("AI R2, %d", (-val) & 0xFFFF);
                else
                    AddCodeLine("AI R1, %d", (-val) & 0xFFFF);
                break;
            case CF_LONG:
                NotViaR2();
                flags &= CF_CONST;
                g_push(flags & ~CF_CONST, 0);
                oper(flags, val, ops);
                break;
        }
    } else {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                AddCodeLine("S @R3+, R1");
                return;
        }
        NotViaR2();
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

    NotViaR2();
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
                AddCodeLine("LI R0, %d", val & 0xFFFF);
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
                AddCodeLine("CLR R0");
                if ((flags & CF_UNSIGNED) == 0)
                    /* We have to use a literal for this */
                    AddCodeLine("DIVS @%s", LocalLabelName(L));
                else
                    AddCodeLine("DIV @%s, R0", LocalLabelName(L));
               /* Quotient is the bit we want */
                AddCodeLine("MOV R0,R1");
                return;
            default:
                /* lhs is not on stack */
                flags &= ~CF_FORCECHAR;     /* Handle chars as ints */
                g_push (flags & ~CF_CONST, 0);
            }
        } else {
            NotViaR2();
            /* TOS / Primary */
            switch(flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF__INT:
                /* Some shuffling needed. We need R0/R1 to be the base value
                   and we need the existing R1 somewhere */
                AddCodeLine("MOV R1, R2");
                AddCodeLine("MOV *R3+, R1");
                AddCodeLine("CLR R0");
                if (flags & CF_UNSIGNED) == 0)
                    AddCodeLine("DIVS R2");
                else
                    AddCodeLine("DIV R0, R2");
                AddCodeLine("MOV R0, R1");
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

    NotViaR2();
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
                AddCodeLine("CLR R0");
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
            NotViaR2();
            /* TOS / Primary */
            switch(flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF__INT:
                /* Some shuffling needed. We need R0/R1 to be the base value
                   and we need the existing R1 somewhere */
                AddCodeLine("MOV R1, R2");
                AddCodeLine("MOV *R3+, R1");
                AddCodeLine("CLR R0");
                if (flags & CF_UNSIGNED) == 0)
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
    NotViaR2();	/* FIXME */
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                val &= 0xFFFF;
            case CF_LONG:
                if (val & 0xFFFF)
                    AddCodeLine("ORI R1, %d", val & 0xFFFF);
                if (val >> 16)
                    AddCodeLine("ORI R0, %d", val >> 16);
                break;

            default:
                typeerror (flags);
        }
        return;
    }

    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            AddCodeLine("OR *R3+, R1");
            pop(flags);
            return;
        case CF_LONG:
            AddCodeLine("OR *R3+, R0");
            AddCodeLine("OR *R3+, R1");
            pop(flags);
            return;
    }
}

/*
 *	XOR has no immediate form
 */
void g_xor (unsigned flags, unsigned long val)
/* Primary = TOS ^ Primary */
{
    NotViaR2();

    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        switch (flags & CF_TYPEMASK) {

            case CF_CHAR:
            case CF_INT:
                if (val & 0xFFFF) {
                    AddCodeLine("LI R2,%d", val);
                    AddCodeLine("XOR R2, R1");
                }
                return;
            case CF_LONG:
                /* There are optimisations on these for near values using
                   INC/DEC/INCT/DECT - needs a helper for OR/XOR/AND etc */
                if (WordsSame(val)) {
                    AddCodeLine("LI R2,%d", val);
                    AddCodeLine("XOR R2, R1");
                    AddCodeLine("XOR R2, R0");
                } else {
                    if (val & 0xFFFF) {
                        AddCodeLine("LI R2,%d", val);
                        AddCodeLine("XOR R2, R1");
                    }
                    if (val >> 16) {
                        AddCodeLine("LI R2,%d", val >> 16);
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
            AddCodeLine("XOR *R3+, R1");
            pop(flags);
            return;
        case CF_LONG:
            AddCodeLine("XOR *R3+, R0");
            AddCodeLine("XOR *R3+, R1");
            pop(flags);
            return;
    }
}

void g_and (unsigned Flags, unsigned long Val)
/* Primary = TOS & Primary */
{
    NotViaR2();
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (Flags & CF_CONST) {

        switch (Flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                AddCodeLine("ANDI R1, %d", val & 0xFFFF);
                return;
            case CF_LONG:
                /* Trap the many common easy 32bit cases we can do inline */
                if (Val <= 0xFFFF) {
                    AddCodeLine("CLR R0");
                    AddCodeLine("ANDI R1, %d", val & 0xFFFF);
                    return;
                } else if (Val >= 0xFFFF0000UL) {
                    AddCodeLine("ANDI R1, %d", val & 0xFFFF);
                    return;
                } else if (!(Val & 0xFFFF)) {
                    AddCodeLine("ANDI R0, %d", val >> 16);
                    AddCodeLine("CLR R1");
                    return;
                }
                break;
            default:
                typeerror (Flags);
        }
    }
    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            AddCodeLine("AND *R3+,R1");
            pop(Flags);
            return;
        case CF_LONG:
            AddCodeLine("AND *R3+,R0");
            AddCodeLine("AND *R3+,R1");
            pop(Flags);
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

    NotViaR2();

    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                /* C lets us do this however we like - be nice */
                if (val > 15)
                    AddCodeLine("CLR R1");
                else if (flags & CF_UNSIGNED)
                    AddCodeLine("SRL R1, %d", val & 15);
                else
                    AddCodeLine("SRA R1, %d", val & 15);
                return;
            case CF_LONG:
                if (val == 16 && (flags & CF_UNSIGNED)) {
                    AddCodeLine("MOV R0,R1");
                    AddCodeLine("CLR R0");
                    return;
                }
                if (val > 16 && (flags & CF_UNSIGNED)) {
                    AddCodeLine("MOV R0,R1");
                    AddCodeLine("LI R0, %d", val - 16);
                    AddCodeLine("SRL R1, R0");
                    AddCodeLine("CLR R0");
                    return;
                }
                /* Need to be sure we can use R2 here.. */
                L = GetLocalLabel();
                L2 = GetLocalLabel();
                L3 = GetLocalLabel();
                AddCodeLine("LI R2, %d", val & 31);
                g_defcodelabel(L);
                if (flags & CF_UNSIGNED)
                    AddCodeLine("SRL R0");
                else
                    AddCodeLine("SRA R0")
                AddCodeLine("JOC %s", LocalLabelName(L2));
                AddCodeLine("SRL R1");
                AddCodeLine("JMP %s", LocalLabelName(L3));
                g_defcodelabel(L2);
                AddCodeLine("SRL R1");
                AddCodeLine("ORI R1,0x8000");
                g_defcodelabel(L3);
                AddCodeLine("DEC R2");
                AddCodeLine("JNE %s", LocalLabelName(L);
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
                AddCodeLine("MOV R1, R0");	/* For shifts */
                AddCodeLine("MOV *R3+, R1");
                if (flags & CF_UNSIGNED)
                    AddCodeLine("SRL R3, R0");
                else
                    AddCodeLine("SRA R3, R0");
                pop(Flags);
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

    NotViaR2();

    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                /* C lets us do this however we like - be nice */
                if (val > 15)
                    AddCodeLine("CLR R1");
                else
                    AddCodeLine("SLA R1, %d", val);
                return;

            case CF_LONG:
                if (val == 16) {
                    AddCodeLine("MOV R1, R0");
                    AddCodeLine("CLR R1");
                    return;
                }
                if (val > 16) {
                    AddCodeLine("LI R0,%d", val - 16);
                    AddCodeLine("SLA R1, R0");
                    AddCodeLine("MOV R1, R0");
                    AddCodeLine("CLR R1");
                    return;
                }
                /* The hard way */
                /* Need to be sure we can use R2 here.. */
                L = GetLocalLabel();
                L2 = GetLocalLabel();
                L3 = GetLocalLabel();
                AddCodeLine("LI R2, %d", val & 31);
                g_defcodelabel(L);
                AddCodeLine("SLA R1");
                AddCodeLine("JOC %s", LocalLabelName(L2));
                AddCodeLine("SLA R0");
                AddCodeLine("JMP %s", LocalLabelName(L3));
                g_defcodelabel(L2);
                AddCodeLine("SLA R0");
                AddCodeLine("INC R0");
                g_defcodelabel(L3);
                AddCodeLine("DEC R2");
                AddCodeLine("JNE %s", LocalLabelName(L);
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
                AddCodeLine("MOV R1, R0");	/* shift size */
                AddCodeLine("MOV *R3+, R1");
                AddCodeLine("SLA R1, R0");
                pop(Flags);
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

    NotViaR2();

    switch (Flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            AddCodeLine("NEG R1");
            break;
        case CF_LONG:
            AddCodeLine ("INV R0");
            AddCodeLine ("INV R1");
            AddCodeLine ("INC R1");
            AddCodeLine ("JNC %s", LocalLabelName(L));
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

    NotViaR2();
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
            L = GetLocalLabel();
            /* FIXME: is this needed */
            AddCodeLine ("ANDI R1,0xFF"); /* Test for zero, short way */
            AddCodeLine ("CLR R1");	/* Doesn't touch flags */
            AddCodeLine ("JNE %s", LocalLabelName(L));
            AddCodeLine ("INC R1");	/* Was zero so make one */
            g_defcodelabel(L);
            break;

        case CF_INT:
            L = GetLocalLabel();
            AddCodeLine ("OR R1,R1");	/* Test for zero, short way */
            AddCodeLine ("CLR R1");	/* Doesn't touch flags */
            AddCodeLine ("JNE %s", LocalLabelName(L));
            AddCodeLine ("INC R1");	/* Was zero so make one */
            g_defcodelabel(L);
            break;

        case CF_LONG:
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
    NotViaR2();
    switch (Flags & CF_TYPEMASK) {

        case CF_CHAR:
        case CF_INT:
            AddCodeLine("INV R1");
            break;

        case CF_LONG:
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
    unsigned Label;
    int r;

    /* Don't inc by zero */
    if (val == 0)
        return;

    r = 1;
    if (flags & CF_USINGR2)
        r = 2;
    /* Generate code for the supported types */
    flags &= ~CF_CONST;
    switch (flags & CF_TYPEMASK) {
        case CF_CHAR:
        case CF_INT:
            val &= 0xFFFF;
            switch (val) {
            case 1:
                AddCodeLine("INC R%d", r);
                break;
            case 2:
                AddCodeLine("INCT R%d", r);
                break;
            case 0xFFFF:
                AddCodeLine("DEC R%d", r);
                break;
            case 0xFFFE:
                AddCodeLine("DECT R%d", r);
                break;
            default:
                AddCodeLine("AI R%d, %d", r);
                break;
            }
            break;

        case CF_LONG:
            NotViaR2();
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

    NotViaR2();
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
            case CF_INT:
                AddCodeLine("CI R1, %d", val);
                AddCodeLine ("BL booleq");
                return;

            case CF_LONG:
                L = GetLocalLabel();
                AddCodeLine("CI R1, %d", val & 0xFFFF);
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("CI R0, %d", val >> 16);
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
            AddCodeLine("C R1, *R3+");
            AddCodeLine ("BL booleq");
            pop(flags);
            return;
        case CF_LONG:
            L = GetLocalLabel();
            AddCodeLine("CI *R3+,R0");
            AddCodeLine("JNE %s", LocalLabelName(L));
            AddCodeLine("CI *R3, R1");
            g_defcodelabel(L);
            AddCodeLine("BL booleq");
            AddCodeLine("INCT R3");
            pop(flags);
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
    NotViaR2();
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        switch (flags & CF_TYPEMASK) {

            case CF_CHAR:
            case CF_INT:
                AddCodeLine("CI R1, %d", val);
                AddCodeLine ("BL boolne");
                return;

            case CF_LONG:
                L = GetLocalLabel();
                AddCodeLine("CI R1, %d", val & 0xFFFF);
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("CI R0, %d", val >> 16);
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
            AddCodeLine("C *R3+, R1");
            AddCodeLine ("BL boolne");
            pop(flags);
            return;
        case CF_LONG:
            L = GetLocalLabel();
            AddCodeLine("CI *R3+,R0");
            AddCodeLine("JNE %s", LocalLabelName(L));
            AddCodeLine("CI *R3, R1");
            g_defcodelabel(L);
            AddCodeLine("BL boolne");
            AddCodeLine("INCT R3");
            pop(flags);
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

    NotViaR2();
    
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
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                if (flags & CF_FORCECHAR)
                    AddCodeLine ("ANDI R1,0xFF");
            case CF_INT:
                AddCodeLine("CI R1, %d", val);
                AddCodeLine ("BL %s", f);
                return;
            case CF_LONG:
                L = GetLocalLabel();
                AddCodeLine ("CI R0, %d", val >> 16);
                AddCodeLine ("JNE %s", LocalLabelName(L));
                AddCodeLine ("CI R1, %d", val & 0xFFFF);
                g_defcodelabel(L);
                AddCodeLine ("BL %s", f);
                return;
            default:
                typeerror (flags);
        }
    } else {
        int offs;
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                if (flags & CF_FORCECHAR)
                    AddCodeLine ("ANDI R1,0xFF");
            case CF_INT:
                AddCodeLine("C @R3+, R1");
                AddCodeLine ("BL %s", f);
                pop (flags);
                return;
            case CF_LONG:
                L = GetLocalLabel();
                AddCodeLine("C @R3+, R0");
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("C @R3, R1");
                g_defcodelabel(L):
                AddCodeLine("BL %s", f);
                AddCodeLine("INCT R3");
                pop (flags);
                return;
        }
    }
}

void g_le (unsigned flags, unsigned long val)
/* Test for less than or equal to */
{
    unsigned L;

    NotViaR2();
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        /* Look at the type */
        switch (flags & CF_TYPEMASK) {

            case CF_CHAR:
                if (val > 0xFF)
                    Warning("Condition is always true");
                    AddCodeLine("BL return1");
                    return;
                } 
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
    }
    else {
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                AddCodeLine("ANDI R1, 0xFF");
            case CF_INT:
                AddCodeLine("C @R3+, R1", val);
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL boolule");
                else
                    AddCodeLine ("BL boolle");
                return;
            case CF_LONG:
                L = GetLocalLabel();
                AddCodeLine("C @R3+, R0");
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("C @R3, R1");
                g_defcodelabel(L):
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL boolule");
                else
                    AddCodeLine ("BL boolle");
                AddCodeLine("INCT R3");
                pop (flags);
                return;
        }
    }
}

void g_gt (unsigned flags, unsigned long val)
/* Test for greater than */
{
    unsigned L;
    NotViaR2();
    /* If the right hand side is const, the lhs is not on stack but still
    ** in the primary register.
    */
    if (flags & CF_CONST) {

        /* Look at the type */
        switch (flags & CF_TYPEMASK) {

            case CF_CHAR:
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
                if (flags & CF_FORCECHAR)
                    AddCodeLine("AND R1, 0xFF");
            case CF_INT:
                AddCodeLine("C @R3+, R1");
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL boolugt");
                else
                    AddCodeLine ("BL boolgt");
                pop (flags);
                return;
            case CF_LONG:
                L = GetLocalLabel();
                AddCodeLine("C @R3+, R0");
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("C @R3, R1");
                g_defcodelabel(L):
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL boolugt");
                else
                    AddCodeLine ("BL boolgt");
                AddCodeLine("INCT R3");
                pop (flags);
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
    NotViaR2();

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
                if (flags & CF_FORCECHAR)
                    AddCodeLine("ANDI R1, 0xFF");
            case CF_INT:
                AddCodeLine("CI R1, %d", val);
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL booluge");
                else
                    AddCodeLine ("BL boolge");
                return;
            case CF_LONG:
                L = GetLocalLabel();
                AddCodeLine ("CI R0, %d", val >> 16);
                AddCodeLine ("JNE %s", LocalLabelName(L));
                AddCodeLine ("CI R1, %d", val & 0xFFFF);
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
        int offs;
        switch (flags & CF_TYPEMASK) {
            case CF_CHAR:
                if (flags & CF_FORCECHAR)
                    /* FIXME probably should do this to both sides ? */
                    AddCodeLine("ANDI R1, 0xFF");
            case CF_INT:
                AddCodeLine("C @R3+,R1");
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL booluge");
                else
                    AddCodeLine ("BL boolge");
                pop (flags);
                return;
            case CF_LONG:
                L = GetLocalLabel();
                AddCodeLine("C @R3+, R0");
                AddCodeLine("JNE %s", LocalLabelName(L));
                AddCodeLine("C @R3, R1");
                g_defcodelabel(L):
                if (flags & CF_UNSIGNED)
                    AddCodeLine ("BL booluge");
                else
                    AddCodeLine ("BL boolge");
                AddCodeLine("INCT R3");
                pop (flags);
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
        const char* Label = GetLabelName (flags, val, offs, 1);
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
    /* TODO */
}


/*
 *	Make the stack space and fill it. As we keep any variable on
 *	the stack word aligned this is safe I think ;)
 */
void g_initauto (unsigned Label, unsigned Size)
{
    unsigned CodeLabel = GetLocalLabel ();

    /* FIXME: for short blocks inline ? */
    AddCodeLine("LI R2, %s", GetLabelName (CF_STATIC, Label, 0, 1));
    AddCodeLine("LI R1, %d", Size / 2);
    AddCodeLine("AI R3, %d", -Size);
    AddCodeLine("LI R0, R3");
    g_defcodelabel(CodeLabel);
    AddCodeLine("MOV *R2+,*R0+");
    AddCodeLine("DEC R1"):
    AddCodeLine("JNE %s", LocalLabelName(CodeLabel)
    
    if (Size & 1)
        AddCodeLine("MOVB *R2+,*R0+");
    /* The caller will adjust the stack on the basis we made the pushes */
}

void g_initstatic (unsigned InitLabel, unsigned VarLabel, unsigned Size)
/* Initialize a static local variable from static initialization data */
{
    if (Size <= 8) {
        while(Size > 1) {
            Size -= 2;
            AddCodeLine("MOV @%s+%d, @%s+%d",
                GetLabelName(CF_STATIC, InitLabel, Size, 0));
                GetLabelName(CF_STATIC, VarLabel, Size, 0));
        }
        if (Size)
            AddCodeLine("MOVB @%s+%d, @%s+%d",
                GetLabelName(CF_STATIC, InitLabel, 0, 0));
                GetLabelName(CF_STATIC, VarLabel, 0, 0));
        }
    } else {
        /* Use the easy way here: memcpy() */
        AddCodeLine("AI R3,-6");
        g_getimmed (CF_STATIC, VarLabel, 0);
        AddCodeLine ("MOV R1, *R3");
        g_getimmed (CF_STATIC, InitLabel, 0);
        AddCodeLine ("MOV R1, @2(R3)");
        g_getimmed (CF_INT | CF_UNSIGNED | CF_CONST, Size, 0);
        AddCodeLine ("MOV R1, @4(R3)");
        AddCodeLine ("BL %s", GetLabelName (CF_EXTERNAL, (uintptr_t) "memcpy", 0, 0));
        AddCodeLine ("AI R3, 6");
    }
}


/* For one byte registers we end up saving an extra byte. It avoids having
   to touch D so who cares. We don't allow long registers */

void g_save_regvar(int Offset, int Reg, unsigned Size)
{
    if (CPU == CPU_6800) {
        AddCodeLine("ldaa @reg+%u", Reg);
        AddCodeLine("ldab @reg+%u", Reg + 1);
        AddCodeLine("pshb");
        AddCodeLine("psha");
    } else {
        AddCodeLine("ldx @reg+%u", Reg);
        AddCodeLine("pshx");
    }
    push(CF_INT);
    NotViaR2();
}

void g_restore_regvar(int Offset, int Reg, unsigned Size)
{
    AddCodeLine(";offset %d\n", Offset);
    PullX(1);
    AddCodeLine("stx @reg+%u", Reg);
    pop(CF_INT);
    NotViaR2();
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

    NotViaR2();

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
