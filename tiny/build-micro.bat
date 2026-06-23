@echo off
setlocal

set ROOT=%~dp0..
set OUT=%ROOT%\build
set SRC=%~dp0dbpadmicro.c
set OBJ=%OUT%\dbpadmicro.obj
set LINK_EXE=%OUT%\dbpadmicro-link.exe
set PACKED_EXE=%OUT%\dbpadmicro.exe

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
cl /nologo /W4 /WX /O1 /GS- /GR- /TC /c /Fo"%OBJ%" "%SRC%"
if errorlevel 1 exit /b 1

link /nologo /nodefaultlib /entry:WinMain /subsystem:windows /opt:ref /opt:icf /out:"%LINK_EXE%" "%OBJ%" kernel32.lib user32.lib
if errorlevel 1 exit /b 1

for %%A in ("%LINK_EXE%") do echo MS LINK Micro build: %%~zA bytes  %%~fA

where crinkler >nul 2>nul
if errorlevel 1 (
  echo crinkler.exe not found; skipped Micro packed build.
  exit /b 0
)

crinkler "%OBJ%" ^
  /OUT:"%PACKED_EXE%" ^
  /ENTRY:WinMain ^
  /SUBSYSTEM:WINDOWS ^
  /NOINITIALIZERS ^
  /TINYIMPORT ^
  /COMPMODE:SLOW ^
  /ORDERTRIES:8192 ^
  kernel32.lib user32.lib
if errorlevel 1 exit /b 1

for %%A in ("%PACKED_EXE%") do echo Crinkler Micro build: %%~zA bytes  %%~fA
