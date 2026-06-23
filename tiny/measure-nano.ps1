param(
    [switch]$Strict
)

$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$exe = Join-Path $root 'build\dbpadnano.exe'
$alias = Join-Path $root 'build\dbpadnano-3632bit.exe'
$budgetBytes = 512
$budgetBits = $budgetBytes * 8

if (-not (Test-Path $exe)) {
    throw 'No DBYTEPAD NANO executable found. Expected build\dbpadnano.exe'
}

$item = Get-Item $exe
$bits = $item.Length * 8
$status = if ($item.Length -le $budgetBytes) { 'PASS' } else { 'OVER' }

Write-Host ("DBYTEPAD NANO executable: {0} bytes / {1} bits [{2}]" -f $item.Length, $bits, $status)
Write-Host ("  TARGET <= {0} bytes / {1} bits" -f $budgetBytes, $budgetBits)

if (Test-Path $alias) {
    $aliasItem = Get-Item $alias
    Write-Host ("DBYTEPAD NANO release alias: {0} bytes / {1} bits  {2}" -f $aliasItem.Length, ($aliasItem.Length * 8), $aliasItem.Name)
}

if ($Strict -and $item.Length -gt $budgetBytes) {
    throw "DBYTEPAD NANO strict budget failed: $($item.Length) > $budgetBytes bytes"
}
