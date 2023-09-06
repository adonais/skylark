# The toplevel directory of the source tree.
#
ROOT = ..\..\..
!include "$(ROOT)\system.mak"

SRC = .\src

# variables
STATICLIB = $(BIND)\sqlite3.lib

CFLAGS  = -Zi -Oy -DTRIMMED=1 -DSQLITE_SECURE_DELETE=1 -DSQLITE_THREADSAFE=0 \
          -DSQLITE_CORE=1 -DSQLITE_ENABLE_UNLOCK_NOTIFY=1 -DSQLITE_ENABLE_DBSTAT_VTAB=1 \
          -DSQLITE_DEFAULT_PAGE_SIZE=32768 -DSQLITE_MAX_DEFAULT_PAGE_SIZE=32768 -DSQLITE_WIN32_GETVERSIONEX=0 \
          -DSQLITE_ALLOW_URI_AUTHORITY=1 -DSQLITE_TEMP_STORE=2 -DSQLITE_ENABLE_EXPLAIN_COMMENTS -DSQLITE_ENABLE_COLUMN_METADATA \
          -DSQLITE_OMIT_DEPRECATED -DSQLITE_OMIT_BUILTIN_TEST -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_OMIT_JSON -DSQLITE_OMIT_UTF16 \
          $(RELEASE) $(CFLAGS)

!IF "$(USE_CL)" == "1"
CFLAGS  = $(CFLAGS) -DSQLITE_USE_SEH=1
!ENDIF

OBJS = $(OBJD)\sqlite3.obj

# targets
all: dirs $(STATICLIB)

dirs:
    @if not exist "$(BIND)" mkdir "$(BIND)" && echo.   Created $(BIND)
    @if not exist "$(OBJD)" mkdir "$(OBJD)" && echo.   Created $(OBJD)

$(STATICLIB): $(OBJS)
	$(AR) -out:$@ $(OBJS)

$(OBJS): $(SRC)\sqlite3.c $(SRC)\sqlite3.h
	$(CC) -c $(SRC)\sqlite3.c $(CFLAGS) -Fo$(OBJD)/ -Fd$(BIND)\euapi.pdb

# cleanup
clean:
	-del /q $(STATICLIB) 2>nul
	-del /q $(OBJS) 2>nul
