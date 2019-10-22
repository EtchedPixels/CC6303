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
    &CodeHead
};

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

void TextListSplice(struct TextList *at, struct TextList *start, struct TextList *end)
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

void RemoveCode(const CodeMark *m)
{
    TextListRemoveTail(&CodeHead, m->Text);
}

void GetCodePos(CodeMark *m)
{
    m->Text = CodeHead.prev;
}

/* Move the code between Start (inclusive) and End (exclusive) to
** (before) Target. The code marks aren't updated.
*/
void MoveCode(const CodeMark *start,  const CodeMark *end, const CodeMark *target)
{
    TextListSplice(target->Text->prev, start->Text, end->Text);
}

void AppendCode(const char *txt)
{
    TextListAppend(&CodeHead, txt);
}

void PrintCode(void)
{
    struct TextList *t = CodeHead.next;
    while(t != &CodeHead) {
        printf("C| %s\n", t->str);
        t = t->next;
    }
}
