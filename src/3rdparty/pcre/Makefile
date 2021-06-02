# The toplevel directory of the source tree.
#
ROOT = ..\..\..
!include "$(ROOT)\system.mak"

SRC = .\src

# variables
STATICLIB = $(BIND)\pcre.lib


CFLAGS  = -Zi -Oy -Ob1 -DHAVE_CONFIG_H -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS $(RELEASE) $(CFLAGS)

OBJS =                            \
    $(OBJD)\pcre_byte_order.obj   \
    $(OBJD)\pcre_chartables.obj	  \
    $(OBJD)\pcre_compile.obj	  \
    $(OBJD)\pcre_config.obj	      \
    $(OBJD)\pcre_dfa_exec.obj	  \
    $(OBJD)\pcre_exec.obj	      \
    $(OBJD)\pcre_fullinfo.obj	  \
    $(OBJD)\pcre_get.obj	      \
    $(OBJD)\pcre_globals.obj	  \
    $(OBJD)\pcre_jit_compile.obj  \
    $(OBJD)\pcre_maketables.obj	  \
    $(OBJD)\pcre_newline.obj	  \
    $(OBJD)\pcre_ord2utf8.obj	  \
    $(OBJD)\pcre_refcount.obj	  \
    $(OBJD)\pcre_string_utils.obj \
    $(OBJD)\pcre_study.obj	      \
    $(OBJD)\pcre_tables.obj	      \
    $(OBJD)\pcre_ucd.obj	      \
    $(OBJD)\pcre_valid_utf8.obj	  \
    $(OBJD)\pcre_version.obj	  \
    $(OBJD)\pcre_xclass.obj	      \
    
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

$(STATICLIB): $(OBJS)
	$(AR) -out:$@ $(OBJS)

$(OBJS): $(SRC)\*.c $(SRC)\*.h

# cleanup
clean:
	-del /q $(STATICLIB) 2>nul
	-del /q $(OBJS) 2>nul
