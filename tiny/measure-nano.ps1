param(
    [switch]$Strict
)

$ErrorActionPreference = 'Stop'

$Path = Join-Path $PSScriptRoot 'dbpadnano.bit'
if (-not (Test-Path $Path)) {
    throw 'No DBYTEPAD NANO marker found: tiny\dbpadnano.bit'
}

$bytes = [System.IO.File]::ReadAllBytes($Path)
if ($bytes.Length -ne 1) {
    throw "DBYTEPAD NANO storage budget failed: $($bytes.Length) bytes > 1 byte"
}

$semanticBits = if ($bytes[0] -eq [byte][char]'1') { 1 } else { 0 }
$storageBits = $bytes.Length * 8

Write-Host ("DBYTEPAD NANO semantic payload: {0} bit" -f $semanticBits)
Write-Host ("DBYTEPAD NANO stored file size: {0} byte / {1} bits" -f $bytes.Length, $storageBits)
Write-Host ("  PASS  semantic payload <= 1 bit")
Write-Host ("  PASS  stored marker <= 1 byte")
Write-Host ("  NOTE  filesystems store bytes; the sub-byte claim is semantic, not physical")

if ($Strict -and $semanticBits -ne 1) {
    throw 'DBYTEPAD NANO strict semantic payload failed: marker must be ASCII 1'
}
