$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

New-Item -ItemType Directory -Force build | Out-Null

$CompileArgs = @(
    "/nologo",
    "/W4",
    "/WX",
    "/O1",
    "/DUNICODE",
    "/D_UNICODE",
    "src\dbytepad.c",
    "/link",
    "/SUBSYSTEM:WINDOWS",
    "/OUT:build\dbytepad.exe",
    "user32.lib",
    "gdi32.lib",
    "comdlg32.lib",
    "shell32.lib",
    "comctl32.lib"
)

$Cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($Cl) {
    & cl.exe @CompileArgs
    exit $LASTEXITCODE
}

$VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $VsWhere)) {
    Write-Host "MSVC was not found. Install Visual Studio Build Tools with Desktop development with C++."
    Write-Host "Then run this script again, or use an x64 Native Tools Command Prompt."
    exit 1
}

$InstallPath = & $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $InstallPath) {
    Write-Host "Visual Studio exists, but the C++ compiler tools are missing."
    Write-Host "Open Visual Studio Installer and add Desktop development with C++."
    exit 1
}

$VcVars = Join-Path $InstallPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $VcVars)) {
    Write-Host "vcvars64.bat was not found. Repair or reinstall the C++ build tools."
    exit 1
}

$Command = '"' + $VcVars + '" >nul && cl ' + ($CompileArgs -join ' ')
cmd.exe /d /c $Command
exit $LASTEXITCODE
