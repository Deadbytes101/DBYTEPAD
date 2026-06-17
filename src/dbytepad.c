#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <richedit.h>
#include <shellapi.h>
#include <strsafe.h>
#include "resource.h"

#define APP_NAME L"DBYTEPAD"
#define APP_VERSION L"1.1.0"
#define EDIT_CLASS MSFTEDIT_CLASS
#define MAIN_CLASS L"DBYTEPAD_WINDOW"
#define MAX_PATH_CHARS 32768
#define STATUS_H 22
#define FIND_TEXT_CHARS 256
#define RECENT_MAX 5

#define IDM_FILE_NEW 1001
#define IDM_FILE_OPEN 1002
#define IDM_FILE_SAVE 1003
#define IDM_FILE_SAVE_AS 1004
#define IDM_FILE_RELOAD 1005
#define IDM_FILE_FACTS 1006
#define IDM_FILE_EXIT 1007
#define IDM_FILE_OPEN_READ_ONLY 1008
#define IDM_FILE_RECENT_BASE 1100

#define IDM_EDIT_UNDO 2001
#define IDM_EDIT_CUT 2002
#define IDM_EDIT_COPY 2003
#define IDM_EDIT_PASTE 2004
#define IDM_EDIT_SELECT_ALL 2005
#define IDM_EDIT_FIND 2006
#define IDM_EDIT_FIND_NEXT 2007
#define IDM_EDIT_REPLACE 2008

#define IDM_VIEW_WORD_WRAP 3001
#define IDM_VIEW_READ_ONLY 3002
#define IDM_VIEW_FONT 3003

#define IDM_HELP_ABOUT 4001

static HINSTANCE g_instance;
static HWND g_main;
static HWND g_edit;
static HWND g_status;
static HWND g_find_dialog;
static UINT g_find_msg;
static FINDREPLACEW g_find;
static WCHAR g_find_text[FIND_TEXT_CHARS];
static WCHAR g_replace_text[FIND_TEXT_CHARS];
static WCHAR g_path[MAX_PATH_CHARS];
static WCHAR g_ini[MAX_PATH_CHARS];
static WCHAR g_recent[RECENT_MAX][MAX_PATH_CHARS];
static int g_recent_count;
static int g_dirty;
static int g_loading;
static int g_word_wrap = 1;
static int g_read_only;
static WCHAR g_font_face[LF_FACESIZE] = L"Cascadia Mono";
static int g_font_tenths = 105;
static int g_win_x = CW_USEDEFAULT;
static int g_win_y = CW_USEDEFAULT;
static int g_win_w = 900;
static int g_win_h = 620;
static HMENU g_recent_menu;

static int ask_save_if_dirty(HWND hwnd);
static int save_file(HWND hwnd);
static int save_file_as(HWND hwnd);
static int open_path(HWND hwnd, const WCHAR *path);
static void set_read_only(int read_only);
static void refresh_recent_menu(void);

static const WCHAR *base_name(const WCHAR *path) {
    const WCHAR *name = path;
    const WCHAR *p = path;

    while (*p) {
        if (*p == L'\\' || *p == L'/') name = p + 1;
        p++;
    }

    return name;
}

static void show_last_error(HWND hwnd, const WCHAR *what) {
    WCHAR text[512];
    DWORD err = GetLastError();
    StringCchPrintfW(text, 512, L"%ls failed. Win32 error %lu.", what, err);
    MessageBoxW(hwnd, text, APP_NAME, MB_OK | MB_ICONERROR);
}

static void write_ini_int(const WCHAR *section, const WCHAR *key, int value) {
    WCHAR text[32];
    StringCchPrintfW(text, 32, L"%d", value);
    WritePrivateProfileStringW(section, key, text, g_ini);
}

static void make_ini_path(void) {
    WCHAR *p;

    GetModuleFileNameW(NULL, g_ini, MAX_PATH_CHARS);
    p = g_ini + lstrlenW(g_ini);
    while (p > g_ini && p[-1] != L'\\' && p[-1] != L'/') p--;
    *p = 0;
    StringCchCatW(g_ini, MAX_PATH_CHARS, L"dbytepad.ini");
}

static void load_config(void) {
    int i;
    WCHAR key[32];
    WCHAR value[MAX_PATH_CHARS];

    make_ini_path();

    g_word_wrap = GetPrivateProfileIntW(L"view", L"word_wrap", 1, g_ini) ? 1 : 0;
    g_font_tenths = GetPrivateProfileIntW(L"view", L"font_tenths", 105, g_ini);
    if (g_font_tenths < 60 || g_font_tenths > 360) g_font_tenths = 105;

    GetPrivateProfileStringW(L"view", L"font", L"Cascadia Mono", g_font_face, LF_FACESIZE, g_ini);
    if (!g_font_face[0]) StringCchCopyW(g_font_face, LF_FACESIZE, L"Cascadia Mono");

    g_win_x = GetPrivateProfileIntW(L"window", L"x", CW_USEDEFAULT, g_ini);
    g_win_y = GetPrivateProfileIntW(L"window", L"y", CW_USEDEFAULT, g_ini);
    g_win_w = GetPrivateProfileIntW(L"window", L"w", 900, g_ini);
    g_win_h = GetPrivateProfileIntW(L"window", L"h", 620, g_ini);
    if (g_win_w < 320) g_win_w = 900;
    if (g_win_h < 240) g_win_h = 620;

    g_recent_count = 0;
    for (i = 0; i < RECENT_MAX; i++) {
        StringCchPrintfW(key, 32, L"file%d", i);
        value[0] = 0;
        GetPrivateProfileStringW(L"recent", key, L"", value, MAX_PATH_CHARS, g_ini);
        if (value[0]) {
            StringCchCopyW(g_recent[g_recent_count], MAX_PATH_CHARS, value);
            g_recent_count++;
        }
    }
}

static void save_config(void) {
    RECT r;
    int i;
    WCHAR key[32];

    if (!g_ini[0]) return;

    write_ini_int(L"view", L"word_wrap", g_word_wrap);
    write_ini_int(L"view", L"font_tenths", g_font_tenths);
    WritePrivateProfileStringW(L"view", L"font", g_font_face, g_ini);

    if (g_main && !IsIconic(g_main) && GetWindowRect(g_main, &r)) {
        write_ini_int(L"window", L"x", r.left);
        write_ini_int(L"window", L"y", r.top);
        write_ini_int(L"window", L"w", r.right - r.left);
        write_ini_int(L"window", L"h", r.bottom - r.top);
    }

    for (i = 0; i < RECENT_MAX; i++) {
        StringCchPrintfW(key, 32, L"file%d", i);
        WritePrivateProfileStringW(L"recent", key, i < g_recent_count ? g_recent[i] : NULL, g_ini);
    }
}

static void set_title(void) {
    WCHAR title[MAX_PATH_CHARS + 64];
    const WCHAR *name = g_path[0] ? base_name(g_path) : L"Untitled";
    StringCchPrintfW(title, MAX_PATH_CHARS + 64, L"%ls%ls%ls - %ls", g_read_only ? L"[RO] " : L"", g_dirty ? L"*" : L"", name, APP_NAME);
    SetWindowTextW(g_main, title);
}

static void set_dirty(int dirty) {
    g_dirty = dirty;
    if (g_edit) SendMessageW(g_edit, EM_SETMODIFY, (WPARAM)dirty, 0);
    set_title();
}

static LRESULT text_chars(void) {
    GETTEXTLENGTHEX q;
    q.flags = GTL_NUMCHARS;
    q.codepage = 1200;
    return SendMessageW(g_edit, EM_GETTEXTLENGTHEX, (WPARAM)&q, 0);
}

static WCHAR *get_text_alloc(DWORD flags) {
    LRESULT chars = text_chars();
    SIZE_T cap;
    WCHAR *wide;
    GETTEXTEX q;

    if (chars < 0) return NULL;

    cap = ((SIZE_T)chars * 2) + 2;
    wide = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cap * sizeof(WCHAR));
    if (!wide) return NULL;

    q.cb = (DWORD)(cap * sizeof(WCHAR));
    q.flags = flags;
    q.codepage = 1200;
    q.lpDefaultChar = NULL;
    q.lpUsedDefChar = NULL;
    SendMessageW(g_edit, EM_GETTEXTEX, (WPARAM)&q, (LPARAM)wide);
    wide[cap - 1] = 0;
    return wide;
}

static int utf8_byte_count(void) {
    WCHAR *wide;
    int bytes;

    wide = get_text_alloc(GT_USECRLF);
    if (!wide) return 0;

    bytes = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, wide);

    return bytes > 0 ? bytes - 1 : 0;
}

static const WCHAR *line_ending_name(void) {
    return L"CRLF on save";
}

static void update_status(void) {
    DWORD start = 0;
    DWORD end = 0;
    int line;
    int line_start;
    int col;
    WCHAR text[256];
    LRESULT chars;
    int bytes;

    SendMessageW(g_edit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

    line = (int)SendMessageW(g_edit, EM_LINEFROMCHAR, (WPARAM)start, 0);
    line_start = (int)SendMessageW(g_edit, EM_LINEINDEX, (WPARAM)line, 0);
    col = (int)start - line_start;
    chars = text_chars();
    bytes = utf8_byte_count();

    StringCchPrintfW(
        text,
        256,
        L"Ln %d, Col %d    Chars %lld    UTF-8 bytes %d    %ls%ls",
        line + 1,
        col + 1,
        (long long)chars,
        bytes,
        g_read_only ? L"read-only " : L"",
        g_dirty ? L"modified" : L"saved");

    SetWindowTextW(g_status, text);
}

static void resize_parts(HWND hwnd) {
    RECT rc;
    int h;

    GetClientRect(hwnd, &rc);
    h = rc.bottom - STATUS_H;
    if (h < 0) h = 0;

    MoveWindow(g_edit, 0, 0, rc.right, h, TRUE);
    MoveWindow(g_status, 0, h, rc.right, STATUS_H, TRUE);
}

static void set_edit_format(void) {
    CHARFORMAT2W cf;
    ZeroMemory(&cf, sizeof(cf));

    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE | CFM_SIZE;
    cf.yHeight = g_font_tenths * 2;
    StringCchCopyW(cf.szFaceName, LF_FACESIZE, g_font_face);

    SendMessageW(g_edit, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
    SendMessageW(g_edit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessageW(g_edit, EM_EXLIMITTEXT, 0, 0x7FFFFFFE);
}

static void choose_font(HWND hwnd) {
    CHOOSEFONTW cf;
    LOGFONTW lf;
    HDC dc;

    ZeroMemory(&lf, sizeof(lf));
    StringCchCopyW(lf.lfFaceName, LF_FACESIZE, g_font_face);
    dc = GetDC(hwnd);
    lf.lfHeight = -MulDiv(g_font_tenths, GetDeviceCaps(dc, LOGPIXELSY), 720);
    ReleaseDC(hwnd, dc);

    ZeroMemory(&cf, sizeof(cf));
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = hwnd;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
    cf.iPointSize = g_font_tenths;

    if (ChooseFontW(&cf)) {
        StringCchCopyW(g_font_face, LF_FACESIZE, lf.lfFaceName);
        g_font_tenths = cf.iPointSize;
        if (g_font_tenths < 60) g_font_tenths = 60;
        if (g_font_tenths > 360) g_font_tenths = 360;
        set_edit_format();
        update_status();
    }
}

static void set_read_only(int read_only) {
    g_read_only = read_only;

    if (g_edit) SendMessageW(g_edit, EM_SETREADONLY, (WPARAM)(read_only ? TRUE : FALSE), 0);
    if (g_main) CheckMenuItem(GetMenu(g_main), IDM_VIEW_READ_ONLY, read_only ? MF_CHECKED : MF_UNCHECKED);

    set_title();
    update_status();
}

static void apply_word_wrap(void) {
    DWORD style = (DWORD)GetWindowLongPtrW(g_edit, GWL_STYLE);

    if (g_word_wrap) {
        style &= ~(WS_HSCROLL | ES_AUTOHSCROLL);
    } else {
        style |= WS_HSCROLL | ES_AUTOHSCROLL;
    }

    SetWindowLongPtrW(g_edit, GWL_STYLE, (LONG_PTR)style);
    SetWindowPos(g_edit, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

static int read_text(HWND hwnd, const WCHAR *path, WCHAR **out) {
    HANDLE file;
    LARGE_INTEGER size;
    DWORD len;
    DWORD got;
    BYTE *bytes;
    WCHAR *wide;
    int off;
    UINT cp;

    *out = NULL;

    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        show_last_error(hwnd, L"Open file");
        return 0;
    }

    if (!GetFileSizeEx(file, &size) || size.QuadPart > 256LL * 1024LL * 1024LL) {
        CloseHandle(file);
        MessageBoxW(hwnd, L"File is too large for this build.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    len = (DWORD)size.QuadPart;
    if (len == 0) {
        CloseHandle(file);
        wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR));
        if (!wide) {
            MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
            return 0;
        }
        wide[0] = 0;
        *out = wide;
        return 1;
    }

    bytes = (BYTE *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)len + 2);
    if (!bytes) {
        CloseHandle(file);
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    got = 0;
    if (!ReadFile(file, bytes, len, &got, NULL) || got != len) {
        HeapFree(GetProcessHeap(), 0, bytes);
        CloseHandle(file);
        show_last_error(hwnd, L"Read file");
        return 0;
    }
    CloseHandle(file);

    bytes[len] = 0;
    bytes[len + 1] = 0;

    off = 0;
    cp = CP_UTF8;

    if (len >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
        off = 2;
        cp = 1200;
    } else if (len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        off = 3;
    }

    wide = NULL;

    if (cp == 1200) {
        int count = (int)((len - (DWORD)off) / 2);
        wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)count + 1) * sizeof(WCHAR));
        if (wide) {
            CopyMemory(wide, bytes + off, (SIZE_T)count * sizeof(WCHAR));
            wide[count] = 0;
        }
    } else {
        int need = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)(bytes + off), (int)(len - (DWORD)off), NULL, 0);
        if (need <= 0) {
            need = MultiByteToWideChar(CP_ACP, 0, (LPCCH)bytes, (int)len, NULL, 0);
            if (need > 0) {
                wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)need + 1) * sizeof(WCHAR));
                if (wide) {
                    MultiByteToWideChar(CP_ACP, 0, (LPCCH)bytes, (int)len, wide, need);
                    wide[need] = 0;
                }
            }
        } else {
            wide = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)need + 1) * sizeof(WCHAR));
            if (wide) {
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)(bytes + off), (int)(len - (DWORD)off), wide, need);
                wide[need] = 0;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, bytes);

    if (!wide) {
        MessageBoxW(hwnd, L"Could not decode file text.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    *out = wide;
    return 1;
}

static int write_text(HWND hwnd, const WCHAR *path) {
    WCHAR *wide;
    char *utf8;
    int need;
    DWORD written;
    DWORD want;
    HANDLE file;
    int ok;

    wide = get_text_alloc(GT_USECRLF);
    if (!wide) {
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    need = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (need <= 0) {
        HeapFree(GetProcessHeap(), 0, wide);
        MessageBoxW(hwnd, L"Could not encode text as UTF-8.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    utf8 = (char *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)need);
    if (!utf8) {
        HeapFree(GetProcessHeap(), 0, wide);
        MessageBoxW(hwnd, L"Out of memory.", APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, need, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, wide);

    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        HeapFree(GetProcessHeap(), 0, utf8);
        show_last_error(hwnd, L"Create file");
        return 0;
    }

    want = (DWORD)(need - 1);
    written = 0;
    ok = WriteFile(file, utf8, want, &written, NULL) && written == want;
    CloseHandle(file);
    HeapFree(GetProcessHeap(), 0, utf8);

    if (!ok) {
        show_last_error(hwnd, L"Write file");
        return 0;
    }

    return 1;
}

static int choose_open(HWND hwnd, WCHAR *path) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    path[0] = 0;

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH_CHARS;
    ofn.lpstrFilter = L"Text files\0*.txt;*.md;*.c;*.h;*.asm;*.bat\0All files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    return GetOpenFileNameW(&ofn) != 0;
}

static int choose_save(HWND hwnd, WCHAR *path) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH_CHARS;
    ofn.lpstrFilter = L"Text files\0*.txt\0All files\0*.*\0";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    return GetSaveFileNameW(&ofn) != 0;
}

static void add_recent(const WCHAR *path) {
    int i;
    int n;

    if (!path || !path[0]) return;

    for (i = 0; i < g_recent_count; i++) {
        if (lstrcmpiW(g_recent[i], path) == 0) {
            for (; i > 0; i--) StringCchCopyW(g_recent[i], MAX_PATH_CHARS, g_recent[i - 1]);
            StringCchCopyW(g_recent[0], MAX_PATH_CHARS, path);
            refresh_recent_menu();
            return;
        }
    }

    n = g_recent_count;
    if (n >= RECENT_MAX) n = RECENT_MAX - 1;
    for (i = n; i > 0; i--) StringCchCopyW(g_recent[i], MAX_PATH_CHARS, g_recent[i - 1]);
    StringCchCopyW(g_recent[0], MAX_PATH_CHARS, path);
    if (g_recent_count < RECENT_MAX) g_recent_count++;
    refresh_recent_menu();
}

static void refresh_recent_menu(void) {
    int i;
    WCHAR item[MAX_PATH_CHARS + 16];

    if (!g_recent_menu) return;

    while (GetMenuItemCount(g_recent_menu) > 0) RemoveMenu(g_recent_menu, 0, MF_BYPOSITION);

    if (g_recent_count == 0) {
        AppendMenuW(g_recent_menu, MF_STRING | MF_GRAYED, 0, L"(empty)");
        return;
    }

    for (i = 0; i < g_recent_count; i++) {
        StringCchPrintfW(item, MAX_PATH_CHARS + 16, L"%d  %ls", i + 1, base_name(g_recent[i]));
        AppendMenuW(g_recent_menu, MF_STRING, IDM_FILE_RECENT_BASE + i, item);
    }
}

static void new_file(HWND hwnd) {
    if (!ask_save_if_dirty(hwnd)) return;

    g_loading = 1;
    SetWindowTextW(g_edit, L"");
    g_loading = 0;
    g_path[0] = 0;
    set_read_only(0);
    set_dirty(0);
    update_status();
}

static int open_path(HWND hwnd, const WCHAR *path) {
    WCHAR *text = NULL;

    if (!ask_save_if_dirty(hwnd)) return 0;
    if (!read_text(hwnd, path, &text)) return 0;

    g_loading = 1;
    SetWindowTextW(g_edit, text);
    g_loading = 0;
    HeapFree(GetProcessHeap(), 0, text);

    StringCchCopyW(g_path, MAX_PATH_CHARS, path);
    add_recent(path);
    set_dirty(0);
    update_status();
    return 1;
}

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

static void open_recent(HWND hwnd, int index) {
    WCHAR path[MAX_PATH_CHARS];

    if (index < 0 || index >= g_recent_count) return;
    StringCchCopyW(path, MAX_PATH_CHARS, g_recent[index]);
    if (open_path(hwnd, path)) set_read_only(0);
}

static void reload_file(HWND hwnd) {
    WCHAR path[MAX_PATH_CHARS];
    int ro;

    if (!g_path[0]) return;

    ro = g_read_only;
    StringCchCopyW(path, MAX_PATH_CHARS, g_path);
    if (open_path(hwnd, path)) set_read_only(ro);
}

static int save_file_as(HWND hwnd) {
    WCHAR path[MAX_PATH_CHARS];
    StringCchCopyW(path, MAX_PATH_CHARS, g_path);

    if (!choose_save(hwnd, path)) return 0;
    if (!write_text(hwnd, path)) return 0;

    StringCchCopyW(g_path, MAX_PATH_CHARS, path);
    add_recent(path);
    set_dirty(0);
    update_status();
    return 1;
}

static int save_file(HWND hwnd) {
    if (g_read_only) {
        MessageBoxW(hwnd, L"Read-only buffer. Disable Read Only before saving.", APP_NAME, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    if (!g_path[0]) return save_file_as(hwnd);
    if (!write_text(hwnd, g_path)) return 0;

    add_recent(g_path);
    set_dirty(0);
    update_status();
    return 1;
}

static int ask_save_if_dirty(HWND hwnd) {
    int answer;
    WCHAR text[MAX_PATH_CHARS + 64];

    if (!g_dirty) return 1;

    StringCchPrintfW(text, MAX_PATH_CHARS + 64, L"Save changes to %ls?", g_path[0] ? base_name(g_path) : L"Untitled");
    answer = MessageBoxW(hwnd, text, APP_NAME, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL) return 0;
    if (answer == IDYES) return save_file(hwnd);
    return 1;
}

static void show_file_facts(HWND hwnd) {
    WCHAR text[1200];
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
        StringCchPrintfW(mtime, 64, L"%04u-%02u-%02u %02u:%02u:%02u", local.wYear, local.wMonth, local.wDay, local.wHour, local.wMinute, local.wSecond);
    }

    StringCchPrintfW(
        text,
        1200,
        L"Path: %ls\nDisk bytes: %llu\nModified: %ls\nLines: %d\nChars: %lld\nUTF-8 bytes from buffer: %d\nLine endings: %ls\nState: %ls%ls",
        g_path[0] ? g_path : L"Untitled buffer",
        (unsigned long long)size.QuadPart,
        mtime[0] ? mtime : L"not on disk",
        lines,
        (long long)chars,
        bytes,
        line_ending_name(),
        g_read_only ? L"read-only " : L"",
        g_dirty ? L"modified" : L"saved");

    MessageBoxW(hwnd, text, L"DBYTEPAD facts", MB_OK | MB_ICONINFORMATION);
}

static int find_next(HWND hwnd) {
    CHARRANGE sel;
    FINDTEXTEXW ft;
    DWORD flags;

    (void)hwnd;

    if (!g_find_text[0]) {
        MessageBeep(MB_ICONINFORMATION);
        return 0;
    }

    SendMessageW(g_edit, EM_EXGETSEL, 0, (LPARAM)&sel);

    flags = FR_DOWN;
    if (g_find.Flags & FR_MATCHCASE) flags |= FR_MATCHCASE;
    if (g_find.Flags & FR_WHOLEWORD) flags |= FR_WHOLEWORD;

    ft.chrg.cpMin = sel.cpMax;
    ft.chrg.cpMax = -1;
    ft.lpstrText = g_find_text;

    if (SendMessageW(g_edit, EM_FINDTEXTEXW, (WPARAM)flags, (LPARAM)&ft) < 0 && sel.cpMin > 0) {
        ft.chrg.cpMin = 0;
        ft.chrg.cpMax = sel.cpMin;
    }

    if (SendMessageW(g_edit, EM_FINDTEXTEXW, (WPARAM)flags, (LPARAM)&ft) >= 0) {
        SendMessageW(g_edit, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);
        SetFocus(g_edit);
        update_status();
        return 1;
    }

    MessageBeep(MB_ICONINFORMATION);
    return 0;
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

static void show_replace(HWND hwnd) {
    if (g_find_dialog) {
        SetForegroundWindow(g_find_dialog);
        return;
    }

    ZeroMemory(&g_find, sizeof(g_find));
    g_find.lStructSize = sizeof(g_find);
    g_find.hwndOwner = hwnd;
    g_find.lpstrFindWhat = g_find_text;
    g_find.wFindWhatLen = FIND_TEXT_CHARS;
    g_find.lpstrReplaceWith = g_replace_text;
    g_find.wReplaceWithLen = FIND_TEXT_CHARS;
    g_find.Flags = FR_DOWN;

    g_find_dialog = ReplaceTextW(&g_find);
}

static int selection_matches(void) {
    CHARRANGE sel;
    TEXTRANGEW tr;
    WCHAR *text;
    LONG len;
    int ok;

    if (!g_find_text[0]) return 0;

    SendMessageW(g_edit, EM_EXGETSEL, 0, (LPARAM)&sel);
    len = sel.cpMax - sel.cpMin;
    if (len <= 0 || len > 32767) return 0;

    text = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, ((SIZE_T)len + 1) * sizeof(WCHAR));
    if (!text) return 0;

    tr.chrg = sel;
    tr.lpstrText = text;
    SendMessageW(g_edit, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
    text[len] = 0;

    ok = CompareStringOrdinal(text, -1, g_find_text, -1, (g_find.Flags & FR_MATCHCASE) ? FALSE : TRUE) == CSTR_EQUAL;
    HeapFree(GetProcessHeap(), 0, text);
    return ok;
}

static void replace_selection(void) {
    SendMessageW(g_edit, EM_REPLACESEL, TRUE, (LPARAM)g_replace_text);
    set_dirty(1);
    update_status();
}

static void replace_one(HWND hwnd) {
    if (g_read_only) {
        MessageBoxW(hwnd, L"Read-only buffer.", APP_NAME, MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (!selection_matches()) {
        find_next(hwnd);
        return;
    }

    replace_selection();
    find_next(hwnd);
}

static void replace_all(HWND hwnd) {
    int count = 0;
    CHARRANGE start;
    WCHAR msg[80];

    if (g_read_only) {
        MessageBoxW(hwnd, L"Read-only buffer.", APP_NAME, MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (!g_find_text[0]) return;

    start.cpMin = 0;
    start.cpMax = 0;
    SendMessageW(g_edit, EM_EXSETSEL, 0, (LPARAM)&start);

    while (find_next(hwnd)) {
        replace_selection();
        count++;
        if (count > 100000) break;
    }

    StringCchPrintfW(msg, 80, L"Replaced %d.", count);
    MessageBoxW(hwnd, msg, APP_NAME, MB_OK | MB_ICONINFORMATION);
}

static void show_about(HWND hwnd) {
    MessageBoxW(hwnd, L"DBYTEPAD " APP_VERSION L"\nNative Win32 text editor.\nNo Electron. No webview. No telemetry.", L"About DBYTEPAD", MB_OK | MB_ICONINFORMATION);
}

static HMENU make_menu(void) {
    HMENU menu = CreateMenu();
    HMENU file = CreatePopupMenu();
    HMENU edit = CreatePopupMenu();
    HMENU view = CreatePopupMenu();
    HMENU help = CreatePopupMenu();

    g_recent_menu = CreatePopupMenu();

    AppendMenuW(file, MF_STRING, IDM_FILE_NEW, L"New\tCtrl+N");
    AppendMenuW(file, MF_STRING, IDM_FILE_OPEN, L"Open...\tCtrl+O");
    AppendMenuW(file, MF_STRING, IDM_FILE_OPEN_READ_ONLY, L"Open Read Only...");
    AppendMenuW(file, MF_POPUP, (UINT_PTR)g_recent_menu, L"Recent");
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE, L"Save\tCtrl+S");
    AppendMenuW(file, MF_STRING, IDM_FILE_SAVE_AS, L"Save As...\tCtrl+Shift+S");
    AppendMenuW(file, MF_STRING, IDM_FILE_RELOAD, L"Reload\tF5");
    AppendMenuW(file, MF_STRING, IDM_FILE_FACTS, L"Facts");
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
    AppendMenuW(file, MF_STRING, IDM_FILE_EXIT, L"Exit");

    AppendMenuW(edit, MF_STRING, IDM_EDIT_UNDO, L"Undo\tCtrl+Z");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_CUT, L"Cut\tCtrl+X");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_COPY, L"Copy\tCtrl+C");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_PASTE, L"Paste\tCtrl+V");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_FIND, L"Find...\tCtrl+F");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_FIND_NEXT, L"Find Next\tF3");
    AppendMenuW(edit, MF_STRING, IDM_EDIT_REPLACE, L"Replace...\tCtrl+H");
    AppendMenuW(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(edit, MF_STRING, IDM_EDIT_SELECT_ALL, L"Select All\tCtrl+A");

    AppendMenuW(view, (g_word_wrap ? MF_CHECKED : 0) | MF_STRING, IDM_VIEW_WORD_WRAP, L"Word Wrap");
    AppendMenuW(view, MF_STRING, IDM_VIEW_READ_ONLY, L"Read Only");
    AppendMenuW(view, MF_STRING, IDM_VIEW_FONT, L"Font...");

    AppendMenuW(help, MF_STRING, IDM_HELP_ABOUT, L"About");

    AppendMenuW(menu, MF_POPUP, (UINT_PTR)file, L"File");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)edit, L"Edit");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)view, L"View");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)help, L"Help");

    refresh_recent_menu();
    return menu;
}

static void run_command(HWND hwnd, WORD id) {
    if (id >= IDM_FILE_RECENT_BASE && id < IDM_FILE_RECENT_BASE + RECENT_MAX) {
        open_recent(hwnd, id - IDM_FILE_RECENT_BASE);
        return;
    }

    switch (id) {
    case IDM_FILE_NEW: new_file(hwnd); break;
    case IDM_FILE_OPEN: open_file(hwnd); break;
    case IDM_FILE_OPEN_READ_ONLY: open_file_read_only(hwnd); break;
    case IDM_FILE_SAVE: save_file(hwnd); break;
    case IDM_FILE_SAVE_AS: save_file_as(hwnd); break;
    case IDM_FILE_RELOAD: reload_file(hwnd); break;
    case IDM_FILE_FACTS: show_file_facts(hwnd); break;
    case IDM_FILE_EXIT: SendMessageW(hwnd, WM_CLOSE, 0, 0); break;
    case IDM_EDIT_UNDO: SendMessageW(g_edit, EM_UNDO, 0, 0); break;
    case IDM_EDIT_CUT: SendMessageW(g_edit, WM_CUT, 0, 0); break;
    case IDM_EDIT_COPY: SendMessageW(g_edit, WM_COPY, 0, 0); break;
    case IDM_EDIT_PASTE: SendMessageW(g_edit, WM_PASTE, 0, 0); break;
    case IDM_EDIT_FIND: show_find(hwnd); break;
    case IDM_EDIT_FIND_NEXT: find_next(hwnd); break;
    case IDM_EDIT_REPLACE: show_replace(hwnd); break;
    case IDM_EDIT_SELECT_ALL: SendMessageW(g_edit, EM_SETSEL, 0, -1); break;
    case IDM_VIEW_WORD_WRAP:
        g_word_wrap = !g_word_wrap;
        CheckMenuItem(GetMenu(hwnd), IDM_VIEW_WORD_WRAP, g_word_wrap ? MF_CHECKED : MF_UNCHECKED);
        apply_word_wrap();
        resize_parts(hwnd);
        break;
    case IDM_VIEW_READ_ONLY:
        set_read_only(!g_read_only);
        break;
    case IDM_VIEW_FONT:
        choose_font(hwnd);
        break;
    case IDM_HELP_ABOUT:
        show_about(hwnd);
        break;
    default: break;
    }
}

static void open_command_line_file(HWND hwnd) {
    int argc;
    LPWSTR *argv;

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return;

    if (argc > 1) open_path(hwnd, argv[1]);
    LocalFree(argv);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == g_find_msg) {
        FINDREPLACEW *fr = (FINDREPLACEW *)lparam;
        if (fr->Flags & FR_DIALOGTERM) {
            g_find_dialog = NULL;
            return 0;
        }
        g_find.Flags = fr->Flags;
        if (fr->Flags & FR_FINDNEXT) find_next(hwnd);
        if (fr->Flags & FR_REPLACE) replace_one(hwnd);
        if (fr->Flags & FR_REPLACEALL) replace_all(hwnd);
        return 0;
    }

    switch (msg) {
    case WM_CREATE:
        g_loading = 1;

        g_edit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            EDIT_CLASS,
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_NOHIDESEL | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd,
            NULL,
            g_instance,
            NULL);

        g_status = CreateWindowExW(
            0,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
            0, 0, 0, 0,
            hwnd,
            NULL,
            g_instance,
            NULL);

        if (!g_edit || !g_status) return -1;

        SendMessageW(g_edit, EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_SELCHANGE);
        set_edit_format();
        apply_word_wrap();
        DragAcceptFiles(hwnd, TRUE);

        g_loading = 0;
        set_dirty(0);
        update_status();
        return 0;

    case WM_SIZE:
        resize_parts(hwnd);
        return 0;

    case WM_DROPFILES: {
        HDROP drop = (HDROP)wparam;
        WCHAR path[MAX_PATH_CHARS];
        if (DragQueryFileW(drop, 0, path, MAX_PATH_CHARS)) {
            if (open_path(hwnd, path)) set_read_only(0);
        }
        DragFinish(drop);
        return 0;
    }

    case WM_COMMAND:
        if ((HWND)lparam == g_edit && HIWORD(wparam) == EN_CHANGE) {
            if (!g_loading) set_dirty(1);
            update_status();
            return 0;
        }

        run_command(hwnd, LOWORD(wparam));
        return 0;

    case WM_NOTIFY:
        if (((NMHDR *)lparam)->hwndFrom == g_edit && ((NMHDR *)lparam)->code == EN_SELCHANGE) {
            update_status();
            return 0;
        }
        break;

    case WM_CLOSE:
        if (ask_save_if_dirty(hwnd)) DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        save_config();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev, PWSTR cmd, int show) {
    WNDCLASSW wc;
    HWND hwnd;
    HACCEL accel;
    MSG msg;
    ACCEL accels[] = {
        { FVIRTKEY | FCONTROL, 'N', IDM_FILE_NEW },
        { FVIRTKEY | FCONTROL, 'O', IDM_FILE_OPEN },
        { FVIRTKEY | FCONTROL, 'S', IDM_FILE_SAVE },
        { FVIRTKEY | FCONTROL | FSHIFT, 'S', IDM_FILE_SAVE_AS },
        { FVIRTKEY | FCONTROL, 'A', IDM_EDIT_SELECT_ALL },
        { FVIRTKEY | FCONTROL, 'F', IDM_EDIT_FIND },
        { FVIRTKEY | FCONTROL, 'H', IDM_EDIT_REPLACE },
        { FVIRTKEY, VK_F3, IDM_EDIT_FIND_NEXT },
        { FVIRTKEY, VK_F5, IDM_FILE_RELOAD },
        { FVIRTKEY | FCONTROL, 'Z', IDM_EDIT_UNDO },
        { FVIRTKEY | FCONTROL, 'X', IDM_EDIT_CUT },
        { FVIRTKEY | FCONTROL, 'C', IDM_EDIT_COPY },
        { FVIRTKEY | FCONTROL, 'V', IDM_EDIT_PASTE },
    };

    (void)prev;
    (void)cmd;

    g_instance = instance;
    g_find_msg = RegisterWindowMessageW(L"commdlg_FindReplace");
    load_config();

    if (!LoadLibraryW(L"Msftedit.dll")) {
        MessageBoxW(NULL, L"Could not load Msftedit.dll.", APP_NAME, MB_OK | MB_ICONERROR);
        return 1;
    }

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_IBEAM);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = MAIN_CLASS;

    if (!RegisterClassW(&wc)) {
        show_last_error(NULL, L"RegisterClass");
        return 1;
    }

    hwnd = CreateWindowExW(
        0,
        MAIN_CLASS,
        APP_NAME,
        WS_OVERLAPPEDWINDOW,
        g_win_x,
        g_win_y,
        g_win_w,
        g_win_h,
        NULL,
        make_menu(),
        instance,
        NULL);

    if (!hwnd) {
        show_last_error(NULL, L"CreateWindow");
        return 1;
    }

    g_main = hwnd;
    accel = CreateAcceleratorTableW(accels, (int)(sizeof(accels) / sizeof(accels[0])));

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    open_command_line_file(hwnd);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!g_find_dialog || !IsDialogMessageW(g_find_dialog, &msg)) {
            if (!accel || !TranslateAcceleratorW(hwnd, accel, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
    }

    if (accel) DestroyAcceleratorTable(accel);
    return (int)msg.wParam;
}
