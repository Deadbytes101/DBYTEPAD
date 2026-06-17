$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Path = Join-Path $Root "src\dbytepad.c"
$Text = Get-Content -LiteralPath $Path -Raw

function Replace-Once([string]$Needle, [string]$Replacement) {
    $count = ([regex]::Matches($script:Text, [regex]::Escape($Needle))).Count
    if ($count -ne 1) {
        throw "expected one match, got $count"
    }
    $script:Text = $script:Text.Replace($Needle, $Replacement)
}

Replace-Once @'
#define APP_NAME L"DBYTEPAD"
'@ @'
#define APP_NAME L"DBYTEPAD"
#define APP_VERSION L"1.0.0"
'@

Replace-Once @'
#define IDM_VIEW_READ_ONLY 3002
'@ @'
#define IDM_VIEW_READ_ONLY 3002
#define IDM_HELP_ABOUT 4001
'@

Replace-Once @'
static HMENU make_menu(void) {
'@ @'
static void show_about(HWND hwnd) {
    MessageBoxW(
        hwnd,
        L"DBYTEPAD 1.0.0\nNative Win32 text editor.\nNo Electron. No webview. No telemetry.",
        L"About DBYTEPAD",
        MB_OK | MB_ICONINFORMATION);
}

static HMENU make_menu(void) {
'@

Replace-Once @'
    HMENU edit = CreatePopupMenu();
    HMENU view = CreatePopupMenu();
'@ @'
    HMENU edit = CreatePopupMenu();
    HMENU view = CreatePopupMenu();
    HMENU help = CreatePopupMenu();
'@

Replace-Once @'
    AppendMenuW(view, MF_CHECKED | MF_STRING, IDM_VIEW_WORD_WRAP, L"Word Wrap");
    AppendMenuW(view, MF_STRING, IDM_VIEW_READ_ONLY, L"Read Only");

    AppendMenuW(menu, MF_POPUP, (UINT_PTR)file, L"File");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)edit, L"Edit");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)view, L"View");
    return menu;
'@ @'
    AppendMenuW(view, MF_CHECKED | MF_STRING, IDM_VIEW_WORD_WRAP, L"Word Wrap");
    AppendMenuW(view, MF_STRING, IDM_VIEW_READ_ONLY, L"Read Only");

    AppendMenuW(help, MF_STRING, IDM_HELP_ABOUT, L"About");

    AppendMenuW(menu, MF_POPUP, (UINT_PTR)file, L"File");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)edit, L"Edit");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)view, L"View");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)help, L"Help");
    return menu;
'@

Replace-Once @'
    case IDM_VIEW_READ_ONLY:
        set_read_only(!g_read_only);
        break;
    default: break;
'@ @'
    case IDM_VIEW_READ_ONLY:
        set_read_only(!g_read_only);
        break;
    case IDM_HELP_ABOUT:
        show_about(hwnd);
        break;
    default: break;
'@

Set-Content -LiteralPath $Path -Value $Text -Encoding UTF8
Write-Host "Applied v1.0.0 release patch."
