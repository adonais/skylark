# The toplevel directory of the source tree.
#
ROOT = ..\..\..
!include "$(ROOT)\system.mak"

SRC = .\src

# variables
STATICLIB = $(BIND)\iconv.lib

CFLAGS  = -Zi -Oy -utf-8 -I$(SRC) -Iinclude -I$(SRC)\lib -I$(SRC)\libcharset -D_WIN32_WINNT=_WIN32_WINNT_WIN7 -DENABLE_RELOCATABLE=1 \
          -DIN_LIBRARY -DNO_XMALLOC -Dset_relocation_prefix=libiconv_set_relocation_prefix -Drelocate=libiconv_relocate \
          -DENABLE_EXTRA=1 -Drelocate2=libiconv_relocate2 -DHAVE_CONFIG_H $(RELEASE) $(CFLAGS)


OBJS = $(OBJD)\iconv.obj         \
	   $(OBJD)\localcharset.obj  \
	   $(OBJD)\relocatable.obj   \

# targets
all: dirs $(STATICLIB)

dirs:
    @if not exist "$(BIND)" mkdir "$(BIND)" && echo.   Created $(BIND)
    @if not exist "$(OBJD)" mkdir "$(OBJD)" && echo.   Created $(OBJD)

$(STATICLIB): $(OBJS)
	$(AR) -out:$@ $(OBJS)

$(OBJD)\iconv.obj       : $(SRC)\lib\iconv.c
	$(CC) -c $(SRC)\lib\iconv.c $(CFLAGS) -Fo$(OBJD)/ -Fd$(BIND)\euapi

$(OBJD)\localcharset.obj: $(SRC)\libcharset\localcharset.c
	$(CC) -c $(SRC)\libcharset\localcharset.c $(CFLAGS) -Fo$(OBJD)/ -Fd$(BIND)\euapi
	
$(OBJD)\relocatable.obj : $(SRC)\lib\relocatable.c $(SRC)\lib\relocatable.h
	$(CC) -c $(SRC)\lib\relocatable.c $(CFLAGS) -Fo$(OBJD)/ -Fd$(BIND)\euapi
		
# cleanup
clean:
	-del /q $(STATICLIB) 2>nul
	-del /q $(OBJS) 2>nul
	-del /q $(BIND)\iconv.pdb 2>nul
