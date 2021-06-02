# Makefile for zlib using Microsoft (Visual) C
# zlib is copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler
#
# The toplevel directory of the source tree.
#
ROOT = ..
!include "$(ROOT)\system.mak"

SRC = .\src

# variables
RFLAGS = /nologo /i$(ROOT)\src
SHARED_ZHCN = $(BIND)\locales\zh-cn.dll
SHARED_ENUS = $(BIND)\locales\en-us.dll

{$(SRC)\}.rc{$(OBJD)}.res:
    $(RC) $(RFLAGS) /Fo$@ $<

# targets
all: dirs $(SHARED_ZHCN) $(SHARED_ENUS)

dirs:
    @if not exist "$(BIND)" mkdir "$(BIND)" && echo.   Created $(BIND)
    @if not exist "$(OBJD)" mkdir "$(OBJD)" && echo.   Created $(OBJD)
    @if not exist "$(BIND)\\locales" mkdir "$(BIND)\\locales" && echo. Created $(BIND)\\locales

$(SHARED_ZHCN): $(OBJD)\zh-cn.res
	$(LD) /DLL /out:$@ $(OBJD)\zh-cn.res /NOENTRY /MACHINE:$(PLATFORM)
	
$(SHARED_ENUS): $(OBJD)\en-us.res
	$(LD) /DLL /out:$@ $(OBJD)\en-us.res /NOENTRY /MACHINE:$(PLATFORM)

$(OBJD)\zh-cn.res: $(SRC)\zh-cn.rc $(ROOT)\src\resource.h $(ROOT)\src\targetver.h
$(OBJD)\en-us.res: $(SRC)\en-us.rc $(ROOT)\src\resource.h $(ROOT)\src\targetver.h

# cleanup
clean:
	-del /q $(SHARED_ZHCN) $(SHARED_ENUS) 2>nul
	-del /q $(OBJS) 2>nul
