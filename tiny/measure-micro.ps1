param(
    [switch]$Strict
)

$ErrorActionPreference = 'Stop'

$Root = Resolve-Path (Join-Path $PSScriptRoot '..')
$Build = Join-Path $Root 'build'
$Path = Join-Path $Build 'dbpadmicro.exe'
$Budget = 2476

if (-not (Test-Path $Path)) {
    throw 'No DBYTEPAD Micro executable found. Run tiny\build-micro.bat first.'
}

$item = Get-Item $Path
$status = if ($item.Length -le $Budget) { 'PASS' } else { 'OVER' }
Write-Host ("DBYTEPAD Micro: {0} bytes / {1} budget [{2}]  {3}" -f $item.Length, $Budget, $status, $item.FullName)

$Score = @(
    @{ Name = 'RetroPad full-size target'; Bytes = 2476 },
    @{ Name = 'DBYTEPAD-1K hard target'; Bytes = 1024 },
    @{ Name = 'DTE 2.x bare target'; Bytes = 981 },
    @{ Name = 'DTE 1.x aggressive target'; Bytes = 890 }
)

foreach ($score in $Score) {
    $scoreStatus = if ($item.Length -le $score.Bytes) { 'PASS' } else { 'MISS' }
    Write-Host ("  {0,-5} {1} <= {2} bytes" -f $scoreStatus, $score.Name, $score.Bytes)
}

if ($Strict -and $item.Length -gt $Budget) {
    throw "DBYTEPAD Micro strict budget failed: $($item.Length) > $Budget bytes"
}
