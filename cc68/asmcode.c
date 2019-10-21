/*****************************************************************************/
/*                                                                           */
/*                                 asmcode.c                                 */
/*                                                                           */
/*          Assembler output code handling for the cc65 C compiler           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2000-2009, Ullrich von Bassewitz                                      */
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



/* common */
#include "check.h"

/* cc65 */
#include "asmcode.h"
#include "codeseg.h"
#include "dataseg.h"
#include "segments.h"
#include "stackptr.h"
#include "symtab.h"



/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/



int CodeRangeIsEmpty (const CodeMark* Start, const CodeMark* End)
/* Return true if the given code range is empty (no code between Start and End) */
{
    int Empty;
//RE-ADD WHEN MARK DONE FIXME    PRECONDITION (Start->Pos <= End->Pos);
    Empty = (Start->Pos == End->Pos);
    if (Empty) {
        /* Safety */
        CHECK (Start->SP == End->SP);
    }
    return Empty;
}



void WriteAsmOutput (void)
/* Write the final assembler output to the output file */
{
    SymTable* SymTab;
    SymEntry* Entry;

    /* Output the global data segment */
    OutputSegments (CS);

    /* Output all global or referenced functions */
    SymTab = GetGlobalSymTab ();
    Entry  = SymTab->SymHead;
    while (Entry) {
        if (SymIsOutputFunc (Entry)) {
            /* Function which is defined and referenced or extern */
            OutputSegments (Entry->V.F.Seg);
        }
        Entry = Entry->NextSym;
    }
}
