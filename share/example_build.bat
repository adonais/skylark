@echo Building %1...
@set PATH=%PATH%;D:\msys2\mingw64\bin
@set "target=%~n1"
call g++ %1 -g -o %temp%\%target%.exe

@if ERRORLEVEL 1 GOTO END-ERR

@echo ----------------------------------------------------------

call %temp%\%target%.exe

@echo ----------------------------------------------------------

:END-ERR

@pause
@if exist %temp%\%target%.exe del /f %temp%\%target%.exe
