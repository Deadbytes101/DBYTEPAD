param(
    [switch]$Strict
)

$ErrorActionPreference = 'Stop'

$Root = Resolve-Path (Join-Path $PSScriptRoot '..')
$Build = Join-Path $Root 'build'
$Targets = @(
    @{ Name = 'MS LINK'; Path = Join-Path $Build 'dbpad1k-link.exe'; Budget = $null },
    @{ Name = 'Crinkler'; Path = Join-Path $Build 'dbpad1k.exe'; Budget = 1024 }
)

$Score = @(
    @{ Name = 'Bronze: beat reported full RetroPad size'; Bytes = 2476 },
    @{ Name = 'Silver: DBYTEPAD-1K hard budget'; Bytes = 1024 },
    @{ Name = 'Gold: beat reported DTE 2.x bare RICHEDIT base'; Bytes = 981 },
    @{ Name = 'Black: beat reported DTE 1.x aggressive EDIT base'; Bytes = 890 }
)

$seen = $false
foreach ($target in $Targets) {
    if (Test-Path $target.Path) {
        $seen = $true
        $item = Get-Item $target.Path
        if ($null -eq $target.Budget) {
            Write-Host ("{0}: {1} bytes  {2}" -f $target.Name, $item.Length, $item.FullName)
        } else {
            $status = if ($item.Length -le $target.Budget) { 'PASS' } else { 'OVER' }
            Write-Host ("{0}: {1} bytes / {2} budget [{3}]  {4}" -f $target.Name, $item.Length, $target.Budget, $status, $item.FullName)

            foreach ($score in $Score) {
                $scoreStatus = if ($item.Length -le $score.Bytes) { 'PASS' } else { 'MISS' }
                Write-Host ("  {0,-5} {1} <= {2} bytes" -f $scoreStatus, $score.Name, $score.Bytes)
            }

            if ($Strict -and $item.Length -gt $target.Budget) {
                throw "DBYTEPAD-1K strict budget failed: $($item.Length) > $($target.Budget) bytes"
            }
        }
    }
}

if (-not $seen) {
    throw 'No DBYTEPAD-1K executable found. Run tiny\build.bat first.'
}
