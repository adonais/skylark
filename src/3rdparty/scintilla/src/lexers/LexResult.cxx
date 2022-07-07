// Scintilla source code edit control
/** @file LexOpal.cxx
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

static char *
stristr(const char *String, const char *Pattern)
{
    char *pptr, *sptr, *start;
    uintptr_t slen, plen;

    for (start = (char *) String, pptr = (char *) Pattern, slen = strlen(String), plen = strlen(Pattern);
         /* while string length not shorter than pattern length */
         slen >= plen;
         start++, slen--)
    {
        /* find start of pattern in string */
        while (toupper(*start) != toupper(*Pattern))
        {
            start++;
            slen--;

            /* if pattern longer than string */
            if (slen < plen) return (NULL);
        }

        sptr = start;
        pptr = (char *) Pattern;
        while (toupper(*sptr) == toupper(*pptr))
        {
            sptr++;
            pptr++;
            /* if end of pattern then pattern was found */
            if ('\0' == *pptr) return (start);
        }
    }
    return (NULL);
}

static void
handle_word(char *lineBuffer, size_t startLine, size_t endPos, Accessor &styler, int linenum, WordList *keywordlists[])
{
    WordList &keywords = *keywordlists[0];
    int stat = SCE_RESULT_DEFAULT;
    size_t line_len = strlen(lineBuffer);
    // file header comment
    if (!linenum && line_len > 1 && lineBuffer[0] == '#' && lineBuffer[1] == '*')
    {
        styler.ColourTo(endPos, SCE_RESULT_COMMENT);
    }
    else if (linenum > 0)
    {
        char *p = NULL;
        
        const char *key = keywords.WordAt(0);
        if (key)
        {
            size_t key_len = strlen(key);
            if ((p = stristr(lineBuffer, key)) != NULL)
            {
                // End of keyword, colourise it
                styler.ColourTo(startLine + (p - lineBuffer) - 1, SCE_RESULT_DEFAULT);
                styler.ColourTo(startLine + (p - lineBuffer) + key_len - 1, SCE_RESULT_KEYWORD);
            }
        }
    }
    styler.ColourTo(endPos, stat);
}

static void
ColouriseResultDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[], Accessor &styler)
{
    char lineBuffer[SC_LINE_LENGTH];
    styler.StartAt(startPos);
    styler.StartSegment(startPos);
    unsigned int linePos = 0;
    size_t startLine = startPos;
    (void) initStyle;
    for (size_t i = startPos; i < startPos + length; i++)
    {
        lineBuffer[linePos++] = styler[i];
        if (at_eol(styler, i) || (linePos >= sizeof(lineBuffer) - 1))
        {
            lineBuffer[linePos] = '\0';
            handle_word(lineBuffer, startLine, i, styler, styler.GetLine(startLine), keywordlists);
            linePos = 0;
            startLine = i + 1;

            while (!at_eol(styler, i))
                i++;
        }
    }
    if (linePos > 0) // Last line does not have ending characters
    {
        handle_word(lineBuffer, startLine, startPos + length - 1, styler, styler.GetLine(startLine), keywordlists);
    }
}

static const char *const resultWordList[] = { "Keywords", 0 };

LexerModule lmResult(SCLEX_RESULT, ColouriseResultDoc, "result", 0, resultWordList);
