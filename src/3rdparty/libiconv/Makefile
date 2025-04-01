# The toplevel directory of the source tree.
#
ROOT = ..\..\..
!include "$(ROOT)\system.mak"

SRC = .\src

# variables
STATICLIB = $(BIND)\iconv.lib

CFLAGS  = -Zi -Oy -utf-8 -I$(SRC) -D_WIN32_WINNT=_WIN32_WINNT_WIN7 -DHAVE_CONFIG_H $(RELEASE) $(CFLAGS)

ICONV_CFLAGS   = $(CFLAGS) -Iinclude -I$(SRC)\lib -DBUILDING_LIBICONV -DBUILDING_LIBCHARSET
CHARSET_CFLAGS = $(CFLAGS) -I$(SRC)\libcharset -DBUILDING_LIBCHARSET

OBJS = $(OBJD)\iconv.obj              \
	   $(OBJD)\localcharset.obj       \

# targets
all: dirs $(STATICLIB)

dirs:
    @if not exist "$(BIND)" mkdir "$(BIND)" && echo.   Created $(BIND)
    @if not exist "$(OBJD)" mkdir "$(OBJD)" && echo.   Created $(OBJD)

$(STATICLIB): $(OBJS)
	$(AR) -out:$@ $(OBJS)

$(OBJD)\iconv.obj            : $(SRC)\lib\iconv.c
	$(CC) -c $(SRC)\lib\iconv.c $(ICONV_CFLAGS) -Fo$(OBJD)\ -Fd$(BIND)\euapi

$(OBJD)\localcharset.obj     : $(SRC)\libcharset\localcharset.c
	$(CC) -c $(SRC)\libcharset\localcharset.c $(CHARSET_CFLAGS) -Fo$(OBJD)\ -Fd$(BIND)\euapi

# cleanup
clean:
	-del /q $(STATICLIB) 2>nul
	-del /q $(OBJS) 2>nul
	-del /q $(BIND)\iconv.pdb 2>nul
