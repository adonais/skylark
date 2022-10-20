@echo Building %1 ...
@set PATH=%PATH%;D:\msys2\mingw64\bin
@echo FULL_CURRENT_PATH = "%FULL_CURRENT_PATH%"
@echo CURRENT_DIRECTORY = "%CURRENT_DIRECTORY%"
@echo FILE_NAME = "%FILE_NAME%"
@echo NAME_PART = "%NAME_PART%"
@echo EXT_PART = "%EXT_PART%"
@echo CURRENT_LINESTR = "%CURRENT_LINESTR%"
@echo CURRENT_SELSTR = "%CURRENT_SELSTR%"
call g++ %1 -g -o "%temp%\%NAME_PART%.exe"

@if ERRORLEVEL 1 GOTO END-ERR

@echo ----------------------------------------------------------

call "%temp%\%NAME_PART%.exe"

@echo ----------------------------------------------------------

:END-ERR

@pause
@if exist "%temp%\%NAME_PART%.exe" del /f "%temp%\%NAME_PART%.exe"
