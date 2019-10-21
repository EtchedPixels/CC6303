/*****************************************************************************/
/*                                                                           */
/*                                 codelab.c                                 */
/*                                                                           */
/*                           Code label structure                            */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2001-2009, Ullrich von Bassewitz                                      */
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
#include "xmalloc.h"

/* cc65 */
#include "codeent.h"
#include "codelab.h"
#include "output.h"



/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/



CodeLabel* NewCodeLabel (const char* Name, unsigned Hash)
/* Create a new code label, initialize and return it */
{
    /* Allocate memory */
    CodeLabel* L = xmalloc (sizeof (CodeLabel));

    /* Initialize the fields */
    L->Next  = 0;
    L->Name  = xstrdup (Name);
    L->Hash  = Hash;
    L->Owner = 0;
    InitCollection (&L->JumpFrom);

    /* Return the new label */
    return L;
}


