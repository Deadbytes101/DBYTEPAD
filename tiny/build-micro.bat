@echo off
setlocal

set ROOT=%~dp0..
set OUT=%ROOT%\build
set SRC=%~dp0dbpadmicro.c
set EXE=%OUT%\dbpadmicro.exe

if not exist "%OUT%" mkdir "%OUT%"

where cl >nul 2>nul
if %errorlevel%==0 goto have_tools

set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSINSTALL=%%i
)

if defined VSINSTALL (
  call "%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat" x86
)

where cl >nul 2>nul
if errorlevel 1 (
  echo cl.exe was not found.
  echo Install Visual Studio Build Tools with Desktop development with C++.
  exit /b 1
)

:have_tools
cl /nologo /W4 /WX /O1 /GS- /GR- /TC "%SRC%" /link /nodefaultlib /entry:WinMain /subsystem:windows /opt:ref /opt:icf /merge:.rdata=.text /merge:.data=.text /section:.text,erw /filealign:16 /out:"%EXE%" kernel32.lib user32.lib
if errorlevel 1 exit /b 1

for %%A in ("%EXE%") do echo DBYTEPAD Micro build: %%~zA bytes  %%~fA
