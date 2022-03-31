# Make file for Scintilla on Windows Visual C++ version
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile is for using Visual C++ with nmake.
# Usage for Microsoft:
#     nmake -f scintilla.mak
# For debug versions define DEBUG on the command line:
#     nmake DEBUG=1 -f scintilla.mak
# The main makefile uses mingw32 gcc and may be more current than this file.

ROOT = ..\..\..
!include "$(ROOT)\system.mak"

DIR_O=$(OBJD)\scintilla
DIR_BIN=$(BIND)
SRC=src\src
SCI_DEF=src\win32\scintilla.def

LEXCOMPONENT=$(DIR_BIN)\scilexer.lib

CXXFLAGS=-Zi -TP -W4 -EHsc -std:c++17 -utf-8 -Fd"$(BIND)\euapi" $(RELEASE) $(CFLAGS)

!IF "$(CC)" != "clang-cl"
CXXFLAGS=$(CXXFLAGS) -MP
!ENDIF

NAME=-Fo
LIBS=KERNEL32.lib USER32.lib GDI32.lib IMM32.lib OLE32.lib OLEAUT32.lib MSIMG32.lib
NOLOGO=-nologo

CXXFLAGS=$(CXXFLAGS) -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -D_CRT_SECURE_NO_DEPRECATE=1 \
         -D_SCL_SECURE_NO_WARNINGS=1

INCLUDES=-I$(SRC) -Isrc/include -Isrc/lexlib
CXXFLAGS=$(CXXFLAGS) $(INCLUDES)

all:	dirs $(LEXCOMPONENT)

dirs:
    @if not exist "$(BIND)" mkdir "$(BIND)" && echo.   Created $(BIND)
    @if not exist "$(OBJD)" mkdir "$(OBJD)" && echo.   Created $(OBJD)
    @if not exist "$(DIR_O)" mkdir "$(DIR_O)" && echo.   Created $(DIR_O)

clean:
	-del /q $(DIR_O)\*.obj $(DIR_BIN)\scilexer.exp $(DIR_BIN)\scilexer.pdb $(DIR_BIN)\scilexer.lib 2>nul 1>nul
	-rd /s/q $(DIR_O) 2>nul 1>nul

# Required for base Scintilla
SRC_OBJS=\
	$(DIR_O)\AutoComplete.obj \
	$(DIR_O)\CallTip.obj \
	$(DIR_O)\CaseConvert.obj \
	$(DIR_O)\CaseFolder.obj \
	$(DIR_O)\CellBuffer.obj \
	$(DIR_O)\CharacterCategoryMap.obj \
	$(DIR_O)\CharacterType.obj \
	$(DIR_O)\CharClassify.obj \
	$(DIR_O)\ContractionState.obj \
	$(DIR_O)\DBCS.obj \
	$(DIR_O)\Decoration.obj \
	$(DIR_O)\Document.obj \
	$(DIR_O)\EditModel.obj \
	$(DIR_O)\Editor.obj \
	$(DIR_O)\EditView.obj \
	$(DIR_O)\Geometry.obj \
	$(DIR_O)\Indicator.obj \
	$(DIR_O)\KeyMap.obj \
	$(DIR_O)\LineMarker.obj \
	$(DIR_O)\MarginView.obj \
	$(DIR_O)\PerLine.obj \
	$(DIR_O)\PositionCache.obj \
	$(DIR_O)\RESearch.obj \
	$(DIR_O)\RunStyles.obj \
	$(DIR_O)\Selection.obj \
	$(DIR_O)\Style.obj \
	$(DIR_O)\UniConversion.obj \
	$(DIR_O)\UniqueString.obj \
	$(DIR_O)\ViewStyle.obj \
	$(DIR_O)\XPM.obj \
	
LEX_OBJS=\
	$(DIR_O)\LexA68k.obj \
	$(DIR_O)\LexAbaqus.obj \
	$(DIR_O)\LexAda.obj \
	$(DIR_O)\LexAPDL.obj \
	$(DIR_O)\LexAsciidoc.obj \
	$(DIR_O)\LexAsm.obj \
	$(DIR_O)\LexAsn1.obj \
	$(DIR_O)\LexASY.obj \
	$(DIR_O)\LexAU3.obj \
	$(DIR_O)\LexAVE.obj \
	$(DIR_O)\LexAVS.obj \
	$(DIR_O)\LexBaan.obj \
	$(DIR_O)\LexBash.obj \
	$(DIR_O)\LexBasic.obj \
	$(DIR_O)\LexBatch.obj \
	$(DIR_O)\LexBibTeX.obj \
	$(DIR_O)\LexBullant.obj \
	$(DIR_O)\LexCaml.obj \
	$(DIR_O)\LexCIL.obj \
	$(DIR_O)\LexCLW.obj \
	$(DIR_O)\LexCmake.obj \
	$(DIR_O)\LexCOBOL.obj \
	$(DIR_O)\LexCoffeeScript.obj \
	$(DIR_O)\LexConf.obj \
	$(DIR_O)\LexCPP.obj \
	$(DIR_O)\LexCrontab.obj \
	$(DIR_O)\LexCsound.obj \
	$(DIR_O)\LexCSS.obj \
	$(DIR_O)\LexD.obj \
	$(DIR_O)\LexDataflex.obj \
	$(DIR_O)\LexDiff.obj \
	$(DIR_O)\LexDMAP.obj \
	$(DIR_O)\LexDMIS.obj \
	$(DIR_O)\LexECL.obj \
	$(DIR_O)\LexEDIFACT.obj \
	$(DIR_O)\LexEiffel.obj \
	$(DIR_O)\LexErlang.obj \
	$(DIR_O)\LexErrorList.obj \
	$(DIR_O)\LexEScript.obj \
	$(DIR_O)\LexFlagship.obj \
	$(DIR_O)\LexForth.obj \
	$(DIR_O)\LexFortran.obj \
	$(DIR_O)\LexFSharp.obj \
	$(DIR_O)\LexGAP.obj \
	$(DIR_O)\LexGDScript.obj \
	$(DIR_O)\LexGui4Cli.obj \
	$(DIR_O)\LexHaskell.obj \
	$(DIR_O)\LexHex.obj \
	$(DIR_O)\LexHollywood.obj \
	$(DIR_O)\LexHTML.obj \
	$(DIR_O)\LexIndent.obj \
	$(DIR_O)\LexInno.obj \
	$(DIR_O)\LexJSON.obj \
	$(DIR_O)\LexJulia.obj \
	$(DIR_O)\LexKix.obj \
	$(DIR_O)\LexKVIrc.obj \
	$(DIR_O)\LexLaTeX.obj \
	$(DIR_O)\LexLisp.obj \
	$(DIR_O)\LexLout.obj \
	$(DIR_O)\LexLua.obj \
	$(DIR_O)\LexMagik.obj \
	$(DIR_O)\LexMake.obj \
	$(DIR_O)\LexMarkdown.obj \
	$(DIR_O)\LexMatlab.obj \
	$(DIR_O)\LexMaxima.obj \
	$(DIR_O)\LexMetapost.obj \
	$(DIR_O)\LexMMIXAL.obj \
	$(DIR_O)\LexModula.obj \
	$(DIR_O)\LexMPT.obj \
	$(DIR_O)\LexMSSQL.obj \
	$(DIR_O)\LexMySQL.obj \
	$(DIR_O)\LexNim.obj \
	$(DIR_O)\LexNimrod.obj \
	$(DIR_O)\LexNsis.obj \
	$(DIR_O)\LexNull.obj \
	$(DIR_O)\LexOpal.obj \
	$(DIR_O)\LexOScript.obj \
	$(DIR_O)\LexPascal.obj \
	$(DIR_O)\LexPB.obj \
	$(DIR_O)\LexPerl.obj \
	$(DIR_O)\LexPLM.obj \
	$(DIR_O)\LexPO.obj \
	$(DIR_O)\LexPOV.obj \
	$(DIR_O)\LexPowerPro.obj \
	$(DIR_O)\LexPowerShell.obj \
	$(DIR_O)\LexProgress.obj \
	$(DIR_O)\LexProps.obj \
	$(DIR_O)\LexPS.obj \
	$(DIR_O)\LexPython.obj \
	$(DIR_O)\LexR.obj \
	$(DIR_O)\LexRaku.obj \
	$(DIR_O)\LexRebol.obj \
	$(DIR_O)\LexRegistry.obj \
	$(DIR_O)\LexRuby.obj \
	$(DIR_O)\LexRust.obj \
	$(DIR_O)\LexSAS.obj \
	$(DIR_O)\LexScriptol.obj \
	$(DIR_O)\LexSmalltalk.obj \
	$(DIR_O)\LexSML.obj \
	$(DIR_O)\LexSorcus.obj \
	$(DIR_O)\LexSpecman.obj \
	$(DIR_O)\LexSpice.obj \
	$(DIR_O)\LexSQL.obj \
	$(DIR_O)\LexStata.obj \
	$(DIR_O)\LexSTTXT.obj \
	$(DIR_O)\LexTACL.obj \
	$(DIR_O)\LexTADS3.obj \
	$(DIR_O)\LexTAL.obj \
	$(DIR_O)\LexTCL.obj \
	$(DIR_O)\LexTCMD.obj \
	$(DIR_O)\LexTeX.obj \
	$(DIR_O)\LexTxt2tags.obj \
	$(DIR_O)\LexVB.obj \
	$(DIR_O)\LexVerilog.obj \
	$(DIR_O)\LexVHDL.obj \
	$(DIR_O)\LexVisualProlog.obj \
	$(DIR_O)\LexX12.obj \
	$(DIR_O)\LexYAML.obj \

#--Autogenerated -- end of automatically generated section

# Required by lexers
LEXLIB_OBJS= \
	$(DIR_O)\Accessor.obj \
	$(DIR_O)\CharacterCategory.obj \
	$(DIR_O)\CharacterSet.obj \
	$(DIR_O)\DefaultLexer.obj \
	$(DIR_O)\LexerBase.obj \
	$(DIR_O)\LexAccessor.obj \
	$(DIR_O)\LexerModule.obj \
	$(DIR_O)\LexerNoExceptions.obj \
	$(DIR_O)\LexerSimple.obj \
	$(DIR_O)\PropSetSimple.obj \
	$(DIR_O)\StyleContext.obj \
	$(DIR_O)\WordList.obj \
	$(DIR_O)\Lexilla.obj \

# Required by libraries and DLLs that include lexing
SCILEX_OBJS = \
	$(SRC_OBJS) \
	$(LEXLIB_OBJS) \
	$(LEX_OBJS) \
	$(DIR_O)\HanjaDic.obj \
	$(DIR_O)\PlatWin.obj \
	$(DIR_O)\ScintillaBaseL.obj \
	$(DIR_O)\ScintillaDLL.obj \
	$(DIR_O)\ScintillaWin.obj

STATICS_OBJS = \
	$(SRC_OBJS) \
	$(LEXLIB_OBJS) \
	$(LEX_OBJS) \
	$(DIR_O)\HanjaDic.obj \
	$(DIR_O)\PlatWin.obj \
	$(DIR_O)\ScintillaBaseL.obj \
	$(DIR_O)\ScintillaWin.obj

!IF EXISTS(src\boostregex\BoostReg.mak)
# To have PCRE
!INCLUDE src\boostregex\BoostReg.mak
!ENDIF

$(LEXCOMPONENT): $(STATICS_OBJS)
	$(AR) -out:$@ $(STATICS_OBJS)

# Define how to build all the objects and what they depend on
.SUFFIXES: .cxx

{$(SRC)}.cxx{$(DIR_O)}.obj::
	$(CXX) $(CXXFLAGS) -c $(NAME)$(DIR_O)\ $<
{src\lexlib}.cxx{$(DIR_O)}.obj::
	$(CXX) $(CXXFLAGS) -c $(NAME)$(DIR_O)\ $<
{src\lexers}.cxx{$(DIR_O)}.obj::
	$(CXX) $(CXXFLAGS) -c $(NAME)$(DIR_O)\ $<
{src\win32}.cxx{$(DIR_O)}.obj::
	$(CXX) $(CXXFLAGS) -c $(NAME)$(DIR_O)\ $<
{.}.cxx{$(DIR_O)}.obj::
	$(CXX) $(CXXFLAGS) -c $(NAME)$(DIR_O)\ $<

# Some source files are compiled into more than one object because of different conditional compilation
$(DIR_O)\ScintillaBaseL.obj: $(SRC)\ScintillaBase.cxx
	$(CXX) $(CXXFLAGS) -DSCI_LEXER -c $(NAME)$@ $(SRC)\ScintillaBase.cxx

# Dependencies

!IF EXISTS(src\win32\nmdeps.mak)

# Protect with !IF EXISTS to handle accidental deletion

!INCLUDE src\win32\nmdeps.mak

!ENDIF

