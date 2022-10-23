ROOT = .
!include "$(ROOT)\system.mak"

all:
    @if exist "$(MAKEDIR)\src\3rdparty\sqlite3\Makefile" cd "$(MAKEDIR)\src\3rdparty\sqlite3" && $(MAKE)  /NOLOGO /$(MAKEFLAGS)
    @if exist "$(MAKEDIR)\src\3rdparty\pcre\Makefile" cd "$(MAKEDIR)\src\3rdparty\pcre" && $(MAKE)  /NOLOGO /$(MAKEFLAGS)
    @if exist "$(MAKEDIR)\src\3rdparty\libiconv\Makefile" cd "$(MAKEDIR)\src\3rdparty\libiconv" && $(MAKE)  /NOLOGO /$(MAKEFLAGS)
    @if exist "$(MAKEDIR)\src\3rdparty\luajit\Makefile" cd "$(MAKEDIR)\src\3rdparty\luajit" && $(MAKE)  /NOLOGO /$(MAKEFLAGS)
    @if exist "$(MAKEDIR)\src\3rdparty\chardet\Makefile" cd "$(MAKEDIR)\src\3rdparty\chardet" && $(MAKE)  /NOLOGO /$(MAKEFLAGS)
    @if exist "$(MAKEDIR)\src\3rdparty\qrencode\Makefile" cd "$(MAKEDIR)\src\3rdparty\qrencode" && $(MAKE)  /NOLOGO /$(MAKEFLAGS)
    @if exist "$(MAKEDIR)\src\3rdparty\boost\Makefile" cd "$(MAKEDIR)\src\3rdparty\boost" && $(MAKE)  /NOLOGO /$(MAKEFLAGS)
    @if exist "$(MAKEDIR)\src\3rdparty\scintilla\Makefile" cd "$(MAKEDIR)\src\3rdparty\scintilla" && $(MAKE)  /NOLOGO /$(MAKEFLAGS)
    cd "$(MAKEDIR)\src"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS)
    cd "$(MAKEDIR)\locales"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS)
    cd "$(MAKEDIR)"
    
package: all
!IF "$(BITS)" == "64"
	@if exist "$(MAKEDIR)\skylark_x64" rd /s/q "$(MAKEDIR)\skylark_x64"
	@if exist "$(MAKEDIR)\skylark_x64.7z" del /s/q "$(MAKEDIR)\skylark_x64.7z"
	@if not exist "$(MAKEDIR)\skylark_x64" mkdir "$(MAKEDIR)\skylark_x64"
	@if exist "$(MAKEDIR)\skylark_x64" xcopy /y /e /d "$(BIND)" "$(MAKEDIR)\skylark_x64"&del /s/q/a/f "$(MAKEDIR)\skylark_x64\*.pdb" 2>&1>NUL
	-@del /s/q/a/f "$(MAKEDIR)\skylark_x64\*.exp" 2>nul 1>nul
	-@del /s/q/a/f "$(MAKEDIR)\skylark_x64\*.lib" 2>nul 1>nul
	-@del /s/q/a/f "$(MAKEDIR)\skylark_x64\conf\*.sqlite3" 2>nul 1>nul
	@if exist "%WindowsSdkDir%Redist\%WindowsSDKVersion%ucrt\DLLs\x64" copy /y "%WindowsSdkDir%Redist\%WindowsSDKVersion%ucrt\DLLs\x64\*" "$(MAKEDIR)\skylark_x64" 2>&1>NUL
	@if exist "%VCToolsRedistDir%x64\Microsoft.VC142.CRT\msvcp140.dll" copy /y "%VCToolsRedistDir%x64\Microsoft.VC142.CRT\msvcp140.dll" "$(MAKEDIR)\skylark_x64" 2>&1>NUL
	@if exist "%VCToolsRedistDir%x64\Microsoft.VC142.CRT\vcruntime140.dll" copy /y "%VCToolsRedistDir%x64\Microsoft.VC142.CRT\vcruntime140*.dll" "$(MAKEDIR)\skylark_x64" 2>&1>NUL
	@7z a -t7z "$(MAKEDIR)\skylark_x64$(RELEASE_VERSION).7z" "$(MAKEDIR)\skylark_x64" -mx9 -r -y
!ELSE
	@if exist "$(MAKEDIR)\skylark_x86" rd /s/q "$(MAKEDIR)\skylark_x86"
	@if exist "$(MAKEDIR)\skylark_x86.7z" del /s/q "$(MAKEDIR)\skylark_x86.7z"
	@if not exist "$(MAKEDIR)\skylark_x86" mkdir "$(MAKEDIR)\skylark_x86"
	@if exist "$(MAKEDIR)\skylark_x86" xcopy /y /e /d "$(BIND)" "$(MAKEDIR)\skylark_x86"&del /s/q/a/f "$(MAKEDIR)\skylark_x86\*.pdb" 2>&1>NUL
	-@del /s/q/a/f "$(MAKEDIR)\skylark_x86\*.exp" 2>nul 1>nul
	-@del /s/q/a/f "$(MAKEDIR)\skylark_x86\*.lib" 2>nul 1>nul
	-@del /s/q/a/f "$(MAKEDIR)\skylark_x86\conf\*.sqlite3" 2>nul 1>nul
	@if exist "%WindowsSdkDir%Redist\%WindowsSDKVersion%ucrt\DLLs\x86" copy /y "%WindowsSdkDir%Redist\%WindowsSDKVersion%ucrt\DLLs\x86\*" "$(MAKEDIR)\skylark_x86" 2>&1>NUL
	@if exist "%VCToolsRedistDir%x86\Microsoft.VC142.CRT\msvcp140.dll" copy /y "%VCToolsRedistDir%x86\Microsoft.VC142.CRT\msvcp140.dll" "$(MAKEDIR)\skylark_x86" 2>&1>NUL
	@if exist "%VCToolsRedistDir%x86\Microsoft.VC142.CRT\vcruntime140.dll" copy /y "%VCToolsRedistDir%x86\Microsoft.VC142.CRT\vcruntime140*.dll" "$(MAKEDIR)\skylark_x86" 2>&1>NUL
	@7z a -t7z "$(MAKEDIR)\skylark_x86$(RELEASE_VERSION).7z" "$(MAKEDIR)\skylark_x86" -mx9 -r -y
!ENDIF	
	
clean:
    @if exist "$(MAKEDIR)\src\3rdparty\sqlite3\Makefile" cd "$(MAKEDIR)\src\3rdparty\sqlite3" && $(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    @if exist "$(MAKEDIR)\src\3rdparty\pcre\Makefile" cd "$(MAKEDIR)\src\3rdparty\pcre" && $(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    @if exist "$(MAKEDIR)\src\3rdparty\libiconv\Makefile" cd "$(MAKEDIR)\src\3rdparty\libiconv" && $(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    @if exist "$(MAKEDIR)\src\3rdparty\luajit\Makefile" cd "$(MAKEDIR)\src\3rdparty\luajit" && $(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    @if exist "$(MAKEDIR)\src\3rdparty\chardet\Makefile" cd "$(MAKEDIR)\src\3rdparty\chardet" && $(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    @if exist "$(MAKEDIR)\src\3rdparty\qrencode\Makefile" cd "$(MAKEDIR)\src\3rdparty\qrencode" && $(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    @if exist "$(MAKEDIR)\src\3rdparty\boost\Makefile" cd "$(MAKEDIR)\src\3rdparty\boost" && $(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    @if exist "$(MAKEDIR)\src\3rdparty\scintilla\Makefile" cd "$(MAKEDIR)\src\3rdparty\scintilla" && $(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    cd "$(MAKEDIR)\src"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    cd "$(MAKEDIR)\locales"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    cd "$(MAKEDIR)"
    -del /q /f /s *~ 2>nul
    -del /f *.7z 2>nul
    -rd /s/q skylark_x64 2>nul
    -rd /s/q skylark_x86 2>nul
