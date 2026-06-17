$ErrorActionPreference = "Stop"

$Path = Join-Path (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)) "src\dbytepad.c"
$Text = Get-Content -LiteralPath $Path -Raw

function Replace-Once([string]$Needle, [string]$Replacement) {
    $count = ([regex]::Matches($script:Text, [regex]::Escape($Needle))).Count
    if ($count -ne 1) {
        throw "expected one match, got $count"
    }
    $script:Text = $script:Text.Replace($Needle, $Replacement)
}

Replace-Once @'
#define IDM_FILE_SAVE_AS 1004
#define IDM_FILE_EXIT 1005
'@ @'
#define IDM_FILE_SAVE_AS 1004
#define IDM_FILE_RELOAD 1005
#define IDM_FILE_EXIT 1006
'@

Replace-Once @'
static int save_file(HWND hwnd);
static int save_file_as(HWND hwnd);
'@ @'
static int save_file(HWND hwnd);
static int save_file_as(HWND hwnd);
static int open_path(HWND hwnd, const WCHAR *path);
'@

Replace-Once @'
static void set_dirty(int dirty) {
    g_dirty = dirty;
    set_title();
}
'@ @'
static void set_dirty(int dirty) {
    g_dirty = dirty;
    if (g_edit) SendMessageW(g_edit, EM_SETMODIFY, (WPARAM)dirty, 0);
    set_title();
}
'@

Replace-Once @'
static int save_file_as(HWND hwnd) {
'@ @'
static void reload_file(HWND hwnd) {
    WCHAR path[MAX_PATH_CHARS];

    if (!g_path[0]) return;

    StringCchCopyW(path, MAX_PATH_CHARS, g_path);
    open_path(hwnd, path);
}

static int save_file_as(HWND hwnd) {
'@

Replace-Once @'
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE_AS, L"Save As...\tCtrl+Shift+S");
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
'@ @'
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE_AS, L"Save As...\tCtrl+Shift+S");
    AppendMenuW(file, MF_STRING, IDM_FILE_RELOAD, L"Reload");
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
'@

Replace-Once @'
    case IDM_FILE_SAVE_AS: save_file_as(hwnd); break;
    case IDM_FILE_EXIT: SendMessageW(hwnd, WM_CLOSE, 0, 0); break;
'@ @'
    case IDM_FILE_SAVE_AS: save_file_as(hwnd); break;
    case IDM_FILE_RELOAD: reload_file(hwnd); break;
    case IDM_FILE_EXIT: SendMessageW(hwnd, WM_CLOSE, 0, 0); break;
'@

Replace-Once @'
static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
'@ @'
static void open_command_line_file(HWND hwnd) {
    int argc;
    LPWSTR *argv;

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return;

    if (argc > 1) open_path(hwnd, argv[1]);
    LocalFree(argv);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
'@

Replace-Once @'
    case WM_CREATE:
        g_edit = CreateWindowExW(
'@ @'
    case WM_CREATE:
        g_loading = 1;

        g_edit = CreateWindowExW(
'@

Replace-Once @'
        set_edit_format();
        DragAcceptFiles(hwnd, TRUE);
        set_dirty(0);
'@ @'
        set_edit_format();
        DragAcceptFiles(hwnd, TRUE);

        g_loading = 0;
        set_dirty(0);
'@

Replace-Once @'
    UpdateWindow(hwnd);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
'@ @'
    UpdateWindow(hwnd);
    open_command_line_file(hwnd);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
'@

Set-Content -LiteralPath $Path -Value $Text -Encoding UTF8
Write-Host "Applied v0.1.1 source patch."
