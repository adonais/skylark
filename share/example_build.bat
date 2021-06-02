@echo Building %1...
@set PATH=%PATH%;D:\msys2\mingw64\bin
call g++ %1 -g -o %temp%\skylark_build_temp.exe

@if ERRORLEVEL 1 GOTO END-ERR

@echo ----------------------------------------------------------

call %temp%\skylark_build_temp.exe

@echo ----------------------------------------------------------

:END-ERR

@pause
@if exist %temp%\skylark_build_temp.exe del /f %temp%\skylark_build_temp.exe
