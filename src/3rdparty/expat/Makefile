# The toplevel directory of the source tree.
#
ROOT = ..\..\..
!include "$(ROOT)\system.mak"

SRC = .\src

# variables
# SHARED_LIB = $(BIND)\expat.dll

STATICLIB = $(BIND)\expat.lib

CFLAGS  = -Oy -DNDEBUG=1 -I. -W3 -EHsc  -Zi -I$(SRC) -D_CRT_SECURE_NO_WARNINGS \
          -D_CRT_NONSTDC_NO_DEPRECATE -D_WIN32_WINNT=0x0600 -DHAVE_CONFIG_H \
          $(RELEASE) $(CFLAGS)

#LDFLAGS = $(DLLFLAGS) -def:src/expat.def

OBJS = \
    $(OBJD)\xmlparse.obj    \
    $(OBJD)\xmltok.obj		\
    $(OBJD)\xmlrole.obj		\

##############################################################################
##
.SUFFIXES: .c .h .obj

{$(SRC)}.c{$(OBJD)}.obj:
    $(CC) $(CFLAGS) -Fd$(BIND)\euapi.pdb -Fo$(OBJD)\ /c $<
    	
# targets
all: dirs $(STATICLIB)

dirs:
    @if not exist "$(BIND)" mkdir "$(BIND)" && echo.   Created $(BIND)
    @if not exist "$(OBJD)" mkdir "$(OBJD)" && echo.   Created $(OBJD)

#$(SHARED_LIB): $(OBJS)
#	$(LD) $(LDFLAGS) -out:$@ $(OBJS)

$(STATICLIB): $(OBJS)
	$(AR) -out:$@ $(OBJS)

$(OBJS): $(SRC)\*.c $(SRC)\*.h

# cleanup
clean:
	-del /q $(STATICLIB) 2>nul
	-del /q $(OBJS) 2>nul
