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
#define STATUS_H 22
'@ @'
#define STATUS_H 22
#define FIND_TEXT_CHARS 256
'@

Replace-Once @'
#define IDM_EDIT_SELECT_ALL 2005
#define IDM_VIEW_WORD_WRAP 3001
'@ @'
#define IDM_EDIT_SELECT_ALL 2005
#define IDM_EDIT_FIND 2006
#define IDM_EDIT_FIND_NEXT 2007
#define IDM_VIEW_WORD_WRAP 3001
'@

Replace-Once @'
static HWND g_status;
static WCHAR g_path[MAX_PATH_CHARS];
'@ @'
static HWND g_status;
static HWND g_find_dialog;
static UINT g_find_msg;
static FINDREPLACEW g_find;
static WCHAR g_find_text[FIND_TEXT_CHARS];
static WCHAR g_path[MAX_PATH_CHARS];
'@

Replace-Once @'
static int ask_save_if_dirty(HWND hwnd) {
    int answer;

    if (!g_dirty) return 1;

    answer = MessageBoxW(hwnd, L"Save changes?", APP_NAME, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL) return 0;
    if (answer == IDYES) return save_file(hwnd);
    return 1;
}

static HMENU make_menu(void) {
'@ @'
static int ask_save_if_dirty(HWND hwnd) {
    int answer;

    if (!g_dirty) return 1;

    answer = MessageBoxW(hwnd, L"Save changes?", APP_NAME, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL) return 0;
    if (answer == IDYES) return save_file(hwnd);
    return 1;
}

static void find_next(HWND hwnd) {
    CHARRANGE sel;
    FINDTEXTEXW ft;
    DWORD flags;

    if (!g_find_text[0]) {
        MessageBeep(MB_ICONINFORMATION);
        return;
    }

    SendMessageW(g_edit, EM_EXGETSEL, 0, (LPARAM)&sel);

    ft.chrg.cpMin = sel.cpMax;
    ft.chrg.cpMax = -1;
    ft.lpstrText = g_find_text;

    flags = FR_DOWN;
    if (g_find.Flags & FR_MATCHCASE) flags |= FR_MATCHCASE;
    if (g_find.Flags & FR_WHOLEWORD) flags |= FR_WHOLEWORD;

    if (SendMessageW(g_edit, EM_FINDTEXTEXW, (WPARAM)flags, (LPARAM)&ft) >= 0) {
        SendMessageW(g_edit, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);
        SetFocus(g_edit);
        update_status();
        return;
    }

    MessageBeep(MB_ICONINFORMATION);
}

static void show_find(HWND hwnd) {
    if (g_find_dialog) {
        SetForegroundWindow(g_find_dialog);
        return;
    }

    ZeroMemory(&g_find, sizeof(g_find));
    g_find.lStructSize = sizeof(g_find);
    g_find.hwndOwner = hwnd;
    g_find.lpstrFindWhat = g_find_text;
    g_find.wFindWhatLen = FIND_TEXT_CHARS;
    g_find.Flags = FR_DOWN;

    g_find_dialog = FindTextW(&g_find);
}

static HMENU make_menu(void) {
'@

Replace-Once @'
    AppendMenuW(edit, MF_STRING, IDM_EDIT_PASTE, L"Paste\tCtrl+V");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_SELECT_ALL, L"Select All\tCtrl+A");
'@ @'
    AppendMenuW(edit, MF_STRING, IDM_EDIT_PASTE, L"Paste\tCtrl+V");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_FIND, L"Find...\tCtrl+F");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_FIND_NEXT, L"Find Next\tF3");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_SELECT_ALL, L"Select All\tCtrl+A");
'@

Replace-Once @'
    case IDM_EDIT_PASTE: SendMessageW(g_edit, WM_PASTE, 0, 0); break;
    case IDM_EDIT_SELECT_ALL: SendMessageW(g_edit, EM_SETSEL, 0, -1); break;
'@ @'
    case IDM_EDIT_PASTE: SendMessageW(g_edit, WM_PASTE, 0, 0); break;
    case IDM_EDIT_FIND: show_find(hwnd); break;
    case IDM_EDIT_FIND_NEXT: find_next(hwnd); break;
    case IDM_EDIT_SELECT_ALL: SendMessageW(g_edit, EM_SETSEL, 0, -1); break;
'@

Replace-Once @'
static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
'@ @'
static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == g_find_msg) {
        FINDREPLACEW *fr = (FINDREPLACEW *)lparam;
        if (fr->Flags & FR_DIALOGTERM) {
            g_find_dialog = NULL;
            return 0;
        }
        if (fr->Flags & FR_FINDNEXT) {
            g_find.Flags = fr->Flags;
            find_next(hwnd);
        }
        return 0;
    }

    switch (msg) {
'@

Replace-Once @'
        { FVIRTKEY | FCONTROL, 'A', IDM_EDIT_SELECT_ALL },
        { FVIRTKEY | FCONTROL, 'Z', IDM_EDIT_UNDO },
'@ @'
        { FVIRTKEY | FCONTROL, 'A', IDM_EDIT_SELECT_ALL },
        { FVIRTKEY | FCONTROL, 'F', IDM_EDIT_FIND },
        { FVIRTKEY, VK_F3, IDM_EDIT_FIND_NEXT },
        { FVIRTKEY | FCONTROL, 'Z', IDM_EDIT_UNDO },
'@

Replace-Once @'
    g_instance = instance;

    if (!LoadLibraryW(L"Msftedit.dll")) {
'@ @'
    g_instance = instance;
    g_find_msg = RegisterWindowMessageW(L"commdlg_FindReplace");

    if (!LoadLibraryW(L"Msftedit.dll")) {
'@

Replace-Once @'
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!accel || !TranslateAcceleratorW(hwnd, accel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
'@ @'
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!g_find_dialog || !IsDialogMessageW(g_find_dialog, &msg)) {
            if (!accel || !TranslateAcceleratorW(hwnd, accel, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
    }
'@

Set-Content -LiteralPath $Path -Value $Text -Encoding UTF8
Write-Host "Applied v0.1.2 find patch."
