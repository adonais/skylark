ROOT = ..

!include "$(ROOT)\system.mak"

CDEP    =  -I3rdparty\libcurl\include -I3rdparty\libopenssl\include -I3rdparty\libpqodbc\include \
           -I3rdparty\libmysql\include -I3rdparty\liboci\include -I3rdparty\libhiredis\include \
           -I3rdparty\libiconv\include -I3rdparty\luajit\src -I3rdparty\nanosvg\include \
           -I3rdparty\pcre\src -I3rdparty\sqlite3\src -I3rdparty\chardet\src \
           -I3rdparty\qrencode\src -I3rdparty\tinyexpr\src \
           -I3rdparty\scintilla\src\include

CFLAGS  = -W3 -WX- -wd4996 -wd4819 -Zi -D "_CRT_SECURE_NO_WARNINGS" -D "UNICODE" -D "_UNICODE" \
          -utf-8 -D "JSON_TRACK_SOURCE" -D "PCRE_STATIC" $(CFLAGS) $(CDEP)

!IF "$(ACTIONS_BUILDING)"=="1"
CFLAGS = $(CFLAGS) -D "ACTIONS_BUILDING"
!ENDIF

API_CLFGAS = $(CFLAGS) -D "EUAPI_BUILDDING"

CXXFLAGS = $(CFLAGS) /TP /EHsc

LDFLAGS = $(LDFLAGS) -MACHINE:$(PLATFORM) -ENTRY:wmainCRTStartup
# /LARGEADDRESSAWARE /ENTRY:mainCRTStartup $(HIDE) $(NO_HIDE)

!IF "$(APP_DEBUG)"=="1"
LDFLAGS = $(LDFLAGS) $(NO_HIDE)
!ELSE
LDFLAGS = $(LDFLAGS) $(HIDE)
!ENDIF

LDEP = $(BIND)\euapi.lib

WINLIBS = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib \
          ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib dbghelp.lib \
          ws2_32.lib imm32.lib msimg32.lib

LIBS = $(LDEP) $(WINLIBS)

API_LIBS = $(BIND)\libchardet.lib $(BIND)\qrencode.lib $(BIND)\sqlite3.lib $(BIND)\pcre.lib \
           $(BIND)\luajit.lib $(BIND)\iconv.lib $(BIND)\scilexer.lib $(BIND)\tinyexpr.lib


API_DEPS = $(API_LIBS) -LIBPATH:$(BIND) $(WINLIBS)

!IF "$(APP_DEBUG)"=="1"
CFLAGS = $(CFLAGS) $(DEBUG_L)
!ELSE
CFLAGS = $(CFLAGS) $(RELEASE)
!ENDIF

OBJS = \
    $(OBJD)\eu_main.obj                        \
    $(OBJD)\resource.res                       \

EUAPI_OBJS = \
    $(OBJD)\eu_map.obj                         \
    $(OBJD)\eu_rbtree.obj                      \
    $(OBJD)\eu_api.obj                         \
    $(OBJD)\eu_sql.obj                         \
    $(OBJD)\eu_doctype.obj                     \
    $(OBJD)\eu_edit.obj                        \
    $(OBJD)\eu_hex.obj                         \
    $(OBJD)\eu_scintilla.obj                   \
    $(OBJD)\eu_encoding.obj                    \
    $(OBJD)\eu_encoding_utf8.obj               \
    $(OBJD)\eu_registry.obj                    \
    $(OBJD)\eu_file.obj                        \
    $(OBJD)\eu_treebar.obj                     \
    $(OBJD)\eu_resultctl.obj                   \
    $(OBJD)\eu_tablectl.obj                    \
    $(OBJD)\eu_remotefs.obj                    \
    $(OBJD)\eu_search.obj                      \
    $(OBJD)\eu_code.obj                        \
    $(OBJD)\eu_theme.obj                       \
    $(OBJD)\eu_theme_dark.obj                  \
    $(OBJD)\eu_theme_menu.obj                  \
    $(OBJD)\eu_symlistctl.obj                  \
    $(OBJD)\eu_symtreectl.obj                  \
    $(OBJD)\eu_tabpage.obj                     \
    $(OBJD)\eu_util.obj                        \
    $(OBJD)\eu_view.obj                        \
    $(OBJD)\eu_proc.obj                        \
    $(OBJD)\eu_menu.obj                        \
    $(OBJD)\eu_config.obj                      \
    $(OBJD)\eu_input.obj                       \
    $(OBJD)\eu_share.obj                       \
    $(OBJD)\eu_log.obj                         \
    $(OBJD)\eu_hook.obj                        \
    $(OBJD)\eu_script.obj                      \
    $(OBJD)\eu_changes.obj                     \
    $(OBJD)\eu_toolbar.obj                     \
    $(OBJD)\eu_statusbar.obj                   \
    $(OBJD)\eu_about.obj                       \
    $(OBJD)\eu_qrgen.obj                       \
    $(OBJD)\eu_json.obj                        \
    $(OBJD)\eu_print.obj                       \
    $(OBJD)\eu_locale.obj                      \
    $(OBJD)\eu_format.obj                      \
    $(OBJD)\eu_splitter.obj                    \
    $(OBJD)\eu_document_map.obj                \
    $(OBJD)\eu_updatechecker.obj               \
    $(OBJD)\eu_snippet.obj                     \
    $(OBJD)\eu_snparser.obj                    \
    $(OBJD)\eu_complete.obj                    \
    $(OBJD)\eu_nphost.obj                      \
    $(OBJD)\eu_pixmap.obj                      \
    $(OBJD)\eu_hyperlink.obj                   \
    $(OBJD)\eu_exporter.obj                    \
    $(OBJD)\eu_session.obj                     \
    $(OBJD)\eu_favorites.obj                   \

SHARED_LIB = $(BIND)\euapi.dll

##############################################################################

.SUFFIXES: .cpp .cc .c .h .obj

.c{$(OBJD)}.obj:
	$(CC) $(API_CLFGAS) /Fd$(BIND)\euapi.pdb /Fo$(OBJD)\ /c $<

.cpp{$(OBJD)}.obj:
	$(CC) $(API_CLFGAS) /Fd$(BIND)\euapi.pdb /Fo$(OBJD)\ /c $<

.rc{$(OBJD)}.res:
	$(RC) $(RFLAGS) /Fo$@ $<

##############################################################################

all: dirs $(SHARED_LIB) $(BIND)\skylark.exe


##############################################################################

clean:
	-rmdir /q /s $(OBJD) $(BIND) 2>nul

##############################################################################

dirs:
	@if not exist "$(BIND)" mkdir "$(BIND)" && echo.   Created $(BIND)
	@if not exist "$(BIND)\\conf" mkdir "$(BIND)\\conf" && echo.   Created $(BIND)\\conf
	@if not exist "$(BIND)\share" mkdir "$(BIND)\share"
	@if not exist "$(BIND)\plugins" mkdir "$(BIND)\plugins"
	@if exist "$(ROOT)\\conf" xcopy "$(ROOT)\\conf" "$(BIND)\\conf" /s /e /q /y 2>&1>NUL
	@if exist "$(ROOT)\share" xcopy "$(ROOT)\share" "$(BIND)\share" /s /e /q /y 2>&1>NUL
	@if exist "$(ROOT)\\README_CN.MD" copy /y "$(ROOT)\\README_CN.MD" "$(BIND)\\README_CN.MD" 2>&1>NUL
	@if exist "$(MAKEDIR)\\3rdparty\\plugins" xcopy "$(MAKEDIR)\\3rdparty\\plugins" "$(BIND)\\plugins" /s /e /q /y 2>&1>NUL
	@if not exist "$(OBJD)" mkdir "$(OBJD)" && echo.   Created $(OBJD)

$(SHARED_LIB): $(EUAPI_OBJS) $(API_LIBS)
	$(LD) $(DLLFLAGS) -out:$@ $(EUAPI_OBJS) $(API_DEPS)

$(BIND)\skylark.exe : $(OBJS) $(SHARED_LIB)
	$(LD) -out:$@ $(OBJS) $(LDFLAGS) $(LIBS)

$(OBJD)\eu_main.obj                        : eu_main.c framework.h
	$(CC) $(CFLAGS) /Fd$(BIND)\skylark.pdb /Fo$(OBJD)\ /c eu_main.c
$(OBJD)\eu_doctype.obj                     : eu_doctype.c eu_doctype.h framework.h
$(OBJD)\eu_edit.obj                        : eu_edit.c eu_edit.h framework.h
$(OBJD)\eu_hex.obj                         : eu_hex.c eu_hex.h framework.h
$(OBJD)\eu_scintilla.obj                   : eu_scintilla.c eu_scintilla.h framework.h
$(OBJD)\eu_encoding.obj                    : eu_encoding.c eu_encoding.h framework.h
$(OBJD)\eu_registry.obj                    : eu_registry.c eu_registry.h framework.h
$(OBJD)\eu_file.obj                        : eu_file.c eu_file.h eu_nphost.h eu_npruntime.h framework.h
$(OBJD)\eu_treebar.obj                     : eu_treebar.c eu_treebar.h framework.h eu_list.h
$(OBJD)\eu_resultctl.obj                   : eu_resultctl.c eu_resultctl.h framework.h
$(OBJD)\eu_tablectl.obj                    : eu_tablectl.c eu_tablectl.h framework.h
$(OBJD)\eu_remotefs.obj                    : eu_remotefs.c eu_remotefs.h framework.h
$(OBJD)\eu_search.obj                      : eu_search.c eu_search.h framework.h
$(OBJD)\eu_code.obj                        : eu_code.c eu_code.h framework.h
$(OBJD)\eu_theme.obj                       : eu_theme.c eu_theme.h framework.h
$(OBJD)\eu_theme_dark.obj                  : eu_theme_dark.c eu_theme_dark.h framework.h
$(OBJD)\eu_theme_menu.obj                  : eu_theme_menu.c eu_theme_menu.h framework.h
$(OBJD)\eu_symlistctl.obj                  : eu_symlistctl.c eu_symlistctl.h framework.h eu_list.h
$(OBJD)\eu_symtreectl.obj                  : eu_symtreectl.c eu_symtreectl.h framework.h eu_list.h
$(OBJD)\eu_tabpage.obj                     : eu_tabpage.c eu_tabpage.h eu_nphost.h eu_npruntime.h framework.h
$(OBJD)\eu_util.obj                        : eu_util.c eu_util.h framework.h
$(OBJD)\eu_view.obj                        : eu_view.c eu_view.h eu_nphost.h eu_npruntime.h framework.h
$(OBJD)\eu_proc.obj                        : eu_proc.c eu_proc.h eu_nphost.h eu_npruntime.h framework.h
$(OBJD)\eu_menu.obj                        : eu_menu.c eu_menu.h framework.h
$(OBJD)\eu_config.obj                      : eu_config.c eu_config.h framework.h
$(OBJD)\eu_input.obj                       : eu_input.c eu_input.h framework.h
$(OBJD)\eu_share.obj                       : eu_share.c eu_share.h framework.h
$(OBJD)\eu_log.obj                         : eu_log.c eu_log.h framework.h
$(OBJD)\eu_hook.obj                        : eu_hook.c eu_hook.h
$(OBJD)\eu_script.obj                      : eu_script.c eu_script.h framework.h
$(OBJD)\eu_sql.obj                         : eu_sql.c eu_sql.h framework.h
$(OBJD)\eu_api.obj                         : eu_api.c eu_api.h framework.h
$(OBJD)\eu_map.obj                         : eu_map.c eu_map.h
$(OBJD)\eu_changes.obj                     : eu_changes.c eu_changes.h framework.h
$(OBJD)\eu_toolbar.obj                     : eu_toolbar.c eu_toolbar.h framework.h
$(OBJD)\eu_statusbar.obj                   : eu_statusbar.c eu_statusbar.h framework.h
$(OBJD)\eu_about.obj                       : eu_about.c eu_about.h framework.h skylark.rc targetver.h skylark.exe.manifest $(ROOT)\locales\src\*
$(OBJD)\eu_qrgen.obj                       : eu_qrgen.c eu_qrgen.h framework.h
$(OBJD)\eu_json.obj                        : eu_json.c eu_json.h framework.h
$(OBJD)\eu_print.obj                       : eu_print.c eu_print.h eu_nphost.h eu_npruntime.h framework.h
$(OBJD)\eu_locale.obj                      : eu_locale.c eu_locale.h framework.h
$(OBJD)\eu_format.obj                      : eu_format.c eu_format.h framework.h
$(OBJD)\eu_splitter.obj                    : eu_splitter.c eu_splitter.h framework.h
$(OBJD)\eu_rbtree.obj                      : eu_rbtree.c eu_rbtree.h framework.h
$(OBJD)\eu_document_map.obj                : eu_document_map.c eu_document_map.h eu_cvector.h framework.h
$(OBJD)\eu_updatechecker.obj               : eu_updatechecker.c eu_updatechecker.h eu_cvector.h framework.h
$(OBJD)\eu_snippet.obj                     : eu_snippet.c eu_snippet.h eu_cvector.h framework.h
$(OBJD)\eu_snparser.obj                    : eu_snparser.c eu_snparser.h eu_cvector.h framework.h
$(OBJD)\eu_complete.obj                    : eu_complete.c eu_complete.h eu_cvector.h framework.h
$(OBJD)\eu_nphost.obj                      : eu_nphost.c eu_nphost.h eu_npruntime.h framework.h
$(OBJD)\eu_pixmap.obj                      : eu_pixmap.c eu_pixmap.h framework.h
$(OBJD)\eu_hyperlink.obj                   : eu_hyperlink.c eu_hyperlink.h framework.h
$(OBJD)\eu_exporter.obj                    : eu_exporter.c eu_exporter.h framework.h
$(OBJD)\eu_session.obj                     : eu_session.c eu_session.h framework.h
$(OBJD)\eu_favorites.obj                   : eu_favorites.c eu_favorites.h framework.h
$(OBJD)\eu_encoding_utf8.obj               : eu_encoding_utf8.c eu_encoding_utf8.h framework.h
	$(CC) $(API_CLFGAS) -Fd$(BIND)\euapi.pdb -Fo$(OBJD)\ $(MAVX2) -c eu_encoding_utf8.c
$(OBJD)\resource.res                       : skylark.rc targetver.h skylark.exe.manifest $(ROOT)\resources\*.bmp
	$(RC) $(RFLAGS) -Fo$@ skylark.rc
