@echo off
setlocal

if not exist build mkdir build

cl /nologo /W4 /WX /O1 /DUNICODE /D_UNICODE src\dbytepad.c ^
  /link /SUBSYSTEM:WINDOWS /OUT:build\dbytepad.exe user32.lib gdi32.lib comdlg32.lib shell32.lib comctl32.lib

if errorlevel 1 exit /b 1

echo Built build\dbytepad.exe
