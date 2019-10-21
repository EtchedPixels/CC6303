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
#include "util.h"
#include "codegen.h"



void RemoveCode(const CodeMark *m)
{
}

void GetCodePos(CodeMark *m)
{
}

void MoveCode(const CodeMark *a, const CodeMark *b, const CodeMark *c)
{
}
