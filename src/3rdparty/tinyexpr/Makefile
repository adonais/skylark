# The toplevel directory of the source tree.
#
ROOT = ..\..\..
!include "$(ROOT)\system.mak"

SRC = .\src

# variables
STATICLIB = $(BIND)\tinyexpr.lib

CFLAGS  = -Zi -Oy $(RELEASE) $(CFLAGS)

OBJS = $(OBJD)\tinyexpr.obj

# targets
all: dirs $(STATICLIB)

dirs:
    @if not exist "$(BIND)" mkdir "$(BIND)" && echo.   Created $(BIND)
    @if not exist "$(OBJD)" mkdir "$(OBJD)" && echo.   Created $(OBJD)

$(STATICLIB): $(OBJS)
	$(AR) -out:$@ $(OBJS)

$(OBJS): $(SRC)\tinyexpr.c $(SRC)\tinyexpr.h
	$(CC) -c $(SRC)\tinyexpr.c $(CFLAGS) -Fo$(OBJD)/ -Fd$(BIND)\euapi.pdb

# cleanup
clean:
	-del /q $(STATICLIB) 2>nul
	-del /q $(OBJS) 2>nul
