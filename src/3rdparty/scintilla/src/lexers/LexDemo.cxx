// Scintilla source code edit control
/** @file LexDemo.cxx
 ** Lexer for snippets demo (skylark edit used)
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

#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

static inline bool IsANumChar(int ch) {
	return ((ch >= '1') && (ch <= '9')) || (ch == 'n');
}

static void ColouriseDemoDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[], Accessor &styler)
{
    (void)keywordlists;
    styler.StartAt(startPos);
    StyleContext sc(startPos, length, initStyle, styler);

    for (; sc.More(); sc.Forward())
    {
        // Determine if the current state should terminate.
        switch (sc.state)
        {
            case SCE_DEMO_DEFAULT:
                if (sc.ch == '0' && sc.chPrev == '{' && sc.chNext == ':')
                {
                    sc.SetState(SCE_DEMO_CARETSTART);
                }
                else if (sc.ch == '$' && sc.chPrev != '\\' && (IsADigit(sc.chNext) || sc.chNext == '{'))
                {
                    sc.SetState(SCE_DEMO_CARETSTART);
                }
                else if (IsANumChar(sc.ch) && sc.chPrev == '{' && sc.chNext == ':')
                {
                    sc.SetState(SCE_DEMO_MARKNUMBER);
                }
                break;
            case SCE_DEMO_CARETSTART:
                if (sc.ch == '0')
                    sc.SetState(SCE_DEMO_MARK0);
                else if (IsANumChar(sc.ch) && sc.chPrev != '\\')
                    sc.SetState(SCE_DEMO_MARKNUMBER);
                else
                    sc.SetState(SCE_DEMO_DEFAULT);
                break;
            case SCE_DEMO_MARK0:
            case SCE_DEMO_MARKNUMBER:    
                if (sc.chNext != '{')
                {
                    sc.SetState(SCE_DEMO_DEFAULT);
                }
                break;
            default:
                sc.SetState(SCE_DEMO_DEFAULT);
                break;
        }
    }
    sc.Complete();
}

LexerModule lmDemo(SCLEX_DEMO, ColouriseDemoDoc, "eu_demo", nullptr);
