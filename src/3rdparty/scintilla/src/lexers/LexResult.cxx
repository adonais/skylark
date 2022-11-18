// Scintilla source code edit control
/** @file LexResult.cxx
 ** Lexer for search result (skylark edit used)
 ** Written by adonais <hua.andy@gmail.com>
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "PropSetSimple.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

static inline bool
at_eol(Accessor &styler, size_t i)
{
    return (styler[i] == '\n') || ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

static inline bool
planning_chars(const uint8_t *s)
{
    const size_t len = strlen((const char *)s);
    return (len > 5 && s[0] == 0xc2 && s[1] == 0xbb && s[2] == ' ' && s[3] == 0xc2 && s[4] == 0xbb && s[5] == ' ');
}

static void
handle_word(result_vec *pvec, char *lineBuffer, size_t startLine, size_t endPos, Accessor &styler, int linenum)
{
    int state = SCE_RESULT_DEFAULT;
    // file comment
    if (planning_chars((const uint8_t *)lineBuffer))
    {
        styler.ColourTo(endPos, SCE_RESULT_COMMENT);
    }
    else if (linenum > 0 && pvec && ((size_t *)(pvec))[-2] > (size_t)(linenum - 1) && pvec[linenum - 1].line >= 0)
    {
        result_postion mark = pvec[linenum - 1].mark;
        // line header
        styler.ColourTo(startLine + mark._no, SCE_RESULT_HEADER);
        styler.ColourTo(startLine + mark._no + mark._start, SCE_RESULT_DEFAULT);
        // end of keyword, colourise it
        styler.ColourTo(startLine + mark._no + mark._start + (mark.end - mark.start), SCE_RESULT_KEYWORD);
    }
    styler.ColourTo(endPos, state);
}

static void
ColouriseResultDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[], Accessor &styler)
{
    (void)initStyle;
    (void)keywordlists;
    char lineBuffer[SC_LINE_LENGTH];
    styler.StartAt(startPos);
    styler.StartSegment(startPos);
    unsigned int linePos = 0;
    size_t startLine = startPos;
    result_vec **pmark = NULL;
    const char *ptr_style = ((styler.pprops)->Get(result_extra));
    if (ptr_style && ptr_style[0])
    {
        sscanf(ptr_style, "%p", (void***)&pmark);
    }
    for (size_t i = startPos; i < startPos + length; i++)
    {
        lineBuffer[linePos++] = styler[i];
        if (at_eol(styler, i) || (linePos >= sizeof(lineBuffer) - 1))
        {
            lineBuffer[linePos] = '\0';
            // 为NULL时不返回, 因为我们需要高亮显示第一行文本
            handle_word(pmark ? *pmark : NULL, lineBuffer, startLine, i, styler, styler.GetLine(startLine));
            linePos = 0;
            startLine = i + 1;
            while (!at_eol(styler, i))
            {
                ++i;
            }
        }
    }
    if (linePos > 0) // 处理最后一行, 因为它们不存在换行符
    {
        handle_word(pmark ? *pmark : NULL, lineBuffer, startLine, startPos + length - 1, styler, styler.GetLine(startLine));
    }
}

static const char *const emptyWordList[] = { 0 };

LexerModule lmResult(SCLEX_RESULT, ColouriseResultDoc, "result", 0, emptyWordList);
