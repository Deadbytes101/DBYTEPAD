$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

Write-Host "Building DBYTEPAD release candidate."
& .\build.bat
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

powershell -ExecutionPolicy Bypass -File .\scripts\size.ps1 -Record v1.0.0

Write-Host "Manual release checks"
Write-Host "Open README.md from command line."
Write-Host "Open, save, save as, reload."
Write-Host "Find and Find Next."
Write-Host "Facts before and after editing."
Write-Host "Read Only blocks editing and saving."
Write-Host "Unsaved changes prompt on close, open, reload."
Write-Host "Then commit docs/BYTE_LEDGER.md and tag v1.0.0."
