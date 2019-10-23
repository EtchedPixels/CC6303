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
#include "global.h"
#include "segments.h"
#include "stackptr.h"
#include "stdfunc.h"
#include "textseg.h"
#include "textlist.h"
#include "util.h"
#include "codegen.h"


TextList CodeHead = {
    &CodeHead,
    &CodeHead,
    NULL
};

TextList *CodeStack = NULL;

void TextListAppendAfter(TextList *head, const char *text)
{
    TextList *e = xmalloc(sizeof(TextList) + strlen(text) + 1);
    strcpy(e->str, text);
    e->prev = head;
    e->next = head->next;
    head->next->prev = e;
    head->next = e;
}

void TextListAppend(TextList *head, const char *text)
{
    TextListAppendAfter(head->prev, text);
}

static void TextListRemoveOne(TextList *t)
{
    t->prev->next = t->next;
    t->next->prev = t->prev;
}

void TextListRemoveRange(TextList *last, TextList *tail)
{
    while(last->next != tail)
        TextListRemoveOne(last->next);
}

void TextListRemoveTail(TextList *head, TextList *last)
{
    TextListRemoveRange(last, head);
}

void TextListSplice(TextList *at, TextList *start, TextList *end)
{
    /* Unhook the nodes */
    start->prev->next = end->next;
    end->next->prev = start->prev;
    /* Rehook */
    end->next = at->next;
    start->prev = at;
    at->next->prev = end;
    at->next = start;
}

    

/* This is a bit odd. We return a pointer to the last entry, which means
   m->Text for all the code below is really not the current mark but the
   previous to the current mark (we can't return the current text as it
   doesn't yet exist) */

void GetCodePos(CodeMark *m)
{
    m->Text = CodeHead.prev;
}

/* Turn a mark into the right text pointer remembering that the mark
   is the previous pointer */
static TextList *MarkToText(const CodeMark *m)
{
    return m->Text->next;
}


void RemoveCode(const CodeMark *m)
{
    TextListRemoveTail(&CodeHead, MarkToText(m));
}

/* Move the code between Start (inclusive) and End (exclusive) to
** (before) Target. The code marks aren't updated.
*/
void MoveCode(const CodeMark *start,  const CodeMark *end, const CodeMark *target)
{
    TextListSplice(MarkToText(target)->prev, MarkToText(start), MarkToText(end)->prev);
}

void AppendCode(const char *txt)
{
    TextListAppend(&CodeHead, txt);
}

void PrintCode(void)
{
    TextList *t = CodeHead.next;
    if (CodeStack)
        Internal("Botched codestack");
    while(t != &CodeHead) {
        printf("%s\n", t->str);
        t = t->next;
    }
    TextListRemoveRange(&CodeHead, &CodeHead);
}

void PushCode(void)
{
    TextList *n = xmalloc(sizeof(TextList));
    *n = CodeHead;
    n->stack = CodeStack;
    CodeStack = n;
    CodeHead.prev = CodeHead.next = &CodeHead;
}

void PopCode(void)
{
    TextList *t;
    if (CodeStack == NULL)
        Internal ("code pop without push");
    t = CodeStack;
    CodeStack = CodeStack->stack;
    /* The popped block end is the new tail so pointers to CodeHead */
    t->prev->next = &CodeHead;
    /* The popped block start points to the old tail */
    t->next->prev = CodeHead.prev;
    /* Link the popped block (except head) to our tail */
    CodeHead.prev->next = t->next;
    /* Link our tail to the end of the added block */
    CodeHead.prev = t->prev;
    free(t);
}
