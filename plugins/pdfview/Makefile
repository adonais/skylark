##############################################################################
##

ROOT=..\..
!include "$(ROOT)\system.mak"

INCD = -I$(ROOT)\src\3rdparty\libcurl\include -I$(ROOT)\src\3rdparty\libopenssl\include -I$(ROOT)\src\3rdparty\libpqodbc\include \
       -I$(ROOT)\src\3rdparty\libmysql\include -I$(ROOT)\src\3rdparty\liboci\include -I$(ROOT)\src\3rdparty\libhiredis\include \
       -I$(ROOT)\src\3rdparty\libiconv\include -I$(ROOT)\src\3rdparty\luajit\src \
       -I$(ROOT)\src\3rdparty\pcre\src -I$(ROOT)\src\3rdparty\sqlite3\src -I$(ROOT)\src\3rdparty\chardet\src \
       -I$(ROOT)\src\3rdparty\qrencode\src -I$(ROOT)\src\3rdparty\scintilla\src\include -I$(INCD)
           
SHARED_LIB = $(BIND)\plugins\np_pdfviewer.dll
CFLAGS  = -Zi -Oy -utf-8 -D "_CRT_SECURE_NO_WARNINGS" -D "UNICODE" -D "_UNICODE" $(INCD) -I. $(RELEASE) $(CFLAGS)
LIBS = $(LIBS) $(BIND)\euapi.lib kernel32.lib user32.lib advapi32.lib gdi32.lib
OBJS = $(OBJD)\plugins\np_pdfviewer.obj $(OBJD)\plugins\np_pdfviewer.res

.c{$(OBJD)\plugins}.obj:
    $(CC) -c $(CFLAGS) -Fd$(BIND)\euapi.pdb -Fo$@ $<
    
.cpp{$(OBJD)\plugins}.obj:
    $(CXX) -c $(CXXFLAGS) -Fd$(BIND)\euapi.pdb -Fo$@ $<

all: dirs $(SHARED_LIB)

##############################################################################

clean:
    -del /q /s $(OBJS) 2>nul
    -del /q $(SHARED_LIB) 2>nul

##############################################################################

dirs:
    @if not exist $(OBJD)\plugins mkdir $(OBJD)\plugins && echo.   Created $(OBJD)\plugins
    @if not exist $(BIND)\plugins mkdir $(BIND)\plugins && echo.   Created $(BIND)\plugins

$(OBJD)\plugins\np_pdfviewer.obj : np_pdfviewer.c

$(OBJD)\plugins\np_pdfviewer.res : np_pdfviewer.rc
	$(RC) $(RFLAGS) -Fo$@ %s
	
$(SHARED_LIB) : $(OBJS)
    $(LD) $(DLLFLAGS) -out:$@ $(OBJS) $(LIBS)

################################################################# End of File.
