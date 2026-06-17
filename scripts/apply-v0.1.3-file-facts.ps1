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
#define IDM_FILE_RELOAD 1005
#define IDM_FILE_EXIT 1006
'@ @'
#define IDM_FILE_RELOAD 1005
#define IDM_FILE_FACTS 1006
#define IDM_FILE_EXIT 1007
'@

Replace-Once @'
static void find_next(HWND hwnd) {
'@ @'
static void show_file_facts(HWND hwnd) {
    WCHAR text[1024];
    WCHAR mtime[64];
    SYSTEMTIME utc;
    SYSTEMTIME local;
    WIN32_FILE_ATTRIBUTE_DATA data;
    ULARGE_INTEGER size;
    int lines;
    LRESULT chars;
    int bytes;

    mtime[0] = 0;
    size.QuadPart = 0;
    lines = (int)SendMessageW(g_edit, EM_GETLINECOUNT, 0, 0);
    chars = text_chars();
    bytes = utf8_byte_count();

    if (g_path[0] && GetFileAttributesExW(g_path, GetFileExInfoStandard, &data)) {
        size.LowPart = data.nFileSizeLow;
        size.HighPart = data.nFileSizeHigh;
        FileTimeToSystemTime(&data.ftLastWriteTime, &utc);
        SystemTimeToTzSpecificLocalTime(NULL, &utc, &local);
        StringCchPrintfW(
            mtime,
            64,
            L"%04u-%02u-%02u %02u:%02u:%02u",
            local.wYear,
            local.wMonth,
            local.wDay,
            local.wHour,
            local.wMinute,
            local.wSecond);
    }

    StringCchPrintfW(
        text,
        1024,
        L"Path: %ls\nDisk bytes: %llu\nModified: %ls\nLines: %d\nChars: %lld\nUTF-8 bytes from buffer: %d\nState: %ls",
        g_path[0] ? g_path : L"Untitled buffer",
        (unsigned long long)size.QuadPart,
        mtime[0] ? mtime : L"not on disk",
        lines,
        (long long)chars,
        bytes,
        g_dirty ? L"modified" : L"saved");

    MessageBoxW(hwnd, text, L"DBYTEPAD facts", MB_OK | MB_ICONINFORMATION);
}

static void find_next(HWND hwnd) {
'@

Replace-Once @'
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE_AS, L"Save As...\tCtrl+Shift+S");
    AppendMenuW(file, MF_STRING, IDM_FILE_RELOAD, L"Reload");
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
'@ @'
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE_AS, L"Save As...\tCtrl+Shift+S");
    AppendMenuW(file, MF_STRING, IDM_FILE_RELOAD, L"Reload");
    AppendMenuW(file, MF_STRING, IDM_FILE_FACTS, L"Facts");
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
'@

Replace-Once @'
    case IDM_FILE_RELOAD: reload_file(hwnd); break;
    case IDM_FILE_EXIT: SendMessageW(hwnd, WM_CLOSE, 0, 0); break;
'@ @'
    case IDM_FILE_RELOAD: reload_file(hwnd); break;
    case IDM_FILE_FACTS: show_file_facts(hwnd); break;
    case IDM_FILE_EXIT: SendMessageW(hwnd, WM_CLOSE, 0, 0); break;
'@

Set-Content -LiteralPath $Path -Value $Text -Encoding UTF8
Write-Host "Applied v0.1.3 file facts patch."
