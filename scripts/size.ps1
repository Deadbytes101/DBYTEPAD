param(
    [string]$Record = ""
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Exe = Join-Path $Root "build\dbytepad.exe"
$Source = Join-Path $Root "src\dbytepad.c"
$Ledger = Join-Path $Root "docs\BYTE_LEDGER.md"

if (-not (Test-Path $Exe)) {
    Write-Host "build\dbytepad.exe does not exist. Run build.bat first."
    exit 1
}

$Commit = (& git -C $Root rev-parse --short HEAD).Trim()
$ExeBytes = (Get-Item -LiteralPath $Exe).Length
$SourceBytes = (Get-Item -LiteralPath $Source).Length
$SourceLines = (Get-Content -LiteralPath $Source).Count

Write-Host "DBYTEPAD build facts"
Write-Host "Commit: $Commit"
Write-Host "Executable bytes: $ExeBytes"
Write-Host "Source lines: $SourceLines"
Write-Host "Source bytes: $SourceBytes"

if ($Record.Length -gt 0) {
    $Entry = @"

$Record
Commit: $Commit
Executable bytes: $ExeBytes
Source lines: $SourceLines
Source bytes: $SourceBytes
Note: local measured build
"@
    Add-Content -LiteralPath $Ledger -Value $Entry -Encoding UTF8
    Write-Host "Recorded in docs\BYTE_LEDGER.md"
}
