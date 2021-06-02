# Makefile for zlib using Microsoft (Visual) C
# zlib is copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler
#
# The toplevel directory of the source tree.
#
ROOT = ..\..\..
!include "$(ROOT)\system.mak"

SRC = .\src

# variables
STATICLIB = $(BIND)\libchardet.lib


CFLAGS  = -I. -W3 -EHsc -Zi -I$(SRC) -D_CRT_SECURE_NO_WARNINGS \
          -D_CRT_NONSTDC_NO_DEPRECATE -D_WIN32_WINNT=0x0600 $(RELEASE) $(CFLAGS)

# ----------------------------

OBJS = \
    $(OBJD)\CharDistribution.obj    \
    $(OBJD)\JpCntx.obj		        \
    $(OBJD)\LangArabicModel.obj		\
    $(OBJD)\LangBulgarianModel.obj	\
    $(OBJD)\LangCroatianModel.obj	\
    $(OBJD)\LangCzechModel.obj	    \
    $(OBJD)\LangDanishModel.obj	    \
    $(OBJD)\LangEsperantoModel.obj	\
    $(OBJD)\LangEstonianModel.obj	\
    $(OBJD)\LangFinnishModel.obj	\
    $(OBJD)\LangFrenchModel.obj		\
    $(OBJD)\LangGermanModel.obj		\
    $(OBJD)\LangGreekModel.obj		\
    $(OBJD)\LangHebrewModel.obj	    \
    $(OBJD)\LangHungarianModel.obj	\
    $(OBJD)\LangIrishModel.obj      \
    $(OBJD)\LangItalianModel.obj	\
    $(OBJD)\LangLatvianModel.obj	\
    $(OBJD)\LangLithuanianModel.obj	\
    $(OBJD)\LangMalteseModel.obj	\
    $(OBJD)\LangPolishModel.obj	    \
    $(OBJD)\LangPortugueseModel.obj	\
    $(OBJD)\LangRomanianModel.obj	\
    $(OBJD)\LangRussianModel.obj	\
    $(OBJD)\LangSlovakModel.obj	    \
    $(OBJD)\LangSloveneModel.obj	\
    $(OBJD)\LangSpanishModel.obj    \
    $(OBJD)\LangSwedishModel.obj	\
    $(OBJD)\LangThaiModel.obj	    \
    $(OBJD)\LangTurkishModel.obj	\
    $(OBJD)\LangVietnameseModel.obj	\
    $(OBJD)\nsBig5Prober.obj		\
    $(OBJD)\nsCharSetProber.obj		\
    $(OBJD)\nsEUCJPProber.obj		\
    $(OBJD)\nsEUCKRProber.obj		\
    $(OBJD)\nsEUCTWProber.obj	    \
    $(OBJD)\nsEscCharsetProber.obj	\
    $(OBJD)\nsEscSM.obj		        \
    $(OBJD)\nsGB2312Prober.obj	    \
    $(OBJD)\nsHebrewProber.obj	    \
    $(OBJD)\nsLatin1Prober.obj	    \
    $(OBJD)\nsMBCSGroupProber.obj	\
    $(OBJD)\nsMBCSSM.obj	        \
    $(OBJD)\nsSBCSGroupProber.obj   \
    $(OBJD)\nsSBCharSetProber.obj	\
    $(OBJD)\nsSJISProber.obj	    \
    $(OBJD)\nsUTF8Prober.obj	    \
    $(OBJD)\nsUniversalDetector.obj	\
    $(OBJD)\chardet.obj	            \

##############################################################################
##
.SUFFIXES: .cpp .h .obj

{$(SRC)}.cpp{$(OBJD)}.obj:
    $(CC) $(CFLAGS) /Fd$(BIND)\euapi.pdb /Fo$(OBJD)\ /c $<

{$(SRC)\tables}.cpp{$(OBJD)}.obj:
    $(CC) $(CFLAGS) /Fd$(BIND)\euapi.pdb /Fo$(OBJD)\ /c $<

##############################################################################

# targets
all: dirs $(STATICLIB)

dirs:
    @if not exist "$(BIND)" mkdir "$(BIND)" && echo.   Created $(BIND)
    @if not exist "$(OBJD)" mkdir "$(OBJD)" && echo.   Created $(OBJD)

$(STATICLIB): $(OBJS)
	$(AR) $(ARFLAGS) -out:$@ $(OBJS)

$(OBJS): $(SRC)\*.cpp $(SRC)\*.h

# cleanup
clean:
	-del /q $(STATICLIB) 2>nul
	-del /q $(OBJS) 2>nul
