@echo off
setlocal

where cl >nul 2>nul
if %errorlevel%==0 goto build

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
  for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSINSTALL=%%i
)

if defined VSINSTALL (
  call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
  goto build
)

echo MSVC was not found.
echo Install Visual Studio Build Tools with Desktop development with C++.
echo Or run this from an x64 Native Tools Command Prompt for VS.
exit /b 1

:build
if not exist build mkdir build

cl /nologo /W4 /WX /O1 /DUNICODE /D_UNICODE src\dbytepad.c ^
  /link /SUBSYSTEM:WINDOWS /OUT:build\dbytepad.exe user32.lib gdi32.lib comdlg32.lib shell32.lib comctl32.lib

if errorlevel 1 exit /b 1

echo Built build\dbytepad.exe
