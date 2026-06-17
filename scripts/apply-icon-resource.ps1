$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Source = Join-Path $Root "src\dbytepad.c"
$Build = Join-Path $Root "build.bat"
$Icon = Join-Path $Root "assets\dbytepad.ico"
$ResHeader = Join-Path $Root "src\resource.h"
$RcFile = Join-Path $Root "src\dbytepad.rc"

if (-not (Test-Path $Icon)) {
    throw "assets\dbytepad.ico not found. Create it first from assets\dbytepad-icon.svg."
}

$Text = Get-Content -LiteralPath $Source -Raw
$BuildText = Get-Content -LiteralPath $Build -Raw

function Replace-Source-Once([string]$Needle, [string]$Replacement) {
    $count = ([regex]::Matches($script:Text, [regex]::Escape($Needle))).Count
    if ($count -ne 1) {
        throw "expected one source match, got $count"
    }
    $script:Text = $script:Text.Replace($Needle, $Replacement)
}

function Replace-Build-Once([string]$Needle, [string]$Replacement) {
    $count = ([regex]::Matches($script:BuildText, [regex]::Escape($Needle))).Count
    if ($count -ne 1) {
        throw "expected one build match, got $count"
    }
    $script:BuildText = $script:BuildText.Replace($Needle, $Replacement)
}

if ($Text -notmatch '#include "resource.h"') {
    Replace-Source-Once @'
#include <strsafe.h>
'@ @'
#include <strsafe.h>
#include "resource.h"
'@
}

if ($Text -match 'LoadIconW\(NULL, IDI_APPLICATION\)') {
    Replace-Source-Once @'
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
'@ @'
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
'@
} elseif ($Text -notmatch 'MAKEINTRESOURCEW\(IDI_APP_ICON\)') {
    throw "could not find window icon assignment"
}

Set-Content -LiteralPath $ResHeader -Value @'
#define IDI_APP_ICON 101
'@ -Encoding ASCII

Set-Content -LiteralPath $RcFile -Value @'
#include "resource.h"

IDI_APP_ICON ICON "assets\\dbytepad.ico"
'@ -Encoding ASCII

if ($BuildText -notmatch 'dbytepad\.res') {
    Replace-Build-Once @'
if not exist build mkdir build

cl /nologo /W4 /WX /wd4201 /O1 /DUNICODE /D_UNICODE src\dbytepad.c ^
  /link /SUBSYSTEM:WINDOWS /OUT:build\dbytepad.exe user32.lib gdi32.lib comdlg32.lib shell32.lib comctl32.lib
'@ @'
if not exist build mkdir build

rc /nologo /fo build\dbytepad.res src\dbytepad.rc
if errorlevel 1 exit /b 1

cl /nologo /W4 /WX /wd4201 /O1 /DUNICODE /D_UNICODE src\dbytepad.c build\dbytepad.res ^
  /link /SUBSYSTEM:WINDOWS /OUT:build\dbytepad.exe user32.lib gdi32.lib comdlg32.lib shell32.lib comctl32.lib
'@
}

Set-Content -LiteralPath $Source -Value $Text -Encoding UTF8
Set-Content -LiteralPath $Build -Value $BuildText -Encoding ASCII
Write-Host "Applied native icon resource patch."
