/*
 *	CC6303:  A C compiler for the 6803/6303 processors
 *	(C) 2019 Alan Cox
 *
 *	This compiler is built out of a much modified CC65 and all new code
 *	is placed under the same licence as the original. Please direct all
 *	cc6303 bugs to the author not to the cc65 developers unless you find
 *	a bug that is also present in cc65.
 */
/*****************************************************************************/
/*                                                                           */
/*                                 fileid.h                                  */
/*                                                                           */
/*               Determine the id of a file type by extension                */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2003-2012, Ullrich von Bassewitz                                      */
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



#ifndef FILEID_H
#define FILEID_H



/*****************************************************************************/
/*                                   Data                                    */
/*****************************************************************************/



/* An entry for a table that maps extensions to some sort of identifier. Such
** a table - sorted alphabetically - is passed to GetFileId.
*/
typedef struct FileId FileId;
struct FileId {
    const char  Ext[4];
    int         Id;
};



/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/



int CompareFileId (const void* Key, const void* Id);
/* Compare function used when calling bsearch with a table of FileIds */

const FileId* GetFileId (const char* Name, const FileId* Table, unsigned Count);
/* Determine the id of the given file by looking at file extension of the name.
** The table passed to the function must be sorted alphabetically. If the
** extension is found, a pointer to the matching table entry is returned. If
** no matching table entry was found, the function returns NULL.
*/



/* End of fileid.h */

#endif
