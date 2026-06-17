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
#define IDM_FILE_OPEN 1002
#define IDM_FILE_SAVE 1003
'@ @'
#define IDM_FILE_OPEN 1002
#define IDM_FILE_OPEN_READ_ONLY 1008
#define IDM_FILE_SAVE 1003
'@

Replace-Once @'
#define IDM_VIEW_WORD_WRAP 3001
'@ @'
#define IDM_VIEW_WORD_WRAP 3001
#define IDM_VIEW_READ_ONLY 3002
'@

Replace-Once @'
static int g_loading;
static int g_word_wrap = 1;
'@ @'
static int g_loading;
static int g_word_wrap = 1;
static int g_read_only;
'@

Replace-Once @'
static int save_file_as(HWND hwnd);
static int open_path(HWND hwnd, const WCHAR *path);
'@ @'
static int save_file_as(HWND hwnd);
static int open_path(HWND hwnd, const WCHAR *path);
static void set_read_only(int read_only);
'@

Replace-Once @'
    StringCchPrintfW(title, MAX_PATH_CHARS + 64, L"%ls%ls - %ls", g_dirty ? L"*" : L"", name, APP_NAME);
'@ @'
    StringCchPrintfW(title, MAX_PATH_CHARS + 64, L"%ls%ls%ls - %ls", g_read_only ? L"[RO] " : L"", g_dirty ? L"*" : L"", name, APP_NAME);
'@

Replace-Once @'
        L"Ln %d, Col %d    Chars %lld    UTF-8 bytes %d    %ls",
        line + 1,
        col + 1,
        (long long)chars,
        bytes,
        g_dirty ? L"modified" : L"saved");
'@ @'
        L"Ln %d, Col %d    Chars %lld    UTF-8 bytes %d    %ls%ls",
        line + 1,
        col + 1,
        (long long)chars,
        bytes,
        g_read_only ? L"read-only " : L"",
        g_dirty ? L"modified" : L"saved");
'@

Replace-Once @'
static void apply_word_wrap(void) {
'@ @'
static void set_read_only(int read_only) {
    g_read_only = read_only;

    if (g_edit) {
        SendMessageW(g_edit, EM_SETREADONLY, (WPARAM)(read_only ? TRUE : FALSE), 0);
    }

    if (g_main) {
        CheckMenuItem(GetMenu(g_main), IDM_VIEW_READ_ONLY, read_only ? MF_CHECKED : MF_UNCHECKED);
    }

    set_title();
    update_status();
}

static void apply_word_wrap(void) {
'@

Replace-Once @'
    g_path[0] = 0;
    set_dirty(0);
'@ @'
    g_path[0] = 0;
    set_read_only(0);
    set_dirty(0);
'@

Replace-Once @'
static void open_file(HWND hwnd) {
    WCHAR path[MAX_PATH_CHARS];

    if (choose_open(hwnd, path)) {
        open_path(hwnd, path);
    }
}

static void reload_file(HWND hwnd) {
'@ @'
static void open_file(HWND hwnd) {
    WCHAR path[MAX_PATH_CHARS];

    if (choose_open(hwnd, path)) {
        if (open_path(hwnd, path)) set_read_only(0);
    }
}

static void open_file_read_only(HWND hwnd) {
    WCHAR path[MAX_PATH_CHARS];

    if (choose_open(hwnd, path)) {
        if (open_path(hwnd, path)) set_read_only(1);
    }
}

static void reload_file(HWND hwnd) {
'@

Replace-Once @'
static int save_file(HWND hwnd) {
    if (!g_path[0]) return save_file_as(hwnd);
'@ @'
static int save_file(HWND hwnd) {
    if (g_read_only) {
        MessageBoxW(hwnd, L"Read-only buffer. Disable Read Only before saving.", APP_NAME, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    if (!g_path[0]) return save_file_as(hwnd);
'@

Replace-Once @'
    AppendMenuW(file, MF_STRING, IDM_FILE_NEW, L"New\tCtrl+N");
    AppendMenuW(file, MF_STRING, IDM_FILE_OPEN, L"Open...\tCtrl+O");
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE, L"Save\tCtrl+S");
'@ @'
    AppendMenuW(file, MF_STRING, IDM_FILE_NEW, L"New\tCtrl+N");
    AppendMenuW(file, MF_STRING, IDM_FILE_OPEN, L"Open...\tCtrl+O");
    AppendMenuW(file, MF_STRING, IDM_FILE_OPEN_READ_ONLY, L"Open Read Only...");
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE, L"Save\tCtrl+S");
'@

Replace-Once @'
    AppendMenuW(view, MF_CHECKED | MF_STRING, IDM_VIEW_WORD_WRAP, L"Word Wrap");
'@ @'
    AppendMenuW(view, MF_CHECKED | MF_STRING, IDM_VIEW_WORD_WRAP, L"Word Wrap");
    AppendMenuW(view, MF_STRING, IDM_VIEW_READ_ONLY, L"Read Only");
'@

Replace-Once @'
    case IDM_FILE_NEW: new_file(hwnd); break;
    case IDM_FILE_OPEN: open_file(hwnd); break;
    case IDM_FILE_SAVE: save_file(hwnd); break;
'@ @'
    case IDM_FILE_NEW: new_file(hwnd); break;
    case IDM_FILE_OPEN: open_file(hwnd); break;
    case IDM_FILE_OPEN_READ_ONLY: open_file_read_only(hwnd); break;
    case IDM_FILE_SAVE: save_file(hwnd); break;
'@

Replace-Once @'
    case IDM_VIEW_WORD_WRAP:
        g_word_wrap = !g_word_wrap;
        CheckMenuItem(GetMenu(hwnd), IDM_VIEW_WORD_WRAP, g_word_wrap ? MF_CHECKED : MF_UNCHECKED);
        apply_word_wrap();
        resize_parts(hwnd);
        break;
'@ @'
    case IDM_VIEW_WORD_WRAP:
        g_word_wrap = !g_word_wrap;
        CheckMenuItem(GetMenu(hwnd), IDM_VIEW_WORD_WRAP, g_word_wrap ? MF_CHECKED : MF_UNCHECKED);
        apply_word_wrap();
        resize_parts(hwnd);
        break;
    case IDM_VIEW_READ_ONLY:
        set_read_only(!g_read_only);
        break;
'@

Replace-Once @'
        if (DragQueryFileW(drop, 0, path, MAX_PATH_CHARS)) {
            open_path(hwnd, path);
        }
'@ @'
        if (DragQueryFileW(drop, 0, path, MAX_PATH_CHARS)) {
            if (open_path(hwnd, path)) set_read_only(0);
        }
'@

Set-Content -LiteralPath $Path -Value $Text -Encoding UTF8
Write-Host "Applied v0.1.4 read-only patch."
