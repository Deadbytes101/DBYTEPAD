@echo off
setlocal

set ROOT=%~dp0..
set OUT=%ROOT%\build
set SRC=%~dp0dbpad1k.asm
set OBJ=%OUT%\dbpad1k.obj
set LINK_EXE=%OUT%\dbpad1k-link.exe
set CRINKLER_EXE=%OUT%\dbpad1k.exe

if not exist "%OUT%" mkdir "%OUT%"

where ml >nul 2>nul
if errorlevel 1 (
  echo ml.exe was not found.
  echo Use an x86 Native Tools Command Prompt for Visual Studio.
  exit /b 1
)

where link >nul 2>nul
if errorlevel 1 (
  echo link.exe was not found.
  echo Use an x86 Native Tools Command Prompt for Visual Studio.
  exit /b 1
)

ml /nologo /c /coff /Cp /Fo"%OBJ%" "%SRC%"
if errorlevel 1 exit /b 1

link /nologo /nodefaultlib /entry:start /subsystem:windows /opt:ref /opt:icf /merge:.rdata=.text /merge:.data=.text /section:.text,erw /filealign:16 /out:"%LINK_EXE%" "%OBJ%" kernel32.lib user32.lib
if errorlevel 1 exit /b 1

for %%A in ("%LINK_EXE%") do echo MS LINK build: %%~zA bytes  %%~fA

where crinkler >nul 2>nul
if errorlevel 1 (
  echo crinkler.exe not found; skipped 1K byte-fight build.
  echo Install/place crinkler.exe in PATH to build %CRINKLER_EXE%.
  exit /b 0
)

crinkler "%OBJ%" ^
  /OUT:"%CRINKLER_EXE%" ^
  /ENTRY:start ^
  /SUBSYSTEM:WINDOWS ^
  /NOINITIALIZERS ^
  /TINYIMPORT ^
  /COMPMODE:SLOW ^
  /ORDERTRIES:8192 ^
  kernel32.lib user32.lib
if errorlevel 1 exit /b 1

for %%A in ("%CRINKLER_EXE%") do echo Crinkler build: %%~zA bytes  %%~fA
