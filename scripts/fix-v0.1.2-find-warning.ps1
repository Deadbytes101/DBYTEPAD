$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Path = Join-Path $Root "src\dbytepad.c"
$Text = Get-Content -LiteralPath $Path -Raw

$Needle = @'
static void find_next(HWND hwnd) {
    CHARRANGE sel;
'@

$Replacement = @'
static void find_next(HWND hwnd) {
    CHARRANGE sel;
    (void)hwnd;
'@

$count = ([regex]::Matches($Text, [regex]::Escape($Needle))).Count
if ($count -ne 1) {
    throw "expected one find_next match, got $count"
}

$Text = $Text.Replace($Needle, $Replacement)
Set-Content -LiteralPath $Path -Value $Text -Encoding UTF8
Write-Host "Fixed v0.1.2 find warning."
