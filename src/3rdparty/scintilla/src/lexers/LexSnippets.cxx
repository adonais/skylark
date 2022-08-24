// Scintilla source code edit control
/** @file LexSnippets.cxx
 ** Lexer for snippets file (skylark edit used)
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

static void ColouriseSnippetsDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[], Accessor &styler)
{
    (void)initStyle;
	(void)keywordlists;
	if (length > 0) {
		styler.StartAt(startPos + length - 1);
		styler.StartSegment(startPos + length - 1);
		styler.ColourTo(startPos + length - 1, 0);
	}
}

LexerModule lmSnippets(SCLEX_SNIPPET, ColouriseSnippetsDoc, "snippets");
