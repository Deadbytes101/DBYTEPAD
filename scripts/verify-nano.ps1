param(
    [string]$Path = ".\build\dbpadnano.exe",
    [int]$MaxBytes = 421,
    [switch]$Run,
    [switch]$Strict
)

$ErrorActionPreference = 'Stop'

function Write-BoardLine($name, $release, $bytes, $behavior) {
    $bits = $bytes * 8
    Write-Host ("{0,-12} {1,-20} {2,7} B  {3,8} bits  {4}" -f $name, $release, $bytes, $bits, $behavior)
}

$resolved = Resolve-Path $Path
$item = Get-Item $resolved
$bytes = [int64]$item.Length
$bits = $bytes * 8
$hash = (Get-FileHash -Algorithm SHA256 $resolved).Hash.ToLowerInvariant()
$status = if ($bytes -le $MaxBytes) { 'PASS' } else { 'OVER' }

Write-Host "DBYTEPAD NANO VERIFY"
Write-Host ""
Write-Host ("file:   {0}" -f $item.FullName)
Write-Host ("size:   {0} bytes / {1} bits [{2}]" -f $bytes, $bits, $status)
Write-Host ("sha256: {0}" -f $hash)

if ($Run) {
    $p = Start-Process $item.FullName -Wait -PassThru
    Write-Host ("exit:   {0}" -f $p.ExitCode)
    if ($Strict -and $p.ExitCode -ne 0) {
        throw "DBYTEPAD NANO run failed: exit code $($p.ExitCode)"
    }
}

Write-Host ""
Write-Host "SIZE LAB BOARD"
Write-BoardLine "NANO VOID" "dbpadnano-v0.3.0" 421 "valid exe, exits immediately"
Write-BoardLine "1K" "dbpad1k-v0.1.0" 454 "native Win32 EDIT core"
Write-BoardLine "MICRO" "dbpadmicro-v0.1.0" 1224 "open/edit/Ctrl+S save"
Write-BoardLine "FULL" "v1.2.1" 185856 "full DBYTEPAD editor"

if ($Strict -and $bytes -gt $MaxBytes) {
    throw "DBYTEPAD NANO strict budget failed: $bytes > $MaxBytes bytes"
}
